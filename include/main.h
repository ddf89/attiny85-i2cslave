#ifndef ICCONFIG
#define ICCONFIG

#include "Arduino.h"
#include <TinyWireS.h>

#ifdef I2C_SADDR
  const byte i2cSlaveAddr = I2C_SADDR;
#else
  const byte i2cSlaveAddr = 0x20;
#endif

enum IcRole: byte {
  NONE = 0x00,
  VI_CONTROL = 0x01,
  TF_CONTROL = 0x02
};

#ifdef ROLE
  const IcRole r = static_cast<IcRole>(ROLE);
  #if (ROLE==0x01) && (WITH_CURRENT_LIMITER==0x01)
    #define CURRENT_LIMITER
    #ifdef LIMITER_REGISTER
      const byte limiterRegister = static_cast<byte>(LIMITER_REGISTER);
    #else
      const byte limiterRegister = 0x12;
    #endif
  #else
    const byte limiterRegister = 0x00;
  #endif
#else
  const IcRole r = IcRole::NONE;
#endif

#ifdef PWM_REGISTER
  const byte pwmRegister = static_cast<byte>(PWM_REGISTER);
#else
  const byte pwmRegister = 0x10;
#endif

void setup();
void loop();
void requestEvent();
void receiveEvent(uint8_t len);

#endif
