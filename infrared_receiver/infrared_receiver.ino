// ESP32のTX/RXピンを任意のGPIOに再定義
const int RX_PIN = 16; // GPIO 16 (未使用ピン)
const int TX_PIN = 17; // GPIO 17 (未使用ピン)
const long BAUD_RATE = 115200;

void setup() {
  // ラズパイへの送信に使用するUART2を開始
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.println("ESP32 Serial2 Initialized.");
}

void loop() {
  // 送信するデータを作成（例: センサー値や状態）
  int sensor_value = analogRead(A0); // 例としてA0ピンの値を読む
  String data_to_send = "SensorValue:" + String(sensor_value);
  
  // データを送信し、末尾に改行コードを付ける（ラズパイ側で区切りやすいように）
  Serial2.print(data_to_send);
  Serial2.print("\n");
  
  Serial.print("Sent to RPi: ");
  Serial.println(data_to_send);

  delay(1000); // 1秒ごとに送信
}