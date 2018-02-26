#include "common.hh"
#include "util/fspath.hh"

#include <sys/types.h>
#include <sys/stat.h>

namespace rc {
namespace util {

void FSPath::statFile() {
	if (data) return;
	data = new struct stat();
	int failed = stat(str.c_str(), (struct stat*) data);
	if (failed) {
		((struct stat*) data)->st_mode = 0;
	}
}


FSPath::~FSPath() {
	if (data) delete data;
}


FSPath::FSPath(const FSPath& other) : str(other.str) {
	if (other.data) {
		data = new struct stat();
		memcpy(data, other.data, sizeof(struct stat));
	}
}

FSPath& FSPath::operator=(const FSPath& other) {
	if (this != &other) {
		if (data) delete data;
		data = new struct stat();
		memcpy(data, other.data, sizeof(struct stat));
	}
	str = other.str;

	return *this;
}

bool FSPath::exists() {
	statFile();
	return ((struct stat*) data)->st_mode != 0;
}

bool FSPath::isDirectory() {
	statFile();
	return (((struct stat*) data)->st_mode & S_IFMT) == S_IFDIR;
}

u64 FSPath::lastModifiedTime() {
	statFile();
	return (u64) ((struct stat*) data)->st_mtime;
}

#ifdef PLATFORM_WIN
#include <Windows.h>
vector<FSPath> FSPath::children() {
	vector<FSPath> out;

	WIN32_FIND_DATA findData;
	HANDLE h;

	string strWithSlash = str + "\\*";
	h = FindFirstFile(strWithSlash.c_str(), &findData);
	bool hasNextFile = true;

	while (h != INVALID_HANDLE_VALUE && hasNextFile) {
		if (findData.cFileName[0] != '.')
			out.push_back(*this / findData.cFileName);
		hasNextFile = FindNextFile(h, &findData);
	}

	FindClose(h);

	return out;
}
#else
#include <dirent.h>
std::vector<FSPath> FSPath::children() {
	std::vector<FSPath> out;

	struct dirent* dirEntry;
	DIR* dir = opendir(str.c_str());

	while ((dirEntry = readdir(dir))) {
		if (dirEntry->d_name[0] != '.')
			out.push_back(*this / dirEntry->d_name);
	}

	closedir(dir);

	return out;
}
#endif

FSPath FSPath::operator/(const std::string& child) {
	char lastChar = str.back();
	if (lastChar == '/' || lastChar == '\\') {
		return FSPath(str + child);
	} else {
		return FSPath(str + PATH_SEPARATOR + child);
	}
}

const char* FSPath::fileName() const {
	return strrchr(str.c_str(), PATH_SEPARATOR)+1;
}

const char* FSPath::fileExt() const {
	const char* name = fileName();
	const char* ext = strrchr(name, '.');
	return ext ? ext : name;
}

#ifdef PLATFORM_WIN
bool FSPath::mkDir() {
	return !(CreateDirectory(str.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS);
}
#else
bool FSPath::mkDir() {
	return mkdir(str.c_str(), 0700) == 0;
}

#endif

#include "whereami.h"
FSPath determineGameDir() {
	auto len = wai_getExecutablePath(nullptr, 0, nullptr);
	auto buffer = new char[len+1];
	int dirname_len;
	wai_getExecutablePath(buffer, 1024, &dirname_len);
	buffer[len] = 0;
#if defined(PLATFORM_OSX) and not defined(FAERJOLD_EDITOR)
	const auto nested_count = 4;
#else
	const auto nested_count = 1;
#endif
	for (int i = 0; i < nested_count; i++) {
		*strrchr(buffer, PATH_SEPARATOR) = 0;
	}
	log_debug("gameDir() = '%s'", buffer);
	auto path = FSPath(buffer);
	delete[] buffer;
	return path;
}

FSPath gameDir() {
	static FSPath path = determineGameDir();
	return path;
}

std::string pathFixSeparator(std::string path) {
#ifdef PLATFORM_WIN
	char* p = &path[0];
	while ((p = strchr(p, '/'))) {
		*p = '\\';
	}
#endif
	return path;
}

}
}
