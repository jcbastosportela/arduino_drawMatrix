# General Instructions for Copilot

The webpage provides a user-friendly interface for controlling an 32x24 (width x height) LED matrix using an ESP8266 board. Users can select colors, adjust brightness, and paint on the matrix in real-time. The project is designed to be mobile-friendly and supports touch interactions.

The project itself is on the folder DrawMatrix. The workspace root is the arduino workspace.

# Important do NOT do for copilot
- do not remove `const char DRAW_HTML[] PROGMEM = R"html(` from DrawMatrix/data/draw.html on the 1st line (or its symlink DrawMatrix/DRAW_HTML.hpp): pretend this line is not there, but do not remove it
- do not remove `)html";` from DrawMatrix/data/draw.html on the last line (or its symlink DrawMatrix/DRAW_HTML.hpp): pretend this line is not there, but do not remove it
- do not add DrawMatrix/credentials.hpp to tracked files; this file contains my wifi credentials. this is important

## Code Structure
- DrawMatrix: The project itself
    - DrawMatrix/data: folder with resources for the project
        - DrawMatrix/data/draw.html: this file is the web page served for the user interaction. This is NOT a normal HTML page! It is intended to be included by cpp as a string containing the html content. Do NOT remove the array definition from this file. There is a softlink to DrawMatrix/DRAW_HTML.hpp)
    - DrawMatrix/credentials.hpp: file with my wifi credentials
    - DrawMatrix/DrawMatrix.ino: arduino porject main file
- libraries: Arduino libraries

## About the display

- The display is made of 12 units WS2812B-64 boards (8x8 RGB LED matrixes)
- The WS2812B-64 are organized as 4x3 (horizontal x vertical)
- The input WS2812B-64 (1st board) is on the top-right corner, assembled with the top left corner being the origin (LED address 0)
- all boards are assembled in a way that the top-left corner of each board is the origin on that board (LED address 0 on the board)
- The 2nd board is below the 1st, the 3rd below the 2nd and the 4th is on the left of the 1st, the pattern repeats
- The last WS2812B-64 (12th board) is on the bottom-left corner

# Documentation style
- Doxygen javadoc style