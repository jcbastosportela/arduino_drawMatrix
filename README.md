# LED Matrix Web Controller

A web-based controller for an 8x8 RGB LED matrix using ESP8266 and WS2812 LEDs. Control your LED matrix through an intuitive web interface with real-time color updates and touch support.

## Features

- 🎨 Interactive 8x8 grid interface
- 🔠 RGB color control with individual sliders
- 💡 Brightness adjustment (0-255)
- 📱 Mobile-friendly with touch support
- 🖌️ Click/touch and drag painting
- 🎯 Fill all and clear functions
- ⚡ Real-time updates
- 🌐 Web-based PWA interface

## Hardware Requirements

- ESP8266 board
- WS2812 8x8 LED Matrix (64 LEDs)
- 5V power supply (suitable for powering 64 RGB LEDs)

## Pin Configuration

- WS2812 Data Pin: GPIO0
- Built-in LED: GPIO13 (for status indication)

## Dependencies

Arduino libraries required:
- ArduinoJson
- ESP8266WebServer
- Adafruit_NeoPixel
- AsyncTasker (self made, inspired by Asynchrony)

## Installation

1. Install required libraries through Arduino Library Manager if needed (needed libraries are already in this workspace and it can be opened with Arduino2)
1. We need to enable GNU++20:
   1. create/modify the platform.local.txt (in GNU/Linux `~/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/platform.local.txt` NOTE that instead of `3.1.2` it might be a newer version!)
   1. add this:
   ```
   compiler.cpp.extra_flags=-g -Os -w -std=gnu++20
   ```
1. Configure your WiFi credentials in `DrawMatrix.ino`:
   ```cpp
   #define STASSID "your-ssid"
   #define STAPSK "your-password"
   ```
1. Upload the code to your ESP8266
4. Access the interface at `http://[ESP8266-IP]/draw`

## Web Interface

The web interface provides:
- RGB color sliders for precise color selection
- Real-time color preview
- Brightness control slider
- Matrix cell painting with click/touch and drag
- Fill All and Clear buttons for quick actions

## Project Structure

- `DrawMatrix.ino`: Main Arduino sketch with server setup and endpoints
- `server_sys.cpp`: LED matrix control implementation
- `server_sys.hpp`: Header file with class definitions
- `DRAW_HTML.hpp`: link to HTML, to make Arduino happy
- `data/`: folder with HMTLs, Web interface HTML/CSS/JavaScript

## API Endpoints

- `/draw`: Main web interface
- `/brightness?value=0-255`: Set matrix brightness [DEBUG]
- `/set_matrix`: Update matrix display (POST with JSON color matrix) [DEBUG]
- `/color`: Set single color for testing [DEBUG]

## License

MIT License

## Contributing

Feel free to submit issues and enhancement requests!