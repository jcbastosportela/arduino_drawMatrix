# DrawMatrix Logging System Implementation Plan

## Overview
Replace current `Serial.print`/`Serial.println` calls with a comprehensive logging system that supports:
- Multiple log levels (TRACE, DEBUG, INFO, WARNING, ERROR)
- Optional file logging to LittleFS (configurable due to flash wear concerns)
- Web interface for configuration and log retrieval
- Minimal memory footprint for ESP8266

## Architecture Design

### Log Levels
1. **TRACE** - Detailed execution flow (e.g., function entry/exit)
2. **DEBUG** - Development/debugging information (e.g., variable values)
3. **INFO** - General informational messages (e.g., "WiFi connected")
4. **WARNING** - Warning conditions (e.g., failed NTP sync attempt)
5. **ERROR** - Error conditions (e.g., file system mount failure)

### Core Components

#### 1. Logger Interface (`Logger.hpp` & `Logger.cpp`)
- Singleton pattern for global access
- Format: `[TIMESTAMP][LEVEL][MODULE] message`
- Supports printf-style formatting
- Configurable output destinations (Serial, File, or both)
- Compile-time log level filtering option
- Runtime log level configuration

#### 2. File Logging Manager
- Ring buffer approach for log files to prevent filling flash
- Configurable max file size (default: 50KB per file)
- Two log files: `/logs/current.log` and `/logs/previous.log`
- Auto-rotation when current log reaches size limit
- File logging can be enabled/disabled via web interface

#### 3. Web Interface Extensions
- **Settings Page** (`/log-settings`) for:
  - Current log level selection dropdown
  - Enable/disable file logging checkbox
  - Clear logs button
  - Download current/previous log files
- **Log Viewer Page** (`/logs`) for:
  - Real-time log display (last N entries)
  - Filter by log level
  - Auto-refresh option

## Implementation Steps

### Phase 1: Core Logger Module (Foundation)
**Files to create:**
- `DrawMatrix/Logger.hpp` - Logger interface declarations
- `DrawMatrix/Logger.cpp` - Logger implementation

**Tasks:**
1. Create `Logger` class with singleton pattern
2. Implement log level enumeration and filtering
3. Add Serial output formatting (timestamp, level, module, message)
4. Implement printf-style variadic templates for flexible logging
5. Add compile-time configuration macros:
   - `LOG_LEVEL_COMPILE_TIME` - compile-time level filter (default: DEBUG)
   - `LOG_ENABLE_FILE` - enable/disable file logging at compile time
   - `LOG_MAX_FILE_SIZE` - max size per log file

**Success Criteria:**
- Logger compiles without errors
- Can log to Serial with formatted output
- Log levels filter correctly

---

### Phase 2: File Logging Support
**Files to modify:**
- `DrawMatrix/Logger.cpp` - Add file logging logic

**Tasks:**
1. Implement file rotation logic (current.log → previous.log)
2. Add LittleFS file operations for append/rotate
3. Implement size checking before each write
4. Add file write buffering (write every N messages or every M seconds)
5. Handle file system errors gracefully (fall back to Serial only)
6. Add public API:
   - `Logger::enableFileLogging(bool enable)`
   - `Logger::clearLogs()`
   - `Logger::getLogContents(bool current)`

**Success Criteria:**
- Logs written to `/logs/current.log`
- Auto-rotation works when size limit reached
- No flash wear from excessive writes (buffered writes)
- Graceful fallback if LittleFS unavailable

---

### Phase 3: Replace Existing Serial.print Calls
**Files to modify:**
- `DrawMatrix/DrawMatrix.ino`
- `DrawMatrix/ServerSys.cpp`
- `DrawMatrix/MusicPlayer.cpp`

**Tasks:**
1. Add `#include "Logger.hpp"` to each file
2. Replace all `Serial.print` with appropriate `LOG_*` macros:
   - `Serial.println("WiFi connected")` → `LOG_INFO("MAIN", "WiFi connected")`
   - `Serial.printf("Error: %d", code)` → `LOG_ERROR("MAIN", "Error: %d", code)`
3. Identify appropriate log levels for each message:
   - Startup messages → INFO
   - Button presses → DEBUG
   - WiFi/NTP sync failures → WARNING
   - File system errors → ERROR
   - Detailed DFPlayer messages → DEBUG or TRACE
4. Add module names for context:
   - `"MAIN"` - DrawMatrix.ino
   - `"SERVER"` - ServerSys.cpp
   - `"MUSIC"` - MusicPlayer.cpp
   - `"MATRIX"` - Matrix drawing operations
   - `"ALARM"` - Alarm system

**Success Criteria:**
- All Serial.print calls replaced
- Appropriate log levels assigned
- No compilation errors
- Logs readable and informative

---

### Phase 4: Web Interface - Settings Page
**Files to create:**
- `DrawMatrix/data/log-settings.html` - Log settings UI
- `DrawMatrix/LOG_SETTINGS_HTML.hpp` - PROGMEM wrapper

**Files to modify:**
- `DrawMatrix/DrawMatrix.ino` - Add endpoints

**Tasks:**
1. Create HTML page for log settings with:
   - Dropdown for log level (TRACE to ERROR)
   - Checkbox for enable/disable file logging
   - "Clear Logs" button
   - "Download Current Log" button
   - "Download Previous Log" button
   - Current settings display
2. Add PROGMEM wrapper following pattern of other HTML files
3. Implement endpoints in `DrawMatrix.ino`:
   - `GET /log-settings` - Serve settings page
   - `GET /log-config` - Return current config as JSON
   - `POST /log-config` - Update log level and file logging setting
   - `POST /log-clear` - Clear log files
   - `GET /log-download?file=current|previous` - Download log file
4. Persist log settings to `/config/log.json` in LittleFS
5. Load settings on startup

**Success Criteria:**
- Settings page accessible and functional
- Log level changes take effect immediately
- File logging can be toggled on/off
- Settings persist across reboots
- Log files can be downloaded

---

### Phase 5: Web Interface - Log Viewer Page
**Files to create:**
- `DrawMatrix/data/logs.html` - Log viewer UI
- `DrawMatrix/LOGS_HTML.hpp` - PROGMEM wrapper

**Files to modify:**
- `DrawMatrix/DrawMatrix.ino` - Add endpoints

**Tasks:**
1. Create HTML page for log viewing with:
   - Display last 100 log entries (tail of current log)
   - Filter dropdown (All, ERROR, WARNING, INFO, DEBUG, TRACE)
   - Auto-refresh toggle (every 5 seconds)
   - Color-coded log levels (red=ERROR, yellow=WARNING, etc.)
   - Responsive design for mobile
2. Add PROGMEM wrapper
3. Implement endpoints:
   - `GET /logs` - Serve viewer page
   - `GET /log-entries?level=ALL&count=100` - Return log entries as JSON
4. Implement efficient log parsing (read last N lines from file)
5. Add JavaScript for:
   - Auto-refresh
   - Client-side filtering
   - Smooth scrolling

**Success Criteria:**
- Log viewer displays recent logs
- Filtering works correctly
- Auto-refresh updates display
- Color coding helps identify issues quickly

---

### Phase 6: Integration & Testing
**Tasks:**
1. Test complete logging flow:
   - Verify all log levels work correctly
   - Test file rotation with large logs
   - Verify persistence across reboots
   - Test web interface responsiveness
2. Memory profiling:
   - Check IRAM usage doesn't exceed limits
   - Verify heap fragmentation stays acceptable
   - Monitor flash write cycles
3. Performance testing:
   - Ensure logging doesn't impact matrix refresh rate
   - Verify web server remains responsive
   - Test under high log volume
4. Edge case testing:
   - LittleFS full scenario
   - Corrupted log files
   - Invalid configuration
5. Documentation:
   - Update `README.md` with logging features
   - Add comments to Logger interface
   - Document configuration options

**Success Criteria:**
- No regressions in existing functionality
- Memory usage within acceptable limits
- System stable under various conditions
- All features documented

---

### Phase 7: Optional Enhancements (Future)
**Potential additions:**
1. **Remote logging** - Send logs to external syslog server
2. **Log compression** - Compress old logs to save space
3. **Log filtering by module** - Web UI module filter
4. **Statistics** - Error count, warning count dashboard
5. **Email alerts** - Send email on ERROR level logs
6. **MQTT logging** - Publish logs to MQTT broker

---

## Configuration Summary

### Compile-Time Configuration (Logger.hpp)
```cpp
#define LOG_LEVEL_COMPILE_TIME  LOG_LEVEL_DEBUG  // Minimum level to compile
#define LOG_ENABLE_FILE         1                 // Enable file logging support
#define LOG_MAX_FILE_SIZE       (50 * 1024)      // 50KB per log file
#define LOG_BUFFER_SIZE         512               // Message buffer size
#define LOG_FILE_WRITE_INTERVAL 5000             // Write to file every 5s
```

### Runtime Configuration (via Web UI or log.json)
```json
{
  "log_level": "DEBUG",
  "file_logging_enabled": false,
  "max_file_size": 51200
}
```

## API Reference (After Implementation)

### Logging Macros
```cpp
LOG_TRACE(module, format, ...)   // Most verbose
LOG_DEBUG(module, format, ...)   // Debug information
LOG_INFO(module, format, ...)    // General info
LOG_WARNING(module, format, ...) // Warnings
LOG_ERROR(module, format, ...)   // Errors
```

### Example Usage
```cpp
// Before:
Serial.println("WiFi connected");
Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());

// After:
LOG_INFO("MAIN", "WiFi connected");
LOG_INFO("MAIN", "IP: %s", WiFi.localIP().toString().c_str());
```

## Memory Considerations

### Estimated Memory Impact
- Logger class: ~200 bytes RAM (buffers + state)
- HTML pages: ~3KB PROGMEM (compressed)
- Log files: Up to 100KB flash (configurable)
- Per-log overhead: ~10 bytes (timestamp + formatting)

### Flash Wear Mitigation
- Buffered writes (not every log call)
- File logging disabled by default
- Warning in web UI about flash wear
- Rotation instead of frequent deletes

## Migration Notes

### Breaking Changes
- None - all Serial output preserved for backward compatibility
- Logger is additive enhancement

### Rollback Plan
- Keep all Serial.print calls initially
- Test Logger in parallel
- Only remove Serial calls after validation
- Git tag before migration for easy rollback

---

## Timeline Estimate

- **Phase 1**: 2-3 hours (core implementation)
- **Phase 2**: 2-3 hours (file logging)
- **Phase 3**: 1-2 hours (migration)
- **Phase 4**: 2-3 hours (settings UI)
- **Phase 5**: 2-3 hours (viewer UI)
- **Phase 6**: 2-4 hours (testing)

**Total**: 11-18 hours (conservative estimate)

---

## Risk Assessment

### High Risk
- **Flash wear** - Mitigated by buffering and optional file logging
- **Memory exhaustion** - Mitigated by fixed buffers and streaming

### Medium Risk
- **Performance impact** - Mitigated by efficient formatting and conditional compilation
- **Web UI complexity** - Mitigated by keeping UI simple

### Low Risk
- **Breaking existing code** - Mitigated by keeping Serial output initially

---

## Decision Points (Require Confirmation)

1. **Default log level**: DEBUG or INFO?
2. **File logging default**: Enabled or disabled?
3. **Max log file size**: 50KB or different?
4. **Include timestamp**: Use NTP time or millis()?
5. **Color scheme**: Standard (Red/Yellow/Blue) or custom?

---

## Post-Implementation Checklist

- [ ] All Serial.print calls migrated
- [ ] Logger compiles without warnings
- [ ] File logging works and rotates properly
- [ ] Web settings page functional
- [ ] Web viewer page functional
- [ ] Configuration persists across reboots
- [ ] Memory usage acceptable
- [ ] No performance degradation
- [ ] Documentation updated
- [ ] Tested on actual hardware
- [ ] Git tagged as stable version

---

**Plan Version**: 1.0
**Created**: 2025-10-18
**Status**: ⏳ Awaiting Approval

---

## Next Steps

1. ✅ Review this plan
2. ⏳ Confirm decisions at decision points
3. ⏳ Begin Phase 1 implementation
4. ⏳ Iteratively complete phases 1-6
5. ⏳ Deploy and validate
