// Abstraction for file on user's filesystem

#pragma once
#include "common.hh"
#include <string>

#include "util/fspath.hh"
#include "util/buffer.hh"

namespace rc {
namespace util {

class FSFile {
private:
	// stores FILE handle
	void* f;
public:
	// open a file at path
	FSFile(const std::string& path, bool overwrite = false) : FSFile(FSPath(path), overwrite) {};

	// open a file at path
	FSFile(FSPath&& path, bool overwrite = false);

	// open a file at path
	FSFile(FSPath& path, bool overwrite = false);

	// release file resource
	~FSFile();

	// test if file opened successfully
	bool opened();

	// read from file on disc
	void read(void* dst, unsigned len);

	// write to file on disc
	void write(const void* src, unsigned len);

	// read a struct to value
	template<typename T>
	void read(T* value) {
		read(value, sizeof(T));
	}

	// write a struct from value
	template<typename T>
	void write(const T& value) {
		write(&value, sizeof(T));
	}

	// read a newline-terminated line from file
	std::string readLine();

	// write a newline-terminated line to file
	void writeLine(std::string line);

	// read string from file in u16string format
	std::string read_u16string();

	// write string to file in u16string format
	void write_u16string(const std::string& value);

	// return current offset
	u64 tell();

	// seek to pos
	void seek(u64 pos);

	// return size of file in bytes
	u64 size();

	// test if at end of file
	bool atEnd();

	// return buffer containing file contents
	Buffer toBuffer();

	// truncate or expand file as needed
	// WARNING: file should probably be closed immediately after this
	void resize(u64 size);
};

}
}
