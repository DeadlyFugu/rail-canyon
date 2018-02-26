// BGFX renderer for RenderWare BSP model
// Uses rwstreamlib to read the BSP

#pragma once
#include "common.hh"
#include "world.hh"
#include "bgfx/bgfx.h"
#include "render/TexDictionary.hh"

class VisibilityManager;

namespace rc {
namespace render {

const int BIT_REFLECTIVE = 1 << 0; // if N is not present
const int BIT_SKY = 1 << 1; // if D is present
const int BIT_OPAQUE = 1 << 2; // if O is present
const int BIT_SHADOW = 1 << 3; // if S is present
const int BIT_NO_CULL = 1 << 4; // if W is present
const int BIT_PUNCH_ALPHA = 1 << 5; // if P is present
const int BIT_FULL_ALPHA = 1 << 6; // if A is present
const int BIT_ADDITIVE = 1 << 7; // if K is present
const int BIT_UNKNOWN_F = 1 << 8; // if F is present

class BSPModel {
private:
	struct Material {
		uint32_t color;
		bgfx::TextureHandle texture;
	};
	std::vector<Material> materials;

	struct BinMesh {
		bgfx::IndexBufferHandle indices;
		int material; // todo: make shared_ptr?
	};

	bgfx::VertexBufferHandle vertices;
	std::vector<BinMesh> binMeshes;
	bool hasData = false;
	int renderBits;
	int id;
	void clear();
	void parseName(const char* name);
public:
	~BSPModel();

	void setFromWorldChunk(const char* name, const rw::WorldChunk& worldChunk, TexDictionary* txd);

	void draw();
	int getId();
};

}
}
