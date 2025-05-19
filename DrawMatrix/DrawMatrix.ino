#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#include <AsyncTasker.hpp>
#include <memory>

#include "DRAW_HTML.hpp"
#include "server_sys.hpp"

// #define STASSID "your-ssid"
// #define STAPSK "your-password"

#if !defined(STASSID) && !defined(STAPSK)
#include "credentials.hpp"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
ESP8266WebServer server(80);
std::unique_ptr<ServerSys::App> app;

// --- Robustness: WiFi and server health check ---
constexpr uint64_t WIFI_CHECK_INTERVAL = 10000;    // 10 seconds
constexpr uint64_t SERVER_CHECK_INTERVAL = 5000;  // 30 seconds

// ======================================================================================
void setup(void) {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

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

    app = std::make_unique<ServerSys::App>(server);

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

    AsyncTasker::schedule(
        WIFI_CHECK_INTERVAL,
        [](uint64_t, uint64_t &, bool &) {
            Serial.printf("Checking WiFi\n");

            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi disconnected! Attempting reconnect...");
                WiFi.disconnect();
                WiFi.begin(ssid, password);
            }
        },
        true);

    AsyncTasker::schedule(
        SERVER_CHECK_INTERVAL,
        [](uint64_t, uint64_t &, bool &) {
            static size_t n_fails = 0;
            constexpr size_t max_fails = 3;
            auto c = server.client();
            if (c.connected()) {
                Serial.println("Client connected");
                Serial.println(c.remoteIP());
            }else{
                Serial.println("Client disconnected");
                n_fails++;
                if (n_fails > max_fails) {
                    Serial.println("Too many failed attempts, restarting server...");
                    server.close();
                    server.begin();
                    n_fails = 0;
                }
            }
        },
        true);
}

// ======================================================================================
void loop(void) {
    server.handleClient();
    MDNS.update();
    app->run();
    AsyncTasker::runEventLoop();
}
