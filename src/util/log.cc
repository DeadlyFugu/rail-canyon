#include "common.hh"
#include "util/log.hh"

#include <string>
#include <vector>
#include "stdarg.h"
#include "stdio.h"

static std::vector<std::string> logMessageRecord;

void log_message(const char* prefix, const char* func, const char* format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[512];
	buffer[0] = '\0';
	size_t offset = (size_t) snprintf(buffer, 512, "%s%s: ", prefix, func);
	vsnprintf(&buffer[offset], 512 - offset, format, args);
	puts(buffer);
	logMessageRecord.emplace_back(buffer);
	va_end(args);
}
