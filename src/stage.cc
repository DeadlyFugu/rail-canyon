#include "common.hh"
#include "stage.hh"
#include <../extern/bigg/deps/bgfx.cmake/bgfx/examples/common/debugdraw/debugdraw.h>

Stage::~Stage() {
	models.clear();
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

	if (layout) layout->draw(camPos);
}

void Stage::drawUI(glm::vec3 camPos) {
	for (auto& model : models) {
		ImGui::PushID(model.getName());
		ImGui::LabelText("chunk", "%s", model.getName());
		ImGui::Checkbox("selected", &model.selected);
		ImGui::LabelText("visible", "%s", visibilityManager.isVisible(model.getId(), camPos) ? "yes" : "no");
		ImGui::PopID();
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
	if (layout) layout->drawUI(camPos);
	else ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No stage is loaded");
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

void ObjectLayout::read(FSPath& binFile) {
	Buffer b = binFile.read();
	b.seek(0);

	for (int i = 0; i < 2048; i++) {
		struct {
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
		} instance;

		b.read(&instance);

		if (instance.type) {
			objects.emplace_back();
			auto& obj = objects.back();

			obj.pos_x = swapEndianness(&instance.x);
			obj.pos_y = swapEndianness(&instance.y);
			obj.pos_z = swapEndianness(&instance.z);
			obj.rot_x = swapEndianness(&instance.rx);
			obj.rot_y = swapEndianness(&instance.ry);
			obj.rot_z = swapEndianness(&instance.rz);
			obj.type = swapEndianness(&instance.type);
			obj.linkID = instance.linkID;
			obj.radius = instance.radius;

			auto returnOffs = b.tell();
			b.seek(0x18000u + 0x24 * instance.miscID + 0x04);
			if (b.tell() < b.size())
				b.read(&obj.misc);
			b.seek(returnOffs);
		}
	}
}

void ObjectLayout::draw(glm::vec3 camPos) {
	const Aabb box = {
			{-5, -5, -5},
			{5, 5, 5},
	};
	for (auto& object : objects) {
		ddPush();
		ddSetTranslate(object.pos_x, object.pos_y, object.pos_z);
		ddSetState(true, true, false);
		ddDraw(box);
		ddPop();
	}
}

void ObjectLayout::drawUI(glm::vec3 camPos) {
	static int obSel = 0;
	ImGui::Text("Layout contains %d instances", objects.size());
	ImGui::DragInt("instance", &obSel, 1.0f, 0, objects.size() - 1);

	if (obSel >= 0 && obSel < objects.size()) {
		auto& obj = objects[obSel];
		ImGui::Text("Type: [%02x][%02x]", obj.type >> 8, obj.type & 0xff);
		ImGui::DragFloat3("Position", &obj.pos_x);
	}
}
