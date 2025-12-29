#include "3dslog.h"
#include <3ds.h>
#include <string.h>

static FILE* logFile = NULL;
static bool loggingToFile = false;
static const char* LOG_PATH = "sdmc:/snes9x_3ds_stereo.log";

static const char* levelNames[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void log3dsInit(bool logToFile) {
    loggingToFile = logToFile;

    if (logToFile) {
        // Open log file in append mode
        logFile = fopen(LOG_PATH, "a");
        if (logFile) {
            fprintf(logFile, "\n========================================\n");
            fprintf(logFile, "SNES9x 3DS Stereo - New Session\n");
            fprintf(logFile, "========================================\n");
            fflush(logFile);
            printf("[LOG] Logging to: %s\n", LOG_PATH);
        } else {
            printf("[LOG] ERROR: Could not open log file!\n");
            loggingToFile = false;
        }
    }
}

void log3dsClose() {
    if (logFile) {
        fprintf(logFile, "\n========================================\n");
        fprintf(logFile, "Session End\n");
        fprintf(logFile, "========================================\n\n");
        fclose(logFile);
        logFile = NULL;
    }
}

void log3ds(LogLevel level, const char* tag, const char* format, ...) {
    char buffer[512];
    va_list args;

    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Always print to console (bottom screen)
    printf("[%s:%s] %s\n", levelNames[level], tag, buffer);

    // Also write to file if enabled
    if (loggingToFile && logFile) {
        // Get timestamp
        u64 timeInMs = osGetTime();
        unsigned int seconds = timeInMs / 1000;
        unsigned int ms = timeInMs % 1000;

        fprintf(logFile, "[%u.%03u] [%s:%s] %s\n",
                seconds, ms, levelNames[level], tag, buffer);
        fflush(logFile); // Ensure it's written immediately
    }
}
