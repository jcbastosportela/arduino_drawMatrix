#ifndef elop_h
#define elop_h

#include <Arduino.h>

#define USE_ORIGINAL_ELEGANTOTA 0

#if USE_ORIGINAL_ELEGANTOTA
extern const uint8_t ELEGANT_HTML[10214];
#else
#include "elegantota_interface.html"
#endif // USE_ORIGINAL_ELEGANTOTA
#endif
