#include "common.hh"
#include "BSPModel.hh"
#include <bigg.hpp>

namespace rc {
namespace render {

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
		switch (*p++) {
			case 'N': renderBits |= BIT_REFLECTIVE; break; // note: this gets inverted later
			case 'D': renderBits |= BIT_SKY; break;
			case 'O': renderBits |= BIT_OPAQUE; break;
			case 'S': renderBits |= BIT_SHADOW; break;
			case 'W': renderBits |= BIT_NO_CULL; break;
			case 'P': renderBits |= BIT_PUNCH_ALPHA; break;
			case 'A': renderBits |= BIT_FULL_ALPHA; break;
			case 'K': renderBits |= BIT_ADDITIVE; break;
			case 'F': renderBits |= BIT_UNKNOWN_F; break;
		}
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
				bgfx::makeRef(meshVertices, sizeof(BSPVertex) * vertexCount),
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
					bgfx::makeRef(meshTriStrip, sizeof(uint16_t) * indexCount)
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
	}

	setFromSection(worldChunk.rootSection);

	for (auto materialChunk : worldChunk.materialList->materials) {
		materials.emplace_back();
		auto& mat = materials.back();

		mat.color = materialChunk->color;
		if (materialChunk->isTextured) {
			mat.texture = txd->getTexture(materialChunk->texture->textureName.c_str());
		} else {
			log_info("using no texture");
			mat.texture = bgfx::TextureHandle();
		}
	}

	hasData = true;
}

void BSPModel::draw(TXCAnimation* txc) {
	if (hasData) {
		for (auto& section : sections) {
			for (auto& mesh : section.binMeshes) {
				// set mesh
				bgfx::setVertexBuffer(0, section.vertices);
				bgfx::setIndexBuffer(mesh.indices);

				// set material
				const auto mat = materials[mesh.material];
				//printf("%d\n", mat.texture.idx);
				bgfx::setTexture(0, uSamplerTexture, txc ? txc->getTexture(mat.texture) : mat.texture);
				float matColor[4];
				uint32_t color = mat.color; // todo: is this RGBA or BGRA?
				if (selected) color = 0xff8888ff;
				matColor[0] = (color & 0xff) / 255.f;
				matColor[1] = ((color >> 8) & 0xff) / 255.f;
				matColor[2] = ((color >> 16) & 0xff) / 255.f;
				matColor[3] = ((color >> 24) & 0xff) / 255.f;
				//if (color != 0xffffffff)
				//	log_info("Material with a color!!");
				bgfx::setUniform(uMaterialColor, &matColor);
				uint32_t matBits = 0;
				if (renderBits & BIT_PUNCH_ALPHA)
					matBits |= 1;
				bgfx::setUniform(uMaterialBits, &matBits);

				// set state
				uint64_t state = BGFX_STATE_RGB_WRITE;
				state |= BGFX_STATE_ALPHA_WRITE;

				state |= BGFX_STATE_DEPTH_WRITE;
				state |= BGFX_STATE_DEPTH_TEST_LESS;

				state |= BGFX_STATE_PT_TRISTRIP;

				if (renderBits & BIT_ADDITIVE) {
					state |= BGFX_STATE_BLEND_ADD;
				}
				if (renderBits & BIT_FULL_ALPHA) {
					state |= BGFX_STATE_BLEND_ALPHA | BGFX_STATE_BLEND_INDEPENDENT;
				}
				if (!(renderBits & BIT_NO_CULL)) {
					state |= BGFX_STATE_CULL_CW;
				}
				bgfx::setState(state);

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

}
}
