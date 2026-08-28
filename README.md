# SHT3x-DIS BLE Sensor Peripheral (CC26x2R)

A BLE peripheral that reads temperature and humidity from an SHT3x-DIS over I2C and exposes them as GATT characteristics with notification support. Built on TI's `simple_peripheral` example for the CC26x2R LaunchPad.

Readings are taken once per second while a central is connected, and pushed to the client as notifications.

---

## Hardware

- **Board:** TI CC26x2R1 LaunchPad (LAUNCHXL-CC26X2R1)
- **Sensor:** Sensirion SHT3x-DIS (I2C, address `0x44`)

### Wiring

| SHT3x pin | LaunchPad pin | Notes |
|---|---|---|
| VDD | 3V3 | Do not use 5V |
| GND | GND | |
| SDA | *DIO5* | |
| SCL | *DIO4* | |


Pull-up resistors: most SHT3x breakout boards include them. If yours does not, add 4.7 kΩ from SDA and SCL to 3V3. The internal pull-ups on the CC26x2 are weak (20–40 kΩ) and are unreliable with longer wiring.

The board can run standalone from any USB power source — the firmware is stored in flash and does not require a debugger connection.

---

## Software requirements

- Code Composer Studio (Theia or Eclipse)
- SimpleLink CC13xx/CC26xx SDK **version `X8.33.00.16`**


---

## Build instructions

1. Import the `simple_peripheral` example from the SDK into your workspace.
2. Copy the files from this repository into the project:
   - `Application/sensor.c`, `sensor.h`
   - `Application/i2c_bus.c`, `i2c_bus.h`
   - `Application/simple_peripheral.c` *(replaces the SDK original)*
   - `Profiles/sensor_gatt_profile.c`, `sensor_gatt_profile.h`
3. Add the include paths under **Project Properties → Build → ARM Compiler → Include Options**:
   - `${PROJECT_ROOT}/Application`
   - `${PROJECT_ROOT}/Profiles`
4. In SysConfig, add an I2C instance named `CONFIG_I2C_0` and assign the SDA/SCL pins.
5. Build and flash.

### Changes to `simple_peripheral.c`

The copy in this repository is TI's example with seven additions. If you would rather
apply them by hand to a clean copy than overwrite the file:

- `SP_SENSOR_EVT` application event ID (value `10`, alongside the existing event defines)
- `clkSensor` clock instance and its `argSensor` event data struct
- A `Util_constructClock` call for `clkSensor` in `SimplePeripheral_init` (1 s period, not auto-started)
- `i2c_bus_init()` and `SensorProfile_AddService()` calls in `SimplePeripheral_init`
- An `SP_SENSOR_EVT` branch in `SimplePeripheral_clockHandler`
- An `SP_SENSOR_EVT` case in `SimplePeripheral_processAppMsg` — this is where the sensor is actually read
- `Util_startClock` / `Util_stopClock` on `GAP_LINK_ESTABLISHED_EVENT` and `GAP_LINK_TERMINATED_EVENT`

---

## BLE interface

**Service UUID:** `0xFFF7`

| UUID | Name | Properties | Length | Format |
|---|---|---|---|---|
| `0xFFF6` | Temperature | Read, Notify | 2 bytes | `int16`, °C × 100, little-endian |
| `0xFFF8` | Humidity | Read, Notify | 2 bytes | `uint16`, %RH × 100, little-endian |

Both characteristics carry a Characteristic User Description descriptor (`"Temperature"` / `"Humidity"`) and a CCCD for enabling notifications.

### Decoding

```
temperature_celsius = int16_le(bytes) / 100.0
humidity_percent    = uint16_le(bytes) / 100.0
```

Example: bytes `4C 09` → `0x094C` = 2380 → **23.80 °C**

Temperature is signed to allow sub-zero readings. Humidity is unsigned.

### Testing with nRF Connect

1. Scan and connect to the device.
2. Expand service `0xFFF7`. It will show as "Unknown Service" — `0xFFF0`–`0xFFFF` is the range reserved for custom use, so it is not in the Bluetooth SIG registry.
3. Tap the notification icon on each characteristic to subscribe.
4. Values update once per second. Breathe on the sensor to confirm humidity responds.

---

## Architecture

```
┌──────────────────────────────────────────────┐
│  simple_peripheral.c                         │
│  timing, event dispatch, GATT wiring         │
├──────────────────────────────────────────────┤
│  sensor_gatt_profile.c                       │
│  attribute table, CCCD handling, notify      │
├──────────────────────────────────────────────┤
│  sensor.c        │  (future drivers)         │
│  SHT3x protocol  │                           │
├──────────────────────────────────────────────┤
│  i2c_bus.c                                   │
│  owns the I2C peripheral and handle          │
└──────────────────────────────────────────────┘
```

Each layer only depends on the one below it. `sensor.c` has no knowledge of BLE; `i2c_bus.c` has no knowledge of which devices sit on the bus. Adding a second I2C device means adding a driver alongside `sensor.c`, not modifying it.

### Sampling flow

```
clkSensor fires (1 Hz)                          [SWI context]
   ↓
SimplePeripheral_clockHandler
   ↓  posts SP_SENSOR_EVT to the app queue
Event_pend returns                              [Task context]
   ↓
SimplePeripheral_processAppMsg → case SP_SENSOR_EVT
   ↓
sensor_read()
   ├─ I2C write 0x2400  (single-shot, no clock stretching)
   ├─ Task_sleep(20 ms) (conversion time)
   ├─ I2C read 6 bytes
   └─ convert to float
   ↓
SensorProfile_SetParameter() ×2
   ↓
GATTServApp_ProcessCharCfg → notification to subscribed clients
```

The timer callback runs in SWI context and cannot block, so it only posts a message. All blocking work — the I2C transfers and the conversion delay — happens in task context after the queue wakes the application task.

The sampling clock is started on `GAP_LINK_ESTABLISHED_EVENT` and stopped on `GAP_LINK_TERMINATED_EVENT`; the sensor is not polled while no central is connected.

### Sensor protocol

The SHT3x is driven with command `0x2400` (single-shot, high repeatability, clock stretching disabled). Clock stretching would hold SCL low for the full ~15 ms conversion, blocking the bus for any other device; disabling it means the wait is handled in software instead, which is why the read is split into two separate transactions.

---

## Known limitations

- **CRC is not verified.** The sensor returns a CRC-8 byte after each measurement (`readbuf[2]` and `readbuf[5]`); these are currently discarded. A corrupted transfer will produce a plausible-looking but wrong reading with no way to detect it.
- **I2C is blocking.** `sensor_read` occupies the application task for roughly 23 ms per cycle. Harmless at 1 Hz with a single device; would need converting to callback mode before adding a second sensor or raising the sample rate.
- **No power measurement or optimisation.** Advertising interval, connection interval, and sensor repeatability mode are all left at defaults.
- **Sampling interval is fixed** at 1 s and cannot be changed at runtime.
- **No error reporting.** A failed read is silently skipped; the previous value stays in the characteristic.

---

## Possible next steps

- CRC-8 verification (polynomial `0x31`, init `0xFF`)
- A writable characteristic to set the sampling interval from the client
- Callback-mode I2C with a state machine, removing the blocking wait
- Flash logging via the NVS driver, with history dump over BLE
- A second I2C device sharing the bus

---

## License

Files written for this project are released under the MIT License — see `LICENSE`.

`Application/simple_peripheral.c` is a modified Texas Instruments example file. It is not covered by the MIT license and remains subject to TI's own license terms, which are reproduced in full in the header of that file.
