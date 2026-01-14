#include <WiFi.h>
#include <WiFiUdp.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <IRremote.hpp>
#define IR_SEND_PIN 4
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "types.h"

// =================================================================
// ヘルパー関数
// =================================================================

IPAddress stringToIP(String ipStr) {
    IPAddress ip;
    ip.fromString(ipStr);
    return ip;
}

int splitString(char* str, char delimiter, String* outputArray, int maxElements) {
    int index = 0;
    char* start = str;
    char* end = strchr(start, delimiter);
    while (end != NULL && index < maxElements - 1) {
        *end = '\0';
        outputArray[index++] = String(start);
        start = end + 1;
        end = strchr(start, delimiter);
    }
    if (index < maxElements) {
        outputArray[index++] = String(start);
    }

    return index;
}

bool sendIRSignal(const InfraredFormatReader& data) {
    uint16_t address = (uint16_t)strtol(data.address.c_str(), NULL, 16);
    uint8_t command = (uint8_t)strtol(data.command.c_str(), NULL, 16);
    const String& protocolName = data.protocol;
    const String& customProcess = data.custom_process;
  
    // デバッグ用: 変換後の値が正しいかを確認する
    Serial.print("Converted Address (HEX): ");
    Serial.println(address, HEX); // 16進数表示
    Serial.print("Converted Command (HEX): ");
    Serial.println(command, HEX); // 16進数表示

    if(customProcess.length() == 0) {
      if (protocolName.equalsIgnoreCase("NEC")) {
        IrSender.sendNEC(address, command, 0);
      } else if (protocolName.equalsIgnoreCase("SONY")) {
        IrSender.sendSony(address, command, 12);
        
      } else if (protocolName.equalsIgnoreCase("RC5")) {
        IrSender.sendRC5(address, command, 14); 
        
      } else if (protocolName.equalsIgnoreCase("JVC")) {
        IrSender.sendJVC(address, command, 0);      
      } else if (protocolName.equalsIgnoreCase("SAMSUNG")) {
        IrSender.sendSamsung(address, command, 0);
      } else {
        Serial.println("ERROR: Unknown protocol name. Cannot send.");
        return false;
      }
    } else {
      if(customProcess.equalsIgnoreCase("S_S_ON")) {
        IrSender.sendPulseDistanceWidth(
          38,
          3500, 1750,
          400, 1350,
          400, 450,
          0x353C09522CULL,
          40,
          PROTOCOL_IS_LSB_FIRST,
          0, 0
        );
      } else if (customProcess.equalsIgnoreCase("S_S_OFF")) {
        IrSender.sendPulseDistanceWidth(
          38,           // 周波数 38kHz
          3500, 1750,   // Header: Mark, Space
          400, 1350,    // Logic "1": Mark, Space (1350us)
          400, 500,     // Logic "0": Mark, Space (ここが500usになりました)
          0x363F09522CULL, // データ 40bit (末尾のULL必須)
          40,           // ビット数
          PROTOCOL_IS_LSB_FIRST, 
          0, 0          // Repeat設定 (なし)
        );
      } else if(customProcess.equalsIgnoreCase("R_S_ON")) {
        IrSender.sendPulseDistanceWidth(
          38,           // 周波数 38kHz
          3500, 1750,   // Header: Mark, Space
          400, 1300,    // Logic "1": Mark, Space (1300us)
          400, 450,     // Logic "0": Mark, Space (450us)
          0x3D3409522CULL, // データ 40bit
          40,           // ビット数
          PROTOCOL_IS_LSB_FIRST, 
          0, 0          // Repeat設定
        );
      } else if(customProcess.equalsIgnoreCase("R_S_OFF")) {
        IrSender.sendPulseDistanceWidth(
          38,           // 周波数 38kHz
          3450, 1750,   // Header: Mark, Space (3450, 1750)
          400, 1350,    // Logic "1": Mark, Space (1350us)
          400, 500,     // Logic "0": Mark, Space (500us)
          0x3E3709522CULL, // データ 40bit (末尾のULL必須)
          40,           // ビット数
          PROTOCOL_IS_LSB_FIRST, 
          0, 0          // Repeat設定 (なし)
        );
      }
    }
    
    delay(100);
    return true;
}

// =================================================================
// 構造体 (struct) の実装
// =================================================================

// 赤外線受信データ構造体の実装
InfraredFormatReader InfraredFormatReader_init(const String* raw_array) {
    return {
        .address = raw_array[1],
        .command = raw_array[2],
        .custom_process = raw_array[3],
        .protocol = raw_array[4]
    };
}

// Rssi送信データ構造体の実装
RssiFormatSender RssiFormatSender_init(float rssi) {
    RssiFormatSender sender = {
        .rssi = rssi
    };

    return sender;
}

String RssiFormatSender_to_string(const RssiFormatSender* sender) {
    String result = "";
    result += sender->TYPE;
    result += ",";
    result += String(sender->rssi);
    return result;
}

// BLE送信データ構造体の実装
BLEPresenceFormatSender BLEPresenceFormatSender_init(bool ble_presence) {
    return {
        .ble_presence = ble_presence
    };
}

String BLEPresenceFormatSender_to_string(const BLEPresenceFormatSender* sender) {
    String result = "";
    result += sender->TYPE;
    result += ",";
    result += String(sender->ble_presence ? 1 : 0);
    return result;
}

// 設定ファイル読み込み構造体の実装
SettingFile SettingFile_load(const char *filename) {
    if (!LittleFS.begin()) {
        Serial.println("\nLittleFSのマウント中にエラーが発生しました");
        return { .success = false };
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.printf("エラー: ファイルを開けませんでした %s\n", filename);
        return { .success = false };
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.printf("エラー: JSONの解析に失敗しました: %s\n", error.c_str());
        file.close();
        return { .success = false };
    }
    file.close();

    return {
        .ssid = doc["ssid"].as<String>(),
        .password = doc["password"].as<String>(),
        .ip = stringToIP(doc["ip"].as<String>()),
        .gateway = stringToIP(doc["gateway"].as<String>()),
        .remote_ip = stringToIP(doc["remote_ip"].as<String>()),
        .subnet = stringToIP(doc["subnet"].as<String>()),
        .dns = stringToIP("8.8.8.8"),
        .success = true
    };
}

// =================================================================
// クラス実装
// =================================================================

void RssiProcessor::initBuffer() {
  for (int i = 0; i < 8; i++) rssiBuf[i] = 0;
  bufIndex = 0;
  sampleCount = 0;
}

// グローバル関数からクラスメソッドへ変更
float RssiProcessor::estimateDistanceCm(float avgRssi, float txPowerDbm, float pathLossExp) {
  float exponent = (txPowerDbm - avgRssi) / (10.0 * pathLossExp);
  float dist_m = pow(10.0, exponent);
  return dist_m * 100.0;
}

// グローバル関数からクラスメソッドへ変更
int compareInts(const void* a, const void* b) {
  int arg1 = *(const int*)a;
  int arg2 = *(const int*)b;
  return (arg1 > arg2) - (arg1 < arg2);
}

RssiResult RssiProcessor::processAdvertisedDeviceForRssi(BLEAdvertisedDevice &dev,
  const String &expectedMac,
  float txPowerDbm,
  float pathLossExp) {
  
  RssiResult r;
  r.matched = false;
  r.rawRssi = 0;
  r.avgRssi = 0;
  r.estCm = -1;
  r.samples = 0;

  // ★高速化: String生成をやめ、直前に作ったバイト配列比較ロジックを使うべきですが、
  // いったん既存ロジックのままで解説します。
  // (前のターンで修正した memcmp 方式にするのがベストです)
  String mac = dev.getAddress().toString().c_str();
  mac.toLowerCase();

  if (mac != expectedMac) {
    return r;
  }

  r.matched = true;
  int raw = dev.getRSSI();
  r.rawRssi = raw;

  // 0. 外れ値（0dBmなど）は無視するフィルタ
  if (raw == 0) return r; 

  // 1. バッファ更新
  rssiBuf[bufIndex] = raw;
  bufIndex = (bufIndex + 1) % 8;
  if (sampleCount < 8) sampleCount++;

  // 2. ★精度向上の肝: ソートして外れ値を捨てる
  // バッファをコピーしてソートする
  int sortedBuf[8];
  for(int i=0; i<sampleCount; i++) sortedBuf[i] = rssiBuf[i];
  
  // クイックソート等で並べ替え
  qsort(sortedBuf, sampleCount, sizeof(int), compareInts);

  // 3. トリム平均計算
  long sum = 0;
  int count = 0;
  
  if (sampleCount >= 4) {
    // データが十分ある場合、
    // 「一番小さい値」と「一番大きい値」を捨てて、真ん中だけ平均する
    for (int i = 1; i < sampleCount - 1; i++) {
       sum += sortedBuf[i];
       count++;
    }
  } else {
    // データが少ないうちは普通に平均
    for (int i = 0; i < sampleCount; i++) {
       sum += sortedBuf[i];
       count++;
    }
  }

  if (count > 0) {
      r.avgRssi = (float)sum / count;
  } else {
      r.avgRssi = raw;
  }
  
  r.samples = sampleCount;
  r.estCm = estimateDistanceCm(r.avgRssi, txPowerDbm, pathLossExp);

  return r;
}


// =================================================================
// グローバル変数
// =================================================================

UdpConfig g_udp = {
    .port = 9000,
};

SettingFile g_setting_config;
RssiProcessor g_rssiProcessor;

volatile bool g_rssiUpdated = false;
volatile float g_currentDist = 0.0;
bool g_isBlePresent = false;
bool g_isLatestBlePresent = true;
volatile unsigned long g_lastBleReceivedTime = 0;
const unsigned long BLE_TIMEOUT_MS = 10000;

// =================================================================
// クラス定義
// =================================================================

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  private:
    String targetMac;
  public:
    MyAdvertisedDeviceCallbacks(const String& macAddress)
      : targetMac(macAddress) {
        this->targetMac.toLowerCase();
      }

    void onResult(BLEAdvertisedDevice dev) override {
      RssiResult r = g_rssiProcessor.processAdvertisedDeviceForRssi(dev, this->targetMac);
      if (!r.matched) return;
      g_currentDist = r.estCm;
      g_rssiUpdated = true;

      g_lastBleReceivedTime = millis();
    }
};

// =================================================================
// メイン処理
// =================================================================

void setup() {
  IrSender.begin(IR_SEND_PIN, DISABLE_LED_FEEDBACK, 0);  
  Serial.begin(115200);
  delay(5000);

  g_setting_config = SettingFile_load("/setting.json");
  if (!g_setting_config.success) {
    Serial.println("設定ファイルの読み込みに失敗しました");
    return;
  }

  if (!WiFi.config(g_setting_config.ip, g_setting_config.gateway, g_setting_config.subnet, g_setting_config.dns)) {
    Serial.println("固定IPの設定に失敗しました");
  }

  WiFi.begin(g_setting_config.ssid.c_str(), g_setting_config.password.c_str());
  
  int attempts = 0;
  Serial.print("Wifi接続待機中");
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi接続成功!");
    Serial.printf("IPアドレス: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("ゲートウェイ: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("サブネット: %s\n", WiFi.subnetMask().toString().c_str());
  } else {
    Serial.println("\nWiFi接続に失敗しました");
    return;
  }

  g_udp.udp.begin(g_udp.port);
  Serial.printf("UDPサーバー起動。ポート %d で待機中\n", g_udp.port);

  BLEDevice::init("");
  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks("e3:c3:26:1d:8d:8f"), true);
  scan->setActiveScan(true);
  scan->setInterval(50);
  scan->setWindow(45);
  scan->start(0, nullptr);
}

void loop() {
  int packetSize = g_udp.udp.parsePacket();
  
  if (packetSize) {
    int len = g_udp.udp.read(g_udp.packet_buffer, 255);
    if (len > 0) {
      g_udp.packet_buffer[len] = '\0';
    }

    String raw_array[5]; 
    int count = splitString(g_udp.packet_buffer, ',', raw_array, 5);
    if (count == 5) {
      InfraredFormatReader ir_data = InfraredFormatReader_init(raw_array);

      // 一回だと赤外線が読み取れない時があるため複数回、送る
      for(int i = 0; i < 10; i++) {
        sendIRSignal(ir_data);
      }
    } else {
      Serial.println("ERROR: Packet format invalid (expected 4 comma-separated values).");
    }

    Serial.printf("内容: %s\n", g_udp.packet_buffer);
  }

  if (g_rssiUpdated) {
    g_rssiUpdated = false;
    g_isBlePresent = true;

    RssiFormatSender rssi_data = RssiFormatSender_init(g_currentDist);
    String rssi_message = RssiFormatSender_to_string(&rssi_data);

    g_udp.udp.beginPacket(g_setting_config.remote_ip, g_udp.port);
    g_udp.udp.print(rssi_message);
    g_udp.udp.endPacket();
  }
  
  if (millis() - g_lastBleReceivedTime > BLE_TIMEOUT_MS) {
    Serial.println("BLE Timeout: Device Lost");
    g_isBlePresent = false;
  }

  if(g_isBlePresent != g_isLatestBlePresent) {
    g_isLatestBlePresent = g_isBlePresent;

    BLEPresenceFormatSender ble_data = BLEPresenceFormatSender_init(g_isBlePresent);
    String ble_msg = BLEPresenceFormatSender_to_string(&ble_data);

    g_udp.udp.beginPacket(g_setting_config.remote_ip, g_udp.port);
    g_udp.udp.print(ble_msg);
    g_udp.udp.endPacket();
  }

  delay(10);
}