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
