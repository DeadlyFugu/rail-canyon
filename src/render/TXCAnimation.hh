// Represets a texture animation .txc file from Heroess

#pragma once

#include "common.hh"
#include "TexDictionary.hh"
#include <bigg.hpp>
#include <map>

class TXCAnimation {
private:
	float time;
	int listbox_sel = 0;
public:
	struct FrameData {
		u16 frameID;
		u16 textureID;
	};
	// stores a frame as a duration and texture ID, rather than a start frame and texture ID
	// easier for editing
	struct FrameDeltaData {
		u16 duration;
		u16 textureID;
		bool error;
	};
	struct AnimatedTexture {
		std::string name;
		std::string replaceTexture;
		u32 frameCount;
		std::vector<bgfx::TextureHandle> frames; // optimized layout for lookups
		std::vector<FrameDeltaData> frameDeltas; // used for editing

		// generator params
		int generate_begin_texture = 1;
		int generate_end_texture = 1;
		int generate_frame_duration = 4;
		bool generate_auto = false;
	};
	std::map<u32, int> textureLookup;
	std::vector<AnimatedTexture> animations;

	TXCAnimation(Buffer& data, TexDictionary* txd);
	void setTime(float time);
	bgfx::TextureHandle getTexture(bgfx::TextureHandle lookup);

	void drawUI(TexDictionary* txd);
private:
	bgfx::TextureHandle getTextureNumbered(TexDictionary* txd, AnimatedTexture& anim, int number);
	void recalcFrames(AnimatedTexture& animation, TexDictionary* txd);
	void recalcMapping(TexDictionary* txd);
	int getCurrentFrameOffset(AnimatedTexture& animation);
};
