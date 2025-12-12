#include <WiFi.h>
#include <WiFiUdp.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include "types.h"
#include <IRremote.hpp>
#define IR_SEND_PIN 4

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
    char* token = strtok(str, &delimiter);
    
    while (token != NULL && index < maxElements) {
        outputArray[index++] = String(token);
        token = strtok(NULL, &delimiter);
    }
    return index;
}

bool sendIRSignal(const InfraredFormatReader& data) {
    uint16_t address = (uint16_t)strtol(data.address.c_str(), NULL, 16);
    uint8_t command = (uint8_t)strtol(data.command.c_str(), NULL, 16);
    const String& protocolName = data.protocol;
  
    // デバッグ用: 変換後の値が正しいかを確認する
    Serial.print("Converted Address (HEX): ");
    Serial.println(address, HEX); // 16進数表示
    Serial.print("Converted Command (HEX): ");
    Serial.println(command, HEX); // 16進数表示

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
        .protocol = raw_array[3]
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
        .remote_ip = stringToIP(doc["remote_ip"].as<String>()),
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

unsigned long previousMillis = 0;
const long interval = 5000;

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
}

void loop() {
  int packetSize = g_udp.udp.parsePacket();
  
  if (packetSize) {
    int len = g_udp.udp.read(g_udp.packet_buffer, 255);
    if (len > 0) {
      g_udp.packet_buffer[len] = '\0';
    }

    String raw_array[4]; 
    int count = splitString(g_udp.packet_buffer, ',', raw_array, 4);
    if (count == 4) {
      InfraredFormatReader ir_data = InfraredFormatReader_init(raw_array);
      sendIRSignal(ir_data);
    } else {
      Serial.println("ERROR: Packet format invalid (expected 4 comma-separated values).");
    }

    Serial.printf("内容: %s\n", g_udp.packet_buffer);
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    RssiFormatSender rssi_data = RssiFormatSender_init((float)WiFi.RSSI());
    String rssi_message = RssiFormatSender_to_string(&rssi_data);
    g_udp.udp.beginPacket(g_setting_config.remote_ip, g_udp.port);
    g_udp.udp.print(rssi_message);
    g_udp.udp.endPacket();
    

    StatusValueFormatSender status_data = StatusValueFormatSender_init((millis() / 1000) % 2 == 0);
    String status_value_message = StatusValueFormatSender_to_string(&status_data);
    g_udp.udp.beginPacket(g_setting_config.remote_ip, g_udp.port);
    g_udp.udp.print(status_value_message);
    g_udp.udp.endPacket();
  }
}