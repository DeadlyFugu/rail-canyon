#include "common.hh"
#include "stage.hh"

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

void Stage::readVisibility(rc::util::FSPath& blkFile) {
	visibilityManager.read(blkFile);
}

void Stage::draw(glm::vec3 camPos, TXCAnimation* txc) {
	for (auto& model : models) {
		if (visibilityManager.isVisible(model.getId(), camPos)) {
			model.draw(txc);
		}
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
}

void Stage::drawVisibilityUI(glm::vec3 camPos) {
	visibilityManager.drawUI(camPos);
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

static void swapEndianness(u32* value) {
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
}

void VisibilityManager::read(rc::util::FSPath& blkFile) {
	rc::util::FSFile f_(blkFile);
	rc::util::Buffer b_ = f_.toBuffer();
	rw::util::Buffer b(b_.base_ptr(), b_.size(), false);
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
	int blockIdx = 1;
	for (auto& block : blocks) {
		ImGui::Separator();

		ImGui::PushID(blockIdx);
		if (block.contains(camPos)) ImGui::TextColored(ImVec4(0.0f, 0.75f, 1.0f, 1.0f), "Visibility Block %d", ++blockIdx);
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
