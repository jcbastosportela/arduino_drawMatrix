/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      DrawMatrix.ino                                                                                           *
 * @brief     Main entry point for DrawMatrix ESP8266 LED matrix controller                                            *
 * @date      Sat Aug 23 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#include <map>
#include <memory>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <OneButton.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include <AsyncTasker.hpp>

#include "MusicPlayer.hpp"
#include "ServerSys.hpp"

// #define STASSID "your-ssid"
// #define STAPSK "your-password"

#if !defined(STASSID) && !defined(STAPSK)
#include "credentials.hpp"
#endif

// --- Robustness: WiFi and server health check ---
constexpr uint64_t WIFI_CHECK_INTERVAL = 10000;  // milliseconds
constexpr uint64_t SERVER_CHECK_INTERVAL = 5000; // milliseconds
constexpr size_t MAX_NUM_TRIES_NO_CLIENT = 3;    // how many tries before giving up
constexpr size_t NTP_SYNC_PERIOD_MS = 60 * 1000; // milliseconds

const char *const ssid = STASSID;
const char *const password = STAPSK;
AsyncWebServer server(80);
WiFiUDP ntp_udp;
NTPClient ntpClient(ntp_udp, "pool.ntp.org", 2 * 60 * 60, NTP_SYNC_PERIOD_MS);
std::unique_ptr<ServerSys::App> app;

// Global client activity tracking
volatile unsigned long g_lastClientActivity = 0;
volatile unsigned long g_lastDisplayActivity = 0;

void updateClientActivity() {
    g_lastClientActivity = millis();
}

void updateDisplayActivity() {
    g_lastDisplayActivity = millis();
    updateClientActivity(); // Display activity is also client activity
}

constexpr uint8_t BUTTON_PLAY_PAUSE = D1; // GPIO pin for play/pause button
constexpr uint8_t BUTTON_CTRL = D6;       // GPIO pin for control button

std::map<uint8_t, OneButton> buttons = {
    {BUTTON_PLAY_PAUSE, OneButton(BUTTON_PLAY_PAUSE)},
    {BUTTON_CTRL, OneButton(BUTTON_CTRL)},
};

// ======================================================================================
void setup(void) {
    Serial.begin(115200);
    MusicPlayer::init();

    // Configure buttons
    buttons[BUTTON_PLAY_PAUSE].attachClick([]() {
        if(MusicPlayer::get_state() == MusicPlayer::State::STOPPED) {
            Serial.println("No track loaded, playing default track (MUSIC_NATURE)");
            MusicPlayer::play(MusicPlayer::MusicTrack::MUSIC_NATURE);
            return;
        }
        Serial.println("Play/Pause button clicked");
        MusicPlayer::pause();
    });
    buttons[BUTTON_PLAY_PAUSE].attachLongPressStart([]() {
        Serial.println("Play/Pause button long pressed");
        MusicPlayer::stop();
    });
    buttons[BUTTON_PLAY_PAUSE].attachDoubleClick([](){
        Serial.println("Play/Pause button double clicked");
        MusicPlayer::next();
    });

    buttons[BUTTON_CTRL].attachLongPressStart([]() {
        Serial.println("Control button long pressed");
        MusicPlayer::start_volume_change();
    });
    buttons[BUTTON_CTRL].attachLongPressStop([]() {
        Serial.println("Control button long pressed");
        MusicPlayer::stop_volume_change();
    });

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    app = std::make_unique<ServerSys::App>(ntpClient, []() {
        Serial.println("Alarm callback triggered!");
        MusicPlayer::play(MusicPlayer::MusicTrack::MUSIC_ALARM);
        MusicPlayer::set_volume(MusicPlayer::MAX_VOLUME);  // Set volume to maximum
    });

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }

    server.on("/", [](AsyncWebServerRequest *request){
        updateClientActivity();
        app->handle_root(request);
    });
    server.on("/draw", [](AsyncWebServerRequest *request){
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_draw(request);
    });
    server.on("/alarm", [](AsyncWebServerRequest *request){
        updateClientActivity();
        app->handle_alarm(request);
    });
    server.on("/music", [](AsyncWebServerRequest *request){
        updateClientActivity();
        app->handle_music(request);
    });

    server.on("/status_led_control", [](AsyncWebServerRequest *request){
        updateClientActivity();
        app->handle_status_led_control(request);
    });
    server.on("/set_display_brightness", [](AsyncWebServerRequest *request){
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_set_display_brightness(request);
    });
    server.on("/set_display_color", [](AsyncWebServerRequest *request){
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_set_display_color(request);
    });
    server.on("/gif", [](AsyncWebServerRequest *request){
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_gif(request);
    });
    server.on("/set_display_matrix", HTTP_POST, [](AsyncWebServerRequest *request){
        updateDisplayActivity(); // Display-related - disable clock
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        app->handle_set_display_matrix(request, data, len, index, total);
    });
    server.on("/list-alarms", [](AsyncWebServerRequest *request){
        updateClientActivity();
        app->handle_list_alarms(request);
    });
    server.on("/delete-alarm", HTTP_POST, [](AsyncWebServerRequest *request){
        updateClientActivity();
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        app->handle_delete_alarm(request, data, len, index, total);
    });
    server.on("/modify-alarm", HTTP_POST, [](AsyncWebServerRequest *request){
        updateClientActivity();
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        app->handle_modify_alarm(request, data, len, index, total);
    });
    server.on("/set_alarm", HTTP_POST, [](AsyncWebServerRequest *request){
        updateClientActivity();
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        app->handle_set_alarm(request, data, len, index, total);
    });
    server.onNotFound([](AsyncWebServerRequest *request){
        updateClientActivity();
        app->handle_not_found(request);
    });
    server.on("/info", [](AsyncWebServerRequest *request) {
        updateClientActivity(); // Info requests don't affect display
        StaticJsonDocument<512> doc;
        doc["chip_id"] = ESP.getChipId();
        doc["core_version"] = ESP.getCoreVersion();
        doc["sdk_version"] = ESP.getSdkVersion();
        doc["flash_chip_id"] = ESP.getFlashChipId();
        doc["flash_chip_size"] = ESP.getFlashChipSize();
        doc["sketch_size"] = ESP.getSketchSize();
        doc["free_sketch_space"] = ESP.getFreeSketchSpace();
        // doc["heap_size"] = ESP.getHeapSize(); // Uncomment if available
        doc["free_heap"] = ESP.getFreeHeap();
        doc["max_free_block_size"] = ESP.getMaxFreeBlockSize();
        doc["heap_fragmentation"] = ESP.getHeapFragmentation();
        doc["free_stack"] = ESP.getFreeContStack();
        doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
        doc["boot_version"] = ESP.getBootVersion();
        doc["boot_mode"] = ESP.getBootMode();
        doc["reset_reason"] = ESP.getResetReason();

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    server.on("/wifi_off", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        Serial.println("Turning WiFi off...");
        WiFi.disconnect();
        request->send(200, "text/plain", "WiFi turned off");
    });
    server.on("/music_play", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        if (request->hasParam("track")) {
            String trackStr = request->getParam("track")->value();
            Serial.printf("Playing music track: %s\n", trackStr.c_str());
            // convert to int
            int trackInt = trackStr.toInt();
            MusicPlayer::play(static_cast<MusicPlayer::MusicTrack>(trackInt));
            request->send(200, "text/plain", "Playing track: " + trackStr);
            return;
        } else { // pause/play toggle
            MusicPlayer::pause();
            request->send(200, "text/plain", "Toggling play/pause");
            return;
        }
    });
    server.on("/music_stop", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        Serial.println("Stopping music...");
        MusicPlayer::stop();
        request->send(200, "text/plain", "Music stopped");
    });

#if 0
    /////////////////////////////////////////////////////////
    // Hook examples
    server.addHook([](const String &method, const String &url, WiFiClient *client,
                      ESP8266WebServer::ContentTypeFunction contentType) {
        (void)method;       // GET, PUT, ...
        (void)url;          // example: /root/myfile.html
        (void)client;       // the webserver tcp client connection
        (void)contentType;  // contentType(".html") => "text/html"
        Serial.printf("A useless web hook has passed\n");
        Serial.printf("(this hook is in 0x%08x area (401x=IRAM 402x=FLASH))\n", esp_get_program_counter());
        return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
    });

    server.addHook([](const String &, const String &url, WiFiClient *, ESP8266WebServer::ContentTypeFunction) {
        if (url.startsWith("/fail")) {
            Serial.printf("An always failing web hook has been triggered\n");
            return ESP8266WebServer::CLIENT_MUST_STOP;
        }
        return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
    });

    server.addHook([](const String &, const String &url, WiFiClient *client, ESP8266WebServer::ContentTypeFunction) {
        if (url.startsWith("/dump")) {
            Serial.printf("The dumper web hook is on the run\n");

            // Here the request is not interpreted, so we cannot for sure
            // swallow the exact amount matching the full request+content,
            // hence the tcp connection cannot be handled anymore by the
            // webserver.
#ifdef STREAMSEND_API
            // we are lucky
            client->sendAll(Serial, 500);
#else
            auto last = millis();
            while ((millis() - last) < 500) {
                char buf[32];
                size_t len = client->read((uint8_t *)buf, sizeof(buf));
                if (len > 0) {
                    Serial.printf("(<%d> chars)", (int)len);
                    Serial.write(buf, len);
                    last = millis();
                }
            }
#endif
            // Two choices: return MUST STOP and webserver will close it
            //                       (we already have the example with '/fail' hook)
            // or                  IS GIVEN and webserver will forget it
            // trying with IS GIVEN and storing it on a dumb WiFiClient.
            // check the client connection: it should not immediately be closed
            // (make another '/dump' one to close the first)
            Serial.printf("\nTelling server to forget this connection\n");
            static WiFiClient forgetme = *client;  // stop previous one if present and transfer client refcounter
            return ESP8266WebServer::CLIENT_IS_GIVEN;
        }
        return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
    });
    // Hook examples
    /////////////////////////////////////////////////////////
#endif // 0
    server.begin();
    Serial.println("HTTP server started");

    AsyncTasker::schedule(
        SERVER_CHECK_INTERVAL,
        [](uint64_t, uint64_t &, bool &) {
            static size_t n_fails = 0;
            
            // Check if we've had recent DISPLAY activity (not just any client activity)
            unsigned long now = millis();
            bool hasDisplayActivity = (now - g_lastDisplayActivity) < (SERVER_CHECK_INTERVAL * 2);
            
            if (hasDisplayActivity) {
                n_fails = 0;
                Serial.println("Display activity detected - clock mode OFF");
                app->clock_mode(false);
            } else {
                Serial.println("No display activity");
                n_fails++;
                if (n_fails > MAX_NUM_TRIES_NO_CLIENT) {
                    Serial.println("No display activity - enabling clock mode");
                    n_fails = 0;
                    app->clock_mode(true);
                }
            }
        },
        true);

    ntpClient.begin();
    AsyncTasker::schedule(
        WIFI_CHECK_INTERVAL,
        [](uint64_t, uint64_t &, bool &) {
            static size_t fail_sync_count = 0;
            if (!ntpClient.update()) {
                if ((WiFi.status() != WL_CONNECTED) ||
                    (++fail_sync_count > ((NTP_SYNC_PERIOD_MS / WIFI_CHECK_INTERVAL) + 1))) {
                    Serial.println("NTP sync failed. No WiFi? Wifi status: " + String(WiFi.status()) +
                                   ". Re-connecting...");
                    fail_sync_count = 0;
                    WiFi.disconnect();
                    WiFi.begin(ssid, password); // Wait for connection
                    while (WiFi.status() != WL_CONNECTED) {
                        delay(500);
                        Serial.print(".");
                    }
                }

            } else {
                fail_sync_count = 0;
            }
        },
        true);
}

// ======================================================================================
void loop(void) {
    MDNS.update();
    app->run();
    for(auto& [_, button] : buttons) {
        button.tick();
    }
    AsyncTasker::runEventLoop();
}
