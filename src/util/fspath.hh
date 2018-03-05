// Abstraction for paths on user's filesystem

#pragma once
#include "common.hh"

#ifdef PLATFORM_WIN
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

#include <string>
#include <vector>

namespace rc {
namespace util {

class FSPath {
	// data stored is implementation dependent
	void* data = nullptr;

	// convenience function to fill out data if needed
	void statFile();

public:
	// construct FSPath object from string representing path
	FSPath(const std::string& path) : str(path), data(nullptr) {}

	// delete FSPath object
	~FSPath();

	// copy constructor
	FSPath(const FSPath& other);

	// copy assignment operator
	FSPath& operator=(const FSPath& other);

	// test if file exists
	bool exists();

	// test if directory
	bool isDirectory();

	// get modification time
	u64 lastModifiedTime();

	// list child paths (returns empty if not directory)
	std::vector<FSPath> children();

	// get child path
	FSPath operator/(const std::string& child);

	// get file name
	const char* fileName() const;

	// get file extension (returns whole file name if no extension)
	const char* fileExt() const;

	// string containing path
	std::string str;

	// create a directory on the user's filesystem
	// returns true if successful
	bool mkDir();

	// read to buffer
	Buffer read();

	// write to file (returns true on success)
	bool write(Buffer& data);
};

// return FSPath representing directory containing game contents
FSPath gameDir();

// will correct forwardslashes to backslashes on Windows
// will not work the other way around
std::string pathFixSeparator(std::string path);

}
}
