#include "common.hh"
#include "stage.hh"
#include <../extern/bigg/deps/bgfx.cmake/bgfx/examples/common/debugdraw/debugdraw.h>
#include "render/Camera.hh"

Stage::~Stage() {
	models.clear();

	if (layout) delete layout;
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
	}
}

void Stage::readVisibility(FSPath& blkFile) {
	visibilityManager.read(blkFile);
}

void Stage::draw(glm::vec3 camPos, TXCAnimation* txc) {
	for (auto& model : models) {
		if (visibilityManager.isVisible(model.getId(), camPos)) {
			model.draw(txc);
		}
	}

	if (layout) layout->draw(camPos, cache, objdb);
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

void Stage::readLayout(FSPath& binFile) {
	layout = new ObjectLayout();
	layout->read(binFile);
}

void Stage::readCache(FSPath& oneFile, TexDictionary* txd) {
	if (!cache) cache = new DFFCache();
	cache->addFromArchive(oneFile, txd);
}

void Stage::drawLayoutUI(glm::vec3 camPos) {
	if (layout) layout->drawUI(camPos, objdb);
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

		log_info("chunk(%d)", block.chunk);
		log_info("low(%d, %d, %d)", block.low_x, block.low_y, block.low_z);
		log_info("high(%d, %d, %d)", block.high_x, block.high_y, block.high_z);

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

			cache[std::string(fileName)] = dff;
		}
	}
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

	for (int i = 0; i < 2048; i++) {
		InstanceData instance;

		b.read(&instance);

		if (instance.type) {
			objects.emplace_back();
			auto& obj = objects.back();

			obj.pos_x = swapEndianness(&instance.x);
			obj.pos_y = swapEndianness(&instance.y);
			obj.pos_z = swapEndianness(&instance.z);
			obj.rot_x = swapEndianness(&instance.rx) * 0.0054931641f;
			obj.rot_y = swapEndianness(&instance.ry) * 0.0054931641f;
			obj.rot_z = swapEndianness(&instance.rz) * 0.0054931641f;
			obj.type = swapEndianness(&instance.type);
			obj.linkID = instance.linkID;
			obj.radius = instance.radius;
			swapEndianness(&instance.miscID);

			auto returnOffs = b.tell();
			b.seek(0x18000u + 0x24 * instance.miscID + 0x04);
			if (b.tell() < b.size())
				b.read(obj.misc, 32);
			else
				log_warn("Invalid miscID for object %d", i);
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

void ObjectLayout::draw(glm::vec3 camPos, DFFCache* cache, ObjectList* objdb) {
	const Aabb box = {
			{-5, -5, -5},
			{5, 5, 5},
	};
	for (auto& object : objects) {
		if (object.type == 0x0180) {
			const char* flowerDFFNames[] = {
					"S01_PN_HANA1_1.DFF",
					"S01_PN_HANA1_2.DFF",
					"S01_PN_HANA1_3.DFF",
					"S01_PN_HANA2.DFF",
					"S01_PN_HANA3_1.DFF",
					"S01_PN_HANA3_2.DFF",
					"S01_PN_HANA4_1.DFF",
					"S01_PN_HANA4_2.DFF",
					"S01_PN_HANA5.DFF",
			};
			auto model = cache->getDFF(flowerDFFNames[clamp(object.misc[0], 0, 8)]);
			if (model) model->draw(glm::vec3(object.pos_x, object.pos_y, object.pos_z), BIT_PUNCH_ALPHA);
		} else if (object.type == 0x0181) {
			auto model = cache->getDFF("S01_ON_HATA1_1.DFF");
			model->draw(glm::vec3(object.pos_x, object.pos_y, object.pos_z), 0);
		} else if (object.type == 0x0182) {
			auto model = cache->getDFF("S01_ON_SYATI.DFF");
			model->draw(glm::vec3(object.pos_x, object.pos_y, object.pos_z), 0);
		} else {
			ddPush();
			ddSetTranslate(object.pos_x, object.pos_y, object.pos_z);
			ddSetState(true, true, false);
			ddDraw(box);
			ddPop();
		}
	}
}

static char* propNameCurrent(const char* p) {
	static char buffer[32];
	int i = 0;
	while (i < 32 && p[i] != ';') {
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

void ObjectLayout::drawUI(glm::vec3 camPos, ObjectList* objdb) {
	static int obSel = 0;
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

		ImGui::DragInt("Type", &obj.type, 1.0f, 0, 0xffff);
		ImGui::DragFloat3("Position", &obj.pos_x);
		ImGui::DragFloat3("Rotation", &obj.rot_x);
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
						}
						ptr += sizeof(u16);
					}
						break;
					case 'i':
					case 'I': {
						ptr = align(ptr, 4);
						u32 value = *(u32*) ptr;
						swapEndianness(&value);
						if (ImGui::DragInt(propName, (i32*) &value, 1.0f)) {
							swapEndianness(&value);
							*(u32*) ptr = (u32) value;
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
