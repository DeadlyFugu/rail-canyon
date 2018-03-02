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
		std::string name(buffer);

		int idx = txd->getTexture(buffer).idx;
		textureLookup.insert(std::make_pair(idx, (int) (animations.size() - 1)));

		data.read(buffer, 32);

		char* bufferEnd = buffer + strlen(buffer);
		*bufferEnd = 0;
		size_t n = buffer + 32 - bufferEnd;

		while (true) {
			union {
				struct {
					u16 frame;
					u16 textureID;
				} part;
				u32 whole;
			} frame;
			data.read(&frame);
			if (frame.whole == 0xffffffff) break;

			if (frame.part.frame < animation.frameCount) {
				snprintf(bufferEnd, n, ".%d", frame.part.textureID);
				animation.frames[frame.part.frame] = txd->getTexture(buffer);
			} else {
				log_warn("invalid frame in txc for texture %s", name.c_str());
			}
		}

		// hacky fix for weirdly formatted txcs?
		if (animation.frames[0].idx == 0) animation.frames[0].idx = animation.frames[1].idx;

		uint16_t lastIndex = 0;
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
