// Read ONE archive from heroes
// Based heavily on https://github.com/sonicretro/HeroesONE/blob/master/HeroesONELib/HeroesONEFile.cs

#pragma once
#include "util/fspath.hh"

#include <vector>

namespace rc {
namespace io {

class ONEArchive {
private:
	Buffer f;
	std::string name;

	struct FileEntry {
		std::string name;
		u32 offset;
		u32 length;
	};
	std::vector<FileEntry> fileTable;

	enum class ArchiveType {
		Heroes, HeroesE3, HeroesPreE3, Shadow050, Shadow060, Unknown
	};
	ArchiveType type;
public:
	ONEArchive(util::FSPath& path);

	int getFileCount();
	const char* getFileName(int index);
	int findFile(const char* name);
	virtual Buffer readFile(int file);
	virtual Buffer readFile(const char* name);
};

}
}
