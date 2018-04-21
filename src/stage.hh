#pragma once
#include "common.hh"
#include "render/BSPModel.hh"
#include "io/ONEArchive.hh"
#include <list>
#include <bigg.hpp>
#include "render/TexDictionary.hh"
#include "render/TXCAnimation.hh"
#include "render/DFFModel.hh"
#include "util/ObjectList.hh"

class VisibilityManager {
	struct VisibilityBlock {
		i32 chunk;
		i32 low_x;
		i32 low_y;
		i32 low_z;
		i32 high_x;
		i32 high_y;
		i32 high_z;

		inline bool contains(glm::vec3 pos) {
			i32 x = (i32) pos.x;
			i32 y = (i32) pos.y;
			i32 z = (i32) pos.z;

			return x >= low_x && y >= low_y && z >= low_z &&
				   x < high_x && y < high_y && z < high_z;
		}
	};
	std::vector<VisibilityBlock> blocks;
	bool fileExists = false;
	bool showChunkBorders = false;
	bool forceShowAll = false;
	bool usePadding = true;
public:
	bool isVisible(int chunkId, glm::vec3 camPos);
	void read(FSPath& blkFile);

	void drawUI(glm::vec3 camPos);

	void drawDebug(glm::vec3 camPos);
};

class DFFCache {
private:
	std::map<std::string, DFFModel*> cache;
public:
	~DFFCache();
	void addFromArchive(FSPath& onePath, TexDictionary* txd);
	DFFModel* getDFF(const char* name);
};

class ObjectLayout {
public:
	struct CachedModel {
		glm::mat4 transform;
		DFFModel* model;
		int renderBits;
	};
	struct ObjectInstance {
		float pos_x;
		float pos_y;
		float pos_z;
		float rot_x;
		float rot_y;
		float rot_z;
		int type;
		int linkID;
		int radius;
		u8 misc[32];
		bool cache_invalid = true;
		bool fallback_render = false;
		std::vector<CachedModel> cache;
	};
private:
	std::vector<ObjectInstance> objects;
	void buildObjectCache(ObjectInstance& object, DFFCache* cache, ObjectList* objdb, int id);
public:
	void read(FSPath& binFile);
	void write(FSPath& binFile);

	void draw(glm::vec3 camPos, DFFCache* cache, ObjectList* objdb, int picking);
	void drawUI(glm::vec3 camPos, ObjectList* objdb);

	ObjectInstance* get(int id);
};

void setSelectedObject(int list, int ob);

class Stage {
	std::list<BSPModel> models;
	VisibilityManager visibilityManager;
	ObjectLayout* layout_db = nullptr;
	ObjectLayout* layout_pb = nullptr;
	ObjectLayout* layout_p1 = nullptr;
	DFFCache* cache = nullptr;
	ObjectList* objdb = nullptr;
public:
	Stage();
	~Stage();
	void fromArchive(ONEArchive* x, TexDictionary* txd);
	void readVisibility(FSPath& blkFile);
	void readLayout(const char* dvdroot, const char* stgname);
	void draw(glm::vec3 camPos, TXCAnimation* txc, bool picking);
	void drawUI(glm::vec3 camPos);
	void drawVisibilityUI(glm::vec3 camPos);
	void drawLayoutUI(glm::vec3 camPos);
	void drawDebug(glm::vec3 camPos);

	void readCache(FSPath& oneFile, TexDictionary* txd);
};
