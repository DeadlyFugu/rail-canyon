// BGFX renderer for RenderWare BSP model
// Uses rwstreamlib to read the BSP

#pragma once
#include "common.hh"
#include "world.hh"
#include "bgfx/bgfx.h"
#include "render/TexDictionary.hh"
#include "render/TXCAnimation.hh"
#include "render/MaterialList.hh"

class VisibilityManager;

class BSPModel {
private:
	MaterialList* matList;

	struct BinMesh {
		bgfx::IndexBufferHandle indices;
		int material; // todo: make shared_ptr?
	};

	struct Section {
		bgfx::VertexBufferHandle vertices;
		std::vector<BinMesh> binMeshes;
	};

	std::vector<Section> sections;

	bool hasData = false;
	int renderBits = 0;
	int id;
	std::string name;
	void clear();
	void parseName(const char* name);
	void setFromSection(rw::AbstractSectionChunk* sectionChunk);
public:
	bool selected = false;
	~BSPModel();

	void setFromWorldChunk(const char* name, const rw::WorldChunk& worldChunk, TexDictionary* txd);

	void draw(TXCAnimation* txc);
	int getId();
	const char* getName();
};
