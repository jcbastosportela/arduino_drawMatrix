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
#include <NTP.h>

#include <list>
#include <cstdint>

#include "IMatrixApp.hpp"
#include "IServer.hpp"
#include "ITask.hpp"

namespace ServerSys {

// Clock display modes
enum class ClockMode : uint8_t {
    TEXT = 0,        // Scrolling text display
    PROGRESS_BAR = 1, // Filled progress bars
    INDICATOR = 2     // First/last/current dots only
};

// Clock configuration (mode + colors)
struct ClockConfig {
    ClockMode mode = ClockMode::INDICATOR;
    uint32_t hours_color = 0xFF0000;    // Red
    uint32_t minutes_color = 0x00FF00;  // Green
    uint32_t seconds_color = 0x0000FF;  // Blue
};

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

    /**
     * @brief Draw the clock frame on the matrix.
     *
     * Schedules a small sub-task to animate seconds horizontally. Honors
     * clock mode enable flag; when disabled, it returns immediately.
     *
     * @param t Current time (ms)
     * @param d Delay until next execution (out)
     * @param repeat Repeat flag (out)
     * @param ntp NTP client providing hours/minutes/seconds
     * @param clock_mode Whether clock mode is enabled
    * @param clock_config Clock display configuration (mode and colors)
     */
    void draw_clock_task(uint64_t t, uint64_t &d, bool &repeat, NTP &ntp, const bool &clock_mode, const ClockConfig &clock_config);

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
     * @param ntp Reference to the NTP client.
     * @param alarm_callback Callback function for alarm events.
     */
    App(NTP &ntp, std::function<void()> alarm_callback);

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
    virtual void handle_root(AsyncWebServerRequest *request) override;

    /**
     * @brief Handle draw page requests
     */
    virtual void handle_draw(AsyncWebServerRequest *request);

    /**
     * @brief Handle music page requests
     */
    virtual void handle_music(AsyncWebServerRequest *request);

    /**
     * @brief Handle alarm page requests
     */
    virtual void handle_alarm(AsyncWebServerRequest *request);

    /**
     * @brief Handle logging settings page request
     */
    void handle_log_settings(AsyncWebServerRequest *request);

    /**
     * @brief Handle logging config GET (returns JSON) and POST (updates config)
     */
    void handle_log_config(AsyncWebServerRequest *request, uint8_t *data = nullptr, size_t len = 0, size_t index = 0, size_t total = 0);

    /**
     * @brief Handle clearing log files
     */
    void handle_log_clear(AsyncWebServerRequest *request);

    /**
     * @brief Handle downloading log files
     */
    void handle_log_download(AsyncWebServerRequest *request);

    /**
     * @brief Serve live logs viewer page
     */
    void handle_logs_page(AsyncWebServerRequest *request);

    /**
     * @brief Provide JSON tail of logs with optional min level filtering
     */
    void handle_log_entries(AsyncWebServerRequest *request);

    /**
     * @brief Handle not found web requests.
     */
    virtual void handle_not_found(AsyncWebServerRequest *request) override;

    /**
     * @brief Handle status LED control requests.
     */
    virtual void handle_status_led_control(AsyncWebServerRequest *request) override;

    /**
     * @brief Handle GIF display requests.
     */
    virtual void handle_gif(AsyncWebServerRequest *request) override;

    /**
     * @brief Handle display brightness setting requests.
     */
    virtual void handle_set_display_brightness(AsyncWebServerRequest *request) override;

    /**
     * @brief Handle display color setting requests.
     */
    virtual void handle_set_display_color(AsyncWebServerRequest *request) override;

    /**
     * @brief Handle display matrix setting requests.
     */
    virtual void handle_set_display_matrix(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;

    /**
     * @brief Handle alarm setting requests.
     */
    virtual void handle_set_alarm(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;

    /**
     * @brief Handle request to list all alarms
     */
    virtual void handle_list_alarms(AsyncWebServerRequest *request);

    /**
     * @brief Handle request to delete an alarm
     */
    virtual void handle_delete_alarm(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

    /**
     * @brief Handle request to modify an alarm
     */
    virtual void handle_modify_alarm(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

    /**
     * @brief Enable or disable clock mode.
     * @param enable True to enable clock mode, false to disable.
     */
    void clock_mode(bool enable);

        /**
         * @brief Handle clock settings page request
         */
        void handle_clock_settings(AsyncWebServerRequest *request);

        /**
         * @brief Handle clock config GET (returns JSON) and POST (updates config)
         */
        void handle_clock_config(AsyncWebServerRequest *request, uint8_t *data = nullptr, size_t len = 0, size_t index = 0, size_t total = 0);

  private:
    /**
     * @brief Save all alarms to file
     * @return true if successful, false otherwise
     */
    bool save_alarms_to_file();

    /**
     * @brief Save clock settings to file
     * @return true if successful, false otherwise
     */
    bool save_clock_config_to_file();

    /**
     * @brief Load clock settings from file
     * @return true if successful, false otherwise
     */
    bool load_clock_config_from_file();

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

    NTP &m_ntp;
    bool m_status_led_state;
    HeartBeatBlink m_task_heart_beat_blink;
    DrawMatrix m_task_draw_matrix;
    bool m_clock_mode;
    std::list<AlarmConfig> m_alarms;
    ClockConfig m_clock_config;
    std::function<void()> m_alarm_callback;
};

} // namespace ServerSys

#endif /* DRAWMATRIX_SERVERSYS */
