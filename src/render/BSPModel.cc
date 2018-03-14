#include "common.hh"
#include "BSPModel.hh"
#include <bigg.hpp>

bgfx::ProgramHandle bspProgram;
bgfx::UniformHandle uSamplerTexture;
bgfx::UniformHandle uMaterialColor;
bgfx::UniformHandle uMaterialBits;
static bool bspStaticValuesLoaded = false;

struct BSPVertex
{
	float x;
	float y;
	float z;
	uint32_t abgr;
	float u;
	float v;
	static void init()
	{
		ms_decl
				.begin()
				.add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float )
				.add( bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true )
				.add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float )
				.end();
	}
	static bgfx::VertexDecl ms_decl;
};
bgfx::VertexDecl BSPVertex::ms_decl;

void BSPModel::clear() {
	if (hasData) {
		for (auto& section : sections) {
			bgfx::destroy(section.vertices);
			for (auto& mesh : section.binMeshes) {
				bgfx::destroy(mesh.indices);
			}
			section.binMeshes.clear();
		}
		sections.clear();
		hasData = false;
	}
}

void BSPModel::parseName(const char* name) {
	char buffer[16];
	strncpy(buffer, name, 16);

	char* p = buffer;
	char* end = buffer + 16;
	while (*p && *p++ != '_');
	if (!*p) {
		log_warn("Invalid format for BSP name: %s (expected '_')", name);
		return;
	}

	while (*p && *p != '_') {
		renderBits |= matFlagFromChar(*p++);
	}
	// invert reflective bit; as reflective bit is actually indicated by lack of 'N' in material flags
	renderBits ^= BIT_REFLECTIVE;

	if (!*p) {
		log_warn("Invalid format for BSP name: %s (expected '_')", name);
		return;
	}

	char* idxStr = ++p;
	while (*p && *p != '.') p++;
	if (p != idxStr + 2) {
		log_warn("Invalid format for BSP name: %s (should end with 2 digit index)", name);
		return;
	}
	id = 10 * (idxStr[0] - '0') + (idxStr[1] - '0');
	log_info("flags(%02x) id(%d)", renderBits, id);
}

BSPModel::~BSPModel() {
	clear();
	delete matList;
	for (auto& section : sections) {
		bgfx::destroy(section.vertices);
		for (auto& binmesh : section.binMeshes) {
			bgfx::destroy(binmesh.indices);
		}
	}
}

void BSPModel::setFromSection(rw::AbstractSectionChunk* sectionChunk) {
	if (sectionChunk->isAtomic()) {
		auto atomicSection = (rw::AtomicSectionChunk*) sectionChunk;

		const auto vertexCount = atomicSection->vertexCount;
		const auto& vertexPositions = atomicSection->vertexPositions;
		const auto& vertexColors = atomicSection->vertexColors;
		const auto& vertexUVs = atomicSection->vertexUVs;

		sections.emplace_back();
		auto& section = sections.back();
		BSPVertex* meshVertices = new BSPVertex[vertexCount];

		for (int i = 0; i < vertexCount; i++) {
			const auto& vertex = vertexPositions[i];
			const auto& color = vertexColors[i];
			const auto& uv = vertexUVs[i];
			meshVertices[i] = {
					vertex.x, vertex.y, vertex.z,
					uint32_t((color.a << 24) | (color.b << 16) | (color.g << 8) | color.r),
					uv.u, uv.v};
		}

		section.vertices = bgfx::createVertexBuffer(
				bgfx::makeRef(meshVertices,
							  sizeof(BSPVertex) * vertexCount,
							  [](void* p, void* _) {delete[] (BSPVertex*) p;}),
				BSPVertex::ms_decl
		);

		const auto objectCount = atomicSection->binMeshPLG->objectCount;
		log_debug("objectCount: %d", objectCount);
		for (int bmIdx = 0; bmIdx < objectCount; bmIdx++) {
			const auto indexCount = atomicSection->binMeshPLG->objects[bmIdx].meshIndexCount;
			const auto& indices = atomicSection->binMeshPLG->objects[bmIdx].indices;
			uint16_t* meshTriStrip = new uint16_t[indexCount];

			for (int i = 0; i < indexCount; i++) {
				if (indices[i] > vertexCount) log_warn("invalid index %08x", indices[i]);
				meshTriStrip[i] = (uint16_t) indices[i];
			}

			section.binMeshes.emplace_back();

			section.binMeshes[bmIdx].indices = bgfx::createIndexBuffer(
					bgfx::makeRef(meshTriStrip,
								  sizeof(uint16_t) * indexCount,
								  [](void* p, void* _) {delete[] (uint16_t*) p;})
			);

			section.binMeshes[bmIdx].material = atomicSection->binMeshPLG->objects[bmIdx].material;
		}
	} else {
		auto planeSection = (rw::PlaneSectionChunk*) sectionChunk;

		setFromSection(planeSection->left);
		setFromSection(planeSection->right);
	}
}

void BSPModel::setFromWorldChunk(const char* name, const rw::WorldChunk& worldChunk, TexDictionary* txd) {
	clear();

	this->name = name;
	parseName(name);

	if (!bspStaticValuesLoaded) {
		bspProgram = bigg::loadProgram("shaders/glsl/vs_bspmesh.bin", "shaders/glsl/fs_bspmesh.bin");
		BSPVertex::init();
		uSamplerTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Int1);
		uMaterialColor = bgfx::createUniform("u_materialColor", bgfx::UniformType::Vec4);
		uMaterialBits = bgfx::createUniform("u_materialBits", bgfx::UniformType::Int1);
		bspStaticValuesLoaded = true;
	}

	setFromSection(worldChunk.rootSection);

	// set materials
	matList = new MaterialList(worldChunk.materialList, txd);

	hasData = true;
}

void BSPModel::draw(TXCAnimation* txc) {
	if (hasData) {
		for (auto& section : sections) {
			for (auto& mesh : section.binMeshes) {
				// set mesh
				bgfx::setVertexBuffer(0, section.vertices);
				bgfx::setIndexBuffer(mesh.indices);

				// set material & state
				matList->bind(mesh.material, txc, renderBits, true);

				// draw
				bgfx::submit(0, bspProgram);
			}
		}
	}
}

int BSPModel::getId() {
	return id;
}

const char* BSPModel::getName() {
	return name.c_str();
}
