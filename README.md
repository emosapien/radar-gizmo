# Radar Gizmo

ESP32-C3 Super Mini mmWave presence detection dashboard using the LD2410B sensor and an ST7789 TFT display.

## Hardware

| Component | Model | Spec |
|-----------|-------|------|
| MCU | ESP32-C3 Super Mini | RISC-V, 160MHz, 400KB SRAM |
| Radar Sensor | HiLink LD2410B | 24GHz mmWave, UART 256000 baud |
| Display Module | baishundianzi 2.0" TFT + EC11 | ST7789, 320x240 RGB, SPI |
| Input | EC11 Rotary Encoder (on display PCB) | Quadrature A/B + Push button |
| Extra Button | K0 (on display PCB) | Momentary, shared with BOOT/GPIO 9 |

### Display Module PCB

The display and encoder are integrated on a single 65mm x 42mm PCB (baishundianzi 2.0-inch model).
The PCB exposes a single pin header with the following order:

```
GND | VDD | SCL | SDA | RES | DC | CS | BLK | A | B | PUSH | K0
```

- **BLK** is fully enabled by default on the PCB — can be left unconnected if no brightness control is needed
- **K0** is an independent momentary button on the PCB (labeled KEY0)

## Pin Mapping

### TFT ST7789
| Signal | GPIO |
|--------|------|
| SCL (Clock) | 4 |
| SDA (MOSI) | 6 |
| RES (Reset) | 3 |
| DC (Data/Command) | 5 |
| CS (Chip Select) | 7 |
| BLK (Backlight) | 10 |

### LD2410B Sensor
| Signal | GPIO |
|--------|------|
| TX | 21 |
| RX | 20 |

### EC11 Rotary Encoder
| Signal | GPIO | Note |
|--------|------|------|
| Phase A | 1 | |
| Phase B | 0 | |
| Push Button | 2 | Strapping pin — do not hold during boot |

### Other
| Signal | GPIO | Note |
|--------|------|------|
| K0 (Extra Key) | 9 | Shared with BOOT button — strapping pin |
| Onboard LED | 8 | Active LOW |

## Breadboard Prototype Layout

### Parts

- 1x 400-pin half-size breadboard (30 rows)
- 1x ESP32-C3 Super Mini
- 1x baishundianzi 2.0" TFT+EC11 display module (12-pin header)
- 1x LD2410B radar sensor module
- Jumper wires

### ESP32-C3 Super Mini Pinout Reference

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

### Breadboard Diagram

The ESP32-C3 straddles the center gap at the top. The display module header and
LD2410B sensor plug into rows below it. USB port faces off the top edge of the
breadboard for easy access.

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

## Strapping Pin Warning

GPIO 2 and GPIO 9 are strapping pins. If either is held LOW during power-on or USB plug-in, the board enters download mode (black screen). Press reset or re-plug to recover.

## Arduino IDE Settings

- **Board:** ESP32C3 Dev Module
- **USB CDC On Boot:** Enabled (required for Serial Monitor)
- **Flash Mode:** DIO
- **JTAG Adapter:** Integrated USB JTAG

## Dependencies

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- [LD2410](https://github.com/ncmreynolds/ld2410)
- [RotaryEncoder](https://github.com/mathertel/RotaryEncoder)

## Features

- **Dashboard View** — Live presence status, target distance, moving/stationary energy bars, radar sweep animation
- **Engineering View** — Raw sensor data readout (distances, energy levels, firmware version)
- Rotary encoder and K0 button to switch between views
- Heartbeat LED blink for liveness indication
- Double-buffered sprite rendering (flicker-free)
