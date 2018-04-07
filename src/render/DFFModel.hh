// BGFX renderer for RenderWare DFF model
// Uses rwstreamlib to read the DFF

#pragma once
#include "common.hh"
#include "world.hh"
#include "bgfx/bgfx.h"
#include "render/TexDictionary.hh"
#include "render/TXCAnimation.hh"
#include "render/MaterialList.hh"

class DFFModel {
private:
	struct Atomic {
		struct SubMesh {
			int material;
			bgfx::IndexBufferHandle indices;
		};
		std::vector<SubMesh> subMeshes;
		bgfx::VertexBufferHandle vertices;
		MaterialList* matList;
		glm::mat4 transform;
	};

	std::vector<Atomic> atomics;
public:
	~DFFModel();

	void setFromClump(rw::ClumpChunk* clump, TexDictionary* txd);

	void draw(glm::vec3 pos, int renderBits, int pick_color = 0);
	void draw(const glm::mat4& transform, int renderBits, int pick_color = 0);
	static void draw_solid_box(glm::vec3 position, u32 color, int view);
};
