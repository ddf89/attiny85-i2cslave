#ifndef ICCONFIG
#define ICCONFIG

#include "Arduino.h"
#include <TinyWireS.h>

#ifdef I2C_SADDR
  const byte i2cSlaveAddr = I2C_SADDR;
#else
  const byte i2cSlaveAddr = 0x20;
#endif

#if i2cSlaveAddr==0x26 && defined(WITH_LIMITER_REGISTER)
    #define CURRENT_LIMITER
    const byte limiterRegister = static_cast<byte>(LIMITER_REGISTER);
    const byte constantRegister = static_cast<byte>(CONSTANT_CURRENT_REGISTER);
    const bool currentlimitEnabled = true;
#endif

#ifdef PWM_REGISTER
  const byte pwmRegister = static_cast<byte>(PWM_REGISTER);
#else
  const byte pwmRegister = 0x10;
#endif

#ifdef WITH_TELEMETRY_REGISTER
const byte telemetryEnableRegister = static_cast<byte>(WITH_TELEMETRY_REGISTER);
#endif

void setup();
void loop();
void requestEvent();
void receiveEvent(uint8_t len);

#endif
