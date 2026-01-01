# Ultra Lightweight Human Detection Example

This example targets ESP32-S3-EYE and runs an ultra-light human detector using an ESP-DL quantized UHD model.
Camera frames are resized to 64x64 before inference and drawn on the LCD.

## Prerequisites
- ESP-IDF is installed and configured.
- `IDF_EXTRA_ACTIONS_PATH` points to `esp-who/tools`.
- The UHD model exists at:
  `models/uhd/ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl/`
- BSP name is `esp32_s3_eye` and target is `esp32s3`.

## Build
Commands below assume Linux/macOS unless noted.

1) Set environment variable (first time only)
```bash
cd /path/to/esp-who
export IDF_EXTRA_ACTIONS_PATH=${PWD}/tools
```
PowerShell:
```powershell
$Env:IDF_EXTRA_ACTIONS_PATH="C:\\path\\to\\esp-who\\tools"
```
Windows cmd:
```cmd
set IDF_EXTRA_ACTIONS_PATH=C:\path\to\esp-who\tools
```

2) Set target and BSP defaults
```bash
cd examples/ultra_lightweight_human_detection
rm -rf build && idf.py -DSDKCONFIG_DEFAULTS=sdkconfig.bsp.esp32_s3_eye set-target esp32s3
```
PowerShell:
```powershell
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.bsp.esp32_s3_eye" set-target "esp32s3"
```

3) Build (default model: w16)
```bash
idf.py -B build -DBSP=esp32_s3_eye build
```
To switch the model at build time:
```bash
# Use the w16 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w16 build

# Use the w24 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w24 build

# Use the w32 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w32 build

# Use the w40 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w40 build

# Use the w48 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w48 build

# Use the w56 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w56 build

# Use the w64 model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w64 build

# Use the w16 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w16_highacc build

# Use the w24 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w24_highacc build

# Use the w32 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w32_highacc build

# Use the w40 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w40_highacc build

# Use the w48 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w48_highacc build

# Use the w56 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w56_highacc build

# Use the w64 high-accuracy model
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL=w64_highacc build
```

## Flash and Monitor
```bash
idf.py flash monitor
```
- Adjust the serial port for your environment (e.g. `/dev/ttyACM0`, `COM3`).
- If `-p` is omitted, ESP-IDF will try to auto-detect a port.
PowerShell example:
```powershell
idf.py -B build -DBSP=esp32_s3_eye -p COM3 flash monitor
```

## Runtime Input Mode Selection
On startup, UART shows the prompt below:
```
input-mode? (rgb888|yuv422|y_only|y_ternary|y_binary) [rgb888]:
```
- If nothing is entered within 2 seconds, `rgb888` is used.
- Example inputs: `y_only`, `y_binary`, etc.

## Notes
- The camera is configured to `FRAMESIZE_96X96` and resized to 64x64 for the model input.
- The model is embedded into rodata via CMake `EMBED_FILES`.
