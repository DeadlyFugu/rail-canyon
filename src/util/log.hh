// log.hh: Print logging messages

#pragma once
#include "common.hh"

#ifdef PLATFORM_POSIX
#define CONSOLE_ANSI
#endif

#ifdef CONSOLE_ANSI
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"
#else
#define ANSI_RED
#define ANSI_GREEN
#define ANSI_YELLOW
#define ANSI_BLUE
#define ANSI_MAGENTA
#define ANSI_CYAN
#define ANSI_RESET
#endif

#define MSG_BEGIN "["
#define MSG_END "] "

namespace rc {
namespace util {

// prints message
void log_message(const char* prefix, const char* func, const char* format, ...);

#ifdef PLATFORM_WIN
// print info message to console
#define log_info(fmt)  rc::util::log_info_ (__func__, fmt)
// print warning message to console
#define log_warn(fmt)  rc::util::log_warn_ (__func__, fmt)
// print error message to console
#define log_error(fmt) rc::util::log_error_(__func__, fmt)
// print debug message to console
#define log_debug(fmt) rc::util::log_debug_(__func__, fmt)
#else
// print info message to console
#define log_info(fmt...)  rc::util::log_info_ (__func__, fmt)
// print warning message to console
#define log_warn(fmt...)  rc::util::log_warn_ (__func__, fmt)
// print error message to console
#define log_error(fmt...) rc::util::log_error_(__func__, fmt)
// print debug message to console
#define log_debug(fmt...) rc::util::log_debug_(__func__, fmt)
#endif

template<typename... Args>
void log_info_(const char* func, const char* format, Args... args) {
	log_message(MSG_BEGIN ANSI_BLUE "info" ANSI_RESET MSG_END, func, format, args...);
}

template<typename... Args>
void log_warn_(const char* func, const char* format, Args... args) {
	log_message(MSG_BEGIN ANSI_YELLOW "warn" ANSI_RESET MSG_END, func, format, args...);
}

template<typename... Args>
void log_error_(const char* func, const char* format, Args... args) {
	log_message(MSG_BEGIN ANSI_RED "error" ANSI_RESET MSG_END, func, format, args...);
}

template<typename... Args>
void log_debug_(const char* func, const char* format, Args... args) {
	log_message(MSG_BEGIN ANSI_GREEN "debug" ANSI_RESET MSG_END, func, format, args...);
}

}
}
