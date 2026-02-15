/*
 * Radar Gizmo — ESP32-C3 Super Mini mmWave Dashboard
 * Board: "ESP32C3 Dev Module"
 *
 * IDE Settings: USB CDC On Boot: Enabled, Flash Mode: DIO, JTAG: Integrated USB JTAG
 *
 * --- PIN MAPPING (ESP32-C3 Super Mini → baishundianzi 2.0" TFT+EC11 PCB) ---
 *
 * PCB Header: GND | VDD | SCL | SDA | RES | DC | CS | BLK | A | B | PUSH | K0
 * ESP32 GPIO:  -     3V3    4     6     3     5    7    10    1   0    2      9
 *
 * [LD2410B Sensor]  TX -> GPIO 21, RX -> GPIO 20
 * [Onboard LED]     GPIO 8 (Active LOW)
 *
 * GPIO 2 (PUSH) and GPIO 9 (K0) are strapping pins.
 * Holding either LOW during power-on enters download mode (black screen).
 */

#include <SPI.h>
#include <LovyanGFX.hpp>
#include <ld2410.h>
#include <RotaryEncoder.h>

// --- LovyanGFX display configuration ---
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX(void) {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.pin_sclk = 4;
      cfg.pin_mosi = 6;
      cfg.pin_miso = -1;
      cfg.pin_dc   = 5;
      cfg.spi_3wire = true;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs  = 7;
      cfg.pin_rst = 3;
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.readable   = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

// --- Pin Definitions ---
#define PIN_LD2410_RX 20
#define PIN_LD2410_TX 21
#define PIN_ENC_A     1
#define PIN_ENC_B     0
#define PIN_ENC_BTN   2
#define PIN_K0        9
#define PIN_LED       8
#define PIN_TFT_BL    10

// --- Display Constants ---
#define SCREEN_W      240
#define SCREEN_H      320

// --- Colors (RGB565) ---
#define C_BG          0x0000
#define C_ACCENT      0x03EF
#define C_ALERT       0xF800
#define C_SAFE        0x07E0
#define C_TEXT        0xFFFF
#define C_DIM         0x7BEF
#define C_BAR_MOV     0xFAAA
#define C_BAR_STA     0x05FF
#define C_SWEEP       0x3333

// --- Timing ---
#define UPDATE_INTERVAL_MS  50
#define HEARTBEAT_MS        1000
#define DEBOUNCE_MS         30

// --- Objects ---
LGFX tft;
LGFX_Sprite spr(&tft);
ld2410 ld2410;
RotaryEncoder encoder(PIN_ENC_A, PIN_ENC_B, RotaryEncoder::LatchMode::TWO03);

// --- State ---
int sprH = SCREEN_H;  // Actual sprite height (may be smaller than SCREEN_H)
int currentView = 0;           // 0 = Dashboard, 1 = Engineering
unsigned long lastUpdate = 0;
bool ledState = false;
unsigned long lastBlink = 0;

// Button debounce state
bool lastEncBtn = false;
bool lastK0Btn = false;
unsigned long lastEncBtnChange = 0;
unsigned long lastK0BtnChange = 0;

// Radar sweep animation
int sweepAngle = 0;
int sweepDir = 1;

// Sensor config fetch (one-shot after connection)
bool sensorConfigFetched = false;

// Radar pings (updated every 2s)
#define PING_INTERVAL_MS 2000
#define PING_ANGLE_MOV -50  // 10 o'clock
#define PING_ANGLE_STA  50  // 2 o'clock
unsigned long lastPing = 0;
int pingMoveDist = 0, pingMoveEnergy = 0;
int pingStatDist = 0, pingStatEnergy = 0;
bool pingMoveActive = false, pingStatActive = false;
float pingAge = 1.0f;  // 0.0 = just pinged, 1.0 = fully faded

// Smoothed sensor values (exponential moving average)
float smoothMoveDist = 0;
float smoothStatDist = 0;
float smoothMoveEnergy = 0;
float smoothStatEnergy = 0;
#define SMOOTH_ALPHA 0.3f  // 0.0 = frozen, 1.0 = no smoothing

// Text buffer for snprintf
char buf[64];

// --- Forward declarations ---
void drawDashboard();
void drawEngineeringView();

void setup() {
  Serial.begin(115200);

  // --- GPIO ---
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);      // Off (active LOW)
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);
  pinMode(PIN_K0, INPUT_PULLUP);

  // --- Display ---
  tft.init();
  tft.setRotation(0);               // Portrait: 240x320
  tft.fillScreen(C_BG);

  spr.setColorDepth(8);  // 8-bit color halves sprite memory (76KB vs 153KB)
  spr.createSprite(SCREEN_W, SCREEN_H);
  sprH = SCREEN_H;

  // --- Boot splash ---
  tft.setTextDatum(middle_center);
  tft.setTextColor(C_TEXT, C_BG);
  tft.setTextFont(2);
  tft.drawString("Booting Radar...", SCREEN_W / 2, SCREEN_H / 2);

  // --- Sensor ---
  Serial1.begin(256000, SERIAL_8N1, PIN_LD2410_RX, PIN_LD2410_TX);
  delay(500);
  while (Serial1.available()) Serial1.read();
  ld2410.begin(Serial1);

  delay(500); // Brief splash visibility (only blocking delay — in setup, not loop)

  // Clear boot splash so it doesn't persist behind a half-size sprite
  tft.fillScreen(C_BG);
}

void loop() {
  unsigned long now = millis();
  ld2410.read();

  // Fetch config/firmware info once sensor is connected
  if (!sensorConfigFetched && ld2410.isConnected()) {
    ld2410.requestFirmwareVersion();
    ld2410.requestCurrentConfiguration();
    sensorConfigFetched = true;
  }

  // --- Heartbeat LED ---
  if (now - lastBlink >= HEARTBEAT_MS) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState ? LOW : HIGH);
  }

  // --- Encoder rotation (view select) ---
  encoder.tick();
  int newPos = encoder.getPosition();
  if (newPos > 1) { encoder.setPosition(1); newPos = 1; }
  if (newPos < 0) { encoder.setPosition(0); newPos = 0; }
  currentView = newPos;

  // --- Encoder button (non-blocking debounce) ---
  bool encBtn = !digitalRead(PIN_ENC_BTN);
  if (encBtn != lastEncBtn && (now - lastEncBtnChange >= DEBOUNCE_MS)) {
    lastEncBtnChange = now;
    lastEncBtn = encBtn;
    if (encBtn) {
      Serial.println(F("Encoder Pushed"));
    }
  }

  // --- K0 button (non-blocking debounce) ---
  bool k0Btn = !digitalRead(PIN_K0);
  if (k0Btn != lastK0Btn && (now - lastK0BtnChange >= DEBOUNCE_MS)) {
    lastK0BtnChange = now;
    lastK0Btn = k0Btn;
    if (k0Btn) {
      currentView = (currentView == 0) ? 1 : 0;
      encoder.setPosition(currentView);
    }
  }

  // --- Render UI ---
  if (now - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = now;
    if (currentView == 0) drawDashboard();
    else drawEngineeringView();
  }
}

// --- View 0: Dashboard ---
void drawDashboard() {
  spr.fillSprite(C_BG);

  // Header bar
  spr.setTextColor(C_TEXT, C_BG);
  spr.setTextDatum(top_left);
  spr.setTextFont(2);
  spr.drawString("RADAR GIZMO", 10, 8);

  // Connection indicator
  uint16_t dotColor = ld2410.isConnected() ? C_SAFE : C_ALERT;
  spr.fillCircle(SCREEN_W - 20, 14, 4, dotColor);

  // --- Update smoothed values ---
  smoothMoveDist   = smoothMoveDist   * (1 - SMOOTH_ALPHA) + ld2410.movingTargetDistance()    * SMOOTH_ALPHA;
  smoothStatDist   = smoothStatDist   * (1 - SMOOTH_ALPHA) + ld2410.stationaryTargetDistance() * SMOOTH_ALPHA;
  smoothMoveEnergy = smoothMoveEnergy * (1 - SMOOTH_ALPHA) + ld2410.movingTargetEnergy()      * SMOOTH_ALPHA;
  smoothStatEnergy = smoothStatEnergy * (1 - SMOOTH_ALPHA) + ld2410.stationaryTargetEnergy()  * SMOOTH_ALPHA;

  // --- Left side: Status + Distance ---
  spr.setTextDatum(middle_left);
  if (ld2410.presenceDetected()) {
    spr.setTextColor(C_ALERT, C_BG);
    spr.setTextFont(4);
    spr.drawString("TARGET", 10, 50);
    spr.drawString("DETECTED", 10, 80);

    spr.setTextColor(C_TEXT, C_BG);
    spr.setTextDatum(top_left);
    spr.setTextFont(2);
    int y = 110;
    if (ld2410.movingTargetDetected()) {
      snprintf(buf, sizeof(buf), "MOV: %dcm", (int)smoothMoveDist);
      spr.drawString(buf, 10, y);
      y += 20;
    }
    if (ld2410.stationaryTargetDetected()) {
      snprintf(buf, sizeof(buf), "STA: %dcm", (int)smoothStatDist);
      spr.drawString(buf, 10, y);
    }
  } else {
    spr.setTextColor(C_SAFE, C_BG);
    spr.setTextFont(4);
    spr.drawString("AREA", 10, 50);
    spr.drawString("CLEAR", 10, 80);
  }

  // --- Right side: Energy bars ---
  int barW = 24;
  int barMaxH = 120;
  int barTop = 30;
  int barBottom = barTop + barMaxH;
  int barRightBase = SCREEN_W - 10;

  int moveEnergy = (int)smoothMoveEnergy;
  int staticEnergy = (int)smoothStatEnergy;

  // Moving energy bar
  int mX = barRightBase - barW * 2 - 8;
  int mH = map(moveEnergy, 0, 100, 0, barMaxH);
  spr.drawRect(mX, barTop, barW, barMaxH, C_TEXT);
  spr.fillRect(mX, barBottom - mH, barW, mH, C_BAR_MOV);
  spr.setTextDatum(top_center);
  spr.setTextColor(C_BAR_MOV, C_BG);
  spr.setTextFont(1);
  snprintf(buf, sizeof(buf), "%d%%", moveEnergy);
  spr.drawString(buf, mX + barW / 2, barBottom + 4);
  spr.drawString("MOV", mX + barW / 2, barBottom + 16);

  // Static energy bar
  int sX = barRightBase - barW;
  int sH = map(staticEnergy, 0, 100, 0, barMaxH);
  spr.drawRect(sX, barTop, barW, barMaxH, C_TEXT);
  spr.fillRect(sX, barBottom - sH, barW, sH, C_BAR_STA);
  spr.setTextColor(C_BAR_STA, C_BG);
  snprintf(buf, sizeof(buf), "%d%%", staticEnergy);
  spr.drawString(buf, sX + barW / 2, barBottom + 4);
  spr.drawString("STA", sX + barW / 2, barBottom + 16);

  // --- Radar sweep animation (bottom-center arc) ---
  sweepAngle += 4 * sweepDir;
  if (sweepAngle > 60) sweepDir = -1;
  if (sweepAngle < -60) sweepDir = 1;

  int arcCx = SCREEN_W / 2;
  int arcCy = sprH;       // Center at bottom edge for half-arc look
  int arcR = 90;

  // Draw range rings (brighter color that survives 8-bit depth)
  for (int r = 30; r <= arcR; r += 22) {
    for (int a = -60; a <= 60; a += 3) {
      float rad = radians(a - 90);
      int px = arcCx + cos(rad) * r;
      int py = arcCy + sin(rad) * r;
      if (py < sprH - 16) spr.drawPixel(px, py, 0x39E7);
    }
  }

  // Sweep line
  float sweepRad = radians(sweepAngle - 90);
  int endX = arcCx + cos(sweepRad) * arcR;
  int endY = arcCy + sin(sweepRad) * arcR;
  spr.drawLine(arcCx, arcCy, endX, endY, C_SWEEP);

  // --- Radar pings ---
  unsigned long now2 = millis();
  if (now2 - lastPing >= PING_INTERVAL_MS) {
    lastPing = now2;
    pingMoveActive = ld2410.movingTargetDetected();
    pingStatActive = ld2410.stationaryTargetDetected();
    if (pingMoveActive) {
      pingMoveDist   = (int)smoothMoveDist;
      pingMoveEnergy = (int)smoothMoveEnergy;
    }
    if (pingStatActive) {
      pingStatDist   = (int)smoothStatDist;
      pingStatEnergy = (int)smoothStatEnergy;
    }
  }
  // Ping age: 0.0 = just fired, 1.0 = about to refresh
  pingAge = (float)(now2 - lastPing) / PING_INTERVAL_MS;

  // Draw pings as filled circles on the arc
  // Distance → radial position: 0-200cm = ring1..ring2, 200-1000cm = ring2..beyond ring3
  auto distToRadius = [](int distCm) -> int {
    if (distCm <= 200) return map(distCm, 0, 200, 30, 52);
    return map(constrain(distCm, 200, 1000), 200, 1000, 52, 82);
  };
  // Energy → ping dot size (higher energy = bigger dot), shrinks as ping ages
  auto energyToSize = [&](int energy) -> int {
    int maxR = map(constrain(energy, 0, 100), 0, 100, 2, 8);
    return max(1, (int)(maxR * (1.0f - pingAge * 0.6f)));
  };

  if (pingMoveActive) {
    int pr = distToRadius(pingMoveDist);
    float prad = radians(PING_ANGLE_MOV - 90);
    int px = arcCx + cos(prad) * pr;
    int py = arcCy + sin(prad) * pr;
    if (py < sprH - 16) {
      spr.fillCircle(px, py, energyToSize(pingMoveEnergy), C_BAR_MOV);
    }
  }
  if (pingStatActive) {
    int pr = distToRadius(pingStatDist);
    float prad = radians(PING_ANGLE_STA - 90);
    int px = arcCx + cos(prad) * pr;
    int py = arcCy + sin(prad) * pr;
    if (py < sprH - 16) {
      spr.fillCircle(px, py, energyToSize(pingStatEnergy), C_BAR_STA);
    }
  }

  // Footer
  spr.setTextDatum(bottom_center);
  spr.setTextColor(C_DIM, C_BG);
  spr.setTextFont(1);
  spr.drawString("[Turn] View  [K0] Toggle", SCREEN_W / 2, sprH - 2);

  spr.pushSprite(0, 0);
}

// --- View 1: Engineering / Raw Data ---
void drawEngineeringView() {
  spr.fillSprite(C_BG);

  spr.setTextColor(C_TEXT, C_BG);
  spr.setTextDatum(top_left);
  spr.setTextFont(2);
  spr.drawString("RAW SENSOR DATA", 10, 8);

  // Connection indicator
  uint16_t dotColor = ld2410.isConnected() ? C_SAFE : C_ALERT;
  spr.fillCircle(SCREEN_W - 20, 14, 4, dotColor);

  int y = 32;
  int lh = 18;

  // Detection status
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "Detection: %s", ld2410.presenceDetected() ? "YES" : "NO");
  spr.drawString(buf, 10, y);
  y += lh + 4;

  // Moving target
  spr.setTextColor(C_BAR_MOV, C_BG);
  spr.drawString("MOVING TARGET", 10, y);
  y += lh;
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "  Dist: %d cm", ld2410.movingTargetDistance());
  spr.drawString(buf, 10, y);
  y += lh;
  snprintf(buf, sizeof(buf), "  Energy: %d%%", ld2410.movingTargetEnergy());
  spr.drawString(buf, 10, y);
  y += lh + 4;

  // Stationary target
  spr.setTextColor(C_BAR_STA, C_BG);
  spr.drawString("STATIC TARGET", 10, y);
  y += lh;
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "  Dist: %d cm", ld2410.stationaryTargetDistance());
  spr.drawString(buf, 10, y);
  y += lh;
  snprintf(buf, sizeof(buf), "  Energy: %d%%", ld2410.stationaryTargetEnergy());
  spr.drawString(buf, 10, y);
  y += lh + 6;

  // Configuration
  spr.setTextColor(C_ACCENT, C_BG);
  spr.drawString("CONFIG", 10, y);
  y += lh;
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "  Max gate: %d (mov %d / sta %d)",
           ld2410.max_gate, ld2410.max_moving_gate, ld2410.max_stationary_gate);
  spr.drawString(buf, 10, y);
  y += lh;
  snprintf(buf, sizeof(buf), "  Idle timeout: %ds", ld2410.sensor_idle_time);
  spr.drawString(buf, 10, y);
  y += lh + 4;

  // Firmware version
  spr.setTextColor(C_DIM, C_BG);
  snprintf(buf, sizeof(buf), "FW: v%d.%02d.%08X",
           ld2410.firmware_major_version, ld2410.firmware_minor_version,
           ld2410.firmware_bugfix_version);
  spr.drawString(buf, 10, y);

  // Footer
  spr.setTextDatum(bottom_center);
  spr.setTextColor(C_DIM, C_BG);
  spr.setTextFont(1);
  spr.drawString("[Turn] View  [K0] Toggle", SCREEN_W / 2, sprH - 4);

  spr.pushSprite(0, 0);
}
