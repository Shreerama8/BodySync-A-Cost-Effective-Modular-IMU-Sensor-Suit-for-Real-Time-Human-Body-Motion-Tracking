# Discovery protocol — shared contract between hub.ino and sensor.ino

Three packet shapes travel over ESP-NOW. They are distinguished by `len` in
the recv callback, so sizes must stay distinct.

```cpp
#pragma pack(1)
struct SensorData {     // sizeof = 8 + 4*4 = 24
  char label[8];
  float qw, qx, qy, qz;
};
struct ProbePacket {    // sizeof = 4
  char magic[4];        // "WHO?"
};
struct ProbeReply {     // sizeof = 5
  char magic[4];        // "HERE"
  uint8_t channel;
};
#pragma pack()
```

Sensor -> Hub routing (fixed, not auto-assigned):
  CHEST hub (E8:3D:C1:9C:50:14) <- L_FA, L_UA, R_FA, R_UA
  HIPS  hub (A4:CB:8F:1E:A5:B8) <- L_SH, L_TH, R_SH, R_TH

Discovery sequence (sensor side):
  1. esp_now_init() once, register recv callback for ProbeReply.
  2. For each channel 1..13: set channel, add hub as peer on that channel,
     send ProbePacket, wait 300ms for a ProbeReply carrying matching channel,
     remove peer, move to next channel.
  3. If no reply after full 1..13 sweep, repeat the whole sweep (forever,
     with a distinct "searching" LED pattern) — never fall back to a guess.
  4. On reply: lock esp_wifi_set_channel() to that channel, re-add hub as a
     normal peer, proceed to DMP init and normal SensorData send loop.

Hub side: never channel-hops. Stays on whatever channel WiFi.channel()
reports after STA association. Just answers ProbePacket with ProbeReply
on demand, in addition to its existing SensorData forwarding job.

Why this survives nightly AP channel rotation: the channel value is never
hardcoded or cached across boots anywhere. Every boot, both hub and sensor
re-derive it from scratch — hub from WiFi.channel(), sensor from the sweep.
