#include <memory>

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <AsyncTasker.hpp>

#include "DRAW_HTML.hpp"
#include "ServerSys.hpp"

// #define STASSID "your-ssid"
// #define STAPSK "your-password"

#if !defined(STASSID) && !defined(STAPSK)
#include "credentials.hpp"
#endif

// --- Robustness: WiFi and server health check ---
constexpr uint64_t WIFI_CHECK_INTERVAL = 10000;             // milliseconds
constexpr uint64_t SERVER_CHECK_INTERVAL = 5000;            // milliseconds
constexpr size_t MAX_NUM_TRIES_NO_CLIENT = 3;               // how many tries before giving up
constexpr size_t NTP_SYNC_PERIOD = 60*1000;                 // milliseconds

const char *ssid = STASSID;
const char *password = STAPSK;
ESP8266WebServer server(80);
WiFiUDP ntp_udp;
NTPClient ntpClient(ntp_udp, "pool.ntp.org", 2*60*60, NTP_SYNC_PERIOD);
std::unique_ptr<ServerSys::App> app;

// ======================================================================================
void setup(void) {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    app = std::make_unique<ServerSys::App>(server, ntpClient);

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

    server.on("/", std::bind(&ServerSys::App::handle_root, app.get()));
    server.on("/status_led_control", std::bind(&ServerSys::App::handle_status_led_control, app.get()));
    server.on("/set_display_brightness", std::bind(&ServerSys::App::handle_set_display_brightness, app.get()));
    server.on("/set_display_color", std::bind(&ServerSys::App::handle_set_display_color, app.get()));
    server.on("/gif", std::bind(&ServerSys::App::handle_gif, app.get()));
    server.on("/set_display_matrix", HTTP_POST, std::bind(&ServerSys::App::handle_set_display_matrix, app.get()));
    server.on("/draw", []() { server.send(200, "text/html", DRAW_HTML); });
    server.onNotFound(std::bind(&ServerSys::App::handle_not_found, app.get()));

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
#endif  // 0
    server.begin();
    Serial.println("HTTP server started");

    static bool no_client_for_too_long = false;
    AsyncTasker::schedule(
        WIFI_CHECK_INTERVAL,
        [](uint64_t, uint64_t &, bool &) {
            Serial.printf("Checking WiFi\n");

            if (WiFi.status() != WL_CONNECTED || no_client_for_too_long) {
                Serial.println(no_client_for_too_long ? 
                    "No client for too long! Attempting reconnect WiFi..." : 
                    "WiFi disconnected! Attempting reconnect...");
                WiFi.disconnect();
                WiFi.begin(ssid, password);
            }
        },
        true);

    // AsyncTasker::schedule(
    //     SERVER_CHECK_INTERVAL,
    //     [](uint64_t, uint64_t &, bool &) {
    //         static size_t n_fails = 0;
    //         static bool consecutive_failure = false;
    //         auto c = server.client();
    //         if (c.connected()) {
    //             consecutive_failure = false;
    //             Serial.println("Client connected");
    //             Serial.println(c.remoteIP());
    //         }else{
    //             Serial.println("Client disconnected");
    //             n_fails++;
    //             if (n_fails > MAX_NUM_TRIES_NO_CLIENT) {
    //                 Serial.println("Too many failed attempts, restarting server...");
    //                 server.close();
    //                 server.begin();
    //                 n_fails = 0;
    //             }
    //             if (consecutive_failure) {
    //                 no_client_for_too_long = true;
    //             }
    //             consecutive_failure = true;
    //         }
    //     },
    //     true);

    AsyncTasker::schedule(
        5000,
        [](uint64_t, uint64_t &, bool &) {
            Serial.printf("Free heap %u bytes", ESP.getFreeHeap());
        },
        true
    );

    ntpClient.begin();
    // AsyncTasker::schedule(
    //     1000,
    //     [](uint64_t, uint64_t &, bool &) {
    //         ntpClient.update();
    //         Serial.printf("NTP time: %s\n", ntpClient.getFormattedTime().c_str());
    //     },
    //     true
    // );
}

// ======================================================================================
void loop(void) {
    server.handleClient();
    MDNS.update();
    ntpClient.update();
    app->run();
    AsyncTasker::runEventLoop();
}
