## Compilation

```bash
g++ -std=c++17 -o imu_visualizer \
    main.cpp sensor_manager.cpp udp_receiver.cpp \
    input_handler.cpp renderer.cpp gltf_model.cpp \
    csv_logger.cpp \
    -lglfw -lGL -lGLU -lglut -lpthread \
    -I/usr/include/glm -Ithird_party
```

## Execution

```bash
./imu_visualizer
```

Place `human.glb` in the same directory. The program requires this GLB model (Mixamo‑style skeleton with skinning) – no fallback renderer is used.

## UDP Input Format

The application listens on UDP port 5005. Each packet is a comma‑separated line:

```
<label>,w,x,y,z
```

Supported labels: `L_FA`, `R_FA`, `L_UA`, `R_UA`, `L_TH`, `R_TH`, `L_SH`, `R_SH`, `HIPS`, `CHEST`

Example packet:
```
L_FA,0.707,-0.707,0.000,0.000
```

## Calibration & Controls

Hold the neutral pose (arms down, legs straight, torso upright), then press the calibration keys.  
The active quaternion mode determines how raw sensor data is converted to body motion.

| Key | Action                         |
|-----|--------------------------------|
| `C` | Calibrate **L_FA** (left forearm) |
| `V` | Calibrate **R_FA** (right forearm)|
| `B` | Calibrate **L_UA** (left upper arm)|
| `N` | Calibrate **R_UA** (right upper arm)|
| `Z` | Calibrate **L_TH** (left thigh)|
| `X` | Calibrate **L_SH** (left shin) |
| `G` | Calibrate **R_TH** (right thigh)|
| `H` | Calibrate **R_SH** (right shin)|
| `I` | Calibrate **HIPS** (pelvis)    |
| `O` | Calibrate **CHEST** (torso)    |
| `M` | Cycle quaternion convention (4 modes per orientation) |
| `P` | Toggle vertical sensor mount (modes 5‑8) |
| `Space` | Re‑calibrate **all sensors at once** |
| `1` | Camera: front view             |
| `2` | Camera: back view              |
| `3` | Camera: side view              |

**Quaternion modes overview:**

- **Horizontal (default):** modes 1‑4. Press `M` to cycle 1→2→3→4→1…
- **Vertical:** modes 5‑8. Press `P` to enter vertical mode (mode 5), then `M` to cycle 5→6→7→8→5…

After changing mode with `M` or toggling vertical with `P`, **recalibrate** (individual key or `Space`) for the new setting to take effect.

### Known Working Modes

Based on physical sensor mounting and LED/switch orientation, the following modes have been confirmed to work correctly:

| Mode | Mounting | LED / Switch Orientation |
|------|----------|--------------------------|
| 2    | Horizontal | Switch up |
| 3    | Horizontal | Switch down |
| 5    | Vertical   | LED up |
| 8    | Vertical   | LED down |

Use these as a starting point when setting up your sensors. After selecting the mode, calibrate the corresponding body part and test the motion.

## Troubleshooting

- If the GLB fails to load, check that `human.glb` exists and is a valid glTF binary file.
- If body parts move incorrectly, try a different quaternion mode (`M`) and recalibrate.
- For vertical‑mount sensors, press `P` first, then cycle `M` until motion matches, then recalibrate.
- Ensure sensor data is sent as unit quaternions (normalised, w first).