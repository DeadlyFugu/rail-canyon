// This implementation based heavily on the following C# implementation:
// https://github.com/sonicretro/HeroesONE
// Original implementation by MainMemory and Sewer56

#include <stdlib.h>
#include "common.hh"
#include "ONEArchive.hh"
#include "util/prs.hh"

namespace rc {
namespace io {

using util::FSPath;

static const int ONE_HeroesMagic = 0x1400FFFF;
static const int ONE_HeroesE3Magic = 0x1005FFFF;
static const int ONE_HeroesPreE3Magic = 0x1003FFFF;
static const int ONE_Shadow060Magic = 0x1C020037;
static const int ONE_Shadow050Magic = 0x1C020020;

ONEArchive::ONEArchive(FSPath& path) : f(path), name(path.fileName()) {
	if (!f.opened()) {
		log_error("could not open .one archive %s", path.str.c_str());
	} else {
		f.seek(4);

		u32 filesize;
		f.read(&filesize);
		filesize += 0xc;

		u32 magic;
		f.read(&magic);

		switch (magic) {
			case ONE_HeroesMagic: {
				type = ArchiveType::Heroes;
			}
				break;
			case ONE_HeroesE3Magic: {
				type = ArchiveType::HeroesE3;
			}
				break;
			case ONE_HeroesPreE3Magic: {
				type = ArchiveType::HeroesPreE3;
			}
				break;
			case ONE_Shadow050Magic: {
				type = ArchiveType::Shadow050;
				log_error("Shadow .ONE archives are unsupported");
			}
				break;
			case ONE_Shadow060Magic: {
				type = ArchiveType::Shadow060;
				log_error("Shadow .ONE archives are unsupported");
			}
				break;
			default: {
				type = ArchiveType::Unknown;
				log_error("Unknown archive type");
			}
		}

		if (type == ArchiveType::Heroes || type == ArchiveType::HeroesE3 || type == ArchiveType::HeroesPreE3) {
			f.seek(f.tell() + 4);

			u32 filenames_len;
			f.read(&filenames_len);

			f.seek(f.tell() + 4);

			std::vector<std::string> filenames;
			u32 filenames_end = filenames_len + f.tell();
			char name_buffer[64];

			while (f.tell() < filenames_end) {
				f.read(name_buffer, 64);
				filenames.emplace_back(name_buffer);
			}
			f.seek(filenames_end);

			while (f.tell() < filesize) {
				struct {
					u32 nameidx;
					u32 size;
					u32 unknown;
				} header;
				f.read(&header);

				FileEntry entry;
				entry.offset = (u32) f.tell();
				entry.length = header.size;
				entry.name = std::string(filenames[header.nameidx]);
				fileTable.push_back(entry);

				f.seek(entry.offset + entry.length);
			}
		}
	}
}

int ONEArchive::getFileCount() {
	return fileTable.size();
}

const char* ONEArchive::getFileName(int index) {
	return fileTable[index].name.c_str();
}

int ONEArchive::findFile(const char* _name) {
	std::string name(_name);
	for (int i = 0; i < fileTable.size(); i++) {
		if (fileTable[i].name == name) return i;
	}
	return -1;
}

Buffer ONEArchive::readFile(int index) {
	// read data
	auto offs = fileTable[index].offset;
	auto len = fileTable[index].length;
	auto data = malloc(len);
	f.seek(offs);
	f.read(data, len);

	Buffer b = prs_decode(data, len);
	free(data);

	return std::move(b);
}

Buffer ONEArchive::readFile(const char* name) {
	return readFile(findFile(name));
}

}
}
