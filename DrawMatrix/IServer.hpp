/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      IServer.hpp                                                                                              *
 * @brief     Type alias for the web server interface in DrawMatrix system                                             *
 * @date      Sun Aug 24 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#ifndef DRAWMATRIX_ISERVER
#define DRAWMATRIX_ISERVER

#include <ESP8266WebServer.h>

/**
 * @brief Type alias for the web server interface used in DrawMatrix.
 *
 * Lazy thing to do. A server interface should be defined to be passed to the App class,
 * but for now, we just use the ESP8266WebServer directly.
 */
using IServer = ESP8266WebServer;

#endif /* DRAWMATRIX_ISERVER */
