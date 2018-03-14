#include "TexDictionary.hh"

TexDictionary::TexDictionary(rw::TextureDictionary* txdChunk) {
	for (auto* texture : txdChunk->textures) {
		// determine texture format
		auto format = bgfx::TextureFormat::Unknown;
		if (texture->compression) {
			switch (texture->compression) {
				case 1: format = bgfx::TextureFormat::BC1; break; // DXT1
				default: rw::util::logger.warn("Compression type %d unsupported", texture->compression);
			}
		} else {
			switch (texture->format & 0x0f00) {
				case rw::TextureRasterFormat::RASTER_C8888:
					format = bgfx::TextureFormat::BGRA8;
					break;
				case rw::TextureRasterFormat::RASTER_C888:
					format = bgfx::TextureFormat::BGRA8;
					break;
				default: {
					char* buffer = rw::getRasterFormatLabel(texture->format);
					rw::util::logger.warn("Texture format %s unsupported", buffer);
					delete[] buffer;
				}
			}
		}

		// if unsupported format set to error texture
		if (format == bgfx::TextureFormat::Unknown) {
			TextureEntry e;
			e.handle = bgfx::TextureHandle();
			e.name = texture->name;
			e.width = texture->width;
			e.height = texture->height;
			textures.push_back(e);
			continue;
		}

		// create empty texture
		auto bgfxTexHandle = bgfx::createTexture2D(
				texture->width, texture->height, texture->mipmaps.size() > 1, 1, format, 0,
				nullptr
		);

		// set mips
		u8 mip = 0;
		u16 mipWidth = texture->width;
		u16 mipHeight = texture->height;
		for (auto& mipmap : texture->mipmaps) {
			// copy texture mip data, bgfx will clear it
			auto size = texture->mipmaps[mip].size;
			auto mem = bgfx::alloc(size);
			memcpy(mem->data, texture->mipmaps[mip].data, size);
			bgfx::updateTexture2D(bgfxTexHandle, 0, mip, 0, 0, mipWidth, mipHeight, mem);

			// move to next mipmap
			mip++;
			mipWidth >>= 1;
			mipHeight >>= 1;
		}

		TextureEntry e; // todo: make constructor and use emplace_back
		e.handle = bgfxTexHandle;
		e.name = texture->name;
		e.width = texture->width;
		e.height = texture->height;
		textures.push_back(e);
	}
}

TexDictionary::~TexDictionary() {
	for (auto& texture : textures) {
		bgfx::destroy(texture.handle);
	}
}

bgfx::TextureHandle TexDictionary::getTexture(const char* name) {
	std::string nameStr(name);
	for (auto& tex : textures) {
		if (tex.name == nameStr) {
			return tex.handle;
		}
	}
	return bgfx::TextureHandle();
}

void TexDictionary::drawUI() {
	std::vector<const char*> listbox_items;
	for (auto& tex : textures) {
		listbox_items.push_back(tex.name.c_str());
	}

	static int listbox_item_current = 0;
	ImGui::ListBox("listbox\n(single select)", &listbox_item_current, &listbox_items[0], listbox_items.size(), 6);

	auto& current = textures[listbox_item_current];
	union BiggImTexture { ImTextureID ptr; struct { uint16_t flags; bgfx::TextureHandle handle; } s; };
	BiggImTexture img;
	img.s.flags = 0; // unused
	img.s.handle = current.handle;
	ImGui::Image(img.ptr, ImVec2(current.width, current.height));
	ImGui::LabelText("handle", "%d", current.handle.idx);
}
