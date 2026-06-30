# BodySync-A-Cost-Effective-Modular-IMU-Sensor-Suit-for-Real-Time-Human-Body-Motion-Tracking
BodySync is a low-cost, modular wearable motion capture system that uses ESP32-C3 and MPU6050 IMU sensors to track full-body movements in real time. It delivers accurate wireless motion visualization for healthcare, rehabilitation, sports, animation, and research at a fraction of the cost of commercial motion capture systems.

# BodySync
### A Cost-Effective Modular IMU Sensor Suit for Real-Time Human Body Motion Tracking

<p align="center">
<img src="Images/coverimg.png" width="900">
</p>

<p align="center">
<img src="https://img.shields.io/badge/Platform-ESP32--C3-blue">
<img src="https://img.shields.io/badge/Sensor-MPU6050-green">
<img src="https://img.shields.io/badge/Language-C++-orange">
<img src="https://img.shields.io/badge/PCB-Custom%202-Layer-red">
<img src="https://img.shields.io/badge/License-MIT-yellow">
</p>

---

## Overview

**BodySync** is a low-cost, modular wearable motion capture system designed to perform real-time full-body motion tracking using custom-built ESP32-C3 wearable nodes and MPU6050 IMU sensors.

Unlike expensive commercial motion capture systems costing thousands of dollars, BodySync provides an affordable open-source alternative for:

- Healthcare
- Physiotherapy
- Sports Biomechanics
- Animation
- Robotics
- Human Motion Research
- Education

Each wearable node consists of a custom-designed PCB integrating an ESP32-C3, MPU6050 IMU, IP5306 power management IC, Li-Po battery charging circuit, and wireless communication.

The collected orientation data is transmitted wirelessly and reconstructed into a real-time 3D human skeleton.

---

# Features

- Custom-designed wearable PCB
- ESP32-C3 based wireless sensor nodes
- MPU6050 Digital Motion Processor (DMP)
- Quaternion-based motion tracking
- Real-time Wi-Fi UDP streaming
- Full-body tracking using 10 wearable nodes
- Automatic sensor calibration
- Real-time 3D visualization
- CSV data logging
- Battery-powered wearable design
- Open-source hardware and software

---

# Hardware Architecture

```
                 +---------------------+
                 |    Visualization    |
                 |      Software       |
                 +----------+----------+
                            ^
                            |
                     Wi-Fi UDP Multicast
                            |
 -------------------------------------------------------------
 |      |       |       |      |      |      |      |      |
HIPS CHEST L-UA L-FA R-UA R-FA L-TH L-SH R-TH R-SH
 |      |       |       |      |      |      |      |      |
 ESP32-C3 + MPU6050 + Custom PCB + Battery + IP5306
```

---

# Hardware Components

| Component | Quantity |
|------------|----------|
| ESP32-C3 SuperMini | 10 |
| MPU6050 IMU | 10 |
| IP5306 Power IC | 10 |
| Custom PCB | 10 |
| Li-Po Battery | 10 |
| USB Type-C | 10 |
| Power Switch | 10 |
| Status LED | 10 |
| 3D Printed Enclosure | 10 |
| Velcro Strap | 10 |

---

# Software Stack

- Arduino IDE
- ESP32 Arduino Framework
- C++
- UDP Multicast
- MPU6050 DMP Library
- Wi-Fi
- Quaternion Mathematics
- OpenGL / glTF Visualization
- CSV Logger
- KiCad PCB Design

---

# Working Principle

1. ESP32-C3 reads MPU6050 sensor data.
2. MPU6050 DMP generates orientation quaternions.
3. Sensor calibration removes offsets.
4. Quaternion data is transmitted via UDP multicast.
5. Visualization software reconstructs body posture.
6. Motion is displayed on a rigged 3D human model.
7. Motion data is simultaneously logged into CSV files.

---

# Node Locations

- HIPS
- CHEST
- LEFT UPPER ARM
- LEFT FOREARM
- RIGHT UPPER ARM
- RIGHT FOREARM
- LEFT THIGH
- LEFT SHIN
- RIGHT THIGH
- RIGHT SHIN


# Getting Started

## Clone Repository

```bash
git clone https://github.com/Shreerama8/BodySync-A-Cost-Effective-Modular-IMU-Sensor-Suit-for-Real-Time-Human-Body-Motion-Tracking.git
```

## Flash Firmware

Open the firmware in Arduino IDE.

Select:

```
Board:
ESP32-C3 SuperMini

Upload Speed:
921600

Flash Size:
4MB
```

Upload firmware to each wearable node.

---

# Calibration

1. Power ON all nodes.
2. Keep sensors stationary.
3. Automatic Gyroscope Calibration.
4. Automatic Accelerometer Calibration.
5. Wear the suit.
6. Perform T-Pose calibration.
7. Start motion capture.

---

# Applications

- Motion Capture
- Animation
- Robotics
- VR
- AR
- Physiotherapy
- Sports Analysis
- Medical Research
- Human Activity Recognition
- Education

---

# Future Improvements

- ESP-NOW communication
- BLE Mesh networking
- 9-Axis IMU integration
- Magnetometer support
- Improved drift compensation
- Battery optimization
- AI-based pose estimation
- Mobile application
- Cloud synchronization

---

# Demo

🎥 Demo videos available in the repository.

---

# Author

**Shreerama T D**

Electronics & Communication Engineering  
PES University, Bengaluru

GitHub:
https://github.com/Shreerama8

---

# Acknowledgements

- Espressif Systems
- MPU6050 DMP Library
- Arduino Community
- KiCad
- OpenGL
- glTF
- Motion Capture Research Community

---

# License

This project is released under the **MIT License**.

Feel free to use, modify, and contribute.

⭐ If you found this project useful, please consider giving it a star!
