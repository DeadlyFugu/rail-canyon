#pragma once
#include "common.hh"
#include "render/BSPModel.hh"
#include "io/ONEArchive.hh"
#include <list>
#include <bigg.hpp>
#include "render/TexDictionary.hh"
#include "render/TXCAnimation.hh"

using rc::render::BSPModel;
using rc::io::ONEArchive;

class VisibilityManager {
	struct VisibilityBlock {
		i32 chunk;
		i32 low_x;
		i32 low_y;
		i32 low_z;
		i32 high_x;
		i32 high_y;
		i32 high_z;
	};
	std::vector<VisibilityBlock> blocks;
	bool fileExists = false;
public:
	bool forceShowAll = false;
	bool isVisible(int chunkId, glm::vec3 camPos);
	void read(rc::util::FSPath& blkFile);
};

class Stage {
	std::list<BSPModel> models;
	VisibilityManager visibilityManager;
public:
	~Stage();
	void fromArchive(ONEArchive* x, TexDictionary* txd);
	void readVisibility(rc::util::FSPath& blkFile);
	void draw(glm::vec3 camPos, TXCAnimation* txc);
	void drawUI(glm::vec3 camPos);
};
