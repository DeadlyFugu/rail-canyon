// Stores materials used by a single model

#pragma once
#include "common.hh"
#include <bigg.hpp>
#include "render/TexDictionary.hh"
#include "render/TXCAnimation.hh"
#include "material.hh"

const int BIT_REFLECTIVE = 1 << 0; // if N is not present
const int BIT_SKY = 1 << 1; // if D is present
const int BIT_OPAQUE = 1 << 2; // if O is present
const int BIT_SHADOW = 1 << 3; // if S is present
const int BIT_NO_CULL = 1 << 4; // if W is present
const int BIT_PUNCH_ALPHA = 1 << 5; // if P is present
const int BIT_FULL_ALPHA = 1 << 6; // if A is present
const int BIT_ADDITIVE = 1 << 7; // if K is present
const int BIT_UNKNOWN_F = 1 << 8; // if F is present

// NOTE: always xor with BIT_REFLECTIVE after using this to parse a full string to flags
// todo: maybe just use BIT_NON_REFLECTIVE instead to avoid awkwardness?
int matFlagFromChar(char c);

class MaterialList {
private:
	struct Material {
		uint32_t color;
		bgfx::TextureHandle texture;
	};
	std::vector<Material> materials;
public:
	MaterialList(rw::MaterialListChunk* matList, TexDictionary* txd);
	void bind(int id, TXCAnimation* txc, int renderBits, bool triList);
	void bind_color(u32 color, int triList);
};
