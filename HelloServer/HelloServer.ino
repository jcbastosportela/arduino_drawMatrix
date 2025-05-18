
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WiFiClient.h>

#include <Asynchrony.hpp>

#include "server_sys.hpp"

#ifndef STASSID
#define STASSID "Portela's"
#define STAPSK "Pt:Tr:1988"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
constexpr int led = 13;
ESP8266WebServer server(80);
ServerSys::App app;

/**
 * @brief   Task to blink the LED
 *
 * @param t
 * @param delay
 */
void heart_beat_blink(unsigned long t, unsigned long &delay);

/**
 * @brief Handler for page not found
 *
 */
void handleNotFound();

/**
 * @brief Handler for root page
 *
 */
void handleRoot();

// --------------------------------------------------------------------------------------
void handleRoot() {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "hello from esp8266!\r\n");
    digitalWrite(led, 0);
}

// --------------------------------------------------------------------------------------
void handleNotFound() {
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(led, 0);
}

// ======================================================================================
void handleLEDOn() {
    app.set_led(true);
    JsonDocument jsonDoc;
    jsonDoc["status"] = "LED is ON";
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
}

// ======================================================================================
void handleLEDOff() {
    app.set_led(false);
    JsonDocument jsonDoc;
    jsonDoc["status"] = "LED is OFF";
    String response;
    serializeJson(jsonDoc, response);
    server.send(200, "application/json", response);
}

// ======================================================================================
void handleGif() {
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
    server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
}

// ======================================================================================
void setup(void) {
    pinMode(led, OUTPUT);
    digitalWrite(led, 0);
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

    // inti FS
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    server.on("/", handleRoot);

    server.on("/inline", []() { server.send(200, "text/plain", "this works as well"); });

    server.on("/led/on", handleLEDOn);
    server.on("/led/off", handleLEDOff);
    server.on("/gif", handleGif);
    server.on("/brightness", []() {
        String message = "Brightness: ";
        message += server.arg("value");
        server.send(200, "text/plain", message);
        app.set_display_brightness(static_cast<uint8_t>(server.arg("value").toInt()));
    });
    server.on("/color", []() {
        uint32_t color = (static_cast<uint8_t>(server.arg("w").toInt()) << 24) |
                         (static_cast<uint8_t>(server.arg("r").toInt()) << 16) |
                         (static_cast<uint8_t>(server.arg("g").toInt()) << 8) |
                         (static_cast<uint8_t>(server.arg("b").toInt()));

        // Create a single-color GIF (16x16, color table with only one color)
        static uint8_t gif[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x10, 0x00, 0x10, 0x00,
                                0x80, 0x00, 0x00, 0x00, 0x00, 0x00,  // color table: black
                                0x00, 0x00, 0x00,                    // color table: black (padding)
                                0x2C, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00,
                                0x02, 0x16, 0x8C, 0x8F, 0xA9, 0xCB, 0xED, 0x0F, 0xA3, 0x9C,
                                0xB4, 0xDA, 0x8B, 0xB3, 0xDE, 0xBC, 0xFB, 0x0F, 0x00, 0x3B};
        // Set the color table to the requested color (first entry)
        gif[16] = (color >> 16) & 0xFF;  // Red
        gif[17] = (color >> 8) & 0xFF;   // Green
        gif[18] = color & 0xFF;          // Blue

        server.send(200, "image/gif", gif, sizeof(gif));
        app.set_display_color(color);
    });
    server.on("/draw", []() {
        if (SPIFFS.exists("/draw.html")) {
            File file = SPIFFS.open("/draw.html", "r");
            server.streamFile(file, "text/html");
            file.close();
        } else {
            server.send(404, "text/plain", "draw.html not found");
        }
    });
    server.onNotFound(handleNotFound);

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

    server.begin();
    Serial.println("HTTP server started");
}

// ======================================================================================
void loop(void) {
    server.handleClient();
    MDNS.update();
    app.run();
    Asynchrony::runEventLoop();
}
