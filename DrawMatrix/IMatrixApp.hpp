#ifndef DRAWMATRIX_IMATRIXAPP
#define DRAWMATRIX_IMATRIXAPP

#include <ArduinoJson.h>

#include <cstdint>

#include "IApp.hpp"

class IMatrixApp : public IApp {
   public:
    virtual void handle_root() = 0;
    virtual void handle_not_found() = 0;
    virtual void handle_status_led_control() = 0;
    virtual void handle_gif() = 0;
    virtual void handle_set_display_brightness() = 0;
    virtual void handle_set_display_color() = 0;
    virtual void handle_set_display_matrix() = 0;
};

#endif /* DRAWMATRIX_IMATRIXAPP */
