#include "server_sys.hpp"

#include <Adafruit_GFX.h>
#include <Arduino.h>

#include "AsyncTasker.hpp"

using namespace std::placeholders;
constexpr int ws2812_pin = 0;

namespace ServerSys {
// --------------------------------------------------------------------------------------
App::App(IServer &server, const NTPClient& ntp)
    : m_server(server), m_status_led_state(true), task_draw_matrix(), task_heart_beat_blink(m_status_led_state), m_ntp(ntp) {
    AsyncTasker::schedule(1000, std::bind(&HeartBeatBlink::execute, &task_heart_beat_blink, _1, _2, _3), true);
    AsyncTasker::schedule(
        1000,
        [this](uint64_t t, uint64_t &d, bool &repeat) {
            static uint8_t h_pos_x = 0;
            static uint8_t m_pos_x = 4;
            static uint8_t s_pos_x = 8;
            static uint8_t cnt = 0;
            task_draw_matrix.matrix.setTextWrap(false);
            task_draw_matrix.matrix.fillScreen(Adafruit_NeoMatrix::Color(0, 0, 0));
            
            task_draw_matrix.matrix.setCursor(h_pos_x, 0);
            task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(120, 0, 0));
            task_draw_matrix.matrix.printf("%.2u", m_ntp.getHours());

            task_draw_matrix.matrix.setCursor(m_pos_x, 7);
            task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(0, 120, 0));
            task_draw_matrix.matrix.printf("%.2u", m_ntp.getMinutes());

            cnt = m_ntp.getSeconds();
            task_draw_matrix.matrix.setCursor(s_pos_x, 14);
            task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(0, 0, 200));
            task_draw_matrix.matrix.printf("%.2u", cnt);
            AsyncTasker::schedule(100, [&](uint64_t t, uint64_t &d, bool &repeat) {
                task_draw_matrix.matrix.setCursor(s_pos_x, 14);
                task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(0, 0, 120));
                task_draw_matrix.matrix.printf("%.2u ", cnt);
                task_draw_matrix.matrix.show();
                (++s_pos_x) > (N_COLS - 16) ? (s_pos_x = 0) : s_pos_x;
            });

            (++h_pos_x) > (N_COLS - 16) ? (h_pos_x = 0) : h_pos_x;
            (++m_pos_x) > (N_COLS - 16) ? (m_pos_x = 0) : m_pos_x;

            // task_draw_matrix.matrix.setCursor(0, 0);
            // task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(120, 0, 0));
            // task_draw_matrix.matrix.printf("%.2u", m_ntp.getHours());

            // task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(0, 120, 0));
            // task_draw_matrix.matrix.printf("%.2u", m_ntp.getMinutes());

            // cnt = m_ntp.getSeconds();
            // s_pos_x = task_draw_matrix.matrix.getCursorX();
            // task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(0, 0, 200));
            // task_draw_matrix.matrix.printf("%.2u", cnt);
            // AsyncTasker::schedule(100, [&](uint64_t t, uint64_t &d, bool &repeat) {
            //     task_draw_matrix.matrix.setCursor(s_pos_x, 0);
            //     task_draw_matrix.matrix.setTextColor(Adafruit_NeoMatrix::Color(0, 0, 120));
            //     task_draw_matrix.matrix.printf("%.2u ", cnt);
            //     task_draw_matrix.matrix.show();
            // });

            task_draw_matrix.matrix.show();
            Serial.printf("NTP time: %s\n", m_ntp.getFormattedTime().c_str());
        },
        true
    );
    // AsyncTasker::schedule(1, std::bind(&DrawMatrix::execute, &task_draw_matrix, _1, _2, _3), true);
}

// --------------------------------------------------------------------------------------
App::~App() {}

// --------------------------------------------------------------------------------------
void App::run() {
    // uint64_t v=0;
    // bool repeat = true;
    // task_draw_matrix.execute(0, v, repeat);  // Execute the draw matrix task immediately
}

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
    String error_message;
    if (!m_server.hasArg("value")) {
        error_message = "Missing 'value' argument";
        Serial.printf(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If JSON parsing fails, send an error response
    }
    m_status_led_state = static_cast<bool>(m_server.arg("value").toInt());

    digitalWrite(LED_BUILTIN, m_status_led_state ? LOW : HIGH);  // LED_BUILTIN is active LOW

    JsonDocument jsonDoc;
    jsonDoc["status"] = m_status_led_state ? "LED is ON" : "LED is OFF";
    String response;
    serializeJson(jsonDoc, response);
    m_server.send(200, "application/json", response);
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
    String error_message;

    if (!m_server.hasArg("value")) {
        error_message = "Missing 'value' argument";
        Serial.printf(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If JSON parsing fails, send an error response
    }
    uint8_t brightness = static_cast<uint8_t>(m_server.arg("value").toInt());
    task_draw_matrix.set_brightness(brightness);
    m_server.send(200, "text/plain", "Brightness set to " + String(brightness));
}

// --------------------------------------------------------------------------------------
void App::handle_set_display_color() {
    String error_message;

    uint32_t color = 0;
    // color can be set via 'color' or via 'r', 'g', 'b', 'w'
    if (!m_server.hasArg("color") && !m_server.hasArg("r") && !m_server.hasArg("g") && !m_server.hasArg("b") &&
        !m_server.hasArg("w")) {
        error_message = "Missing 'color' or 'r', 'g', 'b', 'w' arguments";
        Serial.printf(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If JSON parsing fails, send an error response
    }
    if (m_server.hasArg("color")) {
        color = static_cast<uint32_t>(m_server.arg("color").toInt());
    } else if (m_server.hasArg("r") && m_server.hasArg("g") && m_server.hasArg("b")) {
        color = (static_cast<uint8_t>(m_server.arg("w").toInt()) << 24) |
                (static_cast<uint8_t>(m_server.arg("r").toInt()) << 16) |
                (static_cast<uint8_t>(m_server.arg("g").toInt()) << 8) |
                (static_cast<uint8_t>(m_server.arg("b").toInt()));
    } else {
        error_message = "Invalid color arguments";
        Serial.printf(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If JSON parsing fails, send an error response
    }

    task_draw_matrix.set_color(color);
    // Create a single-color GIF (16x16, color table with only one color)
    constexpr size_t GIF_WIDTH = 160;
    constexpr size_t GIF_HEIGHT = 160;
#define LOW_HIGH_BYTES(_v) ((_v) & 0xFF), (((_v) >> 8) & 0xFF)
    static uint8_t gif[] = {0x47,
                            0x49,
                            0x46,
                            0x38,
                            0x39,
                            0x61,
                            LOW_HIGH_BYTES(GIF_WIDTH),
                            LOW_HIGH_BYTES(GIF_HEIGHT),
                            0x80,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x00,  // color table: black
                            0x00,
                            0x00,
                            0x00,  // color table: black (padding)
                            0x2C,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x10,
                            0x00,
                            0x10,
                            0x00,
                            0x00,
                            0x02,
                            0x16,
                            0x8C,
                            0x8F,
                            0xA9,
                            0xCB,
                            0xED,
                            0x0F,
                            0xA3,
                            0x9C,
                            0xB4,
                            0xDA,
                            0x8B,
                            0xB3,
                            0xDE,
                            0xBC,
                            0xFB,
                            0x0F,
                            0x00,
                            0x3B};
    // Set the color table to the requested color (first entry)
    gif[16] = (color >> 16) & 0xFF;  // Red
    gif[17] = (color >> 8) & 0xFF;   // Green
    gif[18] = color & 0xFF;          // Blue

    m_server.send(200, "image/gif", gif, sizeof(gif));
}

// --------------------------------------------------------------------------------------
void App::handle_set_display_matrix() {
    String error_message;
    if (!m_server.hasArg("plain")) {
        error_message = "No data received";
        Serial.printf(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If JSON parsing fails, send an error response
    }

    JsonDocument doc;
    if (auto error = deserializeJson(doc, m_server.arg("plain"))) {
        error_message = String("Invalid JSON: ") + String(error.c_str());
        Serial.printf(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If JSON parsing fails, send an error response
    }

    if (!doc.is<JsonArray>() || doc.size() != N_COLS || !doc[0].is<JsonArray>() || doc[0].size() != N_ROWS) {
        error_message = "Invalid matrix format. doc.is<JsonArray>()" + String(doc.is<JsonArray>()) +
                        ", doc.size()=" + String(doc.size()) + ", doc[0].is<JsonArray>()" +
                        String(doc[0].is<JsonArray>()) + ", doc[0].size()=" + String(doc[0].size());
        Serial.println(error_message.c_str());
        m_server.send(400, "text/plain", error_message.c_str());
        return;  // If matrix format is invalid, send an error response
    }

    // Serial.println(doc.as<String>().c_str());  // Print the received matrix for debugging
    task_draw_matrix.set_matrix(doc);
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
DrawMatrix::DrawMatrix()
    : matrix(8, 8, 4, 3, (uint8_t)ws2812_pin,
             (uint8_t)(NEO_TILE_TOP + NEO_TILE_RIGHT + NEO_TILE_COLUMNS +
                       NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS ),
             (neoPixelType)(NEO_GRB + NEO_KHZ800)) {
    matrix.begin();            // Initialize the NeoPixel strip
    matrix.setBrightness(6);  // Set brightness to 15 (0-255)
    matrix.clear();            // Clear the strip
    matrix.show();             // Update the strip to show the changes
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_brightness(uint8_t brightness) {
    matrix.setBrightness(brightness);
    matrix.show();
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_color(uint32_t color) {
    matrix.fill(color, 0, N_PIXELS);  // Fill the matrix with the specified color
    matrix.show();
    this->color = color;
}

// --------------------------------------------------------------------------------------
inline uint16_t next_pixel(uint16_t pixel, uint16_t more_pixels = 1) {
    return (pixel >= (N_PIXELS - more_pixels)) ? (more_pixels - 1) : (pixel + more_pixels);
}

// --------------------------------------------------------------------------------------
inline uint16_t prev_pixel(uint16_t pixel, uint16_t less_pixels = 1) {
    return (pixel < less_pixels) ? ((N_PIXELS)-less_pixels) : (pixel - less_pixels);
}

// --------------------------------------------------------------------------------------
void DrawMatrix::execute(uint64_t t, uint64_t &d, bool &repeat) {
    // static uint16_t p = 0;  // Current pixel index

    // if (!matrix.canShow()) {
    //     Serial.println("Matrix not ready to show, skipping update");
    //     return;  // If the matrix is not ready to show, skip the update
    // }
    // matrix.fill(matrix.ColorHSV(p += 20, 255, 255), 0, N_PIXELS);  // Fill the matrix with a color based on hue
    // matrix.show();
}

// --------------------------------------------------------------------------------------
// Define the pixel indices for a 32x24 matrix, mapping each (col, row) to a physical LED index.
// This is based on the assumption that the matrix is arranged in a specific order, such as zigzag or column-major
// order. The pixel indices are calculated to match the physical layout of the LEDs in the matrix.
constexpr std::array<size_t, N_COLS * N_ROWS> pixel_indices = {

    // 32x24 display: 4 columns x 3 rows of 8x8 boards
    []() {
        std::array<size_t, N_COLS * N_ROWS> arr = {};
        constexpr int BOARD_COLS = 4, BOARD_ROWS = 3;
        constexpr int BOARD_W = 8, BOARD_H = 8;

        for (int board_col = 0; board_col < BOARD_COLS; ++board_col) {
            for (int board_row = 0; board_row < BOARD_ROWS; ++board_row) {
                for (int y = 0; y < BOARD_H; ++y) {
                    for (int x = 0; x < BOARD_W; ++x) {
                        auto local_addr = x + y * BOARD_W;
                        auto global_addr = local_addr + ((BOARD_W * BOARD_H) * (board_row + board_col * BOARD_ROWS));

                        auto gx = N_COLS - (board_col * BOARD_W) - BOARD_W + x;
                        auto gy = y + (board_row * BOARD_H);
                        arr[gx + gy * N_COLS] = global_addr;
                    }
                }
            }
        }
        return arr;
    }()};

// --------------------------------------------------------------------------------------
inline constexpr size_t pixel_index(size_t col, size_t row) {
    if (col >= N_COLS || row >= N_ROWS) {
        return 0;  // Return 0 for out-of-bounds access
    }
    return pixel_indices[row * N_COLS + col];  // Map (col, row) to physical LED index
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_matrix(uint32_t matrix_disp[N_COLS][N_ROWS]) {
    Serial.println("Setting matrix from arrays");
    for (int col = 0; col < N_COLS; col++) {
        for (int row = 0; row < N_ROWS; row++) {
            uint32_t color = matrix_disp[col][row];
            matrix.setPixelColor(pixel_index(col, row), color);
        }
    }
    matrix.show();
}

// --------------------------------------------------------------------------------------
void DrawMatrix::set_matrix(const JsonDocument &matrix_disp) {
    Serial.println("Setting matrix from JSON document");
    for (size_t col = 0; col < N_COLS; col++) {
        for (size_t row = 0; row < N_ROWS; row++) {
            uint32_t rgb = matrix_disp[col][row].as<uint32_t>();
            matrix.setPixelColor(pixel_index(col, row), rgb);
        }
    }
    matrix.show();
}

}  // namespace ServerSys
