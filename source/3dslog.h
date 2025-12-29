#ifndef _3DSLOG_H
#define _3DSLOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// Log levels
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} LogLevel;

// Initialize logging system
// logToFile: true = log to SD card, false = only printf
void log3dsInit(bool logToFile);

// Close log file
void log3dsClose();

// Main logging function
void log3ds(LogLevel level, const char* tag, const char* format, ...);

// Convenience macros
#define LOG_DEBUG(tag, ...) log3ds(LOG_DEBUG, tag, __VA_ARGS__)
#define LOG_INFO(tag, ...)  log3ds(LOG_INFO, tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)  log3ds(LOG_WARN, tag, __VA_ARGS__)
#define LOG_ERROR(tag, ...) log3ds(LOG_ERROR, tag, __VA_ARGS__)

// Stereo-specific logging helpers
#define LOG_STEREO(...) LOG_INFO("STEREO", __VA_ARGS__)
#define LOG_LAYER(...)  LOG_DEBUG("LAYER", __VA_ARGS__)
#define LOG_DRAW(...)   LOG_DEBUG("DRAW", __VA_ARGS__)
#define LOG_GPU(...)    LOG_DEBUG("GPU", __VA_ARGS__)

#endif // _3DSLOG_H
