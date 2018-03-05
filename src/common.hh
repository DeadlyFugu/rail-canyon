#pragma once
#include <vector>
#include <string>
#include <memory>

using std::unique_ptr;
using std::shared_ptr;
using std::move;

// Platform defines
#ifdef _WIN32
#define PLATFORM_WIN
#endif

#ifdef __APPLE__
#define PLATFORM_OSX
#define PLATFORM_POSIX
#endif

#ifdef __linux__
#define PLATFORM_LINUX
#define PLATFORM_POSIX
#endif

#include "util/types.hh"
using namespace rc::types;

//#include "util/buffer.hh"
//using rc::util::Buffer;

#include "util/log.hh"

#include "util.hh"
using rw::util::logger;
using rw::util::Buffer;

#define assert(x, msg) assert__(x, #x, msg, __func__, __FILE__, __LINE__)
inline void assert__(bool cond, const char* x, const char* msg, const char* func, const char* file, int line);

extern "C" void abort();
inline void assert__(bool cond, const char* x, const char* msg, const char* func, const char* file, int line) {
	if (!cond) {
		rc::util::log_error_(func, "assert(%s) failed: %s", x, msg);
		rc::util::log_error_(func, "at %s:%i", file, line);
		abort();
	}
}
