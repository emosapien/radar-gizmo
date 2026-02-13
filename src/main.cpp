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
#include <TFT_eSPI.h>
#include <LD2410.h>
#include <RotaryEncoder.h>

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
#define SCREEN_W      320
#define SCREEN_H      240

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
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
LD2410 ld2410;
RotaryEncoder encoder(PIN_ENC_A, PIN_ENC_B, RotaryEncoder::LatchMode::TWO03);

// --- State ---
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
  tft.setRotation(1);               // Landscape: 320x240
  tft.fillScreen(C_BG);
  spr.createSprite(SCREEN_W, SCREEN_H);

  // --- Boot splash ---
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("Booting Radar...", SCREEN_W / 2, SCREEN_H / 2, 2);

  // --- Sensor ---
  Serial1.begin(256000, SERIAL_8N1, PIN_LD2410_RX, PIN_LD2410_TX);
  bool connected = ld2410.begin(Serial1);

  if (connected) Serial.println(F("LD2410 connected."));
  else Serial.println(F("LD2410 connection failed."));

  delay(500); // Brief splash visibility (only blocking delay — in setup, not loop)
}

void loop() {
  unsigned long now = millis();
  ld2410.read();

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
  spr.setTextDatum(TL_DATUM);
  spr.drawString("RADAR GIZMO", 10, 8, 2);

  // Connection indicator
  uint16_t dotColor = ld2410.isConnected() ? C_SAFE : C_ALERT;
  spr.fillCircle(SCREEN_W - 20, 14, 4, dotColor);

  // --- Left side: Status + Distance ---
  spr.setTextDatum(ML_DATUM);
  if (ld2410.presenceDetected()) {
    spr.setTextColor(C_ALERT, C_BG);
    spr.drawString("TARGET", 10, 50, 4);
    spr.drawString("DETECTED", 10, 80, 4);

    // Show both distances when available
    spr.setTextColor(C_TEXT, C_BG);
    spr.setTextDatum(TL_DATUM);
    int y = 110;
    if (ld2410.movingTargetDetected()) {
      snprintf(buf, sizeof(buf), "MOV: %dcm", ld2410.movingTargetDistance());
      spr.drawString(buf, 10, y, 2);
      y += 20;
    }
    if (ld2410.stationaryTargetDetected()) {
      snprintf(buf, sizeof(buf), "STA: %dcm", ld2410.stationaryTargetDistance());
      spr.drawString(buf, 10, y, 2);
    }
  } else {
    spr.setTextColor(C_SAFE, C_BG);
    spr.drawString("AREA", 10, 50, 4);
    spr.drawString("CLEAR", 10, 80, 4);
  }

  // --- Right side: Energy bars ---
  int barW = 24;
  int barMaxH = 120;
  int barBottom = 195;
  int barRightBase = SCREEN_W - 30;

  int moveEnergy = ld2410.movingTargetEnergy();
  int staticEnergy = ld2410.stationaryTargetEnergy();

  // Moving energy bar
  int mX = barRightBase - barW - 40;
  int mH = map(moveEnergy, 0, 100, 0, barMaxH);
  spr.drawRect(mX, barBottom - barMaxH, barW, barMaxH, C_TEXT);
  spr.fillRect(mX, barBottom - mH, barW, mH, C_BAR_MOV);
  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(C_BAR_MOV, C_BG);
  snprintf(buf, sizeof(buf), "%d%%", moveEnergy);
  spr.drawString(buf, mX + barW / 2, barBottom + 4, 1);
  spr.drawString("MOV", mX + barW / 2, barBottom + 16, 1);

  // Static energy bar
  int sX = barRightBase - barW;
  int sH = map(staticEnergy, 0, 100, 0, barMaxH);
  spr.drawRect(sX, barBottom - barMaxH, barW, barMaxH, C_TEXT);
  spr.fillRect(sX, barBottom - sH, barW, sH, C_BAR_STA);
  spr.setTextColor(C_BAR_STA, C_BG);
  snprintf(buf, sizeof(buf), "%d%%", staticEnergy);
  spr.drawString(buf, sX + barW / 2, barBottom + 4, 1);
  spr.drawString("STA", sX + barW / 2, barBottom + 16, 1);

  // --- Radar sweep animation (bottom-left arc) ---
  sweepAngle += 4 * sweepDir;
  if (sweepAngle > 60) sweepDir = -1;
  if (sweepAngle < -60) sweepDir = 1;

  int arcCx = 100;
  int arcCy = SCREEN_H + 30;  // Center below screen edge for half-arc look
  int arcR = 80;

  // Draw range rings
  for (int r = 30; r <= arcR; r += 25) {
    for (int a = -60; a <= 60; a += 3) {
      float rad = radians(a - 90);
      int px = arcCx + cos(rad) * r;
      int py = arcCy + sin(rad) * r;
      if (py < SCREEN_H) spr.drawPixel(px, py, 0x18E3);
    }
  }

  // Sweep line
  float sweepRad = radians(sweepAngle - 90);
  int endX = arcCx + cos(sweepRad) * arcR;
  int endY = arcCy + sin(sweepRad) * arcR;
  spr.drawLine(arcCx, arcCy, endX, endY, C_SWEEP);

  // Footer
  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(C_DIM, C_BG);
  spr.drawString("[Turn] View  [K0] Toggle", SCREEN_W / 2, SCREEN_H - 4, 1);

  spr.pushSprite(0, 0);
}

// --- View 1: Engineering / Raw Data ---
void drawEngineeringView() {
  spr.fillSprite(C_BG);

  spr.setTextColor(C_TEXT, C_BG);
  spr.setTextDatum(TL_DATUM);
  spr.drawString("RAW SENSOR DATA", 10, 8, 2);

  // Connection indicator
  uint16_t dotColor = ld2410.isConnected() ? C_SAFE : C_ALERT;
  spr.fillCircle(SCREEN_W - 20, 14, 4, dotColor);

  int y = 38;
  int lh = 22;

  // Detection status
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "Detection: %s", ld2410.presenceDetected() ? "YES" : "NO");
  spr.drawString(buf, 10, y, 2);
  y += lh + 6;

  // Moving target
  spr.setTextColor(C_BAR_MOV, C_BG);
  spr.drawString("MOVING TARGET", 10, y, 2);
  y += lh;
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "  Dist: %d cm", ld2410.movingTargetDistance());
  spr.drawString(buf, 10, y, 2);
  y += lh;
  snprintf(buf, sizeof(buf), "  Energy: %d%%", ld2410.movingTargetEnergy());
  spr.drawString(buf, 10, y, 2);
  y += lh + 6;

  // Stationary target
  spr.setTextColor(C_BAR_STA, C_BG);
  spr.drawString("STATIC TARGET", 10, y, 2);
  y += lh;
  spr.setTextColor(C_TEXT, C_BG);
  snprintf(buf, sizeof(buf), "  Dist: %d cm", ld2410.stationaryTargetDistance());
  spr.drawString(buf, 10, y, 2);
  y += lh;
  snprintf(buf, sizeof(buf), "  Energy: %d%%", ld2410.stationaryTargetEnergy());
  spr.drawString(buf, 10, y, 2);
  y += lh + 10;

  // Firmware version
  spr.setTextColor(C_DIM, C_BG);
  snprintf(buf, sizeof(buf), "FW: v%d.%d", ld2410.firmwareMajorVersion, ld2410.firmwareMinorVersion);
  spr.drawString(buf, 10, y, 2);

  // Footer
  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(C_DIM, C_BG);
  spr.drawString("[Turn] View  [K0] Toggle", SCREEN_W / 2, SCREEN_H - 4, 1);

  spr.pushSprite(0, 0);
}
