## Copilot Project Instructions (DrawMatrix)

High-level: ESP8266 firmware serving a small web UI (HTML embedded as PROGMEM strings) to control a composite 32x24 RGB matrix (12x WS2812B 8x8 tiles arranged 4 (x) by 3 (y)). Extra features: clock mode when idle, alarms persisted in LittleFS, and MP3 playback via DFPlayer Mini.

### Absolutely DO NOT
1. Remove or alter the wrapper lines in `DrawMatrix/data/draw.html` (and its symlink `DrawMatrix/DRAW_HTML.hpp`):
   - Keep first line: `const char DRAW_HTML[] PROGMEM = R"html(`
   - Keep last line: `)html";`
2. Commit or generate `DrawMatrix/credentials.hpp` (contains WiFi credentials). Never add it to version control examples.
3. Break the PROGMEM HTML pattern for other pages (`ALARM_HTML.hpp`, `MUSIC_HTML.hpp`, `INDEX_HTML.hpp`). Treat them as C++ string blobs, not normal standalone HTML files.

### Core Architecture
- Entry point: `DrawMatrix/DrawMatrix.ino` sets up WiFi, NTP, routes, button handlers, schedules periodic tasks via `AsyncTasker`.
- Main app object: `ServerSys::App` (in `ServerSys.hpp/.cpp`) wires HTTP handlers to hardware + state (matrix, alarms, music, clock mode).
- LED Matrix driver: `ServerSys::DrawMatrix` wraps `Adafruit_NeoMatrix` over 12 chained 8x8 boards. Pixel remap logic lives in `ServerSys.cpp` (constexpr `pixel_indices` + `pixel_index()` helper). Maintain this mapping when adding transformations.
- Tasks: All repeating/async behavior scheduled through `AsyncTasker::schedule(...)`; do not introduce raw `delay()` in new logic—prefer tasks.
- Music: `MusicPlayer.(hpp|cpp)` abstracts DFPlayer Mini (folder/track mapping). Use `MusicPlayer::play(MusicTrack::X)`; volume range 0–30 (`MAX_VOLUME`).
- Alarms: Stored in-memory list (`App::m_alarms`) with bitfield day mask, persisted to `/alarms.bin` in LittleFS. File format lines: `HH:MM,<daysBitmask>` (legacy lines without comma mean all days). Modify persistently via existing endpoints; keep backward compatibility when changing format.

### Display / Hardware Layout
- Physical tile arrangement: 4 (horizontal) x 3 (vertical); first data-in tile is top-right; all tiles oriented so each tile's internal address 0 is its top-left.
- Global logical coordinate system: (0,0) = top-left of full 32x24 canvas. `pixel_indices` remaps logical (col,row) to physical LED index; any animation code must use `pixel_index(col,row)` or existing setters—never assume linear order.
- Minimum brightness enforced in clock mode (`MIN_BRIGHTNESS` constant). Avoid hardcoding brightness elsewhere—use `DrawMatrix::set_brightness`.

### Web / Endpoints (ESP8266WebServer)
Registered in `DrawMatrix.ino`; handlers implemented in `ServerSys::App`:
- Pages: `/` (index), `/draw`, `/music`, `/alarm` serve PROGMEM HTML strings.
- Matrix control: `/set_display_brightness?value=..`, `/set_display_color`, `/set_display_matrix` (POST JSON NxM array uint32 colors), `/gif` (demo GIF), `/status_led_control`.
- Alarms: `/set_alarm`, `/list-alarms`, `/delete-alarm`, `/modify-alarm` operate on persisted list.
- Music: `/music_play?track=<id>` (or toggle if no track), `/music_stop`.
- Info / util: `/info`, `/wifi_off`.

### JSON Conventions
- Matrix POST: JSON outer array length == `N_COLS` (32), each inner array length == `N_ROWS` (24); values are 24-bit packed RGB integers. Validate shape before applying.
- Alarm payload: `{ "time": "HH:MM", "days": [0-6...] }` where day 0=Sunday. Days optional => all days.

### Concurrency / Scheduling
- Use `AsyncTasker::schedule(interval_ms, lambda, repeat=true)` for repeating jobs. Do not block inside lambdas. For fast UI feedback, schedule short tasks and update matrix then call `matrix.show()` once per frame.
- Clock mode toggled by client connection absence logic (see `SERVER_CHECK_INTERVAL` task). Respect `App::clock_mode(true/false)`—avoid features that permanently disable it.

### Extending Safely
- When adding endpoints: register in `DrawMatrix.ino` (matching pattern of existing) and implement method on `ServerSys::App`. Keep argument validation + error response style consistent (return 400 plain text with brief reason; log via `Serial.printf`).
- For new persistent data: mount LittleFS early (already in `App` ctor). Use simple line-based formats; minimize writes (flash wear).
- For new animations: operate through `DrawMatrix::set_matrix` or iterate using `pixel_index`; avoid recomputing the mapping.
- For new music tracks: extend `MusicTrack` enum and `trackActions` map (folder-based indexing consistent with current usage).

### Style & Documentation
- Doxygen/Javadoc style in headers for classes & methods; implementation files omit repeated doc comments. Keep added public methods documented at declaration.
- Avoid adding doc blocks to trivial lambdas or private helpers unless clarity demands.

### Build / Tooling
- Primary development via Arduino IDE / ESP8266 core. To enable C++20 features set (user local): `platform.local.txt` with `compiler.cpp.extra_flags=-g -Os -w -std=gnu++20`.
- Repo includes library sources under `libraries/`; treat them as vendored—avoid modifying unless essential (then document rationale in commit message).

### Common Pitfalls
- Forgetting bounds: always ensure arrays match `N_COLS` x `N_ROWS` or reject request.
- Writing blocking loops (e.g., waiting for WiFi) inside scheduled tasks—only setup does blocking waits.
- Accidentally clearing PROGMEM HTML sentinel lines—breaks compilation.
- Committing credentials or changing file format of `/alarms.bin` without backward compatibility.

### Quick Examples
Set whole matrix blue (hex 0x0000FF): GET `/set_display_color?color=255`.
POST matrix JSON (pseudo): `[[16711680, ... 32 cols] x 24 rows]` to `/set_display_matrix` with arg name `plain`.
Add alarm 07:30 weekdays: `{ "time": "07:30", "days": [1,2,3,4,5] }` POST to `/set_alarm`.

### When Unsure
Inspect existing handler patterns in `ServerSys.cpp` and mirror argument validation + response flow. Prefer reusing constants from `ServerSys` namespace. Ask before refactoring pixel mapping or alarm persistence logic.

End of instructions.