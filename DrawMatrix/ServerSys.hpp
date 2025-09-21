/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      ServerSys.hpp                                                                                            *
 * @brief     Core system definitions and interfaces for DrawMatrix server *
 * @date      Sat Aug 23 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#ifndef DRAWMATRIX_SERVERSYS
#define DRAWMATRIX_SERVERSYS

#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

#include <list>
#include <cstdint>

#include "IMatrixApp.hpp"
#include "IServer.hpp"
#include "ITask.hpp"

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

/**
 * @brief Task for blinking a heartbeat LED.
 */
struct HeartBeatBlink : public ITask {
    /**
     * @brief Construct a HeartBeatBlink task.
     * @param led_state Reference to the LED state variable.
     */
    HeartBeatBlink(bool &led_state);

    /**
     * @brief Execute the heartbeat blink logic.
     * @param t Current time in milliseconds.
     * @param d Reference to delay until next execution (output).
     * @param repeat Reference to repeat flag (output).
     */
    void execute(uint64_t t, uint64_t &d, bool &repeat) override;

    /**
     * @brief State for heartbeat blink sequence.
     */
    struct State {
        int v;
        uint64_t d;
    };
    size_t idx = 0;
    std::vector<State> states;
    bool &m_led_state;
};

/**
 * @brief Task for drawing and controlling the LED matrix display.
 */
struct DrawMatrix : public ITask {
    /**
     * @brief Construct a DrawMatrix task.
     */
    DrawMatrix();

    /**
     * @brief Execute the matrix drawing logic.
     * @param t Current time in milliseconds.
     * @param d Reference to delay until next execution (output).
     * @param repeat Reference to repeat flag (output).
     */
    void execute(uint64_t t, uint64_t &d, bool &repeat) override;

    /**
     * @brief Set the display brightness.
     * @param brightness Brightness value (0-255).
     */
    void set_brightness(uint8_t brightness);

    /**
     * @brief Set the display color.
     * @param color RGB color value.
     */
    void set_color(uint32_t color);

    /**
     * @brief Set the display matrix from a 2D array.
     * @param matrix_disp 2D array of pixel colors.
     */
    void set_matrix(uint32_t matrix_disp[N_COLS][N_ROWS]);

    /**
     * @brief Set the display matrix from a JSON document.
     * @param matrix_disp JSON document containing matrix data.
     */
    void set_matrix(const JsonDocument &matrix_disp);

    Adafruit_NeoMatrix matrix;
    uint8_t hue;
    uint32_t color;
    uint8_t pixel;
};

/**
 * @brief Main application class for DrawMatrix server.
 */
class App : public IMatrixApp {
  public:
    /**
     * @brief Construct the App.
     * @param server Reference to the server interface.
     * @param ntp Reference to the NTP client.
     */
    App(IServer &server, const NTPClient &ntp, std::function<void()> alarm_callback);

    /**
     * @brief Destructor.
     */
    ~App();

    /**
     * @brief Run the main application loop.
     */
    virtual void run() override;

    /**
     * @brief Handle the root web request.
     */
    virtual void handle_root() override;

    /**
     * @brief Handle draw page requests
     */
    virtual void handle_draw();

    /**
     * @brief Handle music page requests
     */
    virtual void handle_music();
    
    /**
     * @brief Handle alarm page requests
     */
    virtual void handle_alarm();

    /**
     * @brief Handle not found web requests.
     */
    virtual void handle_not_found() override;

    /**
     * @brief Handle status LED control requests.
     */
    virtual void handle_status_led_control() override;

    /**
     * @brief Handle GIF display requests.
     */
    virtual void handle_gif() override;

    /**
     * @brief Handle display brightness setting requests.
     */
    virtual void handle_set_display_brightness() override;

    /**
     * @brief Handle display color setting requests.
     */
    virtual void handle_set_display_color() override;

    /**
     * @brief Handle display matrix setting requests.
     */
    virtual void handle_set_display_matrix() override;

    /**
     * @brief Handle alarm setting requests.
     */
    virtual void handle_set_alarm() override;

    /**
     * @brief Handle request to list all alarms
     */
    virtual void handle_list_alarms();

    /**
     * @brief Handle request to delete an alarm
     */
    virtual void handle_delete_alarm();

    /**
     * @brief Handle request to modify an alarm
     */
    virtual void handle_modify_alarm();

    /**
     * @brief Enable or disable clock mode.
     * @param enable True to enable clock mode, false to disable.
     */
    void clock_mode(bool enable);

  private:
    /**
     * @brief Save all alarms to file
     * @return true if successful, false otherwise
     */
    bool save_alarms_to_file();

  private:
    /**
     * @brief Structure to hold alarm configuration
     */
    struct AlarmConfig {
        String time;        // Time in HH:MM format
        uint8_t days;      // Bitfield for days (bit 0 = Sunday, bit 1 = Monday, etc.)
        
        bool isActiveOnDay(int day) const {
            return (days & (1 << day)) != 0;
        }

        bool operator==(const AlarmConfig& other) const {
            return time == other.time && days == other.days;
        }
    };
    
    IServer &m_server;
    const NTPClient &m_ntp;
    HeartBeatBlink task_heart_beat_blink;
    DrawMatrix task_draw_matrix;
    bool m_status_led_state;
    bool m_clock_mode;
    std::list<AlarmConfig> m_alarms;
    std::function<void()> m_alarm_callback;
};

} // namespace ServerSys

#endif /* DRAWMATRIX_SERVERSYS */
