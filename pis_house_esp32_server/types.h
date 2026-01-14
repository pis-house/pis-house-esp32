# include <Arduino.h>

// 赤外線受信データ構造体
typedef struct {
    const char* TYPE = "infrared"; 
    
    String address;
    String command;
    String custom_process;
    String protocol;

} InfraredFormatReader;

// Rssi送信データ構造体
typedef struct {
    const char* TYPE = "rssi";
    
    float rssi;

} RssiFormatSender; 

// ステータス送信データ構造体
typedef struct {
    const char* TYPE = "ble-presence";
    
    bool ble_presence;

} BLEPresenceFormatSender;

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

struct RssiResult {
  bool matched;
  int rawRssi;
  float avgRssi;
  float estCm;
  int samples;
};

class RssiProcessor {
    private:
        int rssiBuf[8];  // static 変数からメンバ変数へ
        int bufIndex = 0;    // static 変数からメンバ変数へ
        int sampleCount = 0; // static 変数からメンバ変数へ
        
        float estimateDistanceCm(float avgRssi, float txPowerDbm, float pathLossExp);

    public:
        RssiProcessor() { initBuffer(); } // コンストラクタで初期化
        void initBuffer();
  
        RssiResult processAdvertisedDeviceForRssi(BLEAdvertisedDevice &dev,
            const String &expectedMac,
            float txPowerDbm = -65.0,
            float pathLossExp = 3.0);
};
