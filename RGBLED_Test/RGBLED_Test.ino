#include <Adafruit_NeoPixel.h>

#define PIN 12

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show();
}

void loop() {
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(500);
}
