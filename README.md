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
- Double-buffered sprite rendering in bare-metal firmware (flicker-free, 20fps); ESPHome uses direct rendering at 10fps

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
         5V ──┤ o     o ├── GPIO5
        GND ──┤ o     o ├── GPIO6
        3V3 ──┤ o     o ├── GPIO7
      GPIO4 ──┤ o     o ├── GPIO8  (LED)
      GPIO3 ──┤ o     o ├── GPIO9  (BOOT)
      GPIO2 ──┤ o     o ├── GPIO10
      GPIO1 ──┤ o     o ├── GPIO20
      GPIO0 ──┤ o     o ├── GPIO21
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

Full ESPHome configuration with the same display views reimplemented in display lambdas, including the animated radar sweep. Adds Home Assistant integration.

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

The ESP32-C3 straddles the center gap at the top. The LD2410B sits in the middle section. The display module header plugs into the bottom. USB port faces off the top edge for easy access.

```
        (-)  (a)(b)(c)(d)(e)  (f)(g)(h)(i)(j)  (+)
         ═══════════════════════════════════════════
  Row 1  ─ ─  ─── ESP32-C3 Super Mini ─────────  ─ ─    ← USB-C hangs off top edge
  Row 2  ─ ─  [5V ] . . . .  [GP5] . . . .     ─ ─
  Row 3  ─ ─  [GND] . ◄─┐ .  [GP6] . . . .     ─ ─    GND → (-) rail
  Row 4  ─ ─  [3V3] . . │ .  [GP7] . . . .     ─ ─    3V3 → (+) rail
  Row 5  ─ ─  [GP4] . . │ .  [GP8] . . . .     ─ ─
  Row 6  ─ ─  [GP3] . . │ .  [GP9] . . . .     ─ ─
  Row 7  ─ ─  [GP2] . . │ .  [G10] . . . .     ─ ─
  Row 8  ─ ─  [GP1] . . │ .  [G20] . . . .     ─ ─
  Row 9  ─ ─  [GP0] . . │ .  [G21] . . . .     ─ ─
         ─────────────── │ ─────────────────────────
  Row 10 ─ ─  . . . . .  │  . . . . .           ─ ─    (empty spacer row)
         ─────────────── │ ─────────────────────────
                         │
         ── LD2410B ─────│──────────────────────────
  Row 11 ─ ─  . . . . .  │  . . . . .           ─ ─
  Row 12 ─ ─  [VCC]◄─────┼──────────────────────(+)    LD2410B VCC → 3V3 rail
  Row 13 ─ ─  [GND]◄─────┼──────────────────────(-)    LD2410B GND → GND rail
  Row 14 ─ ─  [TX ]──────┼──wire to row 8 (f)── ─ ─    LD2410B TX → ESP GPIO20
  Row 15 ─ ─  [RX ]──────┼──wire to row 9 (f)── ─ ─    LD2410B RX → ESP GPIO21
  Row 16 ─ ─  . . . . .  │  . . . . .           ─ ─
         ─────────────── │ ─────────────────────────
                         │
         ── Display PCB ─│── (12-pin header) ───────
         Pin: GND VDD SCL SDA RES DC  CS  BLK A  B  PSH K0
  Row 19 ─ ─  [GND]◄─────┼──────────────────────(-)    → GND rail
  Row 20 ─ ─  [VDD]◄─────┘──────────────────────(+)    → 3V3 rail
  Row 21 ─ ─  [SCL]─────── wire to row 5 (a) ── ─ ─    → ESP GPIO4
  Row 22 ─ ─  [SDA]─────── wire to row 3 (f) ── ─ ─    → ESP GPIO6
  Row 23 ─ ─  [RES]─────── wire to row 6 (a) ── ─ ─    → ESP GPIO3
  Row 24 ─ ─  [DC ]─────── wire to row 2 (f) ── ─ ─    → ESP GPIO5
  Row 25 ─ ─  [CS ]─────── wire to row 4 (f) ── ─ ─    → ESP GPIO7
  Row 26 ─ ─  [BLK]─────── wire to row 7 (f) ── ─ ─    → ESP GPIO10
  Row 27 ─ ─  [A  ]─────── wire to row 8 (a) ── ─ ─    → ESP GPIO1
  Row 28 ─ ─  [B  ]─────── wire to row 9 (a) ── ─ ─    → ESP GPIO0
  Row 29 ─ ─  [PSH]─────── wire to row 7 (a) ── ─ ─    → ESP GPIO2
  Row 30 ─ ─  [K0 ]─────── wire to row 6 (f) ── ─ ─    → ESP GPIO9
         ═══════════════════════════════════════════
```

### Wiring Summary

| Wire | From | To | Color Suggestion |
|------|------|----|-----------------|
| Power | Row 3 col b (GND) | (-) rail | Black |
| Power | Row 4 col b (3V3) | (+) rail | Red |
| SPI CLK | Row 21 (SCL) | Row 5 col a (GPIO4) | Yellow |
| SPI DATA | Row 22 (SDA) | Row 3 col f (GPIO6) | Blue |
| TFT Reset | Row 23 (RES) | Row 6 col a (GPIO3) | White |
| TFT DC | Row 24 (DC) | Row 2 col f (GPIO5) | Green |
| TFT CS | Row 25 (CS) | Row 4 col f (GPIO7) | Orange |
| Backlight | Row 26 (BLK) | Row 7 col f (GPIO10) | Purple |
| Encoder A | Row 27 (A) | Row 8 col a (GPIO1) | Gray |
| Encoder B | Row 28 (B) | Row 9 col a (GPIO0) | Gray |
| Enc Push | Row 29 (PSH) | Row 7 col a (GPIO2) | Brown |
| K0 Button | Row 30 (K0) | Row 6 col f (GPIO9) | Brown |
| Radar TX | Row 14 (TX) | Row 8 col f (GPIO20) | Cyan |
| Radar RX | Row 15 (RX) | Row 9 col f (GPIO21) | Cyan |
| Radar VCC | Row 12 (VCC) | (+) rail | Red |
| Radar GND | Row 13 (GND) | (-) rail | Black |

### Assembly Notes

1. **Start with power** — wire GND and 3V3 from the ESP32 to the rails first
2. **Seat the ESP32** with USB-C hanging off the top edge so you can plug in without removing it
3. **Wire the LD2410B** next (only 4 wires) and test serial connection before adding the display
4. **Wire the display module last** — 12 wires, work left-to-right through the header
5. **Do not press K0 or the encoder button while plugging in USB** — strapping pins will enter download mode

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
