# attiny85-i2cslave

### ATtiny85 firmware for I2C-controlled PWM and ADC — part of a homemade, definitely-a-fire-hazard linear PSU cobbled together from salvaged parts. Extremely use-case specific.

**0x26** handles one PWM output and two ADC inputs, with an optional software current limiter configurable via I2C register. The PWM output is RC-smoothed and fed into the non-inverting input of an op-amp to control output voltage. The two ADC channels read output voltage and load current (via a 0.005Ω shunt and amplifier). When the current limiter is active and the measured current exceeds the setpoint, the IC will back off the PWM value autonomously — the written PWM target may be ignored or undercut until the current drops back into range.

**0x27** handles one PWM output and one ADC input. The output is an unfiltered square wave fed directly into the PWM speed-control input of an ATX CPU fan. The ADC reads a voltage divider with an NTC thermistor as the lower leg.

Both ICs use the ATtiny85's internal 1.1V reference for ADC measurements, and both sit on an I2C bus mastered by an ESP32 running ESPHome. The master polls each IC for ADC readings periodically and pushes PWM or current-limit values to the appropriate register as needed.

The I2C address, register addresses, and IC role are all configurable at compile time via build flags.

---

## System Overview

```
                          I2C Bus
          ┌───────────────────────────────────┐
          │                                   │
   ┌──────┴──────┐                   ┌────────┴───────┐
   │  ESP32      │                   │  ATtiny85      │
   │  (ESPHome)  │                   │  0x27          │
   │  I2C Master │                   │  Fan/Temp Ctrl │
   └──────┬──────┘                   │                │
          │                          │  PB1 ──► Fan   │
          │                          │        PWM In  │
          │                          │  A3 ◄── NTC    │
          │                          │        Divider │
          │                          └────────────────┘
          │
          │                          ┌────────────────────────────┐
          │                          │  ATtiny85 0x26             │
          └──────────────────────────┤  Voltage/Current Control   │
                                     │                            │
                                     │  PB1 ──► RC Filter         │
                                     │          └──► OpAmp+ ──►   │
                                     │               Vout control │
                                     │  A3 ◄── Current sense      │
                                     │         (shunt + amp)      │
                                     │  A2 ◄── Output voltage     │
                                     └────────────────────────────┘
```

---

## Build Environments

The project defines two PlatformIO environments in `platformio.ini`, one per ATtiny85:

| Environment     | Address | Role                          |
|-----------------|---------|-------------------------------|
| `t85VICONTROL`  | `0x26`  | Output voltage & current ctrl |
| `t85TFCONTROL`  | `0x27`  | Fan speed & temperature read  |

Build a specific environment:

```bash
pio run -e t85VICONTROL
pio run -e t85TFCONTROL
```

Upload:

```bash
pio run -e t85VICONTROL --target upload
pio run -e t85TFCONTROL --target upload
```

---

## Build Flags Reference

| Flag                      | Description                                                                 |
|---------------------------|-----------------------------------------------------------------------------|
| `I2C_SADDR`               | I2C slave address (default: `0x20`)                                         |
| `PWM_REGISTER`            | Register address for PWM duty cycle writes (default: `0x10`)                |
| `WITH_LIMITER_REGISTER`   | Enable current limiter feature (0x26 only). Value is the register address.  |
| `LIMITER_REGISTER`        | Register address for current limit threshold writes                         |
| `CONSTANT_CURRENT_REGISTER` | Register address to enable/disable constant current mode                  |
| `WITH_TELEMETRY`          | Enable telemetry read mode (compile-time flag)                              |
| `WITH_TELEMETRY_REGISTER` | Register address to trigger a telemetry read on next request                |

The current limiter feature only compiles in when `I2C_SADDR == 0x26` and `WITH_LIMITER_REGISTER` is defined.

---

## I2C Protocol

### Write (master → slave)

Minimum 2 bytes: `[register, value]`

The last byte in the payload is always the value written to the register. Any bytes between the register and the final value byte are discarded.

### Read (master ← slave)

Returns 4 bytes in big-endian format:

**Normal mode (both ICs):**
```
[ADC_Ch1_High, ADC_Ch1_Low, ADC_Ch2_High, ADC_Ch2_Low]
```

**Telemetry mode (0x26 only, when telemetry flag is set):**
```
[pwmOutVal, CC_mode_byte, currentlimit_High, currentlimit_Low]
```
Where `CC_mode_byte` is `0xFF` if constant current mode is active, `0x00` otherwise. The telemetry flag is consumed on read and must be re-set for the next telemetry response.

---

## Register Map

### 0x26 — Voltage/Current Control

| Register            | Direction | Description                                                        |
|---------------------|-----------|--------------------------------------------------------------------|
| `PWM_REGISTER`      | W         | PWM duty cycle (0–255). May be overridden by current limiter.     |
| `LIMITER_REGISTER`  | W         | Current limit threshold (0–255, mapped internally to 0–1023 ADC). |
| `CONSTANT_CURRENT_REGISTER` | W | Write `>0` to enable CC mode, `0` to disable.                   |
| `WITH_TELEMETRY_REGISTER`   | W | Write `>0` to request telemetry on the next read.               |

**ADC channels (read):**
- `adcBuffer[0]` — A3 (PB3): current sense via 0.005Ω shunt + amplifier
- `adcBuffer[1]` — A2 (PB4): output voltage sense

### 0x27 — Fan/Temperature Control

| Register       | Direction | Description                                |
|----------------|-----------|--------------------------------------------|
| `PWM_REGISTER` | W         | Fan PWM duty cycle (0–255, square wave).   |

**ADC channels (read):**
- `adcBuffer[0]` — A3 (PB3): NTC thermistor voltage divider
- `adcBuffer[1]` — A2 (PB4): unused / reads 0

---

## Current Limiter (0x26 only)

When `CURRENT_LIMITER` is compiled in, the main loop runs a software control loop every iteration:

- If the A3 ADC reading (current sense) exceeds `currentlimitRegVal + 5`, the PWM output is decremented by 1 (limit mode — protects against overcurrent).
- If constant current mode is enabled and the reading is more than 5 counts below the setpoint, PWM is incremented by 1 (regulation mode — maintains target current).

The threshold written by the master is an 8-bit value (0–255) that is scaled to the 10-bit ADC range (0–1023) using: `threshold = (r / 255.0) * 1023.0`.

This means the master can write a single byte and the IC handles the ADC scaling internally.

---

## Timer / PWM Frequency

Both setups modify the timer prescalers on startup to run PWM at maximum frequency, reducing audible noise and improving RC filter response:

```cpp
TCCR0B &= 0b00000001; // Timer0: prescaler /1
TCCR1  &= 0b01000001; // Timer1: prescaler /1
```

On an ATtiny85 at 8 MHz this results in approximately **31.4 kHz** PWM on PB1.

---

## Dependencies

- **[TinyWireSio](https://registry.platformio.org/libraries/nickcengel/TinyWireSio)** — I2C slave library for ATtiny85 (`nickcengel/TinyWireSio`)

---

## Development Environment

A devcontainer configuration is included (`.devcontainer/`) for VS Code. It provides a Debian Bookworm container with PlatformIO pre-installed and device passthrough for direct programmer access.

The container runs privileged with `/dev/` bind-mounted so that USB programmers (USBasp, etc.) are accessible from inside the container without host-side setup beyond Docker.

To use it: open the repo in VS Code and select **"Reopen in Container"** when prompted.
