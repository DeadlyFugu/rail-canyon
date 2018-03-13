#include "ObjectList.hh"
#include "util/fspath.hh"

static void advanceToNextLine(char** p) {
	while (**p && **p != '\n') (*p)++; // advance to next newline or eof
	if (**p == '\n') (*p)++; // if reached newline, skip past it
}

static int hexCharValue(char x) {
	if (x <= '9') return x - '0';
	if (x <= 'F') return x - 'A' + 0x0a;
	return x - 'a' + 0x0a;
}

void ObjectList::readFile(const char* file) {
	FSPath path(file);

	// read file to buffer
	Buffer tmp = path.read();
	// todo: add a 'transfer' method to Buffer class to move ownership of memory from one to another
	data.setStretchy(true);
	data.resize(tmp.size());
	data.setStretchy(false);
	tmp.read(data.base_ptr(), data.size());

	// pointer for parsing through text
	char* p = (char*) data.base_ptr();

	// store current object's data
	ObjectTypeData* current = nullptr;

	// loop over each line
	while (*p) {
		if (*p == '[') { // beginning of new object definition
			int listID;
			int typeID;
			//int result = sscanf(p, "[%d][%d]", &listID, &typeID);
			if (*(p+3) != ']' || *(p+4) != '[' || *(p+7) != ']') {
				logger.error("Invalid formatting in line '%s'", p);
			} else {
				listID = hexCharValue(*(p+1)) * 0x10 + hexCharValue(*(p+2));
				typeID = hexCharValue(*(p+5)) * 0x10 + hexCharValue(*(p+6));
			}

			db.emplace_back();
			current = &db.back();

			current->listID = (u8) listID;
			current->typeID = (u8) typeID;

			advanceToNextLine(&p);
		} else if (*p == '#') {
			advanceToNextLine(&p);
		} else if (current) {
			// get pointers to line begin and end and then null-terminate line
			char* begin = p;
			advanceToNextLine(&p);
			char* end = p;
			if (*end) {
				--end; // if not null terminator, move back one to the newline char
				*end = 0; // and make it a null terminator
			}

			char* mid = strchr(begin, '='); // find '=' character
			if (!mid) {
				continue;
			}
			*mid = 0; // replace '=' with null terminator
			char* property = begin; // begin is currently pointing to a string containing the property name
			char* value = mid + 1; // string containing value starts just after mid

			if (!strcmp(property, "Object")) {
				current->name = value;
			} else if (!strcmp(property, "Debug")) {
				current->debugName = value;
			} else if (!strcmp(property, "Description")) {
				current->description = value;
			} else if (!strcmp(property, "Rotation")) {
				current->rotation = value;
			} else if (!strcmp(property, "MiscFormat")) {
				current->miscFormat = value;
			} else if (!strcmp(property, "MiscProperties")) {
				current->miscProperties = value;
			} else {
				logger.warn("Unsupported property in %s: '%s'", file, property);
			}
		} else {
			advanceToNextLine(&p);
		}
	}
}

void ObjectList::writeFile(const char* file) {
	FILE* f = fopen(file, "w");
	fprintf(f, "# Generated by railcanyon\n\n");
	for (auto& object : db) {
		fprintf(f, "[%02x][%02x]\n", object.listID, object.typeID);

		if (object.name) fprintf(f, "Object=%s\n", object.name);
		if (object.debugName) fprintf(f, "Debug=%s\n", object.debugName);
		if (object.description) fprintf(f, "Description=%s\n", object.description);
		if (object.rotation) fprintf(f, "Rotation=%s\n", object.rotation);
		if (object.miscFormat) fprintf(f, "MiscFormat=%s\n", object.miscFormat);
		if (object.miscProperties) fprintf(f, "MiscProperties=%s\n", object.miscProperties);
		fprintf(f, "\n");
	}
	fprintf(f, "EndOfFile");
	fclose(f);
}

ObjectList::ObjectTypeData* ObjectList::getObjectData(u8 listID, u8 typeID) {
	for (int i = 0; i < db.size(); i++) {
		auto object = &db[i];
		if (object->listID == listID && object->typeID == typeID) return object;
	}
	return nullptr;
}

static const char* getString(Buffer& b) {
	const char* p = (const char*) b.head_ptr();
	auto len = strlen(p);
	char* result = new char[len + 1];
	memcpy(result, p, len+1);
	b.skip((u32) len+1);
	return result;
}

// note: leaks memory used for strings, but is okay since this shouldn't be used in release anyways
void ObjectList::readEXE(const char* file) {
	FSPath exepath(file);
	Buffer exe = exepath.read();

	struct ObjectLoader {
		u32 debugName;
		u32 fnInit;
		u32 fnZero;
		u32 fnInstance;
		u32 fnDebug;
		u32 unknown1, unknown2;
		u8 typeID;
		u8 listID;
		u16 unknown3;
		u32 unknown4;
		u32 miscFormat;
		u32 miscPropertyNames;
	};

	u32 offset = 0x3cff90; // table of object loaders
	const u32 ADDR_OFFS = 0x400000;
	u32 loaderPtr;
	ObjectLoader loader;

	while (true) {
		exe.seek(offset);
		exe.read(&loaderPtr);
		if (!loaderPtr) break;

		exe.seek(loaderPtr - ADDR_OFFS);
		exe.read(&loader);

		ObjectTypeData* typedata = getObjectData(loader.listID, loader.typeID);
		if (!typedata) {
			db.emplace_back();
			typedata = &db.back();
			typedata->listID = loader.listID;
			typedata->typeID = loader.typeID;
		}

		exe.seek(loader.debugName - ADDR_OFFS);
		typedata->debugName = getString(exe);

		if (loader.miscFormat) {
			exe.seek(loader.miscFormat - ADDR_OFFS);
			typedata->miscFormat = getString(exe);
		} else {
			typedata->miscFormat = "";
		}

		if (loader.miscPropertyNames) {
			u32 propertyTableAddr = loader.miscPropertyNames - ADDR_OFFS;
			u32 len = strlen(typedata->miscFormat);
			char buffer[512];
			buffer[0] = 0;
			for (int i = 0; i < len; i++) {
				u32 addr;
				exe.seek(propertyTableAddr + i * 4);
				exe.read(&addr);
				exe.seek(addr - ADDR_OFFS);
				if (exe.tell() >= exe.size()) {
					log_error("INVALID MiscFormat on object [%02x][%02x] %s", typedata->listID, typedata->typeID, typedata->debugName);
					break;
				}
				strncat(buffer, getString(exe), 512);
				strncat(buffer, ";", 512);
			}
			char* c = new char[strlen(buffer)];
			typedata->miscProperties = c;
			memcpy(c, buffer, strlen(buffer) + 1);
		} else {
			typedata->miscProperties = "";
		}

		offset += 4;
	}
}