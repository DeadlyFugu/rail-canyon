#include "util/fspath.hh"
#include "TXCAnimation.hh"

TXCAnimation::TXCAnimation(Buffer& data, TexDictionary* txd) {
	u32 value;
	data.read(&value);
	while (value != 0xffffffff) {
		animations.emplace_back();
		auto& animation = animations.back();

		animation.frameCount = value;
		log_info("frameCount %d", value);
		animation.frames.resize(value);

		data.seek(data.tell() + 516);

		char buffer[32];
		data.read(buffer, 32);
		animation.replaceTexture = std::string(buffer);

		int replaceIdx = txd->getTexture(buffer).idx;
		textureLookup.insert(std::make_pair(replaceIdx, (int) (animations.size() - 1)));

		data.read(buffer, 32);
		animation.name = std::string(buffer);

		char* bufferEnd = buffer + strlen(buffer);
		*bufferEnd = 0;
		size_t n = buffer + 32 - bufferEnd;

		int previousFrameID = 0;
		u16 bonusFinalDuration = 0; // counteract odd 'offset by 1' animations
		int previousDuration = -1;
		auto& frameDatas = animation.frameDeltas;
		while (true) {
			// read frame
			FrameData frame;
			data.read(&frame);
			if (frame.frameID == 0xffff) break;

			if (frame.frameID >= animation.frameCount || frame.frameID < previousFrameID) {
				log_warn("invalid frame in txc for texture %s", animation.name.c_str());
				continue;
			}

			// add frame to optimized storage
			// todo: generate this afterwards from editor deltas
			snprintf(bufferEnd, n, ".%d", frame.textureID);
			animation.frames[frame.frameID] = txd->getTexture(buffer);

			// add duration to final frame for offset txcs
			if (frameDatas.size() == 0 && frame.frameID != 0) {
				bonusFinalDuration = frame.frameID;
			}

			// add frame delta for editing
			FrameDeltaData delta;
			delta.textureID = frame.textureID;
			delta.duration = 0; // edited by below section on next iteration
			delta.error = getTextureNumbered(txd, animation, delta.textureID).idx == 0; // test if texture in txd
			frameDatas.push_back(delta);

			// set previous delta's duration
			if (frameDatas.size() > 1) {
				u16 duration = (u16) (frame.frameID - previousFrameID);
				frameDatas[frameDatas.size() - 2].duration = duration;

				// calculate generator duration
				if (frame.frameID - previousFrameID == previousDuration)
					animation.generate_frame_duration = duration;
				previousDuration = duration;
			}

			// calculate generator end frame
			if (frame.textureID > animation.generate_end_texture)
				animation.generate_end_texture = frame.textureID;

			// set previousFrameID for next iteration
			previousFrameID = frame.frameID;
		}

		// set final duration
		if (frameDatas.size() > 0) {
			frameDatas[frameDatas.size() - 1].duration = (u16) (animation.frameCount - previousFrameID) + bonusFinalDuration;

			// calculate generate duration if not enough frames for normal method to work (needs at least 3)
			if (frameDatas.size() <= 2) {
				animation.generate_frame_duration = frameDatas[0].duration;
			}
		}

		uint16_t lastIndex = replaceIdx;
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
		int animFrame = getCurrentFrameOffset(animation);
		if (animFrame == -1) return {0};
		else return animation.frames[animFrame];
	}
}

static void hint(const char* desc) {
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

const char* getOutPath();
const char* getStageFilename();

void TXCAnimation::drawUI(TexDictionary* txd) {
	bool mappingModified = false;

	std::vector<const char*> items;
	for (auto& animation : animations) {
		items.push_back(animation.name.c_str());
	}
	ImGui::ListBox("Animation", &listbox_sel, &items[0], items.size());
	if (ImGui::Button("new")) {
		animations.emplace_back();
		auto& anim = animations.back();
		anim.name = "name";
		anim.replaceTexture = "name.1";
		anim.frameCount = 0;
		listbox_sel = (unsigned) animations.size() - 1;
		mappingModified = true;
	}
	hint("Adds a new animation");
	ImGui::SameLine();
	if (ImGui::Button("delete")) {
		animations.erase(animations.begin() + listbox_sel);
		if (listbox_sel >= animations.size()) listbox_sel--;
		mappingModified = true;
	}
	hint("Deletes the selected animation");
	ImGui::SameLine();
	if (ImGui::Button("save all")) {
		rc::util::FSPath outDVDRoot(getOutPath());
		char filename[32];
		snprintf(filename, 32, "%s.txc", getStageFilename());
		rc::util::FSPath outPath = outDVDRoot / filename;
		Buffer out(0);
		out.setStretchy(true);

		for (auto& animation : animations) {
			auto frameCountOffs = out.tell();
			out.write(0);

			u32 zeroes[129];
			for (int i = 0; i < 129; i++) zeroes[i] = 0;

			out.write(zeroes, 516);

			char buffer[32];
			for (int i = 0; i < 32; i++) buffer[i] = 0;
			strncpy(buffer, animation.replaceTexture.c_str(), 32);
			out.write(buffer, 32);

			for (int i = 0; i < 32; i++) buffer[i] = 0;
			strncpy(buffer, animation.name.c_str(), 32);
			out.write(buffer, 32);

			u16 offset = 0;
			for (auto& frame : animation.frameDeltas) {
				out.write(offset);
				out.write(frame.textureID);
				offset += frame.duration;
			}
			auto returnOffs = out.tell();
			out.seek(frameCountOffs);
			u32 frameCount = offset;
			out.write(frameCount);
			out.seek(returnOffs);

			out.write((u32) 0xffffffff);
		}
		out.write((u32) 0xffffffff);

		outPath.write(out);
	}
	hint("Write animations to file");
	ImGui::Separator();

	if (listbox_sel >= 0 && listbox_sel < animations.size()) {
		bool deltasModified = false;

		auto& animation = animations[listbox_sel];
		char name[32];
		strncpy(name, animation.name.c_str(), 32);
		char replaceTexture[32];
		strncpy(replaceTexture, animation.replaceTexture.c_str(), 32);

		// animation properties
		union BiggImTexture {
			ImTextureID ptr;
			struct {
				uint16_t flags;
				bgfx::TextureHandle handle;
			} s;
		};
		BiggImTexture img;
		img.s.flags = 0; // unused
		img.s.handle = getTexture(txd->getTexture(animation.replaceTexture.c_str()));
		ImGui::Image(img.ptr, ImVec2(64, 64));

		if (ImGui::InputText("name", name, 30)) {
			animation.name = std::string(name);
			snprintf(replaceTexture, 32, "%s.1", name);
			deltasModified = true;
		}
		hint("Name is used to find the texture of each frame\ni.e. the first frame is <name>.1, second frame <name>.2");
		if (ImGui::InputText("replaces", replaceTexture, 32)) {
			animation.replaceTexture = std::string(replaceTexture);
			mappingModified = true;
		}
		hint("This is the name of the texture in the stage that will be replaced with this animation");
		ImGui::Separator();

		// automatic frame generation
		bool generate_dirty = false;
		ImGui::Text("Automatic Frame Generation");
		ImGui::PushItemWidth(-150.0f);
		if (ImGui::DragInt("First Texture", &animation.generate_begin_texture, 0.1f, 1, 99)) generate_dirty = true;
		hint("Number of first texture in the animation (usually 1)");
		if (ImGui::DragInt("Last Texture", &animation.generate_end_texture, 0.1f, 1, 99)) generate_dirty = true;
		hint("Number of last texture in the animation (e.g. 20 if you have 20 frames)");
		if (ImGui::DragInt("Frame Duration", &animation.generate_frame_duration, 0.1f, 1, 99)) generate_dirty = true;
		hint("How many in-game frames to display a frame of the animation for\n'In-game frames' are measured at 60fps");
		ImGui::PopItemWidth();
		if (ImGui::Button("Generate") || animation.generate_auto) {
			auto& frameDeltas = animation.frameDeltas;
			frameDeltas.clear();
			int i = animation.generate_begin_texture;
			do {
				FrameDeltaData delta;
				delta.duration = (u16) animation.generate_frame_duration;
				delta.textureID = (u16) i;
				delta.error = getTextureNumbered(txd, animation, delta.textureID).idx == 0; // test if texture in txd
				frameDeltas.push_back(delta);
			} while ((animation.generate_begin_texture < animation.generate_end_texture ? i++ : i--) != animation.generate_end_texture);
			deltasModified = true;
		}
		hint("Pressing this button will wipe all frames below and automatically generate new ones based on the above parameters");
		ImGui::SameLine();
		ImGui::Checkbox("generate automatically", &animation.generate_auto);
		hint("If this is enabled, frames below will automatically be wiped and regenerated each time a change is detected to the parameters");
		ImGui::Separator();

		// manual frame editing
		ImGui::Text("Manual Frame Editing");
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
		int animFrame = getCurrentFrameOffset(animation);
		int currentOffs = 0;
		for (auto& frame : frameDeltas) {
			int duration = frame.duration;
			int textureID = frame.textureID;
			ImGui::PushID(i);
			bool isFrameActive = animFrame >= currentOffs && animFrame < currentOffs + duration;
			currentOffs += duration;
			bool isFrameError = frame.error;
			if (isFrameActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.75f, 1.0f, 1.0f));
			if (isFrameError) {
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.12f, 0.0f, 0.75f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.24f, 0.0f, 0.87f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 0.24f, 0.0f, 0.87f));
			}
			if (ImGui::DragInt("##texture", &textureID, 0.1f, 0, 99)) {
				frame.textureID = (u16) textureID;
				deltasModified = true;
			}
			if (isFrameError) ImGui::PopStyleColor(3);
			if (isFrameActive) ImGui::PopStyleColor(1);
			if (isFrameError)
				hint("Index of texture to use for this frame (number after the dot)\n"
				"That texture could not be found in the TXD");
			else
				hint("Index of texture to use for this frame (number after the dot)");
			if (isFrameActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.75f, 1.0f, 1.0f));
			ImGui::NextColumn();
			if (ImGui::DragInt("##duration", &duration, 0.1f, 0, animation.frameCount)) {
				frame.duration = (u16) duration;
				deltasModified = true;
			}
			hint("How many frames to show this texture for (measured in frames at 60fps)");
			if (isFrameActive) ImGui::PopStyleColor(1);
			ImGui::NextColumn();
			if (ImGui::Button("...")) {
				ImGui::OpenPopup("Actions");
			}
			if (ImGui::BeginPopup("Actions")) {
				if (ImGui::MenuItem("Insert Before")) {
					action = 1;
					action_self = i;
				}
				hint("Insert a new entry before this one");
				if (ImGui::MenuItem("Insert After")) {
					action = 1;
					action_self = i + 1;
				}
				hint("Insert a new entry after this one");
				ImGui::Separator();
				if (ImGui::MenuItem("Move Up", nullptr, false, i > 0)) {
					action = 2;
					action_self = i;
					action_other = i - 1;
				}
				hint("Move this entry up one place");
				if (ImGui::MenuItem("Move Down", nullptr, false, i < frameDeltas.size() - 1)) {
					action = 2;
					action_self = i;
					action_other = i + 1;
				}
				hint("Move this entry down one place");
				ImGui::Separator();
				if (ImGui::MenuItem("Delete")) {
					action = 3;
					action_self = i;
				}
				hint("Delete this entry");
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
				delta.error = getTextureNumbered(txd, animation, delta.textureID).idx == 0; // test if texture in txd
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
	}

	if (mappingModified) {
		recalcMapping(txd);
	}
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
		// check errors
		delta.error = getTextureNumbered(txd, animation, delta.textureID).idx == 0; // test if texture in txd
	}
	// shrink if necessary
	if (i != frames.size()) {
		frames.resize(i);
	}

	animation.frameCount = i;
}

void TXCAnimation::recalcMapping(TexDictionary* txd) {
	textureLookup.clear();

	int i = 0;
	for (auto& animation : animations) {
		int replaceIdx = txd->getTexture(animation.replaceTexture.c_str()).idx;
		textureLookup.insert(std::make_pair(replaceIdx, i));
		i++;
	}
}

int TXCAnimation::getCurrentFrameOffset(TXCAnimation::AnimatedTexture& animation) {
	auto totalFrames = animation.frameCount;
	auto duration = totalFrames / FPS;
	auto animTime = fmodf(time, duration);
	auto animFrame = (int) (animTime * FPS);

	if (animFrame < 0) animFrame = 0;
	if (animFrame >= totalFrames) animFrame = totalFrames - 1;

	if (totalFrames == 0) return -1;
	return animFrame;
}
