#include "server_sys.hpp"

#include <Arduino.h>

#include "AsyncTasker.hpp"

using namespace std::placeholders;
constexpr int ws2812_pin = 0;

namespace ServerSys {
// --------------------------------------------------------------------------------------
App::App(IServer &server)
    : m_server(server), m_status_led_state(true), task_draw_matrix(), task_heart_beat_blink(m_status_led_state) {
    AsyncTasker::schedule(1000, std::bind(&HeartBeatBlink::execute, &task_heart_beat_blink, _1, _2, _3), true);
    AsyncTasker::schedule(500, std::bind(&DrawMatrix::execute, &task_draw_matrix, _1, _2, _3), true);
}

// --------------------------------------------------------------------------------------
App::~App() {}

// --------------------------------------------------------------------------------------
void App::run() {}

// --------------------------------------------------------------------------------------
void App::handle_root() { m_server.send(200, "text/plain", "hello from esp8266!\r\n"); }

// --------------------------------------------------------------------------------------
void App::handle_not_found() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += m_server.uri();
    message += "\nMethod: ";
    message += (m_server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += m_server.args();
    message += "\n";
    for (uint8_t i = 0; i < m_server.args(); i++) {
        message += " " + m_server.argName(i) + ": " + m_server.arg(i) + "\n";
    }
    m_server.send(404, "text/plain", message);
}

// --------------------------------------------------------------------------------------
void App::handle_status_led_control() {
    try {
        if (!m_server.hasArg("value")) {
            throw std::runtime_error("Missing 'value' argument");
        }
        m_status_led_state = static_cast<bool>(m_server.arg("value").toInt());

        digitalWrite(LED_BUILTIN, m_status_led_state ? LOW : HIGH);  // LED_BUILTIN is active LOW

        JsonDocument jsonDoc;
        jsonDoc["status"] = m_status_led_state ? "LED is ON" : "LED is OFF";
        String response;
        serializeJson(jsonDoc, response);
        m_server.send(200, "application/json", response);
    } catch (const std::exception &e) {
        m_server.send(400, "text/plain", e.what());
    }
}

// --------------------------------------------------------------------------------------
void App::handle_gif() {
    static const uint8_t gif[] PROGMEM = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
                                          0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
                                          0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
                                          0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
                                          0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b};
    uint8_t gif_colored[sizeof(gif)];
    memcpy_P(gif_colored, gif, sizeof(gif));
    // Set the background to a random set of colors
    gif_colored[16] = millis() % 256;
    gif_colored[17] = millis() % 256;
    gif_colored[18] = millis() % 256;
    m_server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
}

// --------------------------------------------------------------------------------------
HeartBeatBlink::HeartBeatBlink(bool &led_state) : m_led_state(led_state) {
    states.push_back({HIGH, 200});
    states.push_back({LOW, 100});
    states.push_back({HIGH, 200});
    states.push_back({LOW, 700});
    pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

// --------------------------------------------------------------------------------------
void App::handle_set_display_brightness() {
    try {
        if (!m_server.hasArg("value")) {
            throw std::runtime_error("Missing 'value' argument");
        }
        uint8_t brightness = static_cast<uint8_t>(m_server.arg("value").toInt());
        task_draw_matrix.set_brightness(brightness);
        m_server.send(200, "text/plain", "Brightness set to " + String(brightness));
    } catch (const std::exception &e) {
        m_server.send(400, "text/plain", e.what());
    }
}

// --------------------------------------------------------------------------------------
void App::handle_set_display_color() {
    try {
        uint32_t color = 0;
        // color can be set via 'color' or via 'r', 'g', 'b', 'w'
        if (!m_server.hasArg("color") && !m_server.hasArg("r") && !m_server.hasArg("g") && !m_server.hasArg("b") &&
            !m_server.hasArg("w")) {
            throw std::runtime_error("Missing 'color' or 'r', 'g', 'b', 'w' arguments");
        }
        if (m_server.hasArg("color")) {
            color = static_cast<uint32_t>(m_server.arg("color").toInt());
        } else if (m_server.hasArg("r") && m_server.hasArg("g") && m_server.hasArg("b")) {
            color = (static_cast<uint8_t>(m_server.arg("w").toInt()) << 24) |
                    (static_cast<uint8_t>(m_server.arg("r").toInt()) << 16) |
                    (static_cast<uint8_t>(m_server.arg("g").toInt()) << 8) |
                    (static_cast<uint8_t>(m_server.arg("b").toInt()));
        } else {
            throw std::runtime_error("Invalid color arguments");
        }

        task_draw_matrix.set_color(color);
        // Create a single-color GIF (16x16, color table with only one color)
        static uint8_t gif[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x10, 0x00, 0x10, 0x00,
                                0x80, 0x00, 0x00, 0x00, 0x00, 0x00,  // color table: black
                                0x00, 0x00, 0x00,                    // color table: black (padding)
                                0x2C, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00,
                                0x02, 0x16, 0x8C, 0x8F, 0xA9, 0xCB, 0xED, 0x0F, 0xA3, 0x9C,
                                0xB4, 0xDA, 0x8B, 0xB3, 0xDE, 0xBC, 0xFB, 0x0F, 0x00, 0x3B};
        // Set the color table to the requested color (first entry)
        gif[16] = (color >> 16) & 0xFF;  // Red
        gif[17] = (color >> 8) & 0xFF;   // Green
        gif[18] = color & 0xFF;          // Blue

        m_server.send(200, "image/gif", gif, sizeof(gif));

    } catch (const std::exception &e) {
        m_server.send(400, "text/plain", e.what());
    }
}

// --------------------------------------------------------------------------------------
void App::handle_set_display_matrix() {
    try {
        if (!m_server.hasArg("plain")) {
            Serial.println("No data received");
            throw std::runtime_error("No data received");
        }

        JsonDocument doc;
        if (auto error = deserializeJson(doc, m_server.arg("plain"))) {
            Serial.printf("Failed to parse JSON: %s\n", error.c_str());
            throw std::runtime_error((String("Invalid JSON: ") + String(error.c_str())).c_str());
        }

        if (!doc.is<JsonArray>() || doc.size() != 8 || !doc[0].is<JsonArray>() || doc[0].size() != 8) {
            Serial.println("Invalid matrix format");
            throw std::runtime_error("Invalid matrix format");
        }
        task_draw_matrix.set_matrix(doc);
    } catch (const std::exception &e) {
        m_server.send(400, "text/plain", e.what());
    }
}

// --------------------------------------------------------------------------------------
void HeartBeatBlink::execute(uint64_t t, uint64_t &d, bool &repeat) {
    if (!m_led_state) {
        return;
    }

    // Serial.printf("heart_beat_blink t %d | d %d | idx %d\n", t, d, idx);

    digitalWrite(LED_BUILTIN, !states[idx].v);
    d = states[idx].d;
    idx = (idx < (states.size() - 1)) ? ++idx : 0;
}

// --------------------------------------------------------------------------------------
DrawMatrix::DrawMatrix() : matrix(8 * 8, ws2812_pin, NEO_GRB + NEO_KHZ800), hue(0), color(0), pixel(0) {
    matrix.begin();            // Initialize the NeoPixel strip
    matrix.setBrightness(50);  // Set brightness to 50 (0-255)
    matrix.clear();            // Clear the strip
    matrix.show();             // Update the strip to show the changes
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_brightness(uint8_t brightness) { matrix.setBrightness(brightness); }

// --------------------------------------------------------------------------------------
void DrawMatrix::set_color(uint32_t color) { this->color = color; }

// --------------------------------------------------------------------------------------
void DrawMatrix::execute(uint64_t t, uint64_t &d, bool &repeat) {
    // nothing to do
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
void DrawMatrix::set_matrix(const JsonDocument &matrix_disp) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            String colorHex = matrix_disp[row][col].as<const char *>();
            uint32_t rgb = strtoul(colorHex.c_str() + 1, NULL, 16);  // Skip the '#' character
            uint8_t r = (rgb >> 16) & 0xFF;
            uint8_t g = (rgb >> 8) & 0xFF;
            uint8_t b = rgb & 0xFF;
            matrix.setPixelColor(row * 8 + col, matrix.Color(r, g, b));
        }
    }
    matrix.show();
}

}  // namespace ServerSys
