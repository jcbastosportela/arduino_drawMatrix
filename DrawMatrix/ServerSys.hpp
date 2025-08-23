/**
 * --------------------------------------------------------------------------- *
 *            DrawMatrix                                                       *
 * @file      ServerSys.hpp                                                    *
 * @brief     Server-side logic for the DrawMatrix application                 *
 * @date      Sun Jul 27 2025                                                  *
 * @author    Joao Carlos Bastos Portela (you@you.you)                         *
 * @copyright 2025 - 2025 Joao Carlos Bastos Portela                           *
 * --------------------------------------------------------------------------- *
 */
/**
 * --------------------------------------------------------------------------- *
 *            DrawMatrix                                                       *
 * @file      ServerSys.hpp                                                    *
 * @brief     Server-side logic for the DrawMatrix application                 *
 * @date      Sunday, 27th July 2025 12:53:51 am                               *
 * @author    Joao Carlos Bastos Portela (you@you.you)                         *
 * @copyright 2025 - 2025 Joao Carlos Bastos Portela                           *
 * --------------------------------------------------------------------------- *
 */
#ifndef DRAWMATRIX_SERVERSYS
#define DRAWMATRIX_SERVERSYS

#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

#include <cstdint>

#include "IMatrixApp.hpp"
#include "IServer.hpp"

namespace ServerSys {

// Number of columns in a single WS2812B-64 LED matrix tile
constexpr uint8_t MATRIX_WIDTH = 8;
// Number of rows in a single WS2812B-64 LED matrix tile
constexpr uint8_t MATRIX_HEIGHT = 8;
// Number of tiles arranged horizontally
constexpr uint8_t N_TILES_X = 4;
// Number of tiles arranged vertically
constexpr uint8_t N_TILES_Y = 3;

// Total columns in the full display (MATRIX_WIDTH * N_TILES_X)
constexpr size_t N_COLS = MATRIX_WIDTH * N_TILES_X;
// Total rows in the full display (MATRIX_HEIGHT * N_TILES_Y)
constexpr size_t N_ROWS = MATRIX_HEIGHT * N_TILES_Y;
// Total number of pixels (N_COLS * N_ROWS)
constexpr size_t N_PIXELS = N_COLS * N_ROWS;

struct HeartBeatBlink : public ITask {
    HeartBeatBlink(bool &led_state);
    void execute(uint64_t t, uint64_t &d, bool &repeat) override;
    struct State {
        int v;
        uint64_t d;
    };
    size_t idx = 0;
    std::vector<State> states;
    bool &m_led_state;
};

struct DrawMatrix : public ITask {
    DrawMatrix();
    void execute(uint64_t t, uint64_t &d, bool &repeat) override;
    void set_brightness(uint8_t brightness);
    void set_color(uint32_t color);
    void set_matrix(uint32_t matrix_disp[N_COLS][N_ROWS]);
    void set_matrix(const JsonDocument &matrix_disp);

    Adafruit_NeoMatrix matrix;
    uint8_t hue;
    uint32_t color;
    uint8_t pixel;
};

class App : public IMatrixApp {
   public:
    App(IServer &server, const NTPClient& ntp);
    ~App();

    virtual void run() override;
    virtual void handle_root() override;
    virtual void handle_not_found() override;
    virtual void handle_status_led_control() override;
    virtual void handle_gif() override;
    virtual void handle_set_display_brightness() override;
    virtual void handle_set_display_color() override;
    virtual void handle_set_display_matrix() override;

   private:
    IServer &m_server;
    const NTPClient& m_ntp;
    HeartBeatBlink task_heart_beat_blink;
    DrawMatrix task_draw_matrix;
    bool m_status_led_state;
};

}  // namespace ServerSys

#endif /* DRAWMATRIX_SERVERSYS */
