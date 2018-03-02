// Represents a RenderWare TXD file

#pragma once

#include "common.hh"
#include "texture.hh"
#include <bigg.hpp>

class TexDictionary {
	struct TextureEntry {
		bgfx::TextureHandle handle;
		rw::TextureNative* textureChunk;
	};
	std::vector<TextureEntry> textures;
	int listbox_item_current = 0;
public:
	TexDictionary(rw::TextureDictionary* txdChunk);
	~TexDictionary();

	bgfx::TextureHandle getTexture(const char* name);

	void showWindow();
};
