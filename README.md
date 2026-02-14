# Radar Gizmo

A compact mmWave presence detection dashboard built on the ESP32-C3 Super Mini. Uses a 24GHz LD2410B radar sensor and a 2.0" TFT display with integrated rotary encoder for a self-contained, interactive radar device.

Available as bare-metal firmware (Arduino/PlatformIO) or as an ESPHome device with full Home Assistant integration.

## Table of Contents

- [Features](#features)
- [Hardware](#hardware)
- [Pin Mapping](#pin-mapping)
- [Firmware Versions](#firmware-versions)
- [Breadboard Prototype](#breadboard-prototype)
- [Flashing & Setup](#flashing--setup)
- [Project Structure](#project-structure)

---

## Features

**Display UI**
- **Dashboard View** — Live presence status, target distances (moving + stationary), energy level bars, animated radar sweep
- **Engineering View** — Raw sensor data readout (distances, energy percentages, firmware version, network info)
- Rotary encoder and K0 button to switch between views
- Double-buffered sprite rendering (flicker-free, 20fps)

**Sensor**
- LD2410B 24GHz mmWave radar — detects presence, movement, and distance through walls, glass, and plastic enclosures
- Separate moving and stationary target tracking with energy levels

**ESPHome Extras** (Home Assistant version only)
- All sensor data auto-discovered in Home Assistant
- Send messages to the display from HA automations (`esphome.radar_gizmo_show_message`)
- Display backlight exposed as a dimmable light entity
- Radar configuration controls (timeout, gate distances, engineering mode)

---

## Hardware

### Bill of Materials

| Component | Model | Spec |
|-----------|-------|------|
| MCU | ESP32-C3 Super Mini | RISC-V, 160MHz, 400KB SRAM, WiFi/BLE |
| Radar Sensor | HiLink LD2410B | 24GHz mmWave, UART 256000 baud |
| Display Module | baishundianzi 2.0" TFT + EC11 | ST7789, 320x240 RGB, SPI, 65x42mm |
| Input | EC11 Rotary Encoder (on display PCB) | Quadrature A/B + Push button |
| Extra Button | K0 (on display PCB) | Momentary switch |

### Display Module PCB

The display and encoder are integrated on a single 65mm x 42mm PCB (baishundianzi 2.0-inch model). The PCB exposes a single 12-pin header:

```
GND | VDD | SCL | SDA | RES | DC | CS | BLK | A | B | PUSH | K0
```

- **BLK** is fully enabled by default on the PCB — can be left unconnected if no brightness control is needed
- **K0** is an independent momentary button on the PCB (labeled KEY0)

### ESP32-C3 Super Mini Pinout

```
              ┌─────────┐
              │  USB-C  │
      GPIO5 ──┤ o     o ├── 5V
      GPIO6 ──┤ o     o ├── GND
      GPIO7 ──┤ o     o ├── 3V3
 (LED) GPIO8 ──┤ o     o ├── GPIO4
(BOOT) GPIO9 ──┤ o     o ├── GPIO3
     GPIO10 ──┤ o     o ├── GPIO2
     GPIO20 ──┤ o     o ├── GPIO1
     GPIO21 ──┤ o     o ├── GPIO0
              └─────────┘
```

### Strapping Pin Warning

GPIO 2 (encoder push) and GPIO 9 (K0 button) are strapping pins. If either is held LOW during power-on or USB plug-in, the board enters download mode (black screen). Press reset or re-plug to recover.

---

## Pin Mapping

### Complete Wiring Reference

| Signal | ESP32 GPIO | PCB Pin | Function |
|--------|-----------|---------|----------|
| SPI Clock | 4 | SCL | TFT display clock |
| SPI MOSI | 6 | SDA | TFT display data |
| TFT Reset | 3 | RES | Display reset |
| TFT DC | 5 | DC | Data/command select |
| TFT CS | 7 | CS | Chip select (active LOW) |
| Backlight | 10 | BLK | Display backlight (PWM capable) |
| Encoder A | 1 | A | Rotary encoder phase A |
| Encoder B | 0 | B | Rotary encoder phase B |
| Encoder Push | 2 | PUSH | Encoder button (strapping pin) |
| K0 Button | 9 | K0 | Extra button (strapping pin) |
| Radar RX | 20 | — | LD2410B TX → ESP32 RX |
| Radar TX | 21 | — | ESP32 TX → LD2410B RX |
| Onboard LED | 8 | — | Blue LED, active LOW |

---

## Firmware Versions

Three implementations are maintained at feature parity for display and sensor behavior.

### Bare-Metal Firmware (`src/main.cpp`)

The primary source of truth. Single-file C++ firmware using TFT_eSPI, LD2410, and RotaryEncoder libraries. Sprite double-buffered rendering, non-blocking input handling, no WiFi overhead.

**Best for:** Lowest latency, no network dependency, standalone operation.

**Dependencies:**
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) by Bodmer
- [ld2410](https://github.com/ncmreynolds/ld2410) by ncmreynolds
- [RotaryEncoder](https://github.com/mathertel/RotaryEncoder) by Matthias Hertel

### Arduino IDE Version (`arduino/RadarGizmo/RadarGizmo.ino`)

Identical code to `src/main.cpp`, packaged as an Arduino sketch with setup instructions in the header comment (library installs, TFT_eSPI `User_Setup.h` configuration).

**Best for:** Users who prefer the Arduino IDE workflow.

**Arduino IDE Settings:**
- **Board:** ESP32C3 Dev Module
- **USB CDC On Boot:** Enabled (required for Serial Monitor)
- **Flash Mode:** DIO
- **JTAG Adapter:** Integrated USB JTAG

### ESPHome Version (`esphome/radar-gizmo.yaml`)

Full ESPHome configuration with the same display views reimplemented in display lambdas. Adds Home Assistant integration.

**Best for:** Home Assistant users who want sensor data in HA and the ability to send messages to the display.

**ESPHome-exclusive features:**
- All LD2410 sensors as HA entities (presence, distances, energy levels)
- `esphome.radar_gizmo_show_message` service — push text to the display with optional auto-clear duration
- `esphome.radar_gizmo_clear_message` service — dismiss displayed message
- Display backlight as a dimmable HA light entity
- Radar config controls (timeout, gate distances, engineering mode, Bluetooth toggle)
- WiFi signal strength sensor
- OTA updates from ESPHome dashboard

---

## Breadboard Prototype

### Parts List

- 1x 400-pin half-size breadboard (30 rows)
- 1x ESP32-C3 Super Mini
- 1x baishundianzi 2.0" TFT+EC11 display module (12-pin header)
- 1x LD2410B radar sensor module
- 16x jumper wires

### Layout Diagram

The ESP32-C3 straddles the center gap at the top of the breadboard with USB-C hanging off the top edge. Power and ground pins are on the right side. The LD2410B sits in the middle. The display module header plugs into the bottom rows.

```
        (-)  (a)(b)(c)(d)(e)  (f)(g)(h)(i)(j)  (+)
         ═══════════════════════════════════════════
  Row 1  ─ ─  ─── ESP32-C3 Super Mini ─────────  ─ ─    ← USB-C hangs off top edge
  Row 2  ─ ─  [GP5] . . . .  [ 5V] . . . .     ─ ─
  Row 3  ─ ─  [GP6] . . . .  [GND] . . ─┐►     (-)    GND → (-) rail
  Row 4  ─ ─  [GP7] . . . .  [3V3] . . ─┘►     (+)    3V3 → (+) rail
  Row 5  ─ ─  [GP8] . . . .  [GP4] . . . .     ─ ─
  Row 6  ─ ─  [GP9] . . . .  [GP3] . . . .     ─ ─
  Row 7  ─ ─  [G10] . . . .  [GP2] . . . .     ─ ─
  Row 8  ─ ─  [G20] . . . .  [GP1] . . . .     ─ ─
  Row 9  ─ ─  [G21] . . . .  [GP0] . . . .     ─ ─
         ═══════════════════════════════════════════
  Row 10 ─ ─  . . . . . . .  . . . . . . .     ─ ─    (spacer)
         ─────────────────────────────────────────────
         ── LD2410B ──────────────────────────────────
  Row 12 ─ ─  [VCC] . ◄─────────────────────────(+)    VCC → 3V3 rail
  Row 13 ─ ─  [GND] . ◄─────────────────────────(-)    GND → GND rail
  Row 14 ─ ─  [TX ] . ──── wire to row 8 (a) ── ─ ─    TX → ESP GPIO20
  Row 15 ─ ─  [RX ] . ──── wire to row 9 (a) ── ─ ─    RX → ESP GPIO21
         ─────────────────────────────────────────────
  Row 17 ─ ─  . . . . . . .  . . . . . . .     ─ ─    (spacer)
         ─────────────────────────────────────────────
         ── Display PCB (12-pin header) ──────────────
  Row 19 ─ ─  [GND] . ◄─────────────────────────(-)    → GND rail
  Row 20 ─ ─  [VDD] . ◄─────────────────────────(+)    → 3V3 rail
  Row 21 ─ ─  [SCL] . ──── wire to row 5 (g) ── ─ ─    → ESP GPIO4
  Row 22 ─ ─  [SDA] . ──── wire to row 3 (a) ── ─ ─    → ESP GPIO6
  Row 23 ─ ─  [RES] . ──── wire to row 6 (g) ── ─ ─    → ESP GPIO3
  Row 24 ─ ─  [DC ] . ──── wire to row 2 (a) ── ─ ─    → ESP GPIO5
  Row 25 ─ ─  [CS ] . ──── wire to row 4 (a) ── ─ ─    → ESP GPIO7
  Row 26 ─ ─  [BLK] . ──── wire to row 7 (a) ── ─ ─    → ESP GPIO10
  Row 27 ─ ─  [A  ] . ──── wire to row 8 (g) ── ─ ─    → ESP GPIO1
  Row 28 ─ ─  [B  ] . ──── wire to row 9 (g) ── ─ ─    → ESP GPIO0
  Row 29 ─ ─  [PSH] . ──── wire to row 7 (g) ── ─ ─    → ESP GPIO2
  Row 30 ─ ─  [K0 ] . ──── wire to row 6 (a) ── ─ ─    → ESP GPIO9
         ═══════════════════════════════════════════
```

**Reading the diagram:** The ESP32's left pins (cols a-e) are GPIO5-GPIO21. Its right pins (cols f-j) are 5V, GND, 3V3, GPIO4-GPIO0. Jumper wires connect to free columns on the same row as the target GPIO — use cols a-d for left-side pins, cols g-j for right-side pins.

### Wiring Summary

**Power (2 wires)**

| Wire | From | To | Color |
|------|------|----|-------|
| GND | ESP row 3 col i | (-) rail | Black |
| 3V3 | ESP row 4 col i | (+) rail | Red |

**LD2410B Radar (4 wires)**

| Wire | From | To | Color |
|------|------|----|-------|
| VCC | Radar row 12 | (+) rail | Red |
| GND | Radar row 13 | (-) rail | Black |
| TX→RX | Radar row 14 | ESP row 8 col a (GPIO20) | Cyan |
| RX←TX | Radar row 15 | ESP row 9 col a (GPIO21) | Cyan |

**Display SPI (5 wires)**

| Wire | From | To | Color |
|------|------|----|-------|
| SCL | Display row 21 | ESP row 5 col g (GPIO4) | Yellow |
| SDA | Display row 22 | ESP row 3 col a (GPIO6) | Blue |
| RES | Display row 23 | ESP row 6 col g (GPIO3) | White |
| DC | Display row 24 | ESP row 2 col a (GPIO5) | Green |
| CS | Display row 25 | ESP row 4 col a (GPIO7) | Orange |

**Display Controls (5 wires)**

| Wire | From | To | Color |
|------|------|----|-------|
| BLK | Display row 26 | ESP row 7 col a (GPIO10) | Purple |
| A | Display row 27 | ESP row 8 col g (GPIO1) | Gray |
| B | Display row 28 | ESP row 9 col g (GPIO0) | Gray |
| PUSH | Display row 29 | ESP row 7 col g (GPIO2) | Brown |
| K0 | Display row 30 | ESP row 6 col a (GPIO9) | Brown |

**Display Power (2 wires)**

| Wire | From | To | Color |
|------|------|----|-------|
| GND | Display row 19 | (-) rail | Black |
| VDD | Display row 20 | (+) rail | Red |

### Assembly Steps

1. **Seat the ESP32-C3** straddling the center gap at the top of the breadboard. USB-C should hang off the top edge so you can plug in without removing the board.

2. **Wire power first.** Run GND (row 3, right side) to the (-) rail and 3V3 (row 4, right side) to the (+) rail. Plug in USB and verify 3.3V on the (+) rail with a multimeter if you have one.

3. **Wire the LD2410B next** (4 wires only). This is the easiest peripheral to test — once wired, uncomment the sensor code in the sketch and check the serial monitor for `LD2410 connected.`

4. **Wire the display SPI lines** (5 wires: SCL, SDA, RES, DC, CS) and display power (GND, VDD). Uncomment the display init code and verify you get the boot splash on screen.

5. **Wire the display controls last** (BLK, A, B, PUSH, K0). Test the encoder and buttons after connecting.

6. **Strapping pin warning:** Do not hold the encoder button (GPIO2) or K0 (GPIO9) while plugging in USB or pressing reset — this forces the ESP into download mode.

---

## Flashing & Setup

### Bare-Metal (PlatformIO)

```bash
# Clone and flash
git clone https://github.com/emosapien/radar-gizmo.git
cd radar-gizmo
# Add platformio.ini and configure TFT_eSPI, then:
pio run -t upload
```

### Bare-Metal (Arduino IDE)

1. Open `arduino/RadarGizmo/RadarGizmo.ino`
2. Install libraries via Library Manager: **TFT_eSPI**, **ld2410**, **RotaryEncoder**
3. Edit TFT_eSPI `User_Setup.h` (see header comment in .ino file for exact values)
4. Select board **ESP32C3 Dev Module**, enable **USB CDC On Boot**, set **Flash Mode: DIO**
5. Upload

### ESPHome

1. Flash a base ESPHome image to the ESP32-C3 via USB (first time only)
2. In the ESPHome dashboard, create a new device and paste the contents of `esphome/radar-gizmo.yaml`
3. Add your WiFi credentials and API key to your `secrets.yaml`
4. Install OTA — all future updates go over WiFi
5. The device auto-discovers in Home Assistant

---

## Project Structure

```
radar-gizmo/
├── src/
│   └── main.cpp                 # Primary firmware (PlatformIO)
├── arduino/
│   └── RadarGizmo/
│       └── RadarGizmo.ino       # Arduino IDE version
├── esphome/
│   └── radar-gizmo.yaml         # ESPHome / Home Assistant version
├── CLAUDE.md                    # Project rules and conventions
├── README.md                    # This file
└── .gitignore
```

---

## License

This project is open source. Feel free to use, modify, and share.
