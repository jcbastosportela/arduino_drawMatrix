# Arduino/ESP8266 Development Tools

This directory contains utility tools for Arduino and ESP8266/ESP32 development.

## decompressHtml.py

A Python tool for extracting and decompressing gzip-compressed HTML/data stored as `uint8_t` arrays in C++ source files. This is commonly used in ESP8266/ESP32 projects for storing web content in PROGMEM to save flash memory.

### Features

- **Automatic content type detection** (HTML, CSS, JS, JSON, XML, plain text)
- **Compression statistics** showing space savings
- **Flexible output options** (custom filename or directory)
- **Error handling** with descriptive messages
- **Verbose mode** for debugging
- **Temporary file preservation** for inspection

### Usage

```bash
# Basic usage - decompress to auto-named file
./decompressHtml.py path/to/source.cpp

# Custom output filename
./decompressHtml.py source.cpp -o web_interface.html

# Output to specific directory
./decompressHtml.py source.cpp -o /path/to/output/

# Show compression statistics
./decompressHtml.py source.cpp --stats

# Verbose output for debugging
./decompressHtml.py source.cpp --verbose

# Keep temporary compressed file
./decompressHtml.py source.cpp --temp
```

### Examples

```bash
# Decompress ElegantOTA interface
./decompressHtml.py ../libraries/ElegantOTA/src/elop.cpp --stats

# Extract to specific location with verbose output
./decompressHtml.py data.cpp -o ./extracted/interface.html -v
```

### Input File Format

The tool expects C++ files containing gzip-compressed data as comma-separated `uint8_t` arrays:

```cpp
const uint8_t webpage[] PROGMEM = {
  31,139,8,0,0,0,0,0,2,3,164,86,231,118,219,184,18,254,159,167,160,121,206,
  // ... more bytes ...
  0,0
};
```

### Output

- Automatically detects content type and adds appropriate file extension
- Creates output directories if they don't exist
- Preserves UTF-8 encoding for text content
- Optional compression statistics showing space savings

### Requirements

- Python 3.6+
- Standard library only (no external dependencies)

### Error Handling

The tool provides clear error messages for common issues:
- File not found
- Invalid gzip data
- Missing or malformed byte arrays
- Write permission errors