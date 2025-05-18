#include "server_sys.hpp"

#include <Arduino.h>

#include "AsyncTasker.hpp"

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
    AsyncTasker::schedule(1000, std::bind(&HeartBeatBlink::execute, &task_heart_beat_blink, _1, _2), true);
    AsyncTasker::schedule(500, std::bind(&DrawMatrix::execute, &task_draw_matrix, _1, _2), true);
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
void App::set_display_matrix(uint32_t matrix_disp[8][8]) {
    task_draw_matrix.set_matrix(matrix_disp);
}

// --------------------------------------------------------------------------------------
void App::set_display_matrix(const JsonDocument& matrix_disp) {
    task_draw_matrix.set_matrix(matrix_disp);
}

// --------------------------------------------------------------------------------------
void HeartBeatBlink::execute(uint64_t t, uint64_t &d) {
    if (!m_led_state) {
        return;
    }

    // Serial.printf("heart_beat_blink t %d | d %d | idx %d\n", t, d, idx);

    digitalWrite(LED_BUILTIN, !states[idx].v);
    d = states[idx].d;
    idx = (idx < (states.size() - 1)) ? ++idx : 0;
}

// --------------------------------------------------------------------------------------
void DrawMatrix::execute(uint64_t t, uint64_t &d) {
    // matrix.clear();
    // matrix.fill(color, 0, matrix.numPixels());
    // matrix.show();
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_matrix(uint32_t matrix_disp[8][8]) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            uint32_t color = matrix_disp[row][col];
            matrix.setPixelColor(row * 8 + col, color);
        }
    }
    matrix.show();
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_matrix(const JsonDocument& matrix_disp) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            String colorHex = matrix_disp[row][col].as<const char*>();
            uint32_t rgb = strtoul(colorHex.c_str() + 1, NULL, 16); // Skip the '#' character
            uint8_t r = (rgb >> 16) & 0xFF;
            uint8_t g = (rgb >> 8) & 0xFF;
            uint8_t b = rgb & 0xFF;
            matrix.setPixelColor(row * 8 + col, matrix.Color(r, g, b));
        }
    }
    matrix.show();
}

}  // namespace ServerSys
