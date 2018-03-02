// Represets a texture animation .txc file from Heroes

#pragma once

#include "common.hh"
#include "TexDictionary.hh"
#include <bigg.hpp>
#include <map>

class TXCAnimation {
private:
	float time;
public:
	struct AnimatedTexture {
		u32 frameCount;
		std::vector<bgfx::TextureHandle> frames;
	};
	std::map<u32, int> textureLookup;
	std::vector<AnimatedTexture> animations;

	TXCAnimation(Buffer& data, TexDictionary* txd);
	void setTime(float time);
	bgfx::TextureHandle getTexture(bgfx::TextureHandle lookup);
};
