# include <Arduino.h>

// 赤外線受信データ構造体
typedef struct {
    const char* TYPE = "infrared"; 
    
    String address;
    String command;
    String protocol;

} InfraredFormatReader;

// Rssi送信データ構造体
typedef struct {
    const char* TYPE = "rssi";
    
    float rssi;

} RssiFormatSender; 

// ステータス送信データ構造体
typedef struct {
    const char* TYPE = "status-value";
    
    bool status_value;

} StatusValueFormatSender;

// 設定ファイル読み込み構造体
typedef struct {
    String ssid;
    String password;
    IPAddress ip;
    IPAddress gateway;
    IPAddress remote_ip;
    IPAddress subnet;
    IPAddress dns;
    bool success;
} SettingFile;

// udp構造体
typedef struct {
  unsigned int port;
  WiFiUDP udp;
  char packet_buffer[255];
} UdpConfig;
