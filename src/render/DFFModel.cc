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

}

void DFFModel::setFromClump(rw::ClumpChunk* clump, TexDictionary* txd) {
	if (!dffStaticValuesLoaded) {
		DFFVertex::init();
		dffProgram = bigg::loadProgram( "shaders/glsl/vs_dffmesh.bin", "shaders/glsl/fs_dffmesh.bin" );
		dffStaticValuesLoaded = true;
	}

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
			const auto& vertex = target0.vertexPositions[i];

			rw::geom::VertexColor color;
			if (geometry->vertexColors.size()) color = geometry->vertexColors[i];
			else color.as_int = 0xffffffff;

			rw::geom::VertexUVs uvs;
			if (geometry->vertexUVLayers.size()) uvs = geometry->vertexUVLayers[0][i];
			else {
				uvs.u = 0; uvs.v = 0;
			}

			meshVertices[i] = {
					vertex.x, vertex.y, vertex.z,
					color.as_int,
					uvs.u, uvs.v
			};

			log_info("vertex[%d] %f %f %f", i, vertex.x, vertex.y, vertex.z);
		}

		atomic.vertices = bgfx::createVertexBuffer(
				bgfx::makeRef(meshVertices, sizeof(DFFVertex) * geometry->vertexCount),
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

			log_info("face %d %d %d", face.vertex1, face.vertex2, face.vertex3);
		}

		for (int i = 0; i < atomic.subMeshes.size(); i++) {
			auto& subMesh = atomic.subMeshes[i];
			auto size = sizeof(uint16_t) * subMeshFaces[i].size() * 3;
			auto mem = bgfx::alloc(size);
			memcpy(mem->data, &subMeshFaces[i][0], size);
			subMesh.indices = bgfx::createIndexBuffer(
					mem
			);
		}

		atomic.matList = new MaterialList(geometry->materialList, txd);

		auto& frame = clump->frameList->frames[atomicChunk->frameIndex];
		glm::translate(atomic.transform, glm::vec3(frame.translation.x, frame.translation.y, frame.translation.z));
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

void DFFModel::draw(glm::vec3 pos, int renderBits) {
	for (auto& atomic : atomics) {
		for (auto& submesh : atomic.subMeshes) {
			glm::mat4 transform = atomic.transform;
			transform = glm::translate(transform, pos);
			bgfx::setTransform(&transform[0][0]);
			bgfx::setVertexBuffer(0, atomic.vertices);
			bgfx::setIndexBuffer(submesh.indices);

			atomic.matList->bind(submesh.material, nullptr, renderBits, false);

			bgfx::submit(0, dffProgram);
		}
	}
}
