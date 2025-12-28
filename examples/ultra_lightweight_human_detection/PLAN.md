# ultra_lightweight_human_detection 実装計画

目的: `examples/human_face_recognition` から必要最低限を抽出し、ESP32-S3-EYE向けの超軽量人物検出アプリを作る。

## 前提・制約
- BSP: `examples/human_face_recognition/sdkconfig.bsp.esp32_s3_eye` を基準にする。
- モデル: `ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl` を使用。
- モデル配置の最終パス: `models/uhd/ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl/`
- 入力: 64x64 RGB (将来: YUV422 / Yのみ / Yの3値化 / Yの2値化)。
- 入力モード切替: 起動時CLIで切替する。
- Pythonや追加パッケージは使わない。

## 1. モデル配置と資産整理
1. `models/` 配下に複数モデル追加を想定し、階層を整理する。
   - 確定パス: `models/uhd/ultratinyod_res_anc8_w64_64x64_opencv_inter_nearest_static_nopost_espdl/`
2. 既存のモデルディレクトリを上記へ移動。
3. 参照するファイル:
   - ESP-DL量子化モデル: `.espdl`
   - 入出力仕様: `.info` (入力名/shape確認に使用)
   - anchors / wh_scale: `.npy`

## 2. 例題アプリの骨組み作成
1. `examples/ultra_lightweight_human_detection/` を新設。
2. 最小構成を `examples/human_face_recognition` / `examples/object_detect` から移植:
   - `CMakeLists.txt`
   - `main/app_main.cpp`
   - `main/frame_cap_pipeline.{hpp,cpp}`
   - `main/CMakeLists.txt`
3. BSP設定をコピー:
   - `sdkconfig.bsp.esp32_s3_eye`
4. 依存コンポーネントは必要最小限に限定:
   - `who_task`, `who_cam`, `who_frame_cap`, `who_frame_lcd_disp`, `who_detect`, `who_app/who_detect_app`, `esp-dl`

## 3. 入力モード切替設計 (起動時CLI)
1. 入力モードの列挙を用意:
   - `RGB888`
   - `YUV422`
   - `Y_ONLY`
   - `Y_TERNARY` (0.0/0.5/1.0)
   - `Y_BINARY` (0.0/1.0)
2. 切替方法:
   - 起動時CLIで `input-mode <mode>` を受け付け、未指定時はデフォルト値を使用。
   - 省コード優先: `uart_read_bytes` で1行だけ読む簡易CLI (一定時間でタイムアウト)。
   - CLIの重さが許容できるなら `esp_console` を使って拡張可能にする。
3. 変更点が局所化するよう、前処理の入口を `preprocess_input(mode, fb)` に集約する。

## 4. カメラ解像度の確認と縮小方針
1. 64x64は `framesize_t` に存在しないため、直接の64x64取得は不可。
   - `sensor.h` に `FRAMESIZE_96X96`, `FRAMESIZE_128X128`, `FRAMESIZE_QQVGA(160x120)` がある。
2. 最小解像度の候補:
   - 最優先: `FRAMESIZE_96X96` (最小)
   - 次点: `FRAMESIZE_128X128`
   - さらに必要なら `FRAMESIZE_QQVGA (160x120)` / `FRAMESIZE_QVGA (320x240)`
3. 取得後に 64x64 へ縮小してモデル入力に使用。
4. 縮小は以下いずれか:
   - `dl::image::ImageTransformer` を使った最近傍縮小 (CPU)
   - PPAはESP32-S3非対応の想定のため、CPU縮小を基本とする
5. コード量を抑えるため、最初は固定解像度(例: 96x96)で実装し、必要なら後で設定化する。

## 5. モデル推論の組み込み (ESP-DL)
1. `.espdl` をrodataに埋め込み、`dl::Model` でロード。
   - `idf_component_register(... EMBED_FILES <model.espdl> ...)`
2. 入力テンソル作成:
   - 64x64x3 のINT8入力を生成 (RGBの場合)。
   - `preprocess_input` がRGB/Y系に応じてバッファ生成。
3. 推論実行:
   - `model.run(input_tensor)` を使用。
   - 出力テンソルから `txtywh_obj_quality_cls_x8` を取得。

## 6. 出力後処理 (バウンディングボックス生成)
1. 出力形状: `[1, 56, 8, 8]` を前提。
2. `demo_uhd.py` のロジックをC++化してRAW出力からBBox算出。
3. anchors / wh_scale:
   - `.npy` からfloat32配列を抽出し、C++定数として埋め込み。
   - Python禁止のため、軽量なC++ツール/関数で `.npy` を読んで出力する。
   - 生成した定数は `uhd_constants.hpp` 等に保存。

## 7. 検出結果の表示
1. `WhoDetectAppLCD` を再利用し、LCDに枠とラベルを表示。
2. 推論結果を `dl::detect::result_t` に詰め替える。
3. LCDの表示解像度とモデル入力解像度の差異は、
   必要なら `set_rescale_params` で補正。

## 8. 実装ファイル構成案
```
examples/ultra_lightweight_human_detection/
  CMakeLists.txt
  sdkconfig.bsp.esp32_s3_eye
  main/
    app_main.cpp
    frame_cap_pipeline.hpp
    frame_cap_pipeline.cpp
    CMakeLists.txt
  components/
    uhd_detect/
      uhd_detect.hpp
      uhd_detect.cpp
      uhd_constants.hpp
```

## 9. 実装ステップ
1. モデル移動と `CMakeLists.txt` での埋め込み設定。
2. 新規example作成 + BSP設定コピー。
3. 起動時CLIの最小実装 (input mode取得 + デフォルト値)。
4. 64x64入力の前処理 (RGB/Y系) を実装。
5. esp-dlモデルロード + 推論処理。
6. postprocess (anchors/wh_scale + bbox計算)。
7. LCD表示連携。
8. 省メモリ/速度調整。

## 10. 動作確認
- `idf.py -B build -DIDF_TARGET=esp32s3 build` でビルド確認。
- LCD上で検出枠が動作することを確認。
- 入力モード切替が効くことを確認。
