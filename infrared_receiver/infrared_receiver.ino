// #include <IRremote.hpp>  // IRremoteライブラリ
// #define IR_RECEIVE_PIN 13 // GPIOピン番号を定義

// void setup() {
//   Serial.begin(115200);  // シリアル通信の初期化
//   IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
//   delay(1000);
  
//   Serial.println("Setup Complete");
// }

// void loop() {
//   if (IrReceiver.decode()) {  // 信号を受信したら
//     Serial.print("Received Raw Data: ");
//     Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);  // 生のデータを16進数で表示

//     IrReceiver.printIRResultShort(&Serial);  // 受信したデータの簡潔な概要を表示
//     IrReceiver.printIRSendUsage(&Serial);    // 受信した信号を送信するためのコードを表示

//     IrReceiver.resume();  // 次の信号を受信できるようにする
//   }
// }


#include <Arduino.h>
#include <IRremote.hpp>

// 送信ピン（ESP32や使用するボードに合わせて変更してください）
#define IR_SEND_PIN 4

void setup() {
    Serial.begin(115200);
    IrSender.begin(IR_SEND_PIN);
    Serial.println(F("Setup Complete."));
}

void loop() {
    Serial.println(F("Sending IR signal (0x353C09522C)..."));

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

    delay(1000);
}