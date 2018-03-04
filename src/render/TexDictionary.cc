#include "TexDictionary.hh"

TexDictionary::TexDictionary(rw::TextureDictionary* txdChunk) {
	for (auto* texture : txdChunk->textures) {
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

		if (format == bgfx::TextureFormat::Unknown) {
			TextureEntry e;
			e.handle = bgfx::TextureHandle();
			e.textureChunk = texture;
			textures.push_back(e);
			continue;
		}

		u16 width = texture->width;
		u16 height = texture->height;

		auto bgfxTexHandle = bgfx::createTexture2D(
				width, height, /*(bool) (texture->format & rw::RASTER_MIPMAP) && !(texture->format & rw::RASTER_AUTOMIPMAP)*/ texture->mipmaps.size() > 1, 1, format, 0,
				nullptr //bgfx::makeRef(texture->mipmaps[0].data, texture->mipmaps[0].size)
		);

		u8 mip = 0;
		for (auto& mipmap : texture->mipmaps) {
			bgfx::updateTexture2D(bgfxTexHandle, 0, mip, 0, 0, width, height,
								  bgfx::makeRef(texture->mipmaps[mip].data, texture->mipmaps[mip].size));

			mip++;
			width >>= 1;
			height >>= 1;
		}

		TextureEntry e; // todo: make constructor and use emplace_back
		e.handle = bgfxTexHandle;
		e.textureChunk = texture;
		textures.push_back(e);
	}
}

TexDictionary::~TexDictionary() {

}

bgfx::TextureHandle TexDictionary::getTexture(const char* name) {
	std::string nameStr(name);
	for (auto& tex : textures) {
		if (tex.textureChunk->name == nameStr) {
			return tex.handle;
		}
	}
	return bgfx::TextureHandle();
}

void TexDictionary::drawUI() {
	std::vector<const char*> listbox_items;
	for (auto& tex : textures) {
		listbox_items.push_back(tex.textureChunk->name.c_str());
	}

	static int listbox_item_current = 0;
	ImGui::ListBox("listbox\n(single select)", &listbox_item_current, &listbox_items[0], listbox_items.size(), 6);

	auto& current = textures[listbox_item_current];
	union BiggImTexture { ImTextureID ptr; struct { uint16_t flags; bgfx::TextureHandle handle; } s; };
	BiggImTexture img;
	img.s.flags = 0; // unused
	img.s.handle = current.handle;
	ImGui::Image(img.ptr, ImVec2(current.textureChunk->width, current.textureChunk->height));
	ImGui::LabelText("handle", "%d", current.handle.idx);
}
