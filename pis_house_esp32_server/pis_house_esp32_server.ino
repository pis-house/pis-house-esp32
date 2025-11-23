#include <WiFi.h>
#include <WiFiUdp.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include "types.h"

// =================================================================
// ヘルパー関数
// =================================================================

IPAddress stringToIP(String ipStr) {
    IPAddress ip;
    ip.fromString(ipStr);
    return ip;
}

// =================================================================
// 構造体 (struct) の実装
// =================================================================

// 赤外線受信データ構造体の実装
InfraredFormatReader InfraredFormatReader_init(const String* raw_array) {
    return {
        .address = raw_array[1],
        .command = raw_array[2]
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

// ステータス送信データ構造体の実装
StatusValueFormatSender StatusValueFormatSender_init(bool status_value) {
    return {
        .status_value = status_value
    };
}

String StatusValueFormatSender_to_string(const StatusValueFormatSender* sender) {
    String result = "";
    result += sender->TYPE;
    result += ",";
    result += String(sender->status_value ? 1 : 0);
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
        .remote_ip = stringToIP("10.234.56.225"),
        .subnet = stringToIP(doc["subnet"].as<String>()),
        .dns = stringToIP("8.8.8.8"),
        .success = true
    };
}


// =================================================================
// グローバル変数
// =================================================================

UdpConfig g_udp = {
    .port = 9000,
};

SettingFile g_setting_config;


// =================================================================
// メイン処理
// =================================================================

void setup() {
  Serial.begin(115200);
  delay(5000);

  g_setting_config = SettingFile_load("/config.json");
  if (!g_setting_config.success) {
    Serial.println("設定ファイルの読み込みに失敗しました");
    return;
  }

  if (!WiFi.config(g_setting_config.ip, g_setting_config.gateway, g_setting_config.subnet, g_setting_config.dns)) {
    Serial.println("固定IPの設定に失敗しました");
  }
  
  int attempts = 0;
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
}

void loop() {
  int packetSize = g_udp.udp.parsePacket();
  
  if (packetSize) {
    int len = g_udp.udp.read(g_udp.packet_buffer, 255);
    if (len > 0) {
      g_udp.packet_buffer[len] = '\0';
    }
    Serial.printf("内容: %s\n", g_udp.packet_buffer);
  }

  RssiFormatSender rssi_data = RssiFormatSender_init((float)WiFi.RSSI());
  String rssi_message = RssiFormatSender_to_string(&rssi_data);
  g_udp.udp.beginPacket(g_setting_config.remote_ip, g_udp.port);
  g_udp.udp.print(rssi_message);
  g_udp.udp.endPacket();

  Serial.printf("送信データ: %s\n", rssi_message.c_str());
  

  StatusValueFormatSender status_data = StatusValueFormatSender_init((millis() / 1000) % 2 == 0);
  String status_value_message = StatusValueFormatSender_to_string(&status_data);
  g_udp.udp.beginPacket(g_setting_config.remote_ip, g_udp.port);
  g_udp.udp.print(status_value_message);
  g_udp.udp.endPacket();

  Serial.printf("送信データ: %s\n", status_value_message.c_str());

  delay(10);
}