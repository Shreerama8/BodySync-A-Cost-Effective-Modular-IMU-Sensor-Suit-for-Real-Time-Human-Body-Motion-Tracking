
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"

// ── Configuration 
const char*  ssid         = "*******";
const char*  password     = "*******";
const char*  multicast_ip = "239.0.0.1";
const unsigned int port   = 5005;

// ← CHANGE per node: L_FA, R_FA, L_UA, R_UA, L_TH, R_TH, L_SH, R_SH, HIPS, CHEST
const char* SENSOR_LABEL  = "HIPS";

// ── Pin definitions
const int LED_RED   = 1;
const int LED_GREEN = 0;

// ── Globals 
WiFiUDP   udp;
MPU6050   mpu;
uint8_t   fifoBuffer[64];

// ── Helper: connect (or reconnect) to WiFi
void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {

        // Blink red while waiting
        digitalWrite(LED_RED, LOW);   delay(100);
        digitalWrite(LED_RED, HIGH);  delay(100);
        Serial.print(".");

        // Give up after 10 s and let the reconnect watchdog retry
        if (millis() - start > 10000) {
            Serial.println("\nWiFi timeout — will retry in loop.");
            return;
        }
    }
    Serial.println("\nWiFi connected — IP: " + WiFi.localIP().toString());
}

void setup() {
    Serial.begin(115200);
    Wire.begin(8, 9);   // SDA=GPIO8, SCL=GPIO9 — verify matches your wiring

    pinMode(LED_RED,   OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_RED,   HIGH);  // active-LOW → HIGH = off
    digitalWrite(LED_GREEN, HIGH);

    // ── 1. POWER-ON BLINK: 5.5 s, alternating R/G every 500 ms
    Serial.println("Power-on sequence...");
    {
        unsigned long start      = millis();
        unsigned long lastToggle = 0;
        bool          redOn      = false;

        while (millis() - start < 5500) {
            unsigned long now = millis();
            if (now - lastToggle >= 500) {
                lastToggle = now;
                redOn      = !redOn;
                digitalWrite(LED_RED,   redOn ? LOW  : HIGH);
                digitalWrite(LED_GREEN, redOn ? HIGH : LOW);
            }
        }
        // Both off after sequence
        digitalWrite(LED_RED,   HIGH);
        digitalWrite(LED_GREEN, HIGH);
    }

    // ── 2. CALIBRATE (sensor must be still & level) 
    
    Serial.println("Initializing MPU6050...");
    mpu.initialize();

    Serial.println("Calibrating gyro — keep still...");
    mpu.CalibrateGyro(6);

    Serial.println("Calibrating accelerometer — keep level...");
    mpu.CalibrateAccel(6);
    Serial.println("Calibration done!");

    // ── 3. CONNECT WiFi 
    connectWiFi();

    // ── 4. GREEN LED ON 2 s → WiFi confirmed
    
    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(LED_GREEN, LOW);   // ON
        delay(2000);
        digitalWrite(LED_GREEN, HIGH);  // OFF
    }

    // ── 5. INIT DMP
    if (mpu.dmpInitialize() == 0) {
        mpu.setDMPEnabled(true);
        Serial.println("DMP ready.");
    } else {
        Serial.println("DMP init FAILED — halting.");
        // Rapid red blink to signal fatal error
        while (1) {
            digitalWrite(LED_RED, LOW);   delay(100);
            digitalWrite(LED_RED, HIGH);  delay(100);
        }
    }

    Serial.println("\n*** System ready — streaming " + String(SENSOR_LABEL) + " ***\n");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    static unsigned long lastSend      = 0;
    static unsigned long lastHeartbeat = 0;
    static unsigned long heartbeatStart = 0;   
    static bool          heartbeatOn   = false;
    static int           packetCount   = 0;

    unsigned long now = millis();

    // ── WiFi reconnect watchdog
   
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_GREEN, HIGH);  // green off
        Serial.println("WiFi lost — reconnecting...");
        connectWiFi();
        return;
    }

    if (!heartbeatOn && now - lastHeartbeat >= 1000) {
        digitalWrite(LED_RED, LOW);     // ON
        heartbeatOn    = true;
        heartbeatStart = now;           // timestamp the START of the pulse
        lastHeartbeat  = now;           // timestamp the 1 s cycle
    }
    if (heartbeatOn && now - heartbeatStart >= 150) {
        digitalWrite(LED_RED, HIGH);    // OFF after 150 ms
        heartbeatOn = false;
    }

    // ── Rate-limit BEFORE reading FIFO 
    //  original read the packet then discarded it if < 15 ms had passed
    if (now - lastSend < 15) return;

    // ── FIFO overflow check
   
    if (mpu.getFIFOCount() >= 1024) {
        Serial.println("FIFO overflow — resetting.");
        mpu.resetFIFO();
        return;
    }

    // ── Wait for fresh DMP packet 
    if (!mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) return;

    lastSend = now;

    // ── Read quaternion
    Quaternion q;
    mpu.dmpGetQuaternion(&q, fifoBuffer);

    // ── Build payload — C-style,─
    char payload[64];
    snprintf(payload, sizeof(payload), "%s,%.4f,%.4f,%.4f,%.4f",
             SENSOR_LABEL, q.w, q.x, q.y, q.z);

    // ── Send multicast —
    if (udp.beginPacket(multicast_ip, port)) {
        udp.print(payload);
        udp.endPacket();
    } else {
        Serial.println("UDP send failed.");
    }

    //  Serial debug every 100 packets 
    if (++packetCount >= 100) {
        packetCount = 0;
        Serial.println(payload);
    }
}
