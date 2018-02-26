#include "TexDictionary.hh"

TexDictionary::TexDictionary(rw::TextureDictionary* txdChunk) {
	for (auto* texture : txdChunk->textures) {
		auto format = bgfx::TextureFormat::Unknown;
		if (texture->compression) {
			switch (texture->compression) {
				case 1: format = bgfx::TextureFormat::BC1; break; // DXT1
				default: log_warn("Compression type %d unsupported", texture->compression);
			}
		} else {
			switch (texture->format & 0x0f00) {
				case rw::TextureRasterFormat::RASTER_C8888:
					format = bgfx::TextureFormat::BGRA8;
					break;
				default: {
					char* buffer = rw::getRasterFormatLabel(texture->format);
					log_warn("Texture format %s unsupported", buffer);
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
				width, height, false, 1, format, 0,
				bgfx::makeRef(texture->data, texture->dataSize)
		);
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

void TexDictionary::showWindow() {
	ImGui::Begin("TXD Archive");

	//const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pineapple", "Strawberry", "Watermelon" };
	std::vector<const char*> listbox_items;
	for (auto& tex : textures) {
		listbox_items.push_back(tex.textureChunk->name.c_str());
	}

	static int listbox_item_current = 0;
	ImGui::ListBox("listbox\n(single select)", &listbox_item_current, &listbox_items[0], listbox_items.size(), 4);

	auto& current = textures[listbox_item_current];
	union BiggImTexture { ImTextureID ptr; struct { uint16_t flags; bgfx::TextureHandle handle; } s; };
	BiggImTexture img;
	img.s.flags = 0; // unused
	img.s.handle = current.handle;
	ImGui::Image(img.ptr, ImVec2(current.textureChunk->width, current.textureChunk->height));
	ImGui::LabelText("handle", "%d", current.handle.idx);
	ImGui::End();
}
