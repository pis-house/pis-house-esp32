#include <IRremote.hpp>
 
#define IR_RECEIVE_PIN 13
 
void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR Receiver ready.");
}
 
void loop() {
  if (IrReceiver.decode()) {
    
    uint8_t protocol = IrReceiver.decodedIRData.protocol;
    uint16_t address = IrReceiver.decodedIRData.address;
    uint16_t command = IrReceiver.decodedIRData.command;

    if (!(address == 0x0 && command == 0x0 && protocol == UNKNOWN)) {
      Serial.print("USB: ADDR=0x");
      Serial.print(address, HEX);
      Serial.print(", CMD=0x");
      Serial.print(command, HEX);
      Serial.print(", PROTO=");
      Serial.println(IrReceiver.getProtocolString());
    }
 
    IrReceiver.resume();
  }
}