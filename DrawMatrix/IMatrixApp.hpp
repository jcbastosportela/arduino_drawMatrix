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
    virtual void handle_root() = 0;

    /**
     * @brief Handle not found web requests.
     */
    virtual void handle_not_found() = 0;

    /**
     * @brief Handle status LED control requests.
     */
    virtual void handle_status_led_control() = 0;

    /**
     * @brief Handle GIF display requests.
     */
    virtual void handle_gif() = 0;

    /**
     * @brief Handle display brightness setting requests.
     */
    virtual void handle_set_display_brightness() = 0;

    /**
     * @brief Handle display color setting requests.
     */
    virtual void handle_set_display_color() = 0;

    /**
     * @brief Handle display matrix setting requests.
     */
    virtual void handle_set_display_matrix() = 0;

    /**
     * @brief Handle alarm setting requests.
     * 
     * This function is called when a client requests to set the alarm.
     */
    virtual void handle_set_alarm() = 0;
};

#endif /* DRAWMATRIX_IMATRIXAPP */
