#ifndef DRAWMATRIX_ISERVER
#define DRAWMATRIX_ISERVER

// lauzy lazy thing to do. A server interface should be defined to be passed to the App class,
// but for now, we just use the ESP8266WebServer directly.
#include <ESP8266WebServer.h>
using IServer = ESP8266WebServer;

#endif /* DRAWMATRIX_ISERVER */
