# benchmark_tool

FastestDetNext (.espdl) inference-only benchmark for ESP32-S3.

## Default model
- `models/fastestdetnext/fastestdetnext_x3_00_x1_00_64x64_opencv_inter_nearest_cls09_espdl/fastestdetnext_x3_00_x1_00_64x64_opencv_inter_nearest_cls09.espdl`

## Build & run
```bash
cd examples/benchmark_tool/
rm -rf build && idf.py -B build -DIDF_TARGET=esp32s3 build flash monitor
```
This example uses a custom `partitions.csv` to enlarge the app partition.

## BSP List

```
esp32_p4_function_ev_board
esp32_p4_function_ev_board_noglib
esp32_s3_korvo_2_noglib
esp32_s3_eye
esp32_s3_korvo_2
esp32_s3_eye_noglib
```

## Options
- Optional BSP (uses `dependencies.lock.<BSP>` if present):
  ```bash
  rm -rf build && idf.py -B build -DIDF_TARGET=esp32s3 -DBSP=esp32_s3_eye build
  ```
- Change model (folder under `models/fastestdetnext/`):
  ```bash
  rm -rf build && idf.py -B build -DIDF_TARGET=esp32s3 -DFASTESTDET_MODEL_DIR=fastestdetnext_x3_00_x1_00_60x80_opencv_inter_nearest_cls09_espdl build
  ```
- Change iterations:
  ```bash
  rm -rf build && idf.py -B build -DIDF_TARGET=esp32s3 -DBENCH_WARMUP=1 -DBENCH_ITERS=10 build
  ```

## Output
The log prints model info, input shape, and inference timing stats (avg/min/max/p50/p90).
