/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      Logger.hpp                                                                                               *
 * @brief     Logging system with multiple levels and optional file output                                            *
 * @date      Fri Oct 18 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */

#ifndef DRAWMATRIX_LOGGER
#define DRAWMATRIX_LOGGER

#include <Arduino.h>
#include <cstdarg>
#include <cstdint>

// ===========================================================================================
// Configuration - Compile-Time Settings
// ===========================================================================================

// Maximum length of a single log message
#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 256
#endif

// Compile-time log level filter (messages below this level won't be compiled)
// Set to LOG_LEVEL_TRACE to include all levels
#ifndef LOG_LEVEL_COMPILE_TIME
#define LOG_LEVEL_COMPILE_TIME LOG_LEVEL_DEBUG
#endif

// Enable file logging support (can be disabled to save memory)
#ifndef LOG_ENABLE_FILE
#define LOG_ENABLE_FILE 1
#endif

// Maximum size per log file before rotation (50KB default)
#ifndef LOG_MAX_FILE_SIZE
#define LOG_MAX_FILE_SIZE (50 * 1024)
#endif

// Write buffered logs to file every N milliseconds
#ifndef LOG_FILE_WRITE_INTERVAL
#define LOG_FILE_WRITE_INTERVAL 5000
#endif

namespace Logger {

// ===========================================================================================
// Log Level Definitions
// ===========================================================================================

/**
 * @brief Log level enumeration (higher value = higher priority)
 */
enum LogLevel : uint8_t {
    LOG_LEVEL_TRACE = 0,   ///< Most verbose - detailed execution flow
    LOG_LEVEL_DEBUG = 1,   ///< Debug information for development
    LOG_LEVEL_INFO = 2,    ///< General informational messages
    LOG_LEVEL_WARNING = 3, ///< Warning conditions
    LOG_LEVEL_ERROR = 4,   ///< Error conditions
    LOG_LEVEL_NONE = 255   ///< Disable all logging
};

// ===========================================================================================
// Logger Class (Singleton)
// ===========================================================================================

/**
 * @brief Main logger class - Singleton pattern for global access
 *
 * Format: [TIMESTAMP][LEVEL][MODULE] message
 * Example: [00:12:34.567][INFO][MAIN] WiFi connected
 */
class Log {
  public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the logger instance
     */
    static Log &instance();

    /**
     * @brief Initialize the logger (call once in setup())
     * @param level Initial log level (default: DEBUG)
     */
    void begin(LogLevel level = LOG_LEVEL_DEBUG);

    /**
     * @brief Set the current log level at runtime
     * @param level New log level
     */
    void setLevel(LogLevel level);

    /**
     * @brief Get the current log level
     * @return Current log level
     */
    LogLevel getLevel() const;

    /**
     * @brief Log a message with printf-style formatting
     * @param level Log level for this message
     * @param module Module/component name (e.g., "MAIN", "WIFI", "MATRIX")
     * @param format Printf-style format string
     * @param ... Variable arguments for format string
     */
    void log(LogLevel level, const char *module, const char *format, ...);

    /**
     * @brief Log a message with va_list (internal use)
     * @param level Log level for this message
     * @param module Module/component name
     * @param format Printf-style format string
     * @param args Variable argument list
     */
    void vlog(LogLevel level, const char *module, const char *format, va_list args);

#if LOG_ENABLE_FILE
    /**
     * @brief Enable or disable file logging
     * @param enable True to enable file logging, false to disable
     * @return True if successful, false if LittleFS not available
     */
    bool enableFileLogging(bool enable);

    /**
     * @brief Check if file logging is enabled
     * @return True if file logging is enabled
     */
    bool isFileLoggingEnabled() const;

    /**
     * @brief Clear all log files
     */
    void clearLogs();

    /**
     * @brief Get log file contents
     * @param current True for current.log, false for previous.log
     * @return String containing log contents (empty if file doesn't exist)
     */
    String getLogContents(bool current = true);

    /**
     * @brief Force flush buffered logs to file
     */
    void flush();

    /**
     * @brief Read last N lines efficiently from a log file
     * @param current true = current.log, false = previous.log
     * @param maxLines number of lines to return (tail)
     * @return String containing up to maxLines of recent log lines (newest last)
     */
    String tailLog(bool current, size_t maxLines);
#endif

    /**
     * @brief Get string representation of log level
     * @param level Log level
     * @return String representation (e.g., "INFO", "ERROR")
     */
    static const char *levelToString(LogLevel level);

    /**
     * @brief Convert string to log level
     * @param str String representation (e.g., "INFO", "ERROR")
     * @return Log level (defaults to INFO if invalid)
     */
    static LogLevel stringToLevel(const char *str);

#if LOG_ENABLE_FILE
    /**
     * @brief Load logger configuration from /config/log.json if present
     * Format: {"level":"INFO","file_logging":1}
     */
    void loadConfig();

    /**
     * @brief Save current logger configuration to /config/log.json
     */
    void saveConfig();
#endif

  private:
    // Private constructor for singleton
    Log();

    // Prevent copying
    Log(const Log &) = delete;
    Log &operator=(const Log &) = delete;

    /**
     * @brief Format timestamp for log entry
     * @param buffer Buffer to write timestamp to
     * @param bufferSize Size of buffer
     */
    void formatTimestamp(char *buffer, size_t bufferSize);

    /**
     * @brief Write formatted log to Serial
     * @param level Log level
     * @param module Module name
     * @param message Formatted message
     */
    void writeToSerial(LogLevel level, const char *module, const char *message);

#if LOG_ENABLE_FILE
    /**
     * @brief Write formatted log to file (buffered)
     * @param level Log level
     * @param module Module name
     * @param message Formatted message
     */
    void writeToFile(LogLevel level, const char *module, const char *message);

    /**
     * @brief Check if log file needs rotation and perform if necessary
     */
    void checkAndRotateLogFile();
#endif

  private:
    LogLevel m_currentLevel;        ///< Current runtime log level
    char m_buffer[LOG_BUFFER_SIZE]; ///< Message formatting buffer

#if LOG_ENABLE_FILE
    bool m_fileLoggingEnabled;     ///< File logging enabled flag
    unsigned long m_lastFlushTime; ///< Last time logs were flushed to file
#endif
};

// ===========================================================================================
// Convenience Macros
// ===========================================================================================

// Internal helper macro to check compile-time level
#define LOG_IF_COMPILED(level) ((level) >= LOG_LEVEL_COMPILE_TIME)

// Log macros - only compile if level meets compile-time threshold
#if LOG_IF_COMPILED(LOG_LEVEL_TRACE)
#define LOG_TRACE(module, format, ...)                                                                                 \
    Logger::Log::instance().log(Logger::LOG_LEVEL_TRACE, module, format, ##__VA_ARGS__)
#else
#define LOG_TRACE(module, format, ...) ((void)0)
#endif

#if LOG_IF_COMPILED(LOG_LEVEL_DEBUG)
#define LOG_DEBUG(module, format, ...)                                                                                 \
    Logger::Log::instance().log(Logger::LOG_LEVEL_DEBUG, module, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(module, format, ...) ((void)0)
#endif

#if LOG_IF_COMPILED(LOG_LEVEL_INFO)
#define LOG_INFO(module, format, ...) Logger::Log::instance().log(Logger::LOG_LEVEL_INFO, module, format, ##__VA_ARGS__)
#else
#define LOG_INFO(module, format, ...) ((void)0)
#endif

#if LOG_IF_COMPILED(LOG_LEVEL_WARNING)
#define LOG_WARNING(module, format, ...)                                                                               \
    Logger::Log::instance().log(Logger::LOG_LEVEL_WARNING, module, format, ##__VA_ARGS__)
#else
#define LOG_WARNING(module, format, ...) ((void)0)
#endif

#if LOG_IF_COMPILED(LOG_LEVEL_ERROR)
#define LOG_ERROR(module, format, ...)                                                                                 \
    Logger::Log::instance().log(Logger::LOG_LEVEL_ERROR, module, format, ##__VA_ARGS__)
#else
#define LOG_ERROR(module, format, ...) ((void)0)
#endif

} // namespace Logger

#endif /* DRAWMATRIX_LOGGER */
