# Pis House ESP32 Firmware ドキュメント

本ドキュメントは、Pis House システムのエッジデバイス（ESP32）上で動作するファームウェア群に関する情報を記述したものです。

本リポジトリには、以下の2つの異なるファームウェアが含まれています。
1.  **`pis_house_esp32_server`**: 家電操作・センサー監視を行うメインのファームウェア。
2.  **`infrared_receiver`**: センサー値をシリアル通信で送信するユーティリティ（または学習用）ファームウェア。

---

## 🟢 1. 利用者向けガイド (User Guide)

### 1.1 これは何？
これは、部屋に設置する「白い箱（コントローラー）」の中に入っているソフトウェアです。
スマートフォンからの指示を受け取ってエアコンに信号を送ったり、部屋に人がいるかを感知してシステムに知らせたりする役割を持っています。

### 1.2 普段の使い方は？
* **操作不要**: 基本的に、一度セットアップ（`pis-house-setup` ツールの実行）が終われば、利用者がこのソフトウェアを操作することはありません。
* **トラブル時**: もし「アプリから操作してもエアコンが動かない」といった場合は、デバイスの電源（USBケーブル）を一度抜き差しして再起動してください。

---

## 🔵 2. 開発者向けドキュメント (Developer Documentation)

このセクションでは、エンジニア向けに各ファームウェアの機能、通信仕様、データ構造について詳述します。

### 2.1 共通スペック

| 項目 | 内容 |
| :--- | :--- |
| **プラットフォーム** | ESP32 (Arduino Framework) |
| **言語** | C++ |
| **依存ハードウェア** | ESP32 Development Board, 赤外線LED, 受信モジュール等 |

---

### 2.2 モジュール詳細: `pis_house_esp32_server`

本システムのメインファームウェアです。`pis-house-control-server` (Raspberry Pi) と UDP 通信を行います。

#### 機能
* **UDPサーバー**: Control Server からの赤外線送信リクエストを待ち受ける。
* **センサー送信**: RSSI（電波強度）やステータス（BLE検知等）を Control Server へ送信する。
* **設定読み込み**: LittleFS 上の `setting.json` からネットワーク設定を読み込む。

#### データ構造 (`types.h`)
Control Server との通信プロトコル定義です。

| 構造体名 | 役割 | メンバ詳細 |
| :--- | :--- | :--- |
| **`InfraredFormatReader`** | **受信**: 赤外線送信指令 | `address`, `command`, `protocol` (NEC等) |
| **`RssiFormatSender`** | **送信**: 電波強度 | `rssi` (float) |
| **`StatusValueFormatSender`** | **送信**: ステータス通知 | `status_value` (bool) <br>※主に在室検知(BLE Presence)用 |
| **`SettingFile`** | **設定**: 内部設定保持 | `ssid`, `password`, `ip`, `gateway`, `remote_ip` (通知先IP) |
