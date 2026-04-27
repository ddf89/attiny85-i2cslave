#include "main.h"

volatile uint16_t adc_values[2];
volatile uint8_t pwmc;
uint8_t pwm;

void setup() {
  analogReference(INTERNAL1V1);

  pwm = 0x00;
  pwmc = 0x00;
  
  pinMode(PB1, OUTPUT);
  analogWrite(PB1, 0x00);

  pinMode(A3, INPUT);
  pinMode(A2, INPUT);

  TCCR0B &= 0b00000001;
  TCCR1 &= 0b01000001;

  TinyWireS.begin(addr);
  TinyWireS.onRequest(requestEvent); // Master wants to read
  TinyWireS.onReceive(receiveEvent);
}

void loop() {
  if (pwm != pwmc) {
    cli();
    pwm = pwmc;
    analogWrite(PB1, pwm);
    sei();
  }

  cli();
  adc_values[0] = analogRead(A3); // PB3
  adc_values[1] = analogRead(A2); // PB4
  sei();
  
  TinyWireS_stop_check(); 
}

void requestEvent() {
  // Send 4 bytes: [Ch1 High, Ch1 Low, Ch2 High, Ch2 Low]
  TinyWireS.send(highByte(adc_values[0]));
  TinyWireS.send(lowByte(adc_values[0]));
  TinyWireS.send(highByte(adc_values[1]));
  TinyWireS.send(lowByte(adc_values[1]));
}

void receiveEvent(uint8_t len) {
  if (len > 1 && TinyWireS.receive() == 0x11) {
    len--;

    for (uint8_t i = 0; i < len; i++) {
        uint8_t r = TinyWireS.receive();

        if (i == len-1) {
          pwmc = r;
          return;
        }
    }
  }
}
