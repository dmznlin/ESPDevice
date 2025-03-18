#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_PIN 8 // 板载RGB灯珠的引脚，根据实际使用的开发板型号而定
#define LED_COUNT 1 // LED灯条的灯珠数量（板载的是一颗）

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50); // 设置亮度（0-255范围）
}

void loop() {
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // 设置灯珠为红色 (R, G, B)
  strip.show(); // 显示颜色
  delay(1000); // 延迟1秒

  strip.setPixelColor(0, strip.Color(0, 255, 0)); // 设置灯珠为绿色 (R, G, B)
  strip.show(); // 显示颜色
  delay(1000); // 延迟1秒

  strip.setPixelColor(0, strip.Color(0, 0, 255)); // 设置灯珠为蓝色 (R, G, B)
  strip.show(); // 显示颜色
  delay(1000); // 延迟1秒
}