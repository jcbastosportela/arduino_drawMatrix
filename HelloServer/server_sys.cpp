#include "server_sys.hpp"

#include <Arduino.h>

#include "Asynchrony.hpp"

using namespace std::placeholders;
constexpr int ws2812_pin = 0;

namespace ServerSys {
HeartBeatBlink::HeartBeatBlink(bool &led_state) : m_led_state(led_state) {
    states.push_back({HIGH, 200});
    states.push_back({LOW, 100});
    states.push_back({HIGH, 200});
    states.push_back({LOW, 700});
    pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

// --------------------------------------------------------------------------------------
DrawMatrix::DrawMatrix() : matrix(8*8, ws2812_pin, NEO_GRB + NEO_KHZ800), hue(0), color(0), pixel(0) {
    matrix.begin();  // Initialize the NeoPixel strip
    matrix.setBrightness(50);  // Set brightness to 50 (0-255)
    matrix.clear();  // Clear the strip
    matrix.show();   // Update the strip to show the changes
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_brightness(uint8_t brightness) {
    matrix.setBrightness(brightness);
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_color(uint32_t color) {
    this->color = color;
}

// --------------------------------------------------------------------------------------
App::App() : led_state(true), task_draw_matrix(), task_heart_beat_blink(led_state) {
    Asynchrony::schedule(1000, std::bind(&HeartBeatBlink::execute, &task_heart_beat_blink, _1, _2), true);
    Asynchrony::schedule(500, std::bind(&DrawMatrix::execute, &task_draw_matrix, _1, _2), true);
}

// --------------------------------------------------------------------------------------
App::~App() {}

// --------------------------------------------------------------------------------------
void App::run() {}

// --------------------------------------------------------------------------------------
void App::set_led(bool state) {
    led_state = state;
    digitalWrite(LED_BUILTIN, led_state ? LOW : HIGH);
}

// --------------------------------------------------------------------------------------
void App::set_display_brightness(uint8_t brightness) {
    task_draw_matrix.set_brightness(brightness);
}

// --------------------------------------------------------------------------------------
void App::set_display_color(uint32_t color) {
    task_draw_matrix.set_color(color);
}

// --------------------------------------------------------------------------------------
void HeartBeatBlink::execute(uint64_t t, uint64_t &d) {
    if (!m_led_state) {
        return;
    }

    Serial.printf("heart_beat_blink t %d | d %d | idx %d\n", t, d, idx);

    digitalWrite(LED_BUILTIN, !states[idx].v);
    d = states[idx].d;
    idx = (idx < (states.size() - 1)) ? ++idx : 0;
}

// --------------------------------------------------------------------------------------
void DrawMatrix::execute(uint64_t t, uint64_t &d) {
    matrix.clear();
    // matrix.setPixelColor(pixel, color);
    matrix.fill(color, 0, matrix.numPixels());
    matrix.show();
    // Serial.printf(">task_draw_matrix color %d\n", color);
    // Serial.printf(">task_draw_matrix time %lu | delay %lu | hue %u | color %lu\n", t, d, hue, color);

    // hue++;
    // color+=500;
    // pixel = (pixel < (matrix.numPixels() - 1)) ? ++pixel : 0;
    // Serial.printf("<task_draw_matrix t %d | d %d | hue %d | color %d\n", t, d, hue, color);
}
}  // namespace ServerSys
