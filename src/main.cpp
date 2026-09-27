#include <M5Unified.h>

namespace {

constexpr uint8_t kVibrationLevel = 128;
constexpr uint32_t kVibrationDurationMs = 80;

void drawCenteredLine(const char* text, int lineOffset) {
  const int centerX = M5.Display.width() / 2;
  const int centerY = M5.Display.height() / 2;
  M5.Display.drawCenterString(text, centerX, centerY + lineOffset * 32);
}

void pulseVibration() {
  M5.Power.setVibration(kVibrationLevel);
  delay(kVibrationDurationMs);
  M5.Power.setVibration(0);
}

void showEvent(const char* text) {
  M5.Display.fillRect(0, M5.Display.height() / 2 + 40, M5.Display.width(), 40, TFT_BLACK);
  drawCenteredLine(text, 2);
  Serial.println(text);
}

}  // namespace

void setup() {
  auto config = M5.config();
  M5.begin(config);
  M5.Display.setTextSize(2);
  M5.Display.fillScreen(TFT_BLACK);
  char boardText[32];
  snprintf(boardText, sizeof(boardText), "board=%d %dx%d", static_cast<int>(M5.getBoard()),
           M5.Display.width(), M5.Display.height());
  drawCenteredLine("Hello StopWatch", -1);
  drawCenteredLine(boardText, 0);
  Serial.println(boardText);
  Serial.printf("psram=%u\n", ESP.getPsramSize());
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    showEvent("BtnA");
    pulseVibration();
  }
  if (M5.BtnB.wasPressed()) {
    showEvent("BtnB");
    pulseVibration();
  }
  const auto touch = M5.Touch.getDetail();
  if (touch.wasClicked()) {
    char touchText[32];
    snprintf(touchText, sizeof(touchText), "tap %d,%d", touch.x, touch.y);
    showEvent(touchText);
    pulseVibration();
  }
  delay(10);
}
