# human_face_recognition C++構造メモ

このドキュメントは `examples/human_face_recognition` のC++コードの構造と依存関係、
エントリポイントからの処理の流れを、C++未経験者向けに整理したものです。

## 1. このサンプルの役割
- カメラからフレームを取得し、顔検出/顔認識を行う。
- LCDがある場合は検出結果を重ねて表示し、ボタンで認識/登録/削除の操作を行う。
- 主要ロジックはESP-WHOのコンポーネント群に分割されている。

## 2. `examples/human_face_recognition` 内の主なファイル
- `examples/human_face_recognition/main/app_main.cpp`
  - アプリの入口 (`app_main`)。ボードごとの初期化とパイプライン作成、アプリ実行を行う。
- `examples/human_face_recognition/main/frame_cap_pipeline.hpp`
- `examples/human_face_recognition/main/frame_cap_pipeline.cpp`
  - カメラ入力の取得パイプライン(フレーム取得の流れ)を組み立てる。
- `examples/human_face_recognition/main/CMakeLists.txt`
  - このサンプルが依存するコンポーネントを宣言する。
- `examples/human_face_recognition/CMakeLists.txt`
  - ESP-IDFのビルド設定と、必要なコンポーネントのディレクトリを指定する。

## 3. 主要コンポーネントと依存関係
このサンプルは複数のESP-WHOコンポーネントが連携して動作する。

### 3.1 依存関係の全体像(簡易)
```
app_main.cpp
  -> frame_cap_pipeline (フレーム取得パイプライン)
  -> WhoRecognitionAppLCD / WhoRecognitionAppTerm (アプリ本体)
      -> WhoFrameCap (フレーム取得のタスク群)
      -> WhoDetect (顔検出タスク)
      -> WhoRecognitionCore (顔認識タスク)
      -> WhoFrameLCDDisp / 表示オーバーレイ
      -> HumanFaceDetect / HumanFaceRecognizer (モデル本体)
```

### 3.2 各コンポーネントの役割
- `components/who_frame_cap`
  - カメラフレーム取得のためのタスク群。
  - ノード(`WhoFetchNode`, `WhoDecodeNode`, `WhoPPAResizeNode`)を直列につなげる。
- `components/who_peripherals/who_cam`
  - 実際のカメラ制御。
  - `WhoS3Cam`, `WhoP4Cam`, `WhoUVCCam`など、ボード/接続方式ごとに実装がある。
- `components/who_detect`
  - フレームを受け取り、検出モデルで顔検出するタスク。
- `components/who_recognition`
  - 検出結果を使って顔認識/登録/削除を行うタスク。
- `components/who_app/who_recognition_app`
  - UI(LCD/ターミナル)を含むアプリ組み立て。
- `components/who_frame_lcd_disp`
  - LCD表示。フレームの描画と、検出結果/テキストの重ね合わせ。
- `components/who_peripherals/who_spiflash_fatfs`
  - 顔データベース(`face.db`)の保存先ストレージ。
- `examples/human_face_recognition/managed_components/*`
  - 外部ライブラリ/モデル本体。
  - 例: `espressif__human_face_detect`, `espressif__human_face_recognition`, `espressif__esp-dl`, `lvgl__lvgl`。

## 4. エントリポイントからの流れ
### 4.1 `app_main` の流れ (`app_main.cpp`)
1. 現在のタスク優先度を設定。
2. ストレージをマウント (SPIFFS / FATFS / SDカード)。
3. ボードに応じたLED初期化(ESP32-S3-EYEなど)。
4. ボードに応じてフレーム取得パイプラインを作る。
   - ESP32-S3: `get_dvp_frame_cap_pipeline()`
   - ESP32-P4: `get_mipi_csi_frame_cap_pipeline()` または `get_uvc_frame_cap_pipeline()`
5. `WhoRecognitionAppLCD` を生成して `run()`。
   - LCDがない場合は `WhoRecognitionAppTerm` を使う。

### 4.2 フレーム取得パイプライン (`frame_cap_pipeline.cpp`)
- `WhoFrameCap` にノードを追加して、フレーム処理の直列パイプラインを作る。
- ESP32-S3 (DVPカメラ):
  - `WhoS3Cam` からフレーム取得。
  - `WhoFetchNode` でフレームを取り出す。
- ESP32-P4 (MIPI CSI):
  - `WhoP4Cam` + `WhoFetchNode`。
- ESP32-P4 (UVC):
  - `WhoUVCCam` + `WhoFetchNode` + `WhoDecodeNode` + `WhoPPAResizeNode`。
- `ringbuf_len` は処理遅延と同期に影響する。

### 4.3 認識アプリの流れ (`WhoRecognitionAppLCD`)
1. `WhoRecognitionAppBase` が `WhoRecognition` を生成。
   - `WhoRecognition` は `WhoDetect` と `WhoRecognitionCore` を保持。
2. LCD表示タスク (`WhoFrameLCDDisp`) を生成してフレーム表示。
3. 検出/認識のモデルを設定。
   - `HumanFaceDetect` (検出モデル)
   - `HumanFaceRecognizer` (認識モデル、DBのパス指定)
4. ボタン入力を設定。
5. 検出結果/認識結果を受け取るコールバックを登録。
6. `run()` で以下のタスクを起動:
   - フレーム取得ノード群
   - LCD表示タスク
   - 検出タスク (`WhoDetect`)
   - 認識タスク (`WhoRecognitionCore`)

## 5. 検出/認識のタスク構成
### 5.1 検出 (`components/who_detect`)
- `WhoDetect` は `WhoFrameCapNode` の新規フレーム通知を受ける。
- `m_model->run(img)` で顔検出を実行。
- 結果はコールバック経由でアプリ側に渡される。

### 5.2 認識 (`components/who_recognition`)
- `WhoRecognitionCore` はイベントで動作を切り替える。
  - `RECOGNIZE`: 認識
  - `ENROLL`: 登録
  - `DELETE`: 削除
- 認識時は `WhoDetect` の結果コールバックを差し替え、
  顔検出結果を受けたら `HumanFaceRecognizer` を実行する。

## 6. 表示とUI
- LCD表示 (`WhoFrameLCDDisp`) はフレームを描画。
- 検出結果の枠や認識結果の文字列は、
  `WhoDetectResultLCDDisp` / `WhoTextResultLCDDisp` により重ねられる。
- 操作は `WhoRecognitionButton` で行う(物理ボタン or LVGL)。

## 7. ビルド時の分岐(重要な設定)
- ボード選択:
  - `CONFIG_IDF_TARGET_ESP32S3` / `CONFIG_IDF_TARGET_ESP32P4`
  - `BSP_BOARD_ESP32_S3_EYE` / `BSP_BOARD_ESP32_S3_KORVO_2` / `BSP_BOARD_ESP32_P4_FUNCTION_EV_BOARD`
- DB保存先:
  - `CONFIG_DB_FATFS_FLASH`, `CONFIG_DB_SPIFFS`, `CONFIG_DB_FATFS_SDCARD`
- モデル選択:
  - `CONFIG_DEFAULT_HUMAN_FACE_DETECT_MODEL`
  - `CONFIG_DEFAULT_HUMAN_FACE_FEAT_MODEL`

## 8. 最初に把握すべき「改変ポイント」候補
- エントリの流れを変える: `examples/human_face_recognition/main/app_main.cpp`
- カメラ/パイプライン構成を変える: `examples/human_face_recognition/main/frame_cap_pipeline.cpp`
- 検出/認識の処理を変える: `components/who_detect/*`, `components/who_recognition/*`
- 表示/UIを変える: `components/who_app/who_recognition_app/*`, `components/who_frame_lcd_disp/*`

