# Ultra Lightweight Human Detection Example

## 概要
- UltraTinyOD (UHD) モデルを使った軽量な人物検出サンプルです。
- カメラ映像を取得し、推論結果を LCD に描画します（デフォルト）。

## サンプルモデル
ダウンロードして `models/uhd` 配下で解凍。
1. https://github.com/PINTO0309/UHD/releases/download/onnx/ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost.tar.gz
2. https://github.com/PINTO0309/UHD/releases/download/onnx/ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo.tar.gz

```
esp-who/models
└── uhd
    ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost
    │   ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_anchors.npy
    │   ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost.espdl
    │   ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost.info
    │   ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost.json
    │   ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost.onnx
    │   └── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_wh_scale.npy
    └── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo
        ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo_anchors.npy
        ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo.espdl
        ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo.info
        ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo.json
        ├── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo.onnx
        └── ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo_wh_scale.npy
```

## 対応ボード / BSP
- 既定の BSP: `esp32_s3_eye`（`IDF_TARGET=esp32s3`）
- 本フォルダには `sdkconfig.bsp.esp32_s3_eye` のみ同梱されています。
  - 別ボードで使う場合は対応する `sdkconfig.bsp.<bsp_name>` を用意し、
    `IDF_TARGET` と BSP の組み合わせが一致する必要があります。

## 前提
- ESP-IDF v5.4 / v5.5 系（ルートの README 参照）
- ESP-IDF の Python 環境が有効であること（`export.sh` / `export.bat`）
- USB でボード接続済み

## セットアップ

### 1) ESP-IDF 環境を有効化
Linux/macOS:
```bash
. /path/to/esp-idf/export.sh
```

Windows (PowerShell):
```powershell
C:\path\to\esp-idf\export.ps1
```

### 2) BSP 拡張を有効化
BSP 連動の挙動を有効にするため、`IDF_EXTRA_ACTIONS_PATH` を設定します。
```bash
cd examples/ultra_lightweight_human_detection
export IDF_EXTRA_ACTIONS_PATH="$(pwd)/../../tools"
```

> これにより `idf.py` が BSP を自動判定し、必要なコンポーネントを `main/idf_component.yml` に反映します。
> 初回は `ruamel.yaml` が必要です（未導入の場合は ESP-IDF の Python 環境で `pip install ruamel.yaml`）。

### 3) ターゲット / SDKCONFIG を設定
```bash
idf.py -DSDKCONFIG_DEFAULTS=sdkconfig.bsp.esp32_s3_eye set-target esp32s3
```

PowerShell の場合はダブルクォート推奨:
```powershell
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.bsp.esp32_s3_eye" set-target "esp32s3"
```

## ビルド
```bash
# I (14564) uhd_detect: latency(ms): pre=0.85 infer=31.97 post=0.91
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL_DIR=ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost -DIDF_TARGET=esp32s3 build
# I (12947) uhd_detect: latency(ms): pre=0.88 infer=29.77 post=0.96
idf.py -B build -DBSP=esp32_s3_eye -DUHD_MODEL_DIR=ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost_nolo -DIDF_TARGET=esp32s3 build
```

- モデルや BSP を変更した後はクリーンビルド推奨:
  ```bash
  idf.py fullclean
  ```
  もしくは `build/` を削除してください。

## 書き込み・実行
```bash
idf.py flash monitor
```

- `-p` を省略すると自動スキャンされます。
- モニタ終了は `Ctrl+]`。

## 実行時の挙動
- デフォルトは LCD へ検出枠を描画します。
- LCD が無い場合は `main/app_main.cpp` の `run_detect_lcd()` を `run_detect_term()` に切り替えてください。

https://github.com/user-attachments/assets/887638ce-d938-478b-bf48-55bcd0f42527

## モデル切り替え
`components/uhd_detect/CMakeLists.txt` で以下の CMake キャッシュ変数を参照しています。

- `UHD_MODEL_FAMILY`（既定: `uhd`）
- `UHD_MODEL_DIR`（既定: `ultratinyod_anc8_w32_64x64_opencv_inter_nearest_static_nopost`）

例: w40 モデルを使う場合
```bash
idf.py -DIDF_TARGET=esp32s3 \
-DSDKCONFIG_DEFAULTS=sdkconfig.bsp.esp32_s3_eye \
-DUHD_MODEL_DIR=ultratinyod_anc8_w40_64x64_opencv_inter_nearest_static_nopost \
set-target esp32s3

idf.py build
```

- `UHD_MODEL_DIR` には `models/` 配下のパスも指定可能です。
  - 例: `-DUHD_MODEL_DIR=models/uhd/ultratinyod_anc8_w40_64x64_opencv_inter_nearest_static_nopost`
- `.espdl` が存在しない場合は `*_nocat.espdl` を探します。
- モデルが見つからない場合は CMake でエラーになります。

## パーティション
- `sdkconfig.bsp.esp32_s3_eye` ではカスタムパーティションが有効です。
- 既定では本フォルダの `partitions.csv` を使用します。
- 変更する場合は `idf.py menuconfig` で `Partition Table` の設定を調整してください。

## トラブルシューティング

- `BSP is not defined` と出る
  - `IDF_EXTRA_ACTIONS_PATH` が未設定、または `ruamel.yaml` が未導入の可能性があります。
  - `export IDF_EXTRA_ACTIONS_PATH=.../tools` を確認し、`idf.py set-target` を再実行してください。

- `SDKCONFIG_DEFAULTS is not set` と出る
  - `idf.py -DSDKCONFIG_DEFAULTS=sdkconfig.bsp.esp32_s3_eye set-target esp32s3` を実行してください。

- `Model .espdl not found` と出る
  - `UHD_MODEL_DIR` の指定とモデルファイルの存在を確認してください。

## 補足（任意）
- ホスト側でモデル挙動を確認したい場合は `demo_uhd.py` / `demo_uhd_lite.py` を利用できます。
  - 必要な Python 依存関係（onnx/onnxruntime/opencv 等）は各自導入してください。
