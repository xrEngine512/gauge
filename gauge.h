#include <Adafruit_GFX.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <float.h>
#include <math.h>

#include "CustomFonts/DSEG7_Modern_Bold_12.h"
#include "colors.h"

struct GaugeTheme {
  uint16_t okColor = OLED_Color_Warm;
  uint16_t lowColor = OLED_Color_Blue;
  uint16_t highColor = OLED_Color_Red;
  uint16_t alertColor = OLED_Color_Red;
};

struct GaugeConfig {
  const char *name = "???";
  const char *units = "???";
  float min = 0;
  float max = 100;
  float lowValue = 10;
  float highValue = 90;
};

float lesserValue(float value) {
  auto intValue = *(int32_t *)&value;
  --intValue;
  return *(float *)(&intValue);
}

struct GaugeData {
  inline static const float offlineValue = FLT_MAX;
  inline static const float badDataValue = lesserValue(offlineValue);

  float currentValue;
};

static const GaugeData offlineGaugeData{
    .currentValue = GaugeData::offlineValue,
};

static const GaugeData badGaugeData{
    .currentValue = GaugeData::badDataValue,
};

struct GaugeVerticalLayout {
  int16_t xPos = 0;
  int16_t yPos = 0;
  int16_t margin = 0;
  int16_t maxWidth = 0;
};

void resetLayout(struct GaugeVerticalLayout &layout, int16_t yPos) {
  layout.yPos = yPos;
}

int16_t valueStringSize(float value) {
  return log10f(value) + 4 /*+1 for log10 +1 for dot and +2 for precision*/;
}

void drawGauge(GFXcanvas16 &canvas, const struct GaugeConfig &config,
               struct GaugeVerticalLayout &layout,
               const struct GaugeTheme &theme, const struct GaugeData &data) {
  enum Status {
    Alert,
    High,
    Low,
    Ok,
  };

  static uint32_t blinkIntervalMs = 500;

  static const int16_t tallNotchH = 8;
  static const int16_t smallNotchH = 6;
  static const int16_t textH = 12;
  static const int16_t textW = 4;
  static const int16_t valueTextW = 11;
  static const int16_t textBottomMargin = 2;
  static const int16_t lineH = 2;
  static const int16_t indicatorW = 0;
  static const int16_t indicatorLeftMargin = 3;
  static const int16_t maxH = tallNotchH + textH + textBottomMargin;

  auto advanceLayout = [&layout] { layout.yPos += maxH + layout.margin; };

  Status status;
  int16_t color;

  if (data.currentValue == GaugeData::offlineValue) {
    status = Alert;
    color = theme.alertColor;
  } else if (data.currentValue == GaugeData::badDataValue) {
    status = Alert;
    color = theme.alertColor;
  } else {
    if (data.currentValue < config.lowValue) {
      status = Low;
      color = theme.lowColor;
    } else if (data.currentValue > config.highValue) {
      status = High;
      color = theme.highColor;
    } else {
      status = Ok;
      color = theme.okColor;
    }
  }

  bool show =
      status != Alert && status != High || (millis() / blinkIntervalMs) % 2;

  if (!show) {
    advanceLayout();
    return;
  }

  canvas.setFont();
  canvas.setTextColor(color);
  canvas.setTextSize(1);
  canvas.setCursor(layout.xPos, layout.yPos);
  canvas.print(config.name);
  canvas.print(' ');

  if (data.currentValue == GaugeData::offlineValue) {
    canvas.print("OFFLINE");
  } else if (data.currentValue == GaugeData::badDataValue) {
    canvas.print("BAD DATA");
  } else {
    {
      canvas.setFont(&DSEG7_Modern_Bold_12);
      // right alignment
      int16_t textXPos =
          canvas.width() -
          (valueTextW * valueStringSize(data.currentValue) +
           (strlen(config.units) + 1 /*+ 1 for space before units*/) * textW);
      // custom fonts seem to have coordinate origin at the bottom of the glyphs
      canvas.setCursor(textXPos, layout.yPos + textH);
      canvas.print(data.currentValue);
      canvas.setFont();
      canvas.setCursor(canvas.getCursorX(), layout.yPos);
    }
    canvas.print(' ');
    canvas.print(config.units);
  }

  int16_t w =
      ((layout.maxWidth - (indicatorLeftMargin + indicatorW)) / 10) * 10;
  int16_t xPos = layout.xPos;
  int16_t yPos = layout.yPos + maxH;

  float factor = constrain(
      (data.currentValue - config.min) / (config.max - config.min), 0.f, 1.f);
  int16_t currentW = factor * w;

  canvas.fillRect(xPos, yPos, w, -lineH, color);
  canvas.fillRect(xPos, yPos, currentW, -tallNotchH, color);
  if (indicatorW > 0) {

    canvas.fillRect(xPos + w + indicatorLeftMargin, yPos, indicatorW,
                    -tallNotchH, color);
  }

  auto drawNotch = [&](int16_t x, bool tall) {
    canvas.drawFastVLine(x, yPos, tall ? -tallNotchH : -smallNotchH, color);
  };

  for (int16_t x = xPos; x <= xPos + w; x += w / 10) {
    drawNotch(x, (x - xPos) % 5 == 0);
  }

  advanceLayout();
}