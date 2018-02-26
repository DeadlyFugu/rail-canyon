#include "common.hh"
#include "util/log.hh"

#include <string>
#include <vector>
#include "stdarg.h"
#include "stdio.h"

#define MSG_BEGIN "["
#define MSG_END "] "

namespace rc {
namespace util {

static std::vector<std::string> logMessageRecord;

void log_message(const char* prefix, const char* func, const char* format, va_list args) {
	char buffer[512];
	size_t offset = (size_t) snprintf(buffer, 512, "%s%s: ", prefix, func);
	vsnprintf(buffer + offset, 512 - offset, format, args);
	puts(buffer);
	logMessageRecord.emplace_back(buffer);
}

// print info message to console
void log_info_(const char* func, const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_message(MSG_BEGIN ANSI_BLUE "info" ANSI_RESET MSG_END, func, format, args);
	va_end(args);
}

// print warning message to console
void log_warn_(const char* func, const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_message(MSG_BEGIN ANSI_YELLOW "warn" ANSI_RESET MSG_END, func, format, args);
	va_end(args);
}

// print error message to console
void log_error_(const char* func, const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_message(MSG_BEGIN ANSI_RED "error" ANSI_RESET MSG_END, func, format, args);
	va_end(args);
}

// print debug message to console
void log_debug_(const char* func, const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_message(MSG_BEGIN ANSI_GREEN "debug" ANSI_RESET MSG_END, func, format, args);
	va_end(args);
}

}
}
