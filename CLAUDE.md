# Radar Gizmo — Project Rules

## Hardware Target
- MCU: ESP32-C3 Super Mini (RISC-V, 160MHz, 400KB SRAM)
- Sensor: LD2410B (24GHz mmWave, 256000 baud UART)
- Display: baishundianzi 2.0" TFT + EC11 integrated PCB (65mm x 42mm)
  - Driver: ST7789, Resolution: 320x240 RGB, Interface: SPI
  - PCB header order: GND, VDD, SCL, SDA, RES, DC, CS, BLK, A, B, PUSH, K0
  - BLK is enabled by default on the PCB (no connection required for always-on backlight)
- Input: EC11 rotary encoder (A/B quadrature + push) + K0 button (all on display PCB)

## Display Layout
- Screen is 320x240 in landscape (rotation=1): 320px wide, 240px tall
- All UI elements must fit within 320x240 — do not draw beyond these bounds
- Sprite buffer is full-screen (320x240) for double-buffered rendering

## Code Style
- Use `snprintf` with char buffers instead of Arduino `String` concatenation in loops (avoids heap fragmentation)
- Use non-blocking patterns (`millis()` comparisons) — never use `delay()` in the main loop
- Keep all pin definitions as `#define` constants at the top of the file
- Use TFT_eSPI sprite double-buffering for all screen draws

## Architecture
- Single-file firmware in `src/main.cpp` (split into separate files only when complexity demands it)
- Views are drawn by dedicated `draw*()` functions called from `loop()`
- State variables are global, grouped and labeled by purpose

## Conventions
- Colors use 16-bit RGB565 format with `C_` prefix (e.g., `C_BG`, `C_ALERT`)
- Pin constants use `PIN_` prefix
- All GPIO setup happens in `setup()`, not scattered through the code
- Comments should explain *why*, not *what* — the code should be self-explanatory

## Things to Avoid
- Blocking calls (`delay()`, `while` loops waiting on hardware) in `loop()`
- Dynamic memory allocation in the render path
- Changing strapping pin (GPIO 2, 9) behavior without documenting boot implications
- Adding WiFi/BLE unless explicitly requested (preserves power budget and loop timing)
