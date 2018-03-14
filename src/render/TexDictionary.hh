// Represents a RenderWare TXD file

#pragma once

#include "common.hh"
#include "texture.hh"
#include <bigg.hpp>

class TexDictionary {
	struct TextureEntry {
		bgfx::TextureHandle handle;
		std::string name;
		int width, height;
	};
	std::vector<TextureEntry> textures;
	int listbox_item_current = 0;
public:
	/// load textures from rw::TextureDictionary chunk
	TexDictionary(rw::TextureDictionary* txdChunk);
	/// frees resources
	~TexDictionary();

	bgfx::TextureHandle getTexture(const char* name);

	void drawUI();
};
