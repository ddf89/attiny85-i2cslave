#include "main.h"

#ifdef CURRENT_LIMITER
const bool currentlimitEnabled = true;
#else
const bool currentlimitEnabled = false;
#endif

volatile uint16_t adcBuffer[2];
volatile uint8_t pwmRegVal = 0x00;
uint8_t pwmOutVal = 0x00;
volatile uint16_t currentlimitRegVal = 0x00;
volatile bool updateFlag = false;

void setup() {
  analogReference(INTERNAL1V1);
  
  pinMode(PB1, OUTPUT);
  analogWrite(PB1, 0x00);

  pinMode(A3, INPUT);
  pinMode(A2, INPUT);

  TCCR0B &= 0b00000001;
  TCCR1 &= 0b01000001;

  TinyWireS.begin(i2cSlaveAddr);
  TinyWireS.onRequest(requestEvent); // Master wants to read
  TinyWireS.onReceive(receiveEvent);
}

void loop() {
  cli();

  adcBuffer[1] = analogRead(A2);
  uint16_t currentReading = analogRead(A3);
  adcBuffer[0] = currentReading;

  if (currentlimitEnabled && currentlimitRegVal > 0 && pwmOutVal > 0 && currentReading > (currentlimitRegVal + 10)) {
    pwmRegVal = pwmOutVal - 1;
  }

  if (pwmOutVal != pwmRegVal || updateFlag) {
    updateFlag = false;
    pwmOutVal = pwmRegVal;
    analogWrite(PB1, pwmOutVal);
  }

  sei();
  
  TinyWireS_stop_check(); 
}

void requestEvent() {
  // Send 4 bytes: [Ch1 High, Ch1 Low, Ch2 High, Ch2 Low]
  TinyWireS.send(highByte(adcBuffer[0]));
  TinyWireS.send(lowByte(adcBuffer[0]));
  TinyWireS.send(highByte(adcBuffer[1]));
  TinyWireS.send(lowByte(adcBuffer[1]));
}

void receiveEvent(uint8_t len) {
  if (len < 2) return;

  uint8_t reg = TinyWireS.receive();
  
  len--;

  for (uint8_t i = 0; i < len; i++) {
    uint8_t r = TinyWireS.receive();

    if (i == len-1) {
      if (reg == pwmRegister) {
        pwmRegVal = r;
      } else if (currentlimitEnabled && reg == limiterRegister) {
        currentlimitRegVal = uint16_t(r / 255.0 * 1023.0);
        updateFlag = true;
      }

      return;
    }
  }
}
