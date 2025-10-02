/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      IMatrixApp.hpp                                                                                           *
 * @brief     Interface for matrix application server request handlers.                                                *
 * @date      Sun Jul 27 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#ifndef DRAWMATRIX_IMATRIXAPP
#define DRAWMATRIX_IMATRIXAPP

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include <cstdint>

#include "IApp.hpp"

class IMatrixApp : public IApp {
  public:
    /**
     * @brief Virtual destructor for IMatrixApp.
     */
    virtual ~IMatrixApp() = default;

    /**
     * @brief Handle the root web request.
     */
    virtual void handle_root(AsyncWebServerRequest *request) = 0;

    /**
     * @brief Handle not found web requests.
     */
    virtual void handle_not_found(AsyncWebServerRequest *request) = 0;

    /**
     * @brief Handle status LED control requests.
     */
    virtual void handle_status_led_control(AsyncWebServerRequest *request) = 0;

    /**
     * @brief Handle GIF display requests.
     */
    virtual void handle_gif(AsyncWebServerRequest *request) = 0;

    /**
     * @brief Handle display brightness setting requests.
     */
    virtual void handle_set_display_brightness(AsyncWebServerRequest *request) = 0;

    /**
     * @brief Handle display color setting requests.
     */
    virtual void handle_set_display_color(AsyncWebServerRequest *request) = 0;

    /**
     * @brief Handle display matrix setting requests.
     */
    virtual void handle_set_display_matrix(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) = 0;

    /**
     * @brief Handle alarm setting requests.
     * 
     * This function is called when a client requests to set the alarm.
     */
    virtual void handle_set_alarm(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) = 0;
};

#endif /* DRAWMATRIX_IMATRIXAPP */
