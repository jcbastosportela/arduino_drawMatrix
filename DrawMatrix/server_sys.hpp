#ifndef DRAWMATRIX_SERVER_SYS
#define DRAWMATRIX_SERVER_SYS

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <cstdint>

#include "IApp.hpp"

namespace ServerSys {


struct ITask {
    virtual void execute(uint64_t t, uint64_t &d) = 0;
    virtual ~ITask() = default;
};

struct HeartBeatBlink : public ITask {
    HeartBeatBlink(bool &led_state);
    void execute(uint64_t t, uint64_t &d) override;
    struct State {
        int v;
        uint64_t d;
    };
    size_t idx = 0;
    std::vector<State> states;
    bool& m_led_state;
};

struct DrawMatrix : public ITask {
    DrawMatrix();
    void execute(uint64_t t, uint64_t &d) override;
    void set_brightness(uint8_t brightness);
    void set_color(uint32_t color);
    void set_matrix(uint32_t matrix_disp[8][8]);
    void set_matrix(const JsonDocument& matrix_disp);

    Adafruit_NeoPixel matrix;
    uint8_t hue;
    uint32_t color;
    uint8_t pixel;
};

class App : public IApp {
   public:
    App();
    ~App();

    virtual void run() override;

    void set_led(bool state);
    void set_display_brightness(uint8_t brightness);
    void set_display_color(uint32_t color);
    void set_display_matrix(uint32_t matrix_disp[8][8]);
    void set_display_matrix(const JsonDocument& matrix_disp);

   private:
    HeartBeatBlink task_heart_beat_blink;
    DrawMatrix task_draw_matrix;
    
    bool led_state;

    uint8_t hue = 0;
    uint32_t color = 0;
    uint8_t pixel = 0;
};

}  // namespace ServerSys

#endif   /* DRAWMATRIX_SERVER_SYS */
