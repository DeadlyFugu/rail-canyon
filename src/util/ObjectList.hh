// ObjectList.hh: Read ObjectList.ini files

#pragma once
#include "common.hh"

class ObjectList {
public:
	struct ObjectTypeData {
		u8 typeID = 0;
		u8 listID = 0;
		const char* name = nullptr;
		const char* debugName = nullptr;
		const char* description = nullptr;
		const char* rotation = nullptr;
		const char* miscFormat = nullptr;
		const char* miscProperties = nullptr;
	};

	Buffer data;
private:
	std::vector<ObjectTypeData> db;
public:
	ObjectList() : data(0) {}

	void readFile(const char* file);
	void writeFile(const char* file);
	void readEXE(const char* file);

	ObjectTypeData* getObjectData(u8 listID, u8 typeID);
};
