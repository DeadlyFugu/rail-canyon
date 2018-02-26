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

namespace rc {
namespace util {

// print info message to console
void log_info_(const char* func, const char* format, ...);

// print warning message to console
void log_warn_(const char* func, const char* format, ...);

// print error message to console
void log_error_(const char* func, const char* format, ...);

// print debug message to console
void log_debug_(const char* func, const char* format, ...);

#define log_info(fmt...)  rc::util::log_info_ (__func__, fmt)
#define log_warn(fmt...)  rc::util::log_warn_ (__func__, fmt)
#define log_error(fmt...) rc::util::log_error_(__func__, fmt)
#define log_debug(fmt...) rc::util::log_debug_(__func__, fmt)

}
}
