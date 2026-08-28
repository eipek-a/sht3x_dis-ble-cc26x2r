# SHT3x-DIS BLE Sensor Peripheral (CC26x2R)

A BLE peripheral that samples temperature and humidity from a Sensirion SHT3x-DIS over I2C and exposes them as GATT characteristics with notification support. Built on TI's `simple_peripheral` example for the CC26x2R LaunchPad.

While a central is connected, readings are taken once per second and pushed to the client as notifications. The sampling clock stops when the link drops.

**What this project involved:** writing an I2C sensor driver against the datasheet, defining a custom GATT service with notify characteristics and CCCDs, and fitting blocking sensor work into TI-RTOS without disturbing the BLE stack's timing. The layering is deliberate — the sensor driver knows nothing about BLE, and the bus layer knows nothing about which devices sit on it.






## Hardware

- **Board:** TI CC26x2R1 LaunchPad (LAUNCHXL-CC26X2R1)
- **Sensor:** Sensirion SHT3x-DIS, I2C address `0x44` (ADDR pin tied low)

### Wiring

| SHT3x pin | LaunchPad pin | Notes |
|---|---|---|
| VDD | 3V3 | Not 5V — the SHT3x is a 3.3 V part |
| GND | GND | |
| SDA | DIO5 | |
| SCL | DIO4 | |

DIO4 and DIO5 are the pins assigned in the bundled `.syscfg`. Any I2C-capable DIO will work if you reassign them in SysConfig — the code refers to the instance by name (`CONFIG_I2C_0`), not by pin.

**Pull-ups:** most SHT3x breakout boards include them. If yours does not, add 4.7 kΩ from SDA and SCL to 3V3. The CC26x2's internal pull-ups are weak (20–40 kΩ) and unreliable with anything longer than a few centimetres of wire.

The board runs standalone from any USB power source once flashed; no debugger connection is required.

<img width="650" alt="LaunchPad wired to SHT3x breakout" src="https://github.com/user-attachments/assets/f0a2dc9a-a462-48ff-a6af-a38f314d4e76" />

---

## Repository layout

```
Application/
  sensor.c              SHT3x protocol — command, timing, conversion
  sensor.h
  i2c_bus.c             Owns the I2C peripheral and handle
  i2c_bus.h
  simple_peripheral.c   TI example + this project's additions
Profiles/
  sensor_gatt_profile.c Attribute table, CCCD handling, notify
  sensor_gatt_profile.h
sysconfig/
  simple_peripheral.syscfg
docs/
  (screenshots)
LICENSE
README.md
```

---

## Software requirements

- Code Composer Studio (Theia or Eclipse)
- SimpleLink CC13xx/CC26xx SDK — **version `8.33.00.16`**

The SDK version matters. `simple_peripheral` changed shape between major SDK releases and the file in this repository will not drop cleanly into a different one.

---

## Build instructions

1. Import the `simple_peripheral` example from the SDK into your workspace.
2. Copy these files from this repository into the project, replacing what is there:
   - `Application/sensor.c`, `sensor.h`
   - `Application/i2c_bus.c`, `i2c_bus.h`
   - `Application/simple_peripheral.c` *(replaces the SDK original)*
   - `Profiles/sensor_gatt_profile.c`, `sensor_gatt_profile.h`
   - `sysconfig/simple_peripheral.syscfg` *(replaces the SDK original)*
3. Add the include paths under **Project Properties → Build → ARM Compiler → Include Options**:
   - `${PROJECT_ROOT}/Application`
   - `${PROJECT_ROOT}/Profiles`
4. Build and flash.

The `.syscfg` file already contains the `CONFIG_I2C_0` instance with SDA on DIO5 and SCL on DIO4, so no manual SysConfig work is needed. If you prefer to configure it by hand instead of replacing the file, add an I2C instance named `CONFIG_I2C_0` and assign the two pins yourself.

### Changes to `simple_peripheral.c`

The copy here is TI's example with seven additions. To apply them by hand to a clean copy instead of overwriting:

- `SP_SENSOR_EVT` application event ID (value `10`, alongside the existing event defines)
- `clkSensor` clock instance and its `argSensor` event data struct
- A `Util_constructClock` call for `clkSensor` in `SimplePeripheral_init` (1 s period, not auto-started)
- `i2c_bus_init()` and `SensorProfile_AddService()` calls in `SimplePeripheral_init`
- An `SP_SENSOR_EVT` branch in `SimplePeripheral_clockHandler`
- An `SP_SENSOR_EVT` case in `SimplePeripheral_processAppMsg` — this is where the sensor is actually read
- `Util_startClock` / `Util_stopClock` on `GAP_LINK_ESTABLISHED_EVENT` and `GAP_LINK_TERMINATED_EVENT`

---

## Serial output

The firmware logs each reading over the LaunchPad's Application/User UART via TI's `Display` driver, so you can confirm it works without a phone.

Open the XDS110 Application/User UART port at **115200 8N1**:

- Linux: `/dev/ttyACM0`
- macOS: `/dev/tty.usbmodem*`
- Windows: check Device Manager for "XDS110 Class Application/User UART"

Expected output once per second while connected:

```
====================
Initialized


Adv Set 1 Enabled

ID Addr: 0xB010A0002D33
RP Addr: 0x6D7B454FA751
====================
Connected to 0x697390D03BE4
Link Param Updated: 0x697390D03BE4

Adv Set 0 Enabled

ID Addr: 0xB010A0002D33
RP Addr: 0x4A785A316D51
T = 25.12 C   RH = 41.54 %

```

### UART output

<img width="650" alt="UART console showing one reading per second" src="https://github.com/user-attachments/assets/d36fdbce-b362-40cc-a2d5-5a64f8291c44" />
Values only appear while a central is connected, since the sampling clock is stopped otherwise.

---

## BLE interface

### BLE notifications in nRF Connect

<img width="280" alt="Temperature characteristic" src="https://github.com/user-attachments/assets/774388e6-cd98-49c1-aefa-7e0f8b69c66b" />
<img width="280" alt="Humidity characteristic" src="https://github.com/user-attachments/assets/72e66f16-a917-47e6-ade2-9503431c248f" />

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

Temperature is signed so sub-zero readings work. Humidity is unsigned.

### Testing with nRF Connect

1. Scan and connect to the device.
2. Expand service `0xFFF7`. It shows as "Unknown Service" — `0xFFF0`–`0xFFFF` is the range reserved for custom use, so it is not in the Bluetooth SIG registry.
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

Each layer depends only on the one below it. `sensor.c` has no knowledge of BLE; `i2c_bus.c` has no knowledge of which devices sit on the bus. A second I2C device is added as a sibling driver next to `sensor.c`, not by modifying it.

That is the structural picture. There is a separate, timing-related caveat about second devices — see Known limitations.

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

The SHT3x is driven with command `0x2400` — single-shot, high repeatability, clock stretching disabled.

Clock stretching would have the sensor hold SCL low for the full ~15 ms conversion, blocking the bus for every other device on it. Disabling it means the wait is handled in software instead, which is why the read is split into two separate transactions with a `Task_sleep` between them. The sleep is 20 ms, giving margin over the datasheet's 15 ms worst case for high repeatability.

---

## Known limitations

- **CRC is not verified.** The sensor returns a CRC-8 byte after each measurement (`readbuf[2]` and `readbuf[5]`); these are currently discarded. A corrupted transfer produces a plausible-looking but wrong reading with no way to detect it.
- **I2C is blocking.** `sensor_read` occupies the application task for roughly 23 ms per cycle, almost all of it the conversion sleep. Harmless at 1 Hz with a single device. Adding a second sensor is structurally easy (see Architecture) but would push total task occupancy up; past a few devices, or at higher sample rates, this needs converting to callback mode.
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

`Application/simple_peripheral.c` is a modified Texas Instruments example file. It is not covered by the MIT license and remains subject to TI's own license terms, reproduced in full in the header of that file.
