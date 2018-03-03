#include "TXCAnimation.hh"

TXCAnimation::TXCAnimation(Buffer& data, TexDictionary* txd) {
	u32 value;
	data.read(&value);
	while (value != 0xffffffff) {
		animations.emplace_back();
		auto& animation = animations.back();

		animation.frameCount = value;
		animation.frames.resize(value);

		data.seek(data.tell() + 516);

		char buffer[32];
		data.read(buffer, 32);
		animation.firstFrame = std::string(buffer);

		int firstTextureIdx = txd->getTexture(buffer).idx;
		textureLookup.insert(std::make_pair(firstTextureIdx, (int) (animations.size() - 1)));

		data.read(buffer, 32);
		animation.name = std::string(buffer);

		char* bufferEnd = buffer + strlen(buffer);
		*bufferEnd = 0;
		size_t n = buffer + 32 - bufferEnd;

		int previousFrameID = 0;
		auto& frameDatas = animation.frameDeltas;
		while (true) {
			// read frame
			FrameData frame;
			data.read(&frame);
			if (frame.frameID == 0xffff) break;

			// add frame to optimized storage
			// todo: generate this afterwards from editor deltas
			if (frame.frameID < animation.frameCount) {
				snprintf(bufferEnd, n, ".%d", frame.textureID);
				animation.frames[frame.frameID] = txd->getTexture(buffer);
			} else {
				log_warn("invalid frame in txc for texture %s", animation.name.c_str());
			}

			// add extra frame at start for txcs missing it
			if (frameDatas.size() == 0 && frame.frameID != 0) {
				FrameDeltaData delta;
				delta.textureID = 1;
				delta.duration = 0;
				frameDatas.push_back(delta);
			}

			// add frame delta for editing
			FrameDeltaData delta;
			delta.textureID = frame.textureID;
			delta.duration = 0; // edited by below section on next iteration
			frameDatas.push_back(delta);

			// set previous delta's duration
			if (frameDatas.size() > 1) {
				frameDatas[frameDatas.size() - 2].duration = (u16) (frame.frameID - previousFrameID);
			}
			previousFrameID = frame.frameID;
		}

		if (frameDatas.size() > 0) {
			frameDatas[frameDatas.size() - 1].duration = (u16) (animation.frameCount - previousFrameID);
		}

		uint16_t lastIndex = firstTextureIdx;
		for (int i = 0; i < animation.frameCount; i++) {
			if (animation.frames[i].idx == 0) {
				animation.frames[i].idx = lastIndex;
			}
			lastIndex = animation.frames[i].idx;
		}

		data.read(&value);
	}
}

void TXCAnimation::setTime(float time) {
	this->time = time;
}

const float FPS = 60.0f;

bgfx::TextureHandle TXCAnimation::getTexture(bgfx::TextureHandle lookup) {
	auto result = textureLookup.find(lookup.idx);

	if (result == textureLookup.end()) {
		return lookup;
	} else {
		auto& animation = animations[result->second];
		auto totalFrames = animation.frameCount;
		auto duration = totalFrames / FPS;
		auto animTime = fmodf(time, duration);
		auto animFrame = (int) (animTime * FPS);

		if (animFrame < 0) animFrame = 0;
		if (animFrame >= totalFrames) animFrame = totalFrames - 1;

		return animation.frames[animFrame];
	}
}

void TXCAnimation::showUI(TexDictionary* txd) {
	ImGui::Begin("TXC Animation");

	std::vector<const char*> items;
	for (auto& animation : animations) {
		items.push_back(animation.name.c_str());
	}
	ImGui::ListBox("Animation", &listbox_sel, &items[0], items.size());
	ImGui::Button("new");
	ImGui::SameLine();
	ImGui::Button("delete");
	ImGui::Separator();

	auto& animation = animations[listbox_sel];
	char name[32];
	strncpy(name, animation.name.c_str(), 32);
	char frame1[32];
	strncpy(frame1, animation.firstFrame.c_str(), 32);

	// animation properties
	union BiggImTexture { ImTextureID ptr; struct { uint16_t flags; bgfx::TextureHandle handle; } s; };
	BiggImTexture img;
	img.s.flags = 0; // unused
	img.s.handle = getTexture(animation.frames[0]);
	ImGui::Image(img.ptr, ImVec2(64, 64));
	if (ImGui::InputText("name", name, 30)) {
		snprintf(frame1, 32, "%s.1", name);
	}
	ImGui::InputText("frame1", frame1, 32);
	ImGui::Separator();

	bool deltasModified = false;

	// automatic frame generation
	// todo: below static values should be fields and auto determined on file load
	static int generate_begin_frame = 1;
	static int generate_end_frame = 1;
	static int generate_frame_duration = 2;
	static bool generate_auto = false;
	bool generate_dirty = false;
	if (ImGui::DragInt("First Texture", &generate_begin_frame, 0.1f, 1, 99)) generate_dirty = true;
	if (ImGui::DragInt("Last Texture", &generate_end_frame, 0.1f, 1, 99)) generate_dirty = true;
	if (ImGui::DragInt("Frame Duration", &generate_frame_duration, 0.1f, 1, 99)) generate_dirty = true;
	if (ImGui::Button("Generate") || generate_auto) {
		auto& frameDeltas = animation.frameDeltas;
		frameDeltas.clear();
		int i = generate_begin_frame;
		do {
			FrameDeltaData delta;
			delta.duration = (u16) generate_frame_duration;
			delta.textureID = (u16) i;
			frameDeltas.push_back(delta);
		} while ((generate_begin_frame < generate_end_frame ? i++ : i--) != generate_end_frame);
		deltasModified = true;
	}
	ImGui::SameLine();
	ImGui::Checkbox("generate automatically", &generate_auto);
	ImGui::Separator();

	// manual frame editing
	ImGui::Columns(3);
	ImGui::TextUnformatted("Texture");
	ImGui::NextColumn();
	ImGui::TextUnformatted("Duration");
	ImGui::NextColumn();
	ImGui::TextUnformatted("Actions");
	ImGui::NextColumn();
	unsigned i = 0;
	unsigned action = 0; // 1: insert after, 2: swap, 3: delete
	unsigned action_self = 0; // index of action
	unsigned action_other = 0; // index of other (for swap only)
	auto& frameDeltas = animation.frameDeltas;
	for (auto& frame : frameDeltas) {
		int duration = frame.duration;
		int textureID = frame.textureID;
		ImGui::PushID(i);
		if (ImGui::DragInt("##texture", &textureID, 0.1f, 0, 99)) {
			frame.textureID = (u16) textureID;
			deltasModified = true;
		}
		ImGui::NextColumn();
		if (ImGui::DragInt("##duration", &duration, 0.1f, 0, animation.frameCount)) {
			frame.duration = (u16) duration;
			deltasModified = true;
		}
		ImGui::NextColumn();
		if (ImGui::Button("...")) {
			ImGui::OpenPopup("Actions");
		}
		if (ImGui::BeginPopup("Actions")) {
			if (ImGui::MenuItem("Insert Before")) {
				action = 1;
				action_self = i;
			}
			if (ImGui::MenuItem("Insert After")) {
				action = 1;
				action_self = i + 1;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Move Up", nullptr, false, i > 0)) {
				action = 2;
				action_self = i;
				action_other = i - 1;
			}
			if (ImGui::MenuItem("Move Down", nullptr, false, i < frameDeltas.size() - 1)) {
				action = 2;
				action_self = i;
				action_other = i + 1;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Delete")) {
				action = 3;
				action_self = i;
			}
			ImGui::EndPopup();
		}
		ImGui::NextColumn();
		ImGui::PopID();
		i++;
	}
	ImGui::Columns(1);

	if (action) {
		log_debug("action");
		if (action == 1) { // insert
			log_debug("insert");
			FrameDeltaData delta;
			delta.duration = 1;
			delta.textureID = 1;
			frameDeltas.insert(frameDeltas.begin() + action_self, delta);
		} else if (action == 2) { // swap
			log_debug("swap");
			std::iter_swap(frameDeltas.begin() + action_self, frameDeltas.begin() + action_other);
		} else if (action == 3) { // delete
			log_debug("delete");
			frameDeltas.erase(frameDeltas.begin() + action_self);
		}
		deltasModified = true;
	}

	if (deltasModified) {
		recalcFrames(animation, txd);
	}

	ImGui::End();
}

bgfx::TextureHandle TXCAnimation::getTextureNumbered(TexDictionary* txd, TXCAnimation::AnimatedTexture& anim, int number) {
	char buffer[32];
	strncpy(buffer, anim.name.c_str(), 32);
	char* bufferEnd = buffer + strlen(buffer);
	*bufferEnd = 0;
	size_t n = buffer + 32 - bufferEnd;
	snprintf(bufferEnd, n, ".%d", number);
	return txd->getTexture(buffer);
}

void TXCAnimation::recalcFrames(TXCAnimation::AnimatedTexture& animation, TexDictionary* txd) {
	auto& frameDeltas = animation.frameDeltas;
	auto& frames = animation.frames;

	u32 i = 0;
	for (auto& delta : frameDeltas) {
		// expand to fit if necessary
		if (i + delta.duration > frames.size()) {
			frames.resize((u32) (i + delta.duration));
		}
		// set frame textures
		for (int j = 0; j < delta.duration; j++) {
			frames[i++] = getTextureNumbered(txd, animation, delta.textureID);
		}
	}
	// shrink if necessary
	if (i != frames.size()) {
		frames.resize(i);
	}

	animation.frameCount = i;
}
