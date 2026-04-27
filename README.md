# attiny85-i2cslave

### Code for I2C control of 2 t85s, used in a home made, fire hazard, linear PSU, built with a random assortment of salvaged parts. This implementation is veryyyyy use case specific

0x26 handles 1 PWM output 2 ADC inputs, with an optional and configurable current limiter (via current limiter register). The output is RC smoothed and fed into the non inverting input of an opamp (output voltage control). The ADCs are used to read the current output voltage and current (via 0.005 shunt / amplifier). The pwm value for voltage control may be altered or not written at all to output, in case the current limiter function is enabled and the device is in current limiting mode.

0x27 handles 1 PWM output and 1 ADC input. The output square wave is fed into the speed control input of an ATX CPU Fan. The ADC reads a voltage divider with an NTC temperature probe as R2.

Both ICs run their ADCs referenced to the t85 internal 1.1V voltage reference. Both are connected to an I2C bus with an ESP32, running esphome, as master. The master polls the ICs for readings periodically and writes the PWM levels (or current limit) as soon as it has that information, to the respective register.

Both Registers, I2C slave address and IC role are configurable via build flags.
