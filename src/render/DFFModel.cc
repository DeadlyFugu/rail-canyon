#include "common.hh"
#include "DFFModel.hh"
#include <bigg.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

static bgfx::ProgramHandle dffProgram;
static bool dffStaticValuesLoaded = false;


struct DFFVertex
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
bgfx::VertexDecl DFFVertex::ms_decl;

DFFModel::~DFFModel() {
	for (auto& atomic : atomics) {
		delete atomic.matList;
		bgfx::destroy(atomic.vertices);
		for (auto& submesh : atomic.subMeshes) {
			bgfx::destroy(submesh.indices);
		}
	}
}

static void static_initialize() {
	if (!dffStaticValuesLoaded) {
		DFFVertex::init();
		dffProgram = bigg::loadProgram( "shaders/glsl/vs_dffmesh.bin", "shaders/glsl/fs_dffmesh.bin" );
		dffStaticValuesLoaded = true;
	}
}

static void permute_vertex(rw::ClumpChunk* clump, int dmtarget, int vtxid, rw::geom::VertexPosition& vpos) {
	int target_idx = 0;
	for (auto geom : clump->geometryList->geometries) {
		for (auto ext : geom->extensions) {
			for (auto& chunk : ((rw::ListChunk*) ext)->children) {
				if (chunk->type != RW_DELTA_MORPH_PLG) continue;

				for (auto& target : ((rw::DeltaMorphPLGChunk*) chunk)->targets) {
					target_idx++;
					if (dmtarget == target_idx) {
						int real_id = 0;
						int cmpr_id = 0;
						int found = -1;
						for (auto& mpctrl : target.mapping) {
							if (mpctrl & 0x80) {
								int count = mpctrl & 0x7f;
								if (vtxid < real_id + count) {
									found = vtxid - real_id + cmpr_id;
									break;
								}
								real_id += count;
								cmpr_id += count;
							} else {
								real_id += mpctrl;
								if (vtxid < real_id) return;
							}
						}
						auto& delta = target.points[found];
						vpos.x += delta.x;
						vpos.y += delta.y;
						vpos.z += delta.z;
						return;
					}
				}
			}
		}
	}
}

void DFFModel::setFromClump(rw::ClumpChunk* clump, TexDictionary* txd, int dmtarget) {
	static_initialize();

	for (auto atomicChunk : clump->atomics) {
		auto geometry = clump->geometryList->geometries[atomicChunk->geometryIndex];
		atomics.emplace_back();
		auto& atomic = atomics.back();

		auto target0 = geometry->morphTargets[0];

		atomic.subMeshes.resize(geometry->materialList->materials.size());
		for (int i = 0; i < atomic.subMeshes.size(); i++) {
			atomic.subMeshes[i].material = i;
		}

		DFFVertex* meshVertices = new DFFVertex[geometry->vertexCount];

		for (int i = 0; i < geometry->vertexCount; i++) {
			rw::geom::VertexPosition vertex = target0.vertexPositions[i];

			rw::geom::VertexColor color;
			if (geometry->vertexColors.size()) color = geometry->vertexColors[i];
			else color.as_int = 0xffffffff;

			rw::geom::VertexUVs uvs;
			if (geometry->vertexUVLayers.size()) {
				uvs = geometry->vertexUVLayers[0][i];
			} else {
				uvs.u = 0; uvs.v = 0;
			}

			if (dmtarget) permute_vertex(clump, dmtarget, i, vertex);

			meshVertices[i] = {
					vertex.x, vertex.y, vertex.z,
					color.as_int,
					uvs.u, uvs.v
			};
		}

		atomic.vertices = bgfx::createVertexBuffer(
				bgfx::makeRef(meshVertices,
							  sizeof(DFFVertex) * geometry->vertexCount,
							  [](void* p, void* _) {delete[] (DFFVertex*) p;}),
				DFFVertex::ms_decl
		);

		struct FaceIndices {
			u16 vertex1;
			u16 vertex2;
			u16 vertex3;
		};
		std::vector<std::vector<FaceIndices>> subMeshFaces;
		subMeshFaces.resize(atomic.subMeshes.size());

		for (int i = 0; i < geometry->triangleCount; i++) {
			const auto& face = geometry->faces[i];
			FaceIndices indices {
					face.vertex1, face.vertex2, face.vertex3
			};
			subMeshFaces[face.material].push_back(indices);
		}

		for (int i = 0; i < atomic.subMeshes.size(); i++) {
			auto& subMesh = atomic.subMeshes[i];
			auto size = sizeof(uint16_t) * subMeshFaces[i].size() * 3;
			auto mem = bgfx::alloc(size);
			memcpy(mem->data, &subMeshFaces[i][0], size);
			subMesh.indices = bgfx::createIndexBuffer(mem);
		}

		atomic.matList = new MaterialList(geometry->materialList, txd);

		auto& frame = clump->frameList->frames[atomicChunk->frameIndex];
		atomic.transform = glm::translate(atomic.transform, glm::vec3(frame.translation.x, frame.translation.y, frame.translation.z));
		atomic.transform[0][0] = frame.rotation.row1.x;
		atomic.transform[0][1] = frame.rotation.row1.y;
		atomic.transform[0][2] = frame.rotation.row1.z;
		atomic.transform[1][0] = frame.rotation.row2.x;
		atomic.transform[1][1] = frame.rotation.row2.y;
		atomic.transform[1][2] = frame.rotation.row2.z;
		atomic.transform[2][0] = frame.rotation.row3.x;
		atomic.transform[2][1] = frame.rotation.row3.y;
		atomic.transform[2][2] = frame.rotation.row3.z;
	}
}

void DFFModel::draw(glm::vec3 pos, int renderBits, int pick_color) {
	glm::mat4 transform;
	transform = glm::translate(transform, pos);
	draw(transform, renderBits, pick_color);
}

void DFFModel::draw(const glm::mat4& render_transform, int renderBits, int pick_color) {
	for (auto& atomic : atomics) {
		for (auto& submesh : atomic.subMeshes) {
			glm::mat4 transform = render_transform * atomic.transform;
			bgfx::setTransform(&transform[0][0]);
			bgfx::setVertexBuffer(0, atomic.vertices);
			bgfx::setIndexBuffer(submesh.indices);

			if (pick_color) {
				atomic.matList->bind_color(pick_color, false);
			} else {
				atomic.matList->bind(submesh.material, nullptr, renderBits, false);
			}

			bgfx::submit(pick_color ? u8(1) : u8(0), dffProgram);
		}
	}
}

void DFFModel::draw_solid_box(glm::vec3 position, u32 color, int view) {
	static bool hasData = false;
	static bgfx::VertexBufferHandle box_vbo;
	static bgfx::IndexBufferHandle box_ibo;
	if (!hasData) {
		static_initialize();
		hasData = true;

		static DFFVertex cube_verts[] = {
				{-5.f, -5.f, -5.f, 0xffffffff, 0.f, 0.f},
				{ 5.f, -5.f, -5.f, 0xffffffff, 0.f, 0.f},
				{-5.f,  5.f, -5.f, 0xffffffff, 0.f, 0.f},
				{ 5.f,  5.f, -5.f, 0xffffffff, 0.f, 0.f},
				{-5.f, -5.f,  5.f, 0xffffffff, 0.f, 0.f},
				{ 5.f, -5.f,  5.f, 0xffffffff, 0.f, 0.f},
				{-5.f,  5.f,  5.f, 0xffffffff, 0.f, 0.f},
				{ 5.f,  5.f,  5.f, 0xffffffff, 0.f, 0.f},
		};

		static u16 cube_indices[] = {
				0, 1, 2, 1, 2, 3,
				4, 5, 6, 5, 6, 7,
				0, 1, 4, 1, 4, 5,
				2, 3, 6, 3, 6, 7,
				0, 2, 4, 2, 4, 6,
				1, 3, 5, 3, 5, 7
		};

		box_vbo = bgfx::createVertexBuffer(bgfx::makeRef(cube_verts, sizeof(cube_verts)), DFFVertex::ms_decl);
		box_ibo = bgfx::createIndexBuffer(bgfx::makeRef(cube_indices, sizeof(cube_indices)));
	}

	glm::mat4 transform;
	transform = glm::translate(transform, position);
	bgfx::setTransform(&transform[0][0]);
	bgfx::setVertexBuffer(0, box_vbo);
	bgfx::setIndexBuffer(box_ibo);

	MaterialList::bind_color(color, true);

	bgfx::submit((u8) view, dffProgram);
}