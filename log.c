// Logging utility
// TSullivan

void logPrint(char level, const char* file, int line, const char* fmt, ...);
#define logInfo(...) logPrint(0, __FILE__, __LINE__, __VA_ARGS__)
#define logWarn(...) logPrint(1, __FILE__, __LINE__, __VA_ARGS__)
#define logError(...) logPrint(2, __FILE__, __LINE__, __VA_ARGS__)
#define logErrorNV(...) logPrint(2, (void*)0, 0, __VA_ARGS__)

#ifndef LOG
#define LOG

#include <stdio.h>
#include <stdarg.h>

#define LOG_COLOURS

#ifdef LOG_COLOURS
const char logInfoStr[] = "\x1b[32mINFO\x1b[0m ";
const char logWarnStr[] = "\x1b[33mWARN\x1b[0m ";
const char logErrStr[] = "\x1b[31mERROR\x1b[0m ";
#elif
const char logInfoStr[] = "INFO";
const char logWarnStr[] = "WARN";
const char logErrStr[] = "ERROR";
#endif

void logPrint(char level, const char* file, int line, const char* fmt, ...) {
	const char* l;
	switch (level) {
	default:
	case 0: l = logInfoStr; break;
	case 1: l = logWarnStr; break;
	case 2: l = logErrStr; break;
	}
#ifdef LOG_COLOURS
	if (file) printf("%s \x1b[90m%s:%i|\x1b[0m ", l, file, line);
#elif
	if (file) printf("%s %s:%i| ", l, file, line);
#endif
	else printf(l);
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

#endif
