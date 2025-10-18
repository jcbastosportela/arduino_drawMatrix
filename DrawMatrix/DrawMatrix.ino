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

/*
 * MEMORY CONFIGURATION NOTES:
 *
 * Arduino IDE Settings Required:
 * - Tools → Board: "NodeMCU 1.0 (ESP-12E Module)" or equivalent
 * - Tools → MMU: "16KB cache + 48KB IRAM" (CRITICAL for IRAM usage)
 * - Tools → CPU Frequency: "160MHz"
 * - Tools → Flash Size: "4MB (FS:2MB OTA:~1019KB)"
 * - Tools → Debug Level: "None"
 *
 * These settings are NOT stored in workspace - must be set in Arduino IDE!
 */
#include <map>
#include <memory>

// Memory optimization defines - MUST be before library includes
#define ASYNC_TCP_SSL_ENABLED 0
#define ASYNCWEBSERVER_REGEX 0
#define WS_MAX_QUEUED_MESSAGES 4
#define DEFAULT_MAX_WS_CLIENTS 2

// LWIP Low Memory Configuration - reduces TCP/IP stack IRAM usage
// Note: Keep IGMP enabled for mDNS compatibility
#define LWIP_IPV6 0
#define TCP_MSS 536
#define TCP_SND_BUF (2 * TCP_MSS)
#define TCP_WND (2 * TCP_MSS)
#define LWIP_TCP_KEEPALIVE 0

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <NTPClient.h>
#include <OneButton.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include <AsyncTasker.hpp>

#include "Logger.hpp"
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

void updateClientActivity() { g_lastClientActivity = millis(); }

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

    // Initialize Logger
    Logger::Log::instance().begin(Logger::LOG_LEVEL_DEBUG);
    LOG_INFO("MAIN", "DrawMatrix starting up...");

    // Configure buttons
    buttons[BUTTON_PLAY_PAUSE].attachClick([]() {
        if (MusicPlayer::get_state() == MusicPlayer::State::STOPPED) {
            LOG_DEBUG("BUTTON", "No track loaded, playing default track (MUSIC_NATURE)");
            Serial.println("No track loaded, playing default track (MUSIC_NATURE)");
            MusicPlayer::play(MusicPlayer::MusicTrack::MUSIC_NATURE);
            return;
        }
        LOG_DEBUG("BUTTON", "Play/Pause button clicked");
        Serial.println("Play/Pause button clicked");
        MusicPlayer::pause();
    });
    buttons[BUTTON_PLAY_PAUSE].attachLongPressStart([]() {
        Serial.println("Play/Pause button long pressed");
        MusicPlayer::stop();
    });
    buttons[BUTTON_PLAY_PAUSE].attachDoubleClick([]() {
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
        MusicPlayer::set_volume(MusicPlayer::MAX_VOLUME); // Set volume to maximum
    });

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("*");
    }
    Serial.println("");
    LOG_INFO("WIFI", "Connected to %s", ssid);
    LOG_INFO("WIFI", "IP address: %s", WiFi.localIP().toString().c_str());
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
        LOG_INFO("MDNS", "MDNS responder started");
        Serial.println("MDNS responder started");
    }
    MusicPlayer::init();

    server.on("/", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        app->handle_root(request);
    });
    server.on("/draw", [](AsyncWebServerRequest *request) {
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_draw(request);
    });
    server.on("/alarm", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        app->handle_alarm(request);
    });
    server.on("/music", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        app->handle_music(request);
    });

    // Returns JSON with music subsystem info (folders, tracks, current track, volume, online)
    server.on("/music_info", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        StaticJsonDocument<768> doc;

        MusicPlayer::run();

        doc["sd_online"] = MusicPlayer::sd_online();
        doc["total_folders"] = MusicPlayer::total_folders();
        doc["total_tracks"] = MusicPlayer::total_tracks();
        doc["current_track"] = MusicPlayer::current_track();
        doc["current_folder"] = MusicPlayer::current_folder();
        doc["volume"] = MusicPlayer::get_volume();
        doc["has_content_data"] = MusicPlayer::has_content_data();
        doc["state"] = (int)MusicPlayer::get_state(); // 0=STOPPED, 1=PLAYING, 2=PAUSED
        doc["playback_mode"] = (int)MusicPlayer::get_playback_mode();
        doc["eq_mode"] = (int)MusicPlayer::get_eq_mode();

        JsonArray arr = doc.createNestedArray("folders");

        // Use content data if available, otherwise show empty
        if (MusicPlayer::has_content_data()) {
            uint8_t folderCount = MusicPlayer::get_content_folder_count();
            for (uint8_t i = 0; i < folderCount; ++i) {
                uint8_t folderId;
                uint16_t trackCount;
                String folderName;
                if (MusicPlayer::get_content_folder(i, folderId, trackCount, folderName)) {
                    JsonObject fo = arr.createNestedObject();
                    fo["folder"] = folderId;
                    fo["tracks"] = trackCount;
                    fo["name"] = folderName;
                }
            }
        }

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    server.on("/status_led_control", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        app->handle_status_led_control(request);
    });
    server.on("/set_display_brightness", [](AsyncWebServerRequest *request) {
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_set_display_brightness(request);
    });
    server.on("/set_display_color", [](AsyncWebServerRequest *request) {
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_set_display_color(request);
    });
    server.on("/gif", [](AsyncWebServerRequest *request) {
        updateDisplayActivity(); // Display-related - disable clock
        app->handle_gif(request);
    });
    server.on(
        "/set_display_matrix", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            updateDisplayActivity(); // Display-related - disable clock
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            app->handle_set_display_matrix(request, data, len, index, total);
        });
    server.on("/list-alarms", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        app->handle_list_alarms(request);
    });
    server.on(
        "/delete-alarm", HTTP_POST, [](AsyncWebServerRequest *request) { updateClientActivity(); }, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            app->handle_delete_alarm(request, data, len, index, total);
        });
    server.on(
        "/modify-alarm", HTTP_POST, [](AsyncWebServerRequest *request) { updateClientActivity(); }, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            app->handle_modify_alarm(request, data, len, index, total);
        });
    server.on(
        "/set_alarm", HTTP_POST, [](AsyncWebServerRequest *request) { updateClientActivity(); }, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            app->handle_set_alarm(request, data, len, index, total);
        });
    server.onNotFound([](AsyncWebServerRequest *request) {
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
    // Play a specific folder + track: /music_play_folder?folder=1&track=2
    server.on("/music_play_folder", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        if (!request->hasParam("folder") || !request->hasParam("track")) {
            request->send(400, "text/plain", "Missing folder or track parameter");
            return;
        }
        int folder = request->getParam("folder")->value().toInt();
        int track = request->getParam("track")->value().toInt();
        Serial.printf("Play folder %d track %d\n", folder, track);
        MusicPlayer::play_folder_track(static_cast<uint8_t>(folder), static_cast<uint16_t>(track));
        request->send(200, "text/plain", "Playing folder " + String(folder) + " track " + String(track));
    });

    // List tracks in a folder: /music_list?folder=1
    // Control endpoints: next, prev, set_volume
    server.on("/music_next", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        MusicPlayer::next();
        request->send(200, "text/plain", "next");
    });
    server.on("/music_prev", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        MusicPlayer::prev();
        request->send(200, "text/plain", "prev");
    });
    // Set volume: /music_set_volume?v=20
    server.on("/music_set_volume", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        if (!request->hasParam("volume")) {
            request->send(400, "text/plain", "Missing volume parameter");
            return;
        }
        uint8_t vol = request->getParam("volume")->value().toInt();
        if (vol > MusicPlayer::MAX_VOLUME) {
            request->send(400, "text/plain", "Volume exceeds maximum");
            return;
        }
        MusicPlayer::set_volume(vol);
        request->send(200, "text/plain", "Volume set to " + String(vol));
    });

    // Upload SD card content description (JSON)
    server.on(
        "/music_upload_content", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            updateClientActivity();
            request->send(200, "text/plain", "Upload complete");
        },
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonContent = "";

            if (index == 0) {
                jsonContent = "";
            }

            for (size_t i = 0; i < len; i++) {
                jsonContent += (char)data[i];
            }

            if (index + len == total) {
                bool success = MusicPlayer::upload_sd_content(jsonContent);
                if (!success) {
                    request->send(400, "text/plain", "Failed to parse or save JSON content");
                }
            }
        });

    // Get list of tracks in a specific folder
    server.on("/music_list", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        if (!request->hasParam("folder")) {
            request->send(400, "text/plain", "Missing folder parameter");
            return;
        }

        uint8_t folder = request->getParam("folder")->value().toInt();
        StaticJsonDocument<1024> doc;

        doc["folder"] = folder;
        doc["has_content_data"] = MusicPlayer::has_content_data();

        JsonArray tracks = doc.createNestedArray("tracks");

        if (MusicPlayer::has_content_data()) {
            // Use the new function to get tracks with proper IDs
            MusicPlayer::get_folder_tracks(folder, tracks);
        } else {
            // Fallback: just list track numbers
            uint16_t trackCount = MusicPlayer::tracks_in_folder(folder);
            for (uint16_t i = 1; i <= trackCount; ++i) {
                JsonObject track = tracks.createNestedObject();
                track["id"] = i;
                track["name"] = "Track " + String(i);
            }
        }

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    server.on("/music_stop", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        Serial.println("Stopping music...");
        MusicPlayer::stop();
        request->send(200, "text/plain", "Music stopped");
    });

    // Set playback mode: /music_set_playback_mode?mode=0 (0=normal, 1=repeat_all, 2=repeat_one, 3=shuffle)
    server.on("/music_set_playback_mode", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        if (!request->hasParam("mode")) {
            request->send(400, "text/plain", "Missing mode parameter");
            return;
        }
        int mode = request->getParam("mode")->value().toInt();
        if (mode < 0 || mode > 3) {
            request->send(400, "text/plain", "Invalid mode (0-3)");
            return;
        }
        MusicPlayer::set_playback_mode(static_cast<MusicPlayer::PlaybackMode>(mode));
        request->send(200, "text/plain", "Playback mode set to " + String(mode));
    });

    // Set EQ mode: /music_set_eq?eq=0 (0=normal, 1=pop, 2=rock, 3=jazz, 4=classic, 5=bass)
    server.on("/music_set_eq", [](AsyncWebServerRequest *request) {
        updateClientActivity();
        if (!request->hasParam("eq")) {
            request->send(400, "text/plain", "Missing eq parameter");
            return;
        }
        int eq = request->getParam("eq")->value().toInt();
        if (eq < 0 || eq > 5) {
            request->send(400, "text/plain", "Invalid EQ (0-5)");
            return;
        }
        MusicPlayer::set_eq_mode(static_cast<MusicPlayer::EQMode>(eq));
        request->send(200, "text/plain", "EQ mode set to " + String(eq));
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
    LOG_INFO("SERVER", "HTTP server started");
    Serial.println("HTTP server started");

    // Initialize AsyncElegantOTA
    ElegantOTA.begin(&server);
    LOG_INFO("OTA", "OTA Update available at: http://%s/update", WiFi.localIP().toString().c_str());
    Serial.println("OTA Update available at: http://" + WiFi.localIP().toString() + "/update");

    AsyncTasker::schedule(
        SERVER_CHECK_INTERVAL,
        [](uint64_t, uint64_t &, bool &) {
            static size_t n_fails = 0;

            // Check if we've had recent DISPLAY activity (not just any client activity)
            unsigned long now = millis();
            bool hasDisplayActivity = (now - g_lastDisplayActivity) < (SERVER_CHECK_INTERVAL * 2);

            if (hasDisplayActivity) {
                n_fails = 0;
                LOG_DEBUG("DISPLAY", "Display activity detected - clock mode OFF");
                Serial.println("Display activity detected - clock mode OFF");
                app->clock_mode(false);
            } else {
                LOG_DEBUG("DISPLAY", "No display activity");
                Serial.println("No display activity");
                n_fails++;
                if (n_fails > MAX_NUM_TRIES_NO_CLIENT) {
                    LOG_INFO("DISPLAY", "No display activity - enabling clock mode");
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
            static bool reconnecting = false;
            static unsigned long reconnect_start = 0;
            constexpr unsigned long RECONNECT_TIMEOUT = 30000; // 30 seconds timeout

            // If currently reconnecting, check status
            if (reconnecting) {
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("\nWiFi reconnected successfully!");
                    reconnecting = false;
                    fail_sync_count = 0;
                    return;
                } else if (millis() - reconnect_start > RECONNECT_TIMEOUT) {
                    Serial.println("\nWiFi reconnection timeout. Retrying...");
                    WiFi.disconnect();
                    WiFi.begin(ssid, password);
                    reconnect_start = millis();
                }
                return; // Exit early while reconnecting
            }

            // Normal NTP sync check
            if (!ntpClient.update()) {
                if ((WiFi.status() != WL_CONNECTED) ||
                    (++fail_sync_count > ((NTP_SYNC_PERIOD_MS / WIFI_CHECK_INTERVAL) + 1))) {
                    LOG_WARNING("NTP", "NTP sync failed. WiFi status: %d. Re-connecting...", WiFi.status());
                    Serial.println("NTP sync failed. No WiFi? Wifi status: " + String(WiFi.status()) +
                                   ". Re-connecting...");
                    fail_sync_count = 0;
                    reconnecting = true;
                    reconnect_start = millis();
                    WiFi.disconnect();
                    WiFi.begin(ssid, password);
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
    MusicPlayer::run();
    for (auto &[_, button] : buttons) {
        button.tick();
    }
    AsyncTasker::runEventLoop();
}
