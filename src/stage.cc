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
			log_warn("Invalid BSP file in ONE archive: %s", bspName);
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

	ImGui::Begin("Stage");
	ImGui::Checkbox("Force Show All", &visibilityManager.forceShowAll);
	if (ImGui::CollapsingHeader("Visibility List")) {
		for (auto& model : models) {
			if (visibilityManager.isVisible(model.getId(), camPos)) {
				ImGui::LabelText("visible", "%s", model.getName());
			}
		}
	}
	if (ImGui::CollapsingHeader("Chunks")) {
		for (auto& model : models) {
			ImGui::PushID(model.getName());
			ImGui::LabelText("chunk", "%s", model.getName());
			ImGui::Checkbox("selected", &model.selected);
			ImGui::LabelText("visible", "%s", visibilityManager.isVisible(model.getId(), camPos) ? "yes" : "no");
			ImGui::PopID();
		}
	}
	ImGui::End();
}

bool VisibilityManager::isVisible(int chunkId, glm::vec3 camPos) {
	if (forceShowAll) return true;

	i32 x = (i32) camPos.x;
	i32 y = (i32) camPos.y;
	i32 z = (i32) camPos.z;

	for (auto& block : blocks) {
		if (block.chunk == chunkId) {
			if (x >= block.low_x && y >= block.low_y && z >= block.low_z &&
					x < block.high_x && y < block.high_y && z < block.high_z) {
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
	rc::util::FSFile f(blkFile);

	for (int i = 0; i < 64; i++) {
		if (f.atEnd()) {
			log_warn("unexpected EoF in %s", blkFile.str.c_str());
			break;
		}
		VisibilityBlock block;
		f.read(&block);

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
	if (!f.atEnd()) {
		log_warn("excess data in %s", blkFile.str.c_str());
	}
}
