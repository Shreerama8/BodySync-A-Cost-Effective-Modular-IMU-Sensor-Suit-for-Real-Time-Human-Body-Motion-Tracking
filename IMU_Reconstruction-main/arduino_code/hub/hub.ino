#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"

const char* SELF_LABEL = "CHEST";
const char* ssid       = "********";
const char* password   = "*********";

const char*  multicast_ip = "239.0.0.1";
const unsigned int port   = 5005;

const int LED_RED   = 1;
const int LED_GREEN = 0;

WiFiUDP udp;
MPU6050 mpu;
uint8_t fifoBuffer[64];

#pragma pack(1)
struct SensorData {
  char label[8];
  float qw, qx, qy, qz;
};
struct ProbePacket {
  char magic[4];   // "WHO?"
};
struct ProbeReply {
  char magic[4];   // "HERE"
  uint8_t channel;
};
#pragma pack()

static SensorData bufA[8], bufB[8];
static volatile SensorData* writeBuf = bufA;
static SensorData* readBuf = bufB;
static volatile int writeCount = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

uint8_t realChannel = 1;

// ---- ESP-NOW receive: demux by packet size ----
// SensorData (24 bytes) -> push into ring buffer, forwarded over UDP in loop()
// ProbePacket (4 bytes) -> a sensor is hunting for our channel, reply directly
void onEspNowReceive(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(SensorData)) {
    portENTER_CRITICAL_ISR(&mux);
    if (writeCount < 8) {
      memcpy((void*)&writeBuf[writeCount], incomingData, sizeof(SensorData));
      writeCount++;
    }
    portEXIT_CRITICAL_ISR(&mux);
    return;
  }

  if (len == sizeof(ProbePacket)) {
    ProbePacket probe;
    memcpy(&probe, incomingData, sizeof(probe));
    if (memcmp(probe.magic, "WHO?", 4) != 0) return;

    // Reply directly to whoever asked. Their MAC must be a registered peer
    // before we can esp_now_send to it — add transiently, send, remove.
    esp_now_peer_info_t replyPeer = {};
    memcpy(replyPeer.peer_addr, info->src_addr, 6);
    replyPeer.channel = realChannel;
    replyPeer.encrypt = false;
    if (esp_now_add_peer(&replyPeer) == ESP_OK) {
      ProbeReply reply;
      memcpy(reply.magic, "HERE", 4);
      reply.channel = realChannel;
      esp_now_send(info->src_addr, (uint8_t*)&reply, sizeof(reply));
      esp_now_del_peer(info->src_addr);
    }
  }
}

void forwardPacket(const char* label, float qw, float qx, float qy, float qz) {
  char payload[64];
  snprintf(payload, sizeof(payload), "%s,%.4f,%.4f,%.4f,%.4f", label, qw, qx, qy, qz);
  udp.beginPacket(multicast_ip, port);
  udp.print(payload);
  udp.endPacket();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_RED, LOW);   delay(100);
    digitalWrite(LED_RED, HIGH);  delay(100);
    if (millis() - start > 10000) return;
  }
}

void setup() {
  Wire.begin(8, 9);
  pinMode(LED_RED,   OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, HIGH);

  for (int i = 0; i < 11; i++) {
    digitalWrite(LED_RED,   (i % 2) ? HIGH : LOW);
    digitalWrite(LED_GREEN, (i % 2) ? LOW  : HIGH);
    delay(500);
  }
  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, HIGH);

  mpu.initialize();
  mpu.CalibrateGyro(6);
  mpu.CalibrateAccel(6);

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    realChannel = WiFi.channel();
    digitalWrite(LED_GREEN, LOW);
    delay(2000);
    digitalWrite(LED_GREEN, HIGH);
  }

  if (mpu.dmpInitialize() == 0) {
    mpu.setDMPEnabled(true);
  } else {
    while (1) {
      digitalWrite(LED_RED, LOW);   delay(100);
      digitalWrite(LED_RED, HIGH);  delay(100);
    }
  }

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(onEspNowReceive);

  udp.begin(multicast_ip, port);
}

void loop() {
  static unsigned long lastOwnSend = 0;
  static unsigned long lastHB = 0;
  static unsigned long lastChannelRefresh = 0;
  static bool hbOn = false;
  static unsigned long hbStart = 0;

  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_GREEN, HIGH);
    connectWiFi();
    realChannel = WiFi.channel();
    return;
  }

  // Some enterprise APs shift channel without a full disconnect (802.11h).
  // Re-read periodically so a silent channel move doesn't go unnoticed —
  // if it does change mid-run, sensors will simply re-discover on their
  // own retry sweep since their ProbeReply will start carrying the new
  // value immediately.
  if (now - lastChannelRefresh >= 5000) {
    lastChannelRefresh = now;
    realChannel = WiFi.channel();
  }

  if (!hbOn && now - lastHB >= 1000) {
    digitalWrite(LED_RED, LOW);
    hbOn = true; hbStart = now; lastHB = now;
  }
  if (hbOn && now - hbStart >= 150) {
    digitalWrite(LED_RED, HIGH);
    hbOn = false;
  }

  // Own MPU first to avoid FIFO overflow
  if (now - lastOwnSend >= 15) {
    if (mpu.getFIFOCount() >= 1024) {
      mpu.resetFIFO();
    } else if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
      lastOwnSend = now;
      Quaternion q;
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      forwardPacket(SELF_LABEL, q.w, q.x, q.y, q.z);
    }
  }

  // Swap buffers atomically, forward whatever sensors sent this tick
  int count = 0;
  portENTER_CRITICAL(&mux);
  count = writeCount;
  writeCount = 0;
  volatile SensorData* tmp = writeBuf;
  writeBuf = (volatile SensorData*)readBuf;
  readBuf = (SensorData*)tmp;
  portEXIT_CRITICAL(&mux);

  for (int i = 0; i < count; i++) {
    forwardPacket(readBuf[i].label, readBuf[i].qw, readBuf[i].qx, readBuf[i].qy, readBuf[i].qz);
  }

  delay(1);
}
