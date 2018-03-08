#include "MaterialList.hh"

static bgfx::UniformHandle uSamplerTexture;
static bgfx::UniformHandle uMaterialColor;
static bgfx::UniformHandle uMaterialBits;
static bool staticValuesLoaded = false;

MaterialList::MaterialList(rw::MaterialListChunk* matList, TexDictionary* txd) {
	for (auto matChunk : matList->materials) {
		materials.emplace_back();
		auto& material = materials.back();

		material.color = matChunk->color;
		if (matChunk->isTextured) {
			if (txd) material.texture = txd->getTexture(matChunk->texture->textureName.c_str());
			else material.texture.idx = 0;
		} else {
			material.texture.idx = 0;
		}
	}
}

void MaterialList::bind(int id, TXCAnimation* txc, int renderBits, bool triList) {
	// set material
	if (!staticValuesLoaded) {
		uSamplerTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Int1);
		uMaterialColor = bgfx::createUniform("u_materialColor", bgfx::UniformType::Vec4);
		uMaterialBits = bgfx::createUniform("u_materialBits", bgfx::UniformType::Int1);
		staticValuesLoaded = true;
	}
	const auto& material = materials[id];

	bgfx::setTexture(0, uSamplerTexture, txc ? txc->getTexture(material.texture) : material.texture);
	float matColor[4];
	uint32_t color = material.color; // todo: is this RGBA or BGRA?
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
	uint64_t state = BGFX_STATE_RGB_WRITE | BGFX_STATE_MSAA;
	state |= BGFX_STATE_ALPHA_WRITE;

	if (!((renderBits & BIT_ADDITIVE) || (renderBits & BIT_FULL_ALPHA)))
		state |= BGFX_STATE_DEPTH_WRITE;
	state |= BGFX_STATE_DEPTH_TEST_LESS;

	if (triList) {
		state |= BGFX_STATE_PT_TRISTRIP;
	}

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

}
