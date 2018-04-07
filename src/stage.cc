#include "common.hh"
#include "stage.hh"
#include <../extern/bigg/deps/bgfx.cmake/bgfx/examples/common/debugdraw/debugdraw.h>
#include "render/Camera.hh"

Stage::~Stage() {
	models.clear();

	if (layout_db) delete layout_db;
	if (layout_pb) delete layout_pb;
	if (layout_p1) delete layout_p1;
	if (cache) delete cache;
	delete objdb;
}

void Stage::fromArchive(ONEArchive* archive, TexDictionary* txd) { // todo: unique_ptr?
	int count = archive->getFileCount();
	for (int i = 0; i < count; i++) {
		models.emplace_back();

		const auto bspName = archive->getFileName(i);
		log_info("opening file %s", bspName);
		auto x = archive->readFile(i);

		sk::Buffer sk_x(x.base_ptr(), x.size(), false); // convert buffer utility classes
		rw::Chunk* root = rw::readChunk(sk_x);

		if (root && root->type == RW_WORLD)
			models.back().setFromWorldChunk(bspName, *((rw::WorldChunk*) root), txd);
		else
			logger.warn("Invalid BSP file in ONE archive: %s", bspName);

		delete root;
	}
}

void Stage::readVisibility(FSPath& blkFile) {
	visibilityManager.read(blkFile);
}

void Stage::draw(glm::vec3 camPos, TXCAnimation* txc, bool picking) {
	for (auto& model : models) {
		if (visibilityManager.isVisible(model.getId(), camPos)) {
			model.draw(txc);
		}
	}

	if (layout_db) layout_db->draw(camPos, cache, objdb, 0);
	if (layout_pb) layout_pb->draw(camPos, cache, objdb, 0);
	if (layout_p1) layout_p1->draw(camPos, cache, objdb, 0);

	if (picking) {
		if (layout_db) layout_db->draw(camPos, cache, objdb, 1);
		if (layout_pb) layout_pb->draw(camPos, cache, objdb, 2);
		if (layout_p1) layout_p1->draw(camPos, cache, objdb, 3);
	}
}

void Stage::drawUI(glm::vec3 camPos) {
	for (auto& model : models) {
		ImGui::PushID(model.getName());
		ImGui::LabelText("chunk", "%s", model.getName());
		ImGui::Checkbox("selected", &model.selected);
		ImGui::LabelText("visible", "%s", visibilityManager.isVisible(model.getId(), camPos) ? "yes" : "no");
		ImGui::PopID();
	}

	static char exeName[512] = "/Users/matt/Shared/Tsonic_win.exe";
	ImGui::Text("Secret zone: (don't touch)");
	ImGui::InputText("exe name", exeName, 512);
	if (ImGui::Button("read exe")) {
		objdb->readEXE(exeName);
		objdb->writeFile("ObjectList2.ini");
	}
}

void Stage::drawVisibilityUI(glm::vec3 camPos) {
	visibilityManager.drawUI(camPos);
}

void Stage::drawDebug(glm::vec3 camPos) {
	visibilityManager.drawDebug(camPos);
}

void Stage::readLayout(const char* dvdroot, const char* stgname) {
	char buffer[512];

	sprintf(buffer, "%s/%s_DB.bin", dvdroot, stgname);
	FSPath path_db(buffer);
	layout_db = new ObjectLayout();
	layout_db->read(path_db);

	sprintf(buffer, "%s/%s_PB.bin", dvdroot, stgname);
	FSPath path_pb(buffer);
	layout_pb = new ObjectLayout();
	layout_pb->read(path_pb);

	sprintf(buffer, "%s/%s_P1.bin", dvdroot, stgname);
	FSPath path_p1(buffer);
	layout_p1 = new ObjectLayout();
	layout_p1->read(path_p1);
}

void Stage::readCache(FSPath& oneFile, TexDictionary* txd) {
	if (!cache) cache = new DFFCache();
	cache->addFromArchive(oneFile, txd);
}

static int listSel = 1;

void Stage::drawLayoutUI(glm::vec3 camPos) {
	ImGui::Combo("Layout", &listSel, "None\0DB\0PB\0P1\0");
	if (listSel == 1) {
		if (layout_db) layout_db->drawUI(camPos, objdb);
	} else if (listSel == 2) {
		if (layout_pb) layout_pb->drawUI(camPos, objdb);
	} else if (listSel == 3) {
		if (layout_p1) layout_p1->drawUI(camPos, objdb);
	}

	// todo: choose layout
	else ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No stage is loaded");
}

Stage::Stage() {
	objdb = new ObjectList();
	objdb->readFile("ObjectList.ini");
}

bool VisibilityManager::isVisible(int chunkId, glm::vec3 camPos) {
	if (forceShowAll) return true;
	if (!chunkId) return true;
	if (!fileExists) return true;

	for (auto& block : blocks) {
		if (block.chunk == chunkId) {
			if (block.contains(camPos)) {
				return true;
			}
		}
	}
	return false;
}

static u32 swapEndianness(u32* value) {
	union IntBytes {
		u32 as_int;
		u8 bytes[4];
	};
	IntBytes tmp = *(IntBytes*)value;
	IntBytes* out = (IntBytes*)value;
	out->bytes[0] = tmp.bytes[3];
	out->bytes[1] = tmp.bytes[2];
	out->bytes[2] = tmp.bytes[1];
	out->bytes[3] = tmp.bytes[0];

	return *value;
}

static u16 swapEndianness(u16* value) {
	u16 tmp = (u8) *value;
	*value = *value >> 8;
	*value |= tmp << 8;

	return *value;
}

static float swapEndianness(float* value) {
	swapEndianness((u32*) value);

	return *value;
}

void VisibilityManager::read(FSPath& blkFile) {
	Buffer b = blkFile.read();
	fileExists = true;

	for (int i = 0; i < 64; i++) {
		if (b.remaining() < sizeof(VisibilityBlock)) {
			logger.warn("unexpected EoF in %s", blkFile.str.c_str());
			break;
		}
		VisibilityBlock block;
		b.read(&block);

		swapEndianness((u32*) &block.chunk);
		swapEndianness((u32*) &block.low_x);
		swapEndianness((u32*) &block.low_y);
		swapEndianness((u32*) &block.low_z);
		swapEndianness((u32*) &block.high_x);
		swapEndianness((u32*) &block.high_y);
		swapEndianness((u32*) &block.high_z);

		blocks.push_back(block);
	}
	if (b.remaining()) {
		logger.warn("excess data in %s", blkFile.str.c_str());
	}
}

void VisibilityManager::drawUI(glm::vec3 camPos) {
	ImGui::Checkbox("Force Show All", &forceShowAll);
	ImGui::Checkbox("Show Chunk Borders", &showChunkBorders);
	if (showChunkBorders) ImGui::Checkbox("Pad Chunk Borders", &usePadding);
	int blockIdx = 1;
	for (auto& block : blocks) {
		ImGui::Separator();

		ImGui::PushID(blockIdx);
		if (block.contains(camPos)) ImGui::TextColored(ImVec4(0.0f, 0.75f, 1.0f, 1.0f), "Visibility Block %d", blockIdx);
		else ImGui::Text("Visibility Block %d", blockIdx);
		if (block.chunk == -1) {
			if (ImGui::Button("Create")) block.chunk = 1;
		} else {
			ImGui::InputInt("Chunk", &block.chunk, 1, 1);
			ImGui::DragInt3("AABB Low", &block.low_x);
			ImGui::DragInt3("AABB High", &block.high_x);
		}
		ImGui::PopID();
		blockIdx++;
	}
}

void VisibilityManager::drawDebug(glm::vec3 camPos) {
	if (showChunkBorders) {
		for (auto& block : blocks) {
			ddPush();
			ddSetWireframe(true);
			float borderOffs = 0;
			if (block.contains(camPos)) {
				ddSetColor(0xffff8800);
				ddSetState(false, false, true);
			} else {
				ddSetColor(0x88888888);
				ddSetStipple(true, 0.001f);
				if (usePadding) borderOffs = 100.f;
			}
			Aabb box = {
					{block.low_x + borderOffs, block.low_y + borderOffs,  block.low_z + borderOffs},
					{block.high_x - borderOffs, block.high_y - borderOffs, block.high_z - borderOffs}
			};
			ddDraw(box);
			ddPop();
		}
	}
}

DFFCache::~DFFCache() {
	for (auto& entry : cache) {
		delete entry.second;
	}
}

void DFFCache::addFromArchive(FSPath& onePath, TexDictionary* txd) {
	ONEArchive* one = new ONEArchive(onePath);

	const auto fileCount = one->getFileCount();
	for (int i = 0; i < fileCount; i++) {
		const auto fileName = one->getFileName(i);
		const auto fileNameLen = strlen(fileName);

		if (fileName[fileNameLen-4] == '.' && fileName[fileNameLen-3] == 'D' && fileName[fileNameLen-2] == 'F' && fileName[fileNameLen-1] == 'F') {
			DFFModel* dff = new DFFModel();
			Buffer b = one->readFile(i);
			rw::ClumpChunk* clump = (rw::ClumpChunk*) rw::readChunk(b);
			dff->setFromClump(clump, txd);
			delete clump;

			cache[std::string(fileName)] = dff;
		}
	}

	delete one;
}

DFFModel* DFFCache::getDFF(const char* name) {
	auto result = cache.find(std::string(name));
	if (result == cache.end()) return nullptr;
	else return result->second;
}

struct InstanceData {
	float x, y, z;
	u32 rx, ry, rz;
	u16 unused_1;
	u8 team;
	u8 must_be_odd;
	u32 unused_2;
	u64 unused_repeat;
	u16 type;
	u8 linkID;
	u8 radius;
	u16 unused_3;
	u16 miscID;
};

void ObjectLayout::read(FSPath& binFile) {
	Buffer b = binFile.read();
	b.seek(0);

	if (b.size() == 0) return;

	for (int i = 0; i < 2048; i++) {
		// check can read
		if (b.remaining() < sizeof(InstanceData)) {
			rw::util::logger.error("Invalid object layout file (unexpected EOF reading format)");
			break;
		}

		// read data
		InstanceData instance;
		b.read(&instance);

		if (instance.type) {
			objects.emplace_back();
			auto& obj = objects.back();

			obj.pos_x = swapEndianness(&instance.x);
			obj.pos_y = swapEndianness(&instance.y);
			obj.pos_z = swapEndianness(&instance.z);
			obj.rot_x = i32(swapEndianness(&instance.rx)) * 0.0054931641f;
			obj.rot_y = i32(swapEndianness(&instance.ry)) * 0.0054931641f;
			obj.rot_z = i32(swapEndianness(&instance.rz)) * 0.0054931641f;
			obj.type = swapEndianness(&instance.type);
			obj.linkID = instance.linkID;
			obj.radius = instance.radius;
			swapEndianness(&instance.miscID);

			auto returnOffs = b.tell();
			b.seek(0x18000u + 0x24 * instance.miscID + 0x04);
			if (((int) b.remaining()) >= 32) // todo: Buffer::remaining() should return signed (or clamp min to zero)
				b.read(obj.misc, 32);
			else
				rw::util::logger.warn("Invalid miscID for object %d", i);
			b.seek(returnOffs);
		}
	}
}

void ObjectLayout::write(FSPath& binFile) {
	// create buffer to hold contents
	Buffer b(2048 * sizeof(InstanceData) + this->objects.size() * 36, true);

	// write layout
	int miscID = 0;
	for (auto& object : this->objects) {
		// set struct values
		InstanceData data;
		data.x = object.pos_x;
		data.y = object.pos_y;
		data.z = object.pos_z;
		data.rx = (u32) (object.rot_x * 182.046567512);
		data.ry = (u32) (object.rot_y * 182.046567512);
		data.rz = (u32) (object.rot_z * 182.046567512);
		data.unused_1 = 0;
		data.team = 0;
		data.must_be_odd = 9; // this should probably be editable, seems to have some meaning
		data.unused_2 = 0;
		data.unused_repeat = 0;
		data.type = (u16) object.type;
		data.linkID = object.linkID;
		data.radius = object.radius;
		data.unused_3 = 0;
		data.miscID = miscID++;

		// write to file
		b.write(data);
	}

	// write misc
	for (auto& object : this->objects) {
		// unused header
		u32 value = 0x00000001;
		b.write(value);

		// content
		b.write(object.misc, 32);
	}

	// write to file
	binFile.write(b);
}

static int clamp(int x, int low, int high) {
	if (x < low) return low;
	if (x > high) return high;
	return x;
}

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

static bool lua_was_init = false;
static lua_State* L;
static std::unordered_map<int, int> object_draw_fns;

struct DrawCall {
	glm::mat4 transform;
	int renderBits;
	std::string modelName;
};
static std::vector<DrawCall> draw_calls;
static DrawCall current_dc;
static ObjectLayout::ObjectInstance* instance_data;
static ObjectList::ObjectTypeData* object_data;
static std::vector<glm::mat4> transformStack;

static int rclua_draw(lua_State* L) {
	const char* c = lua_tostring(L, -1);
	current_dc.modelName = c ? c : "";
	draw_calls.push_back(current_dc);
	return 0;
}

static glm::vec3 _get_vec3(lua_State* L) {
	glm::vec3 out;
	lua_pushstring(L, "x");
	lua_gettable(L, -2);
	out.x = (float) lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "y");
	lua_gettable(L, -2);
	out.y = (float) lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "z");
	lua_gettable(L, -2);
	out.z = (float) lua_tonumber(L, -1);
	lua_pop(L, 1);
	return out;
}

static void _put_vec3(lua_State* L, glm::vec3 v) {
	lua_newtable(L);
	lua_pushstring(L, "x");
	lua_pushnumber(L, v.x);
	lua_settable(L, -3);
	lua_pushstring(L, "y");
	lua_pushnumber(L, v.y);
	lua_settable(L, -3);
	lua_pushstring(L, "z");
	lua_pushnumber(L, v.z);
	lua_settable(L, -3);
}

static int rclua_translate(lua_State* L) {
	float fx = (float) lua_tonumber(L, -3);
	float fy = (float) lua_tonumber(L, -2);
	float fz = (float) lua_tonumber(L, -1);
	current_dc.transform = glm::translate(current_dc.transform, glm::vec3(fx, fy, fz));
	return 0;
}

static int rclua_rotate(lua_State* L) {
	const char* format = lua_tostring(L, 1);
	for (int i = 0; i < strlen(format); i++) {
		glm::vec3 axis;
		int axis_id = 0;
		switch (format[i]) {
			case 'x':
			case 'X': {
				axis = glm::vec3(1, 0, 0);
				axis_id = 0;
			} break;
			case 'y':
			case 'Y': {
				axis = glm::vec3(0, 1, 0);
				axis_id = 1;
			} break;
			case 'z':
			case 'Z': {
				axis = glm::vec3(0, 0, 1);
				axis_id = 2;
			} break;
			default: luaL_error(L, "Invalid rotation axis %c", format[i]);
		}
		float f;
		if (lua_isnumber(L, 2)) {
			f = (float) lua_tonumber(L, i + 2); // +1 for zero indexing, +1 again to skip format arg
		} else {
			glm::vec3 rot = _get_vec3(L);
			f = axis_id ? (axis_id == 1 ? rot.y : rot.z) : rot.x;
		}
		current_dc.transform = glm::rotate(current_dc.transform, glm::radians(f), axis);
	}
	return 0;
}

static int rclua_scale(lua_State* L) {
	int argc = lua_gettop(L);
	if (argc == 1) {
		if (lua_isnumber(L, -1)) {
			float factor = (float) lua_tonumber(L, -1);
			current_dc.transform = glm::scale(current_dc.transform, glm::vec3(factor, factor, factor));
		} else {
			// todo: handle vec3 type
		}
	} else if (argc == 3) {
		float fx = (float) lua_tonumber(L, -3);
		float fy = (float) lua_tonumber(L, -2);
		float fz = (float) lua_tonumber(L, -1);
		current_dc.transform = glm::scale(current_dc.transform, glm::vec3(fx, fy, fz));
	} else {
		luaL_error(L, "Invalid number of arguments to scale() (got %d)", argc);
	}
	return 0;
}

static int rclua_push(lua_State* L) {
	transformStack.push_back(current_dc.transform);
	return 0;
}

static int rclua_pop(lua_State* L) {
	current_dc.transform = transformStack.back();
	transformStack.pop_back();
	return 0;
}

static int rclua_material(lua_State* L) {
	if (lua_isnumber(L, -1)) {
		current_dc.renderBits = (int) lua_tonumber(L, -1);
	} else {
		const char* flags = lua_tostring(L, -1);
		current_dc.renderBits = 0;
		if (flags) for (const char* p = flags; *p; p++) {
			current_dc.renderBits |= matFlagFromChar(*p);
		}
		current_dc.renderBits ^= BIT_REFLECTIVE;
	}
	return 0;
}

static int align(int i, int alignment) {
	while (i % alignment) i++;
	return i;
}

static int rclua_property(lua_State* L) {
	int property = 0;
	// get property index
	if (lua_isnumber(L, -1)) {
		property = (int) lua_tonumber(L, -1);
		if (property < 1 || property > strlen(object_data->miscProperties)) {
			luaL_error(L, "Invalid property index %d", property);
		}
		property--; // make 0 indexed
	} else {
		const char* prop_name = lua_tostring(L, -1);
		int max_prop = (int) strlen(object_data->miscFormat);
		const char* p = object_data->miscProperties;
		property = -1;
		for (int i = 0; i < max_prop; i++) {
			int len = 0;
			while (p[len] != ';') len++;
			if (strlen(prop_name) == len && !strncmp(p, prop_name, len)) {
				property = i;
				break;
			}
			p += len+1;
		}
		if (property == -1) {
			luaL_error(L, "No such property as %s found", prop_name);
		}
	}

	// find offset into misc
	int misc_offs = 0;
	for (int i = 0; i < property; i++) {
		switch (object_data->miscFormat[i]) {
			case 'c':
			case 'C': {
				misc_offs += 1;
			} break;
			case 's':
			case 'S': {
				misc_offs = align(misc_offs, 2);
				misc_offs += 2;
			} break;
			case 'x':
			case 'X':
			case 'i':
			case 'I': {
				misc_offs = align(misc_offs, 4);
				misc_offs += 4;
			} break;
			case 'f':
			case 'F': {
				misc_offs = align(misc_offs, 4);
				misc_offs += 4;
			} break;
			default: {
				log_error("Invalid format option %c", object_data->miscFormat[i]);
				misc_offs += 4;
			}
		}
	}

	switch (object_data->miscFormat[property]) {
		case 'c':
		case 'C': {
			u8 value = instance_data->misc[misc_offs];
			lua_pushnumber(L, value);
		} break;
		case 's':
		case 'S': {
			misc_offs = align(misc_offs, 2);
			u16 value = *((u16*) &instance_data->misc[misc_offs]);
			swapEndianness(&value);
			lua_pushnumber(L, value);
		} break;
		case 'x':
		case 'X':
		case 'i':
		case 'I': {
			misc_offs = align(misc_offs, 4);
			u32 value = *((u32*) &instance_data->misc[misc_offs]);
			swapEndianness(&value);
			lua_pushnumber(L, value);
		} break;
		case 'f':
		case 'F': {
			misc_offs = align(misc_offs, 4);
			float value = *((float*) (instance_data->misc+misc_offs));
			swapEndianness(&value);
			lua_pushnumber(L, value);
		} break;
		default: {
			log_error("Invalid format option %c", object_data->miscFormat[property]);
			lua_pushnil(L);
		}
	}

	return 1;
}

static int rclua_rotation(lua_State* L) {
	_put_vec3(L, glm::vec3(instance_data->rot_x, instance_data->rot_y, instance_data->rot_z));
	return 1;
}

static void register_fn(lua_State* L, int (*fn)(lua_State*), const char* name) {
	lua_pushcfunction(L, fn);
	lua_setglobal(L, name);
}

void ObjectLayout::buildObjectCache(ObjectInstance& object, DFFCache* cache, ObjectList* objdb, int id) {
	// lazy initialize lua state
	if (!lua_was_init) {
		// setup lua interpreter
		L = luaL_newstate();
		luaL_openlibs(L);

		// setup rclua functions
		register_fn(L, rclua_draw, "draw");
		register_fn(L, rclua_translate, "translate");
		register_fn(L, rclua_rotate, "rotate");
		register_fn(L, rclua_scale, "scale");
		register_fn(L, rclua_push, "push");
		register_fn(L, rclua_pop, "pop");
		register_fn(L, rclua_material, "material");
		register_fn(L, rclua_property, "property");
		register_fn(L, rclua_rotation, "rotation");

		// mark lua as loaded
		lua_was_init = true;
	}

	// find object data from ObjectList.ini
	auto objdata = objdb->getObjectData(u8(object.type >> 8), u8(object.type & 0xff));

	// get reference to chunk
	auto mapIterator = object_draw_fns.find(object.type);
	int draw_fn;
	if (mapIterator == object_draw_fns.end()) {
		// check if object is known
		if (objdata) {
			// determine chunk name
			char buffer[32];
			snprintf(buffer, 32, "%s(%d)::Draw", objdata->debugName, id);

			// get chunk source
			const char* src;
			src = objdata->blockDraw;

			if (src) {
				// compile and get reference
				luaL_loadbuffer(L, src, strlen(src), buffer);
				draw_fn = luaL_ref(L, LUA_REGISTRYINDEX);
			} else {
				// indicate that this object has no source
				draw_fn = LUA_NOREF;
			}
		} else {
			// indicate object unknown
			draw_fn = LUA_NOREF;
		}

		// store cached value
		object_draw_fns[object.type] = draw_fn;
	} else {
		// get value from object_draw_fns cache
		draw_fn = mapIterator->second;
	}

	// set object as debug cube if lacking draw function
	if (draw_fn == LUA_NOREF) {
		object.fallback_render = true;
		return;
	}

	// setup globals
	current_dc.transform = mat4();
	current_dc.renderBits = 0;
	current_dc.modelName = "";
	instance_data = &object;
	object_data = objdata;
	draw_calls.clear();

	// execute chunk
	lua_rawgeti(L, LUA_REGISTRYINDEX, draw_fn);
	int result = lua_pcall(L, 0, 0, 0);

	// check for errors
	if (result) {
		auto err = lua_tostring(L, -1);
		log_error("INTERNAL LUA ERROR: %s", err);
		lua_pop(L, 1);
		draw_calls.clear();
		object.fallback_render = true;
	} else {
		log_info("LUA OK");
	}

	// set object cache
	for (auto& draw_call : draw_calls) {
		object.cache.emplace_back();
		auto& cacheModel = object.cache.back();
		cacheModel.transform = draw_call.transform;
		cacheModel.renderBits = draw_call.renderBits;
		cacheModel.model = cache->getDFF(draw_call.modelName.c_str());
	}
}

void ObjectLayout::draw(glm::vec3 camPos, DFFCache* cache, ObjectList* objdb, int picking) {
	const Aabb box = {
			{-5, -5, -5},
			{5, 5, 5},
	};
	int id = 0;
	for (auto& object : objects) {
		vec3 delta = camPos - vec3(object.pos_x,object.pos_y,object.pos_z);
		if (delta.x*delta.x + delta.y*delta.y + delta.z*delta.z > object.radius*object.radius*10000) {
			id++; continue;
		}
		u32 pick_color = (u8(picking) << 16) | (u16(id));
		if (object.fallback_render) {
			// debug cube render for objects without any defined render
			if (picking) {
				DFFModel::draw_solid_box(vec3(object.pos_x, object.pos_y, object.pos_z), pick_color | 0xff000000, 1);
			} else {
				ddPush();
				ddSetTranslate(object.pos_x, object.pos_y, object.pos_z);
				ddSetState(true, true, false);
				ddDraw(box);
				ddPop();
			}
		} else {
			if (object.cache_invalid) {
				object.cache.clear();
				buildObjectCache(object, cache, objdb, id);
				object.cache_invalid = false;
			}
			for (auto& cached : object.cache) {
				mat4 transform = cached.transform;
				mat4 model_transform = glm::translate(glm::mat4(), glm::vec3(object.pos_x, object.pos_y, object.pos_z));
				if (cached.model) {
					cached.model->draw(model_transform * transform, cached.renderBits, picking ? pick_color : 0);
				} else {
					if (picking) {
						DFFModel::draw_solid_box(vec3(object.pos_x, object.pos_y, object.pos_z), pick_color | 0xff000000, 1);
					} else {
						ddPush();
						ddSetTransform(&transform);
						ddSetState(true, true, false);
						ddSetColor(0xff8888ff);
						ddDraw(box);
						ddPop();
					}
				}
			}
		}
		id++;
	}
}

static char* propNameCurrent(const char* p) {
	static char buffer[32];
	int i = 0;
	while (i < 32 && p[i] && p[i] != ';') {
		buffer[i] = p[i];
		i++;
	}
	buffer[i] = 0;
	return buffer;
}

static const char* propNameAdvance(const char* p) {
	const char* result = p;
	while (*result && *result != ';') result++;
	if (*result) result++;
	return result;
}

static u8* align(u8* ptr, int alignment) {
	uptr ptr_int = (uptr) ptr;
	while (ptr_int % alignment) ptr_int++;
	return (u8*) ptr_int;
}

Camera* getCamera();
const char* getOutPath();
const char* getStageFilename();
static int obSel = 0;

void setSelectedObject(int list, int ob) {
	listSel = list;
	obSel = ob;
}

void ObjectLayout::drawUI(glm::vec3 camPos, ObjectList* objdb) {
	ImGui::Text("Layout contains %d instances", objects.size());
	ImGui::DragInt("instance", &obSel, 1.0f, 0, objects.size() - 1);
	if (ImGui::Button("Save")) {
		FSPath outDVDRoot(getOutPath());
		char filename[32];
		snprintf(filename, 32, "%s_DB.bin", getStageFilename());
		FSPath outPath = outDVDRoot / filename;
		write(outPath);
	}

	if (obSel >= 0 && obSel < objects.size()) {
		auto& obj = objects[obSel];
		ImGui::Separator();
		auto objdata = objdb->getObjectData(obj.type >> 8, obj.type);
		ImGui::Text("Type: [%02x][%02x] (%s)", obj.type >> 8, obj.type & 0xff, objdata ? objdata->debugName : "<unknown>");

		if (ImGui::DragInt("Type", &obj.type, 1.0f, 0, 0xffff)) obj.cache_invalid = true;
		if (ImGui::DragFloat3("Position", &obj.pos_x)) obj.cache_invalid = true;
		if (ImGui::DragFloat3("Rotation", &obj.rot_x)) obj.cache_invalid = true;
		ImGui::InputInt("LinkID", &obj.linkID);
		ImGui::DragInt("Radius", &obj.radius);

		if (ImGui::Button("View")) {
			auto camera = getCamera();
			auto objpos = glm::vec3(obj.pos_x, obj.pos_y, obj.pos_z);
			camera->setPosition(objpos + glm::vec3(200.f, 70.f, -30.f));
			camera->lookAt(objpos);
		}

		ImGui::Separator();
		ImGui::Text("Misc");
		ImGui::TextDisabled("format %s", objdata && objdata->miscFormat ? objdata->miscFormat : "unknown");

		if (objdata && objdata->miscFormat && objdata->miscProperties) {
			int propertyCount = strlen(objdata->miscFormat);
			u8* ptr = obj.misc;
			const char* propNamePtr = objdata->miscProperties;

			for (int i = 0; i < propertyCount; i++) {
				const char* propName = propNameCurrent(propNamePtr);
				ImGui::PushID(i);
				switch (objdata->miscFormat[i]) {
					case 'c':
					case 'C': {
						int value = *ptr;
						if (ImGui::DragInt(propName, &value, 1.0f, 0x00, 0xff)) {
							*ptr = (u8) value;
							obj.cache_invalid = true;
						}
						ptr += sizeof(u8);
					}
						break;
					case 's':
					case 'S': {
						ptr = align(ptr, 2);
						u16 value16 = *(u16*) ptr;
						swapEndianness(&value16);
						int value = value16;
						if (ImGui::DragInt(propName, &value, 1.0f, 0x0000, 0xffff)) {
							value16 = (u16) value;
							swapEndianness(&value16);
							*(u16*) ptr = value16;
							obj.cache_invalid = true;
						}
						ptr += sizeof(u16);
					}
						break;
					case 'x':
					case 'X':
					case 'i':
					case 'I': {
						ptr = align(ptr, 4);
						u32 value = *(u32*) ptr;
						swapEndianness(&value);
						if (ImGui::DragInt(propName, (i32*) &value, 1.0f)) {
							swapEndianness(&value);
							*(u32*) ptr = (u32) value;
							obj.cache_invalid = true;
						}
						ptr += sizeof(u32);
					}
						break;
					case 'f':
					case 'F': {
						ptr = align(ptr, 4);
						float value = *(float*) ptr;
						swapEndianness(&value);
						if (ImGui::DragFloat(propName, &value, 1.0f)) {
							swapEndianness(&value);
							*(float*) ptr = value;
							obj.cache_invalid = true;
						}
						ptr += sizeof(float);
					}
						break;
					default: {
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "ERROR: Unknown format %c, please report",
										   objdata->miscFormat[i]);
					}
				}
				ImGui::PopID();

				propNamePtr = propNameAdvance(propNamePtr);
			}
		} else {
			ImGui::TextDisabled("Cannot display property editor");
		}

		for (int i = 0; i < 8; i++) {
			ImGui::Text("%02x%02x%02x%02x", obj.misc[i*4+0], obj.misc[i*4+1], obj.misc[i*4+2], obj.misc[i*4+3]);
		}
	}
}
