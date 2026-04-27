#include "Arduino.h"
#include <TinyWireS.h>

#ifndef ICCONFIG
#define ICCONFIG

#ifdef I2C_SLAVE_ADDR
  const byte addr = I2C_SLAVE_ADDR;
#else
  const byte addr = 0x20;
#endif

#ifdef ROLE
  const IcRole r = static_cast<IcRole>(ROLE);
  #if (ROLE==0x01) && (WITH_CURRENT_LIMITER==0x01)
    const bool currentLimiter = true;
  #else
    const bool currentLimiter = false;
  #endif
#else
  const IcRole r = IcRole::NONE;
#endif

enum IcRole : byte {
  NONE = 0x00,
  VI_CONTROL = 0x01,
  TF_CONTROL = 0x02
};

void setup();
void loop();
void requestEvent();
void receiveEvent(uint8_t len);

#endif
