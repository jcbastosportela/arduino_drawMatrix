/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      Logger.cpp                                                                                               *
 * @brief     Logging system implementation                                                                            *
 * @date      Fri Oct 18 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */

#include "Logger.hpp"

#include <cstring>
#include <strings.h>  // For strcasecmp
#include <vector>

#if LOG_ENABLE_FILE
#include <LittleFS.h>
#include <ArduinoJson.h>
#endif

namespace Logger {

// ===========================================================================================
// Singleton Instance
// ===========================================================================================

Log &Log::instance() {
    static Log instance;
    return instance;
}

// ===========================================================================================
// Constructor
// ===========================================================================================

Log::Log()
    : m_currentLevel(LOG_LEVEL_DEBUG)
#if LOG_ENABLE_FILE
      ,
      m_fileLoggingEnabled(false), m_lastFlushTime(0)
#endif
{
    memset(m_buffer, 0, LOG_BUFFER_SIZE);
}

// ===========================================================================================
// Initialization
// ===========================================================================================

void Log::begin(LogLevel level) {
    m_currentLevel = level;

#if LOG_ENABLE_FILE
    // Try to mount LittleFS if not already mounted
    if (!LittleFS.begin()) {
        Serial.println("[LOGGER] Warning: LittleFS not available, file logging disabled");
        m_fileLoggingEnabled = false;
    }
    else {
        // Attempt to load persisted configuration
        loadConfig();
    }
#endif

    // Log initialization
    log(LOG_LEVEL_INFO, "LOGGER", "Logger initialized (level: %s)", levelToString(level));
}

// ===========================================================================================
// Log Level Management
// ===========================================================================================

void Log::setLevel(LogLevel level) {
    m_currentLevel = level;
    log(LOG_LEVEL_INFO, "LOGGER", "Log level changed to: %s", levelToString(level));
}

LogLevel Log::getLevel() const { return m_currentLevel; }

// ===========================================================================================
// Core Logging Functions
// ===========================================================================================

void Log::log(LogLevel level, const char *module, const char *format, ...) {
    // Filter by runtime level
    if (level < m_currentLevel) {
        return;
    }

    va_list args;
    va_start(args, format);
    vlog(level, module, format, args);
    va_end(args);
}

void Log::vlog(LogLevel level, const char *module, const char *format, va_list args) {
    // Filter by runtime level
    if (level < m_currentLevel) {
        return;
    }

    // Format the message
    vsnprintf(m_buffer, LOG_BUFFER_SIZE, format, args);

    // Write to Serial
    writeToSerial(level, module, m_buffer);

    // Write to file if enabled
    if (m_fileLoggingEnabled) {
        writeToFile(level, module, m_buffer);

        // Periodic flush
        unsigned long now = millis();
        if (now - m_lastFlushTime >= LOG_FILE_WRITE_INTERVAL) {
            flush();
            m_lastFlushTime = now;
        }
    }
}

// ===========================================================================================
// Serial Output
// ===========================================================================================

void Log::writeToSerial(LogLevel level, const char *module, const char *message) {
    char timestamp[16];
    formatTimestamp(timestamp, sizeof(timestamp));

    // Format: [TIMESTAMP][LEVEL][MODULE] message
    Serial.printf("[%s][%-7s][%s] %s\n", timestamp, levelToString(level), module, message);
}

void Log::formatTimestamp(char *buffer, size_t bufferSize) {
    // Use millis() for timestamp (format: MM:SS.mmm)
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    minutes %= 60;
    seconds %= 60;
    unsigned long milliseconds = ms % 1000;

    // Format as HH:MM:SS.mmm
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu.%03lu", hours % 100, minutes, seconds, milliseconds);
}

// ===========================================================================================
// Log Level Conversion
// ===========================================================================================

const char *Log::levelToString(LogLevel level) {
    switch (level) {
    case LOG_LEVEL_TRACE:
        return "TRACE";
    case LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LOG_LEVEL_INFO:
        return "INFO";
    case LOG_LEVEL_WARNING:
        return "WARNING";
    case LOG_LEVEL_ERROR:
        return "ERROR";
    case LOG_LEVEL_NONE:
        return "NONE";
    default:
        return "UNKNOWN";
    }
}

LogLevel Log::stringToLevel(const char *str) {
    if (strcasecmp(str, "TRACE") == 0)
        return LOG_LEVEL_TRACE;
    if (strcasecmp(str, "DEBUG") == 0)
        return LOG_LEVEL_DEBUG;
    if (strcasecmp(str, "INFO") == 0)
        return LOG_LEVEL_INFO;
    if (strcasecmp(str, "WARNING") == 0 || strcasecmp(str, "WARN") == 0)
        return LOG_LEVEL_WARNING;
    if (strcasecmp(str, "ERROR") == 0)
        return LOG_LEVEL_ERROR;
    if (strcasecmp(str, "NONE") == 0)
        return LOG_LEVEL_NONE;

    // Default to INFO if invalid
    return LOG_LEVEL_INFO;
}

// ===========================================================================================
// File Logging (if enabled)
// ===========================================================================================

#if LOG_ENABLE_FILE
// ===========================================================================================
// Configuration Persistence
// ===========================================================================================

void Log::loadConfig() {
    if (!LittleFS.exists("/config/log.json")) {
        return; // No config yet
    }
    File f = LittleFS.open("/config/log.json", "r");
    if (!f) {
        log(LOG_LEVEL_ERROR, "LOGGER", "Failed to open /config/log.json for reading");
        return;
    }
    String content = f.readString();
    f.close();

    JsonDocument doc;
    auto err = deserializeJson(doc, content);
    if (err) {
        log(LOG_LEVEL_ERROR, "LOGGER", "Config JSON parse error: %s", err.c_str());
        return;
    }
    if (doc.containsKey("level")) {
        const char *lvlStr = doc["level"].as<const char*>();
        m_currentLevel = stringToLevel(lvlStr); // avoid logging inside during load
    }
    if (doc.containsKey("file_logging")) {
        bool enable = doc["file_logging"].as<int>() != 0;
        enableFileLogging(enable); // will log outcome
    }
    log(LOG_LEVEL_INFO, "LOGGER", "Loaded config (level=%s, file_logging=%s)", levelToString(m_currentLevel), m_fileLoggingEnabled ? "on" : "off");
}

void Log::saveConfig() {
    if (!LittleFS.exists("/config")) {
        LittleFS.mkdir("/config");
    }
    JsonDocument doc;
    doc["level"] = levelToString(m_currentLevel);
    doc["file_logging"] = m_fileLoggingEnabled ? 1 : 0;
    String out; serializeJson(doc, out);
    File f = LittleFS.open("/config/log.json", "w");
    if (!f) {
        log(LOG_LEVEL_ERROR, "LOGGER", "Failed to open /config/log.json for writing");
        return;
    }
    f.print(out);
    f.close();
    log(LOG_LEVEL_DEBUG, "LOGGER", "Saved config: %s", out.c_str());
}

bool Log::enableFileLogging(bool enable) {
    // Check if LittleFS is available
    if (enable && !LittleFS.begin()) {
        log(LOG_LEVEL_ERROR, "LOGGER", "Cannot enable file logging: LittleFS not available");
        return false;
    }

    m_fileLoggingEnabled = enable;

    // Create logs directory if it doesn't exist
    if (enable) {
        if (!LittleFS.exists("/logs")) {
            if (!LittleFS.mkdir("/logs")) {
                log(LOG_LEVEL_ERROR, "LOGGER", "Failed to create /logs directory");
                m_fileLoggingEnabled = false;
                return false;
            }
        }
    }

    log(LOG_LEVEL_INFO, "LOGGER", "File logging %s", enable ? "enabled" : "disabled");
    return true;
}

bool Log::isFileLoggingEnabled() const { return m_fileLoggingEnabled; }

void Log::writeToFile(LogLevel level, const char *module, const char *message) {
    // Check if rotation is needed
    checkAndRotateLogFile();

    // Open current log file in append mode
    File logFile = LittleFS.open("/logs/current.log", "a");
    if (!logFile) {
        // If we can't open the file, disable file logging to avoid spam
        m_fileLoggingEnabled = false;
        Serial.println("[LOGGER] Error: Cannot open log file, disabling file logging");
        return;
    }

    // Format timestamp
    char timestamp[16];
    formatTimestamp(timestamp, sizeof(timestamp));

    // Write formatted log entry
    logFile.printf("[%s][%-7s][%s] %s\n", timestamp, levelToString(level), module, message);

    logFile.close();
}

void Log::checkAndRotateLogFile() {
    // Check if current log file exists and its size
    if (!LittleFS.exists("/logs/current.log")) {
        return; // No rotation needed if file doesn't exist
    }

    File logFile = LittleFS.open("/logs/current.log", "r");
    if (!logFile) {
        return; // Can't check size if we can't open file
    }

    size_t fileSize = logFile.size();
    logFile.close();

    // Rotate if file exceeds max size
    if (fileSize >= LOG_MAX_FILE_SIZE) {
        // Delete old previous.log if it exists
        if (LittleFS.exists("/logs/previous.log")) {
            LittleFS.remove("/logs/previous.log");
        }

        // Rename current.log to previous.log
        LittleFS.rename("/logs/current.log", "/logs/previous.log");

        log(LOG_LEVEL_INFO, "LOGGER", "Log file rotated (size: %u bytes)", fileSize);
    }
}

void Log::clearLogs() {
    if (LittleFS.exists("/logs/current.log")) {
        LittleFS.remove("/logs/current.log");
    }
    if (LittleFS.exists("/logs/previous.log")) {
        LittleFS.remove("/logs/previous.log");
    }
    log(LOG_LEVEL_INFO, "LOGGER", "Log files cleared");
}

String Log::getLogContents(bool current) {
    const char *filename = current ? "/logs/current.log" : "/logs/previous.log";

    if (!LittleFS.exists(filename)) {
        return String(); // Return empty string if file doesn't exist
    }

    File logFile = LittleFS.open(filename, "r");
    if (!logFile) {
        return String();
    }

    String contents = logFile.readString();
    logFile.close();

    return contents;
}

void Log::flush() {
    // LittleFS automatically flushes on close, so nothing to do here
    // This function is provided for API completeness and future enhancements
}

String Log::tailLog(bool current, size_t maxLines) {
    const char *filename = current ? "/logs/current.log" : "/logs/previous.log";
    if (!LittleFS.exists(filename) || maxLines == 0) {
        return String();
    }
    File f = LittleFS.open(filename, "r");
    if (!f) return String();

    // For small maxLines, just read entire file and split
    // More efficient than complex backward scanning on ESP8266
    String content = f.readString();
    f.close();

    if (content.length() == 0) {
        return String();
    }

    // Count lines from end
    std::vector<int> lineStarts;
    lineStarts.push_back(0);
    for (size_t i = 0; i < content.length(); i++) {
        if (content[i] == '\n' && i + 1 < content.length()) {
            lineStarts.push_back(i + 1);
        }
    }

    // Take last N lines
    size_t totalLines = lineStarts.size();
    size_t startIdx = (totalLines > maxLines) ? (totalLines - maxLines) : 0;

    if (startIdx >= lineStarts.size()) {
        return content;
    }

    return content.substring(lineStarts[startIdx]);
}

#endif // LOG_ENABLE_FILE

} // namespace Logger
