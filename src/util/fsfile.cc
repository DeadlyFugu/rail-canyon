#include "common.hh"
#include "util/fsfile.hh"
#include <vector>
#include "stdlib.h"

#include <stdio.h>

namespace rc {
namespace util {

FSFile::FSFile(FSPath&& path, bool overwrite) {
	if (!overwrite && path.exists()) {
		f = fopen(path.str.c_str(), "rb+");
	} else {
		f = fopen(path.str.c_str(), "wb+");
	}
}

FSFile::FSFile(FSPath& path, bool overwrite) {
	if (!overwrite && path.exists()) {
		f = fopen(path.str.c_str(), "rb+");
	} else {
		f = fopen(path.str.c_str(), "wb+");
	}
}

FSFile::~FSFile() {
	if (f) {
		fclose((FILE*) f);
	}
}

bool FSFile::opened() {
	return f != nullptr;
}

void FSFile::read(void* dst, unsigned len) {
	fread(dst, len, 1, (FILE*) f);
}

void FSFile::write(const void* src, unsigned len) {
	fwrite(src, len, 1, (FILE*) f);
}

std::string FSFile::readLine() {
	std::vector<char> buffer;
	char c;
	read(&c);
	while (c != '\n' && !atEnd()) {
		buffer.push_back(c);
		read(&c);
	}
	if (buffer.size() > 0 && buffer.back() == '\r') buffer[buffer.size() - 1] = 0;
	else buffer.push_back(0);
	return std::string(&buffer[0]);
}

void FSFile::writeLine(std::string line) {
	write(&line[0], line.size());
	write((u8) 0);
}

std::string FSFile::read_u16string() {
	u16 len;
	read(&len);

	char* buffer = (char*) malloc(len + 1);
	read(buffer, len);
	buffer[len] = 0;

	std::string s(buffer);
	free(buffer);

	return s;
}

void FSFile::write_u16string(const std::string& value) {
	u16 len = value.size();
	write(len);

	write(&value[0], len);
}

u64 FSFile::tell() {
	return (u64) ftell((FILE*) f);
}

void FSFile::seek(u64 pos) {
	fseek((FILE*) f, (i64) pos, SEEK_SET);
}

u64 FSFile::size() {
	u64 old_offs = tell();
	fseek((FILE*) f, 0, SEEK_END);
	u64 size = tell();
	seek(old_offs);
	return size;
}

bool FSFile::atEnd() {
	return feof((FILE*) f) != 0;
}

Buffer FSFile::toBuffer() {
	auto len = size();
	auto data = malloc(len);
	seek(0);
	read(data, len);
	return Buffer(data, len, true);
}

#ifdef PLATFORM_POSIX
#include <unistd.h>
void FSFile::resize(u64 size) {
	fflush((FILE*) f);
	ftruncate(fileno((FILE*) f), size);
}
#else

#include <io.h>

void FSFile::resize(u64 size) {
	fflush((FILE*) f);
	_chsize_s(_fileno((FILE*) f), size);
}

#endif

}
}
