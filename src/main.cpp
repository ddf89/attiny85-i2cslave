#include "main.h"

volatile uint16_t adcBuffer[2];
volatile uint8_t pwmRegVal = 0x00;
uint8_t pwmOutVal = 0x00;

#ifdef CURRENT_LIMITER
volatile uint16_t currentlimitRegVal = 0x00;
volatile bool constantCurrentRegValEnabled = false;
volatile bool updateFlag = false;
#endif

#ifdef WITH_TELEMETRY
volatile bool telemetryFlag = false;
#endif

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
  uint16_t a3Reading = analogRead(A3);
  adcBuffer[0] = a3Reading;

  #ifdef CURRENT_LIMITER
  if ((currentlimitEnabled || constantCurrentRegValEnabled) && currentlimitRegVal > 0 && pwmOutVal > 0) {
    if (a3Reading > (currentlimitRegVal + 5)) {
      pwmRegVal = pwmOutVal - 1;
    } else if (constantCurrentRegValEnabled && (a3Reading + 5) < currentlimitRegVal) {
      pwmRegVal = pwmOutVal + 1;
    }
  }

  if (pwmOutVal != pwmRegVal || updateFlag) {
    updateFlag = false;
  #else
  if (pwmOutVal != pwmRegVal) {
  #endif
    pwmOutVal = pwmRegVal;
    analogWrite(PB1, pwmOutVal);
  }

  sei();
  
  TinyWireS_stop_check(); 
}

void requestEvent() {
  #ifdef WITH_TELEMETRY
  if (telemetryFlag) {
    telemetryFlag = false;

    TinyWireS.send(pwmOutVal);

    #ifdef CURRENT_LIMITER
    if (!constantCurrentRegValEnabled) {
      TinyWireS.send(0x00);
    } else {
      TinyWireS.send(0xff);
    }

    TinyWireS.send(highByte(currentlimitRegVal));
    TinyWireS.send(lowByte(currentlimitRegVal));
    #endif
    return;
  }
  #endif

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
      }
      #if CURRENT_LIMITER
      else if (currentlimitEnabled && reg == limiterRegister) {
        currentlimitRegVal = uint16_t(r / 255.0 * 1023.0);
        updateFlag = true;
      } else if (currentlimitEnabled && reg == constantRegister) {
        if (r > 0x00) {
          constantCurrentRegValEnabled = true;
        } else {
          constantCurrentRegValEnabled = false;
        }
        updateFlag = true;
      }
      #endif
      #ifdef WITH_TELEMETRY_REGISTER
      else if (reg == telemetryEnableRegister) {
        if (r > 0x00) {
          telemetryFlag = true;
        } else {
          telemetryFlag = false;
        }
      }
      #endif

      return;
    }
  }
}
