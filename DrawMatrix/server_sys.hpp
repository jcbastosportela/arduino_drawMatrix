#ifndef DRAWMATRIX_SERVER_SYS
#define DRAWMATRIX_SERVER_SYS

#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

#include <cstdint>

#include "IMatrixApp.hpp"
#include "IServer.hpp"

namespace ServerSys {

constexpr size_t N_COLS = 32;
constexpr size_t N_ROWS = 24;
constexpr size_t N_PIXELS = N_COLS * N_ROWS;

struct ITask {
    virtual void execute(uint64_t t, uint64_t &d, bool &repeat) = 0;
    virtual ~ITask() = default;
};

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

#endif /* DRAWMATRIX_SERVER_SYS */
