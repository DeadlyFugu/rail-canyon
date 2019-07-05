
#include <bigg.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <../extern/bigg/deps/bgfx.cmake/bgfx/examples/common/debugdraw/debugdraw.h>

#include "stage.hh"

#include "render/Camera.hh"
#include "render/TexDictionary.hh"
#include "render/TXCAnimation.hh"
#include "render/DMAAnimation.hh"
#include "util/config.hh"
#include "render/DFFModel.hh"
#include "misc/Help.h"
#include "misc/ImGuizmo.h"

const char* stageDisplayNames[] = {
		"<none>",
		"<DFF>",
		"Test Level [stg00]",
		"Seaside Hill [s01]",
		"Ocean Palace [s02]",
		"Grand Metropolis [s03]",
		"Power Plant [s04]",
		"Casino Park [stg05]",
		"BINGO Highway [stg06]",
		"Rail Canyon [stg07]",
		"Bullet Station [stg08]",
		"Frog Forest [stg09]",
		"Lost Jungle [stg10]",
		"Hang Castle [s11]",
		"Mystic Mansion [s12]",
		"Egg Fleet [s13]",
		"Final Fortress [s14]",
		"Egg Hawk [s20]",
		"Team Battle 1 [s21]",
		"Robot Carnival [s22]",
		"Egg Albatross [s23]",
		"Team Battle 2 [stg24]",
		"Robot Storm [s25]",
		"Egg Emperor [stg26]",
		"Metal Madness [s27]",
		"Metal Overlord [s28]",
		"Sea Gate [s29]",
		"Seaside Course (Bobsled Race) [s31]",
		"City Course (Bobsled Race) [s32]",
		"Casino Course (Bobsled Race) [s33]",
		"Bonus Stage 2 [stg40]",
		"Bonus Stage 1 [stg41]",
		"Bonus Stage 3 [stg42]",
		"Bonus Stage 4 [stg43]",
		"Bonus Stage 5 [stg44]",
		"Bonus Stage 6 [stg45]",
		"Bonus Stage 7 [stg46]",
		"Rail Canyon (Team Chaotix) [stg50]",
		"Seaside Hill (Action Race) [s60]",
		"Grand Metropolis (Action Race) [s61]",
		"BINGO Highway (Action Race) [stg62]",
		"City Top (Battle) [s63]",
		"Casino Ring (Battle) [stg64]",
		"Turtle Shell (Battle) [s65]",
		"Egg Treat (Ring Race) [s66]",
		"Pinball Match (Ring Race) [stg67]",
		"Hot Elevator (Ring Race) [s68]",
		"Road Rock (Quick Race) [s69]",
		"Mad Express (Quick Race) [stg70]",
		"Terror Hall (Quick Race) [s71]",
		"Rail Canyon (Expert Race) [stg72]",
		"Frog Forest (Expert Race) [stg73]",
		"Egg Fleet (Expert Race) [s74]",
		"Emerald Challenge 2 [stg80]",
		"Emerald Challenge 1 [stg81]",
		"Emerald Challenge 3 [stg82]",
		"Emerald Challenge 4 [stg83]",
		"Emerald Challenge 5 [stg84]",
		"Emerald Challenge 6 [stg85]",
		"Emerald Challenge 7 [stg86]",
		"Special Stage 1 (Multiplayer) [stg87]",
		"Special Stage 2 (Multiplayer) [stg88]",
		"Special Stage 3 (Multiplayer) [stg89]",
};

const char* stageFileNames[] = {
		0,
		0,
		"stg00",
		"s01",
		"s02",
		"s03",
		"s04",
		"stg05",
		"stg06",
		"stg07",
		"stg08",
		"stg09",
		"stg10",
		"s11",
		"s12",
		"s13",
		"s14",
		"s20",
		"s21",
		"s22",
		"s23",
		"stg24",
		"s25",
		"stg26",
		"s27",
		"s28",
		"s29",
		"s31",
		"s32",
		"s33",
		"stg40",
		"stg41",
		"stg42",
		"stg43",
		"stg44",
		"stg45",
		"stg46",
		"stg50",
		"s60",
		"s61",
		"stg62",
		"s63",
		"stg64",
		"s65",
		"s66",
		"stg67",
		"s68",
		"s69",
		"stg70",
		"s71",
		"stg72",
		"stg73",
		"s74",
		"stg80",
		"stg81",
		"stg82",
		"stg83",
		"stg84",
		"stg85",
		"stg86",
		"stg87",
		"stg88",
		"stg89",
};

std::vector<std::string> error_log;

void recordStreamErrorLog(rw::util::Logger::LogLevel level, const char* str) {
	char buffer[512];
	static const char* levelNameTable = "INFO\0WARN\0ERR";
	snprintf(buffer, 512, "[%s] %s\n", &levelNameTable[level*5], str);
	error_log.emplace_back(buffer);
	log_error(str);
}

static auto cameraStartPos = glm::vec3(0.0f, 10.0f, 35.0f );

static int stageSelect = 0;
static char dvdroot[512] = "D:\\Heroes\\dvdroot\0";
static char dvdroot_out[512] = "";
static float mouse_sensitivity;
static float move_speed_scale;

const char* getOutPath() {
	return dvdroot_out[0] ? dvdroot_out : ".";
}

const char* getStageFilename() {
	return stageFileNames[stageSelect];
}

// from https://github.com/bkaradzic/bgfx/blob/master/examples/07-callback/callback.cpp
#include <bx/file.h>
#include <bx/debug.h>
#include <bimg/bimg.h>
#include <bx/math.h>

static void savePNG(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _srcPitch, const void* _src, bool _grayscale, bool _yflip) {
	bx::FileWriter writer;
	bx::Error err;
	if (bx::open(&writer, _filePath, false, &err) ) {
		const u8* data = (const u8*) _src;
		for (int i = 0; i < _height; i++) {
			for (int j = 0; j < _width; j++) {
				u8* pixel = const_cast<u8*>(&data[j*4]); // dangerous but w/e
				pixel[3] = 0xff;
			}
			data += _srcPitch;
		}
		bimg::imageWritePng(&writer, _width, _height, _srcPitch, _src, _grayscale, _yflip, &err);
		bx::close(&writer);
	}
}

struct BgfxCallback : public bgfx::CallbackI {
	virtual ~BgfxCallback() {}

	virtual void fatal(bgfx::Fatal::Enum _code, const char* _str) override {
		bx::debugPrintf("Fatal error: 0x%08x: %s", _code, _str);
		abort();
	}

	virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override {
		bx::debugPrintf("%s (%d): ", _filePath, _line);
		bx::debugPrintfVargs(_format, _argList);
	}

	virtual void profilerBegin(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override {}
	virtual void profilerBeginLiteral(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override {}

	virtual void profilerEnd() override {}

	virtual uint32_t cacheReadSize(uint64_t _id) override {return 0;}

	virtual bool cacheRead(uint64_t _id, void* _data, uint32_t _size) override {return false;}
	virtual void cacheWrite(uint64_t _id, const void* _data, uint32_t _size) override {}

	virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t /*_size*/, bool _yflip) override {
		savePNG(_filePath, _width, _height, _pitch, _data, false, _yflip);
	}

	virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t _pitch, bgfx::TextureFormat::Enum _format, bool _yflip) override {}
	virtual void captureEnd() override {}
	virtual void captureFrame(const void* _data, uint32_t _size) override {}
};

bool screenshot() {
	FSPath screenshots("screenshots");
	if (!screenshots.exists()) {
		auto result = screenshots.mkDir();
		if (!result) return false;
	}

	FSPath file("");
	char buffer[512];
	int idx = 0;
	do {
		snprintf(buffer, 512, "%s_%03d.png", getStageFilename(), ++idx);
		file = screenshots / buffer;
	} while (file.exists());

	bgfx::requestScreenShot(BGFX_INVALID_HANDLE, file.str.c_str());
	return true;
}

class RailCanyonApp : public bigg::Application {
private:
	Camera camera;
	float mTime = 0.0f;
	Stage* stage = nullptr;
	Help* help = new Help();
	TexDictionary* txd = nullptr;
	TexDictionary* txd_common = nullptr;
	TXCAnimation* txc = nullptr;
	DFFModel* dff = nullptr;
	DMAAnimation* dma = nullptr;
	std::vector<std::string> morphtargets;
	int cur_target = 0;
	std::vector<const char*> archiveDFFs;
	int dffSelect = 0;
	ONEArchive* one;
	bool useDMA = false;

	bgfx::TextureHandle picking_rt;
	bgfx::TextureHandle picking_rt_depth;
	bgfx::TextureHandle blitted;
	bgfx::FrameBufferHandle picking_fb;
	u32 readback_frame = 0;
	u32 current_frame = 0;
	#define PICK_SIZE 64
	u8 blit_data[PICK_SIZE * PICK_SIZE * 4];

	bool showTestWindow = false;
	bool showPanel = true;
	int screenshotNextFrame = 0;
	float bgColor[3];
	int dffRenderBits = 0;
public:
	RailCanyonApp() : camera(glm::vec3(0.f, 100.f, 350.f), glm::vec3(0,0,0), 60, 1.f, 960000.f) {}
private:
	void openTXD(const FSPath& path) {
		FSPath txdPath(path);
		Buffer sk_x = txdPath.read();
		rw::Chunk* root = rw::readChunk(sk_x);
		txd = new TexDictionary((rw::TextureDictionary*) root);
		delete root;
	}

	void openCommonTXD(const FSPath& path) {
		FSPath txdPath(path);
		Buffer sk_x = txdPath.read();
		rw::Chunk* root = rw::readChunk(sk_x);
		txd_common = new TexDictionary((rw::TextureDictionary*) root);
		delete root;
	}

	void openBSPWorld(FSPath& onePath, FSPath& blkPath) {
		// read BSPs
		ONEArchive archive(onePath);
		stage = new Stage();
		stage->fromArchive(&archive, txd);
		if (blkPath.exists())
			stage->readVisibility(blkPath);
	}

	void openStage(const char* name) {
		char buffer[512];
		sprintf(buffer, "%s/textures/%s.txd", dvdroot, name);
		FSPath txdPath(buffer);
		sprintf(buffer, "%s/textures/obj_common.txd", dvdroot);
		FSPath txdCmnPath(buffer);
		sprintf(buffer, "%s/%s.one", dvdroot, name);
		FSPath onePath(buffer);
		sprintf(buffer, "%s/%s_blk.bin", dvdroot, name);
		FSPath blkPath(buffer);
		sprintf(buffer, "%s/%s.txc", dvdroot, name);
		FSPath txcPath(buffer);
		sprintf(buffer, "%s/%sobj.one", dvdroot, name);
		FSPath objPath(buffer);
		sprintf(buffer, "%s/comobj.one", dvdroot);
		FSPath cmnPath(buffer);

		if (!txdPath.exists()) {
			rw::util::logger.error("missing .txd");
		} else if (!onePath.exists()) {
			rw::util::logger.error("missing .one");
		} else {
			openTXD(txdPath);
			openCommonTXD(txdCmnPath);
			openBSPWorld(onePath, blkPath);
			stage->readCache(objPath, txd);
			stage->readCache(cmnPath, txd_common);
			stage->readLayout(dvdroot, name);

			if (txcPath.exists()) {
				auto b = txcPath.read();
				txc = new TXCAnimation(b, txd);
			}
		}
	}

	void closeStage() {
		if (stage) {
			delete stage;
			stage = nullptr;
		}
		if (txd) {
			delete txd;
			txd = nullptr;
		}
		if (txc) {
			delete txc;
			txc = nullptr;
		}
		if (dff) {
			delete dff;
			dff = nullptr;
		}
	}
	void initialize( int _argc, char** _argv ) override {
		// read config
		const char* dvdroot_ = config_get("dvdroot", dvdroot);
		strncpy(dvdroot, dvdroot_, 512);
		const char* dvdroot_out_ = config_get("dvdroot_out", dvdroot_out);
		strncpy(dvdroot_out, dvdroot_out_, 512);

		mouse_sensitivity = config_getf("mouse_sensitivity", 0.15f);
		move_speed_scale = config_getf("move_speed_scale", 1.00f);

		// setup bgfx
		bgfx::setDebug( BGFX_DEBUG_TEXT );
		mTime = 0.0f;
		reset(BGFX_RESET_VSYNC);

		ddInit();

		// setup picking framebuffer
		auto tex_flags = BGFX_TEXTURE_MIN_POINT
					   | BGFX_TEXTURE_MAG_POINT
					   | BGFX_TEXTURE_MIP_POINT
					   | BGFX_TEXTURE_U_CLAMP
					   | BGFX_TEXTURE_V_CLAMP;

		using bgfx::TextureFormat;
		picking_rt = bgfx::createTexture2D(PICK_SIZE, PICK_SIZE, false, 1, TextureFormat::RGBA8, tex_flags | BGFX_TEXTURE_RT);
		picking_rt_depth = bgfx::createTexture2D(PICK_SIZE, PICK_SIZE, false, 1, TextureFormat::D24S8, tex_flags | BGFX_TEXTURE_RT);
		blitted = bgfx::createTexture2D(PICK_SIZE, PICK_SIZE, false, 1, TextureFormat::RGBA8,
										tex_flags | BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);

		bgfx::TextureHandle targets[] = {picking_rt, picking_rt_depth};
		picking_fb = bgfx::createFrameBuffer(2, targets, true);
	}

	int shutdown() override {
		closeStage();
		ddShutdown();
		return 0;
	}

	void setBackground() {
		uint32_t color = 0x000000ff;
		color |= uint8_t(bgColor[0] * 255.f) << 24;
		color |= uint8_t(bgColor[1] * 255.f) << 16;
		color |= uint8_t(bgColor[2] * 255.f) <<  8;
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, /*0x303030ff*/ color, 1.0f, 0);
		bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
	}

	void onReset() override {
		setBackground();
		bgfx::setViewRect( 0, 0, 0, uint16_t( getWidth() ), uint16_t( getHeight() ) );
	}

	void drawMainUI() {
		ImGui::Text("RailCanyon v0.3 [pre-release]");

		// dvdroot
		static bool dvdrootExistsDirty = true;
		static bool dvdrootExists = false;
		if (ImGui::InputText("dvdroot", dvdroot, 512)) {
			dvdrootExistsDirty = true;
		}
		if (dvdrootExistsDirty) {
			FSPath dvdrootPath(dvdroot);
			dvdrootExists = dvdrootPath.exists();

			if (dvdrootExists) {
				config_set("dvdroot", dvdroot);
			}
			dvdrootExistsDirty = false;
		}
		if (!dvdrootExists) {
			ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "Path does not exist");
		}

		// dvdroot out
		// todo: somehow merge this with above
		static bool dvdrootOutExistsDirty = true;
		static bool dvdrootOutExists = false;
		if (ImGui::InputText("dvdroot out", dvdroot_out, 512)) {
			dvdrootOutExistsDirty = true;
		}
		if (dvdrootOutExistsDirty) {
			FSPath dvdrootOutPath(dvdroot_out);
			dvdrootOutExists = dvdrootOutPath.exists() || !dvdroot_out[0]; // if empty string, also counts as exists

			if (dvdrootOutExists) {
				config_set("dvdroot_out", dvdroot_out);
			}
			dvdrootOutExistsDirty = false;
		}
		if (!dvdrootOutExists) {
			ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "Path does not exist");
		}

		if (ImGui::Combo("stage", &stageSelect, stageDisplayNames, sizeof(stageDisplayNames) / sizeof(*stageDisplayNames))) {
			closeStage();
			camera.setPosition(cameraStartPos);
			camera.lookAt(vec3(0, 0, 0));
			if (stageFileNames[stageSelect]) {
				error_log.clear();
				rw::util::logger.setPrintCallback(recordStreamErrorLog);
				openStage(stageFileNames[stageSelect]);
				rw::util::logger.setPrintCallback(rw::util::logger.getDefaultPrintCallback());
				if (error_log.size() > 0) {
					log_info("open popup");
					ImGui::OpenPopup("Error Log");
				}
			}
		}

		if (stageSelect == 1) { // <DFF>
			static char archivePath[128];
			static bool archiveExists = false;
			static bool archiveExistsDirty = true;
			static int dmaSelect = 0;

			if (ImGui::InputText("ONE archive", archivePath, 128)) archiveExistsDirty = true;

			if (archiveExistsDirty) {
				archiveExistsDirty = false;
				archiveDFFs.clear();
				dffSelect = 0;
				if (one) {
					delete one;
					one = nullptr;
				}

				FSPath archiveFSPath = FSPath(dvdroot) / archivePath;

				if (!archiveFSPath.exists()) {
					archiveExists = false;
				} else {
					archiveExists = true;

					one = new ONEArchive(archiveFSPath);

					const auto fileCount = one->getFileCount();
					for (int i = 0; i < fileCount; i++) {
						archiveDFFs.push_back(one->getFileName(i));
					}
				}
			}

			if (archiveExists && archiveDFFs.size() > 0) {
				if (ImGui::Combo("dff", &dffSelect, &archiveDFFs[0], (int) archiveDFFs.size())) {
					if (dff) delete dff;
					morphtargets.clear();
					cur_target = 0;
					dff = new DFFModel();
					Buffer b = one->readFile(archiveDFFs[dffSelect]);
					rw::ClumpChunk* clump = (rw::ClumpChunk*) rw::readChunk(b);
					dff->setFromClump(clump, txd);
					morphtargets.push_back("<none>");
					for (auto geom : clump->geometryList->geometries) {
						for (auto ext : geom->extensions) {
							for (auto& chunk : ((rw::ListChunk*) ext)->children) {
								if (chunk->type != RW_DELTA_MORPH_PLG) continue;

								for (auto& target : ((rw::DeltaMorphPLGChunk*) chunk)->targets) {
									morphtargets.push_back(target.name);
								}
							}
						}
					}
					delete clump;
				}
				ImGui::Checkbox("use DMA", &useDMA);
				if (morphtargets.size() > 1 && !useDMA) {
					std::vector<const char*> target_charps;
					for (auto& target : morphtargets) {
						target_charps.push_back(target.c_str());
					}
					if (ImGui::Combo("dmtarget", &cur_target, &target_charps[0], (int) target_charps.size())) {
						if (dff) delete dff;
						dff = new DFFModel();
						Buffer b = one->readFile(archiveDFFs[dffSelect]);
						rw::ClumpChunk* clump = (rw::ClumpChunk*) rw::readChunk(b);
						dff->setFromClump(clump, txd, cur_target);
						delete clump;
					}
				} else if (morphtargets.size() > 1) {
					if (ImGui::Combo("dma", &dmaSelect, &archiveDFFs[0], (int) archiveDFFs.size())) {
						if (dma) delete dma;
						dma = new DMAAnimation();
						Buffer b = one->readFile(archiveDFFs[dmaSelect]);
						auto chunk = (rw::DMorphAnimationChunk*) rw::readChunk(b);
						dma->loadFromChunk(chunk);
						delete chunk;
					}
				}
			} else {
				ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No archive is chosen");
			}

			static char txdPath[128];
			ImGui::InputText("txd path", txdPath, 128);
			if (ImGui::Button("Force TXD")) {
				FSPath path = FSPath(dvdroot) / txdPath;
				openTXD(path);
			}

			static char matFlags[32];
			if (ImGui::InputText("mat flags", matFlags, 32)) {
				dffRenderBits = 0;
				for (char* p = matFlags; *p; p++) {
					dffRenderBits |= matFlagFromChar(*p);
				}
				dffRenderBits ^= BIT_REFLECTIVE;
			}
		}

		if (ImGui::BeginPopupModal("Error Log", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Errors occured opening stage:\n");

			ImGui::BeginChild("scrolling", ImVec2(500,300), true, ImGuiWindowFlags_HorizontalScrollbar);
			for (auto& line : error_log) {
				ImGui::TextUnformatted(line.c_str());
			}
			ImGui::EndChild();

			if (ImGui::Button("OK", ImVec2(200, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		static bool vsync = config_geti("vsync", 1) != 0;
		static int msaa = config_geti("msaa", 0);
		static const char* aaValues[] = { "none", "2x MSAA", "4x MSAA", "8x MSAA", "16x MSAA" };
		static bool firstResetNeeded = true;
		bool stateDirty =
			ImGui::Checkbox("vsync", &vsync) |
			ImGui::Combo("anti-aliasing", &msaa, aaValues, 5, -1) |
			firstResetNeeded;

		if (stateDirty) {
			int state = 0;
			if (vsync) state |= BGFX_RESET_VSYNC;
			switch (msaa) {
			case 1: state |= BGFX_RESET_MSAA_X2; break;
			case 2: state |= BGFX_RESET_MSAA_X4; break;
			case 3: state |= BGFX_RESET_MSAA_X8; break;
			case 4: state |= BGFX_RESET_MSAA_X16; break;
			}
			reset(state);
			config_seti("vsync", vsync);
			config_seti("msaa", msaa);
			firstResetNeeded = false;
		}

		if (ImGui::SliderFloat("mouse sensitivity", &mouse_sensitivity, 0.01f, 1.00f)) {
			config_setf("mouse_sensitivity", mouse_sensitivity);
		}

		if (ImGui::SliderFloat("move speed scale", &move_speed_scale, 0.01f, 10.00f)) {
			config_setf("move_speed_scale", move_speed_scale);
		}

		ImGui::Checkbox("test window", &showTestWindow);
		ImGui::Checkbox("panel", &showPanel);
		if (ImGui::ColorEdit3("background", &bgColor[0])) {
			setBackground();
		}

		if (ImGui::Button("Screenshot [F2]")) {
			screenshotNextFrame = 1;
		}

		union BiggImTexture { ImTextureID ptr; struct { uint16_t flags; bgfx::TextureHandle handle; } s; };
		BiggImTexture img;
		img.s.flags = 0; // unused
		img.s.handle = picking_rt;
		ImGui::Image(img.ptr, ImVec2(128, 128));
		img.s.handle = blitted;
		ImGui::Image(img.ptr, ImVec2(128, 128));
		if (ImGui::Button("ReadBack")) {
			bgfx::readTexture(blitted, &blit_data);
			bgfx::frame();
			bgfx::frame();
		}
	}

	bool showUI = true;

	void updateInput(float dt) {
		const auto keyboardActive = !ImGui::GetIO().WantCaptureKeyboard;
		const auto mouseActive = !ImGui::GetIO().WantCaptureMouse;

		// move camera
		if (keyboardActive) {
			glm::vec3 inputMove(0, 0, 0);
			if (glfwGetKey(this->mWindow, GLFW_KEY_W))
				inputMove.z += 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_S))
				inputMove.z -= 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_D))
				inputMove.x += 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_A))
				inputMove.x -= 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_Q))
				inputMove.y -= 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_E))
				inputMove.y += 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(this->mWindow, GLFW_KEY_RIGHT_SHIFT))
				inputMove *= 2.5f;
			if (glfwGetKey(this->mWindow, GLFW_KEY_LEFT_ALT) || glfwGetKey(this->mWindow, GLFW_KEY_RIGHT_ALT))
				inputMove *= 10.f;

			inputMove *= move_speed_scale;
			camera.inputMoveAxis(inputMove, dt);
		}

		// look around
		glm::vec2 inputLook(0,0);
		static bool mouseIsDragging;
		static double mouseLastX;
		static double mouseLastY;
		if (glfwGetMouseButton(this->mWindow, GLFW_MOUSE_BUTTON_MIDDLE)) {
			if (mouseIsDragging) {
				double mouseX;
				double mouseY;
				glfwGetCursorPos(this->mWindow, &mouseX, &mouseY);

				inputLook.x = mouseX - mouseLastX;
				inputLook.y = -(mouseY - mouseLastY);
				inputLook *= mouse_sensitivity;

				glfwSetCursorPos(this->mWindow, this->getWidth() / 2.0, this->getHeight() / 2.0);
				//mouseLastX = mouseX;
				//mouseLastY = mouseY;
			} else {
				glfwSetInputMode(this->mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				printf("DISABLE\n");
				mouseIsDragging = true;
				glfwSetCursorPos(this->mWindow, this->getWidth() / 2.0, this->getHeight() / 2.0);
				glfwGetCursorPos(this->mWindow, &mouseLastX, &mouseLastY);
			}
		} else if (keyboardActive) {
			if (glfwGetKey(this->mWindow, GLFW_KEY_UP))
				inputLook.y += 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_DOWN))
				inputLook.y -= 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_RIGHT))
				inputLook.x += 1;
			if (glfwGetKey(this->mWindow, GLFW_KEY_LEFT))
				inputLook.x -= 1;
			if (mouseIsDragging) {
				glfwSetInputMode(this->mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				mouseIsDragging = false;
			}
		}
		camera.inputLookAxis(inputLook, dt);

		// reset camera
		if (keyboardActive && glfwGetKey(this->mWindow, GLFW_KEY_SPACE))
			camera.lookAt(vec3(0,0,0));

		// toggle user interface
		static bool showUIPress = false; // used to get tab as press
		if (keyboardActive && glfwGetKey(this->mWindow, GLFW_KEY_TAB)) {
			if (!showUIPress) {
				showUI = !showUI;
				showUIPress = true;
			}
		} else {
			showUIPress = false;
		}

		// take screenshot
		static bool screenshotPress = false; // used to get F2 as press
		if (keyboardActive && glfwGetKey(this->mWindow, GLFW_KEY_F2)) {
			if (!screenshotPress) {
				screenshotNextFrame = true;
				screenshotPress = true;
			}
		} else {
			screenshotPress = false;
		}
	}

	void update( float dt ) override {
		mTime += dt;

		ImGuizmo::BeginFrame();

		// receive input
		updateInput(dt);

		// setup view
		camera.use(0, (float) getWidth() / getHeight());
		bgfx::setViewRect( 0, 0, 0, uint16_t( getWidth() ), uint16_t( getHeight() ) );
		bgfx::touch( 0 );

		// debug display
		ddBegin(0);
		if (dff) {
			ddDrawAxis(0, 0, 0, 100.0f);
			float gridOrigin[3] = {0.0f, 0.0f, 0.0f};
			ddDrawGrid(Axis::Y, gridOrigin, 20, 10.0f);
		}
		if (stage) stage->drawDebug(camera.getPosition());
		ddEnd();

		// show overlay
		auto campos = camera.getPosition();
		static bool showOverlay = true;
		if (!showPanel) goto end_overlay;
		ImGui::SetNextWindowPos(ImVec2(10,10));
		if (ImGui::Begin("dispoverlay", &showOverlay, ImVec2(240,0), 0.3f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Stage: %s", stageDisplayNames[stageSelect]);
			ImGui::Separator();
			ImGui::Text("Cam: (%.0f, %.0f, %.0f)", campos.x, campos.y, campos.z);
			ImGui::Text("FPS: %0.3f", 1.0f / dt);
		}
		ImGui::End();
		end_overlay:

		// show ui sidebar
		if (screenshotNextFrame || !showUI) goto end_ui;
		ImGui::SetNextWindowPos(ImVec2(10, 85));
		static bool showSidePanel;
		ImGui::SetNextWindowSize(ImVec2(300, getHeight() - 145.0f), ImGuiCond_Always);
		if (ImGui::Begin("sidepanel", &showSidePanel, ImVec2(300, getHeight() - 145.0f), -1.0f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings)) {
			const char* menus[] = {
					"Main",
					"Stage",
					"Visibility",
					"TXD Archive",
					"TXC Animations",
					"Object Layout",
					"DFF Cache",
					"Help & About"
			};
			static int ui_select = 0;
			ImGui::PushItemWidth(-1.0f);
			ImGui::Combo("##uiselect", &ui_select, menus, 8);
			ImGui::PopItemWidth();
			ImGui::Separator();
			ImGui::PushAllowKeyboardFocus(false);

			switch (ui_select) {
				case 0: {
					drawMainUI();
				} break;
				case 1: {
					if (stage) {
						stage->drawUI(campos);
					} else {
						ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No stage is loaded");
					}
				} break;
				case 2: {
					if (stage) {
						stage->drawVisibilityUI(campos);
					} else {
						ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No stage is loaded");
					}
				} break;
				case 3: {
					if (txd) {
						txd->drawUI();
					} else {
						ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No TXD is loaded");
					}
				} break;
				case 4: {
					if (txc) {
						txc->drawUI(txd);
					} else {
						ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No TXC is loaded");
					}
				} break;
				case 5: {
					if (stage) {
						stage->drawLayoutUI(campos);
					} else {
						ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "No stage is loaded");
					}
				} break;
				/*
				case 6: { 
					// DFF Cache
				} break; 
				*/
				case 7: {
					help->drawUI();
				} break;
				default: {
					ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "Invalid menu selection");
				}
			}
			ImGui::PopAllowKeyboardFocus();
		}
		ImGui::End();
		end_ui:

		if (showTestWindow && !screenshotNextFrame) {
			ImGui::ShowTestWindow(&showTestWindow);
		}

		if (txc) {
			txc->setTime(mTime);
		}

		// play DMA animation
		if (useDMA && dma && dff && one) {
			dma->advanceTime(dt);
			dma->dumpDebug();

			int best_target = 0;
			float best_weight = 0.5f;

			std::vector<float> weights;
			
			for (int i = 0; i < dma->getTargetCount(); i++) {
				const auto weight = dma->getTargetValue(i);
				if (weight > best_weight) {
					best_target = i + 1;
					best_weight = weight;
				}
				weights.push_back(weight);
			}

			if (true /*best_target != cur_target*/) {
				cur_target = best_target;
				if (dff) delete dff;
				dff = new DFFModel();
				Buffer b = one->readFile(archiveDFFs[dffSelect]);
				rw::ClumpChunk* clump = (rw::ClumpChunk*) rw::readChunk(b);
				dff->setFromClump(clump, txd, &weights);
				delete clump;
			}
		}

		// calculate picking view
		double mouseX;
		double mouseY;
		glfwGetCursorPos(this->mWindow, &mouseX, &mouseY);

		// below copied directly from https://github.com/bkaradzic/bgfx/blob/master/examples/30-picking/picking.cpp
		// Set up picking pass
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 proj = camera.getPerspMatrix();
		float viewProj[16];
		bx::mtxMul(viewProj, &view[0][0], &proj[0][0]);

		float invViewProj[16];
		bx::mtxInverse(invViewProj, viewProj);

		// Mouse coord in NDC
		float mouseXNDC = ( mouseX             / (float)getWidth() ) * 2.0f - 1.0f;
		float mouseYNDC = ((getHeight() - mouseY) / (float)getHeight()) * 2.0f - 1.0f;

		float pickEye[3];
		float mousePosNDC[3] = { mouseXNDC, mouseYNDC, 0.0f };
		bx::vec3MulMtxH(pickEye, mousePosNDC, invViewProj);

		float pickAt[3];
		float mousePosNDCEnd[3] = { mouseXNDC, mouseYNDC, 0.1f };
		bx::vec3MulMtxH(pickAt, mousePosNDCEnd, invViewProj);

		// Look at our unprojected point
		float pickView[16];
		bx::mtxLookAt(pickView, pickEye, pickAt);

		// Tight FOV is best for picking
		float pickProj[16];
		bx::mtxProj(pickProj, 2, 1, 0.1f, 10000.0f, bgfx::getCaps()->homogeneousDepth);

		// View rect and transforms for picking pass
		bgfx::setViewFrameBuffer(1, picking_fb);
		bgfx::setViewRect(1, 0, 0, PICK_SIZE, PICK_SIZE);
		bgfx::setViewTransform(1, pickView, pickProj);


		if (stage) {
			stage->draw(campos, txc, true);
		}

		if (dff) {
			dff->draw(glm::vec3(0,0,0), dffRenderBits);
		}

		if (screenshotNextFrame == 1) {
			screenshotNextFrame++;
		} else if (screenshotNextFrame) {
			screenshot();
			screenshotNextFrame = 0;
		}

		// perform selection from readback result
		if (readback_frame && readback_frame == current_frame) {
			readback_frame = 0;

			log_info("READBACK ACHIEVED");
			log_debug("pixel[0][0] = #%02x%02x%02x%02x", blit_data[0], blit_data[1], blit_data[2], blit_data[3]);

			u16 idx = blit_data[0] | (blit_data[1] << 8);
			u8 list = blit_data[2];

			log_debug("selected %d:%d", list, idx);
			setSelectedObject(list, idx);
		}

		// perform readback for selection
		if (!readback_frame &&
				glfwGetMouseButton(this->mWindow, GLFW_MOUSE_BUTTON_LEFT) &&
				!ImGui::GetIO().WantCaptureMouse && txd) {
			if ((bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_BLIT) &&
				(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_READ_BACK)) {
				bgfx::blit(2, blitted, 0, 0, picking_rt);
				bgfx::readTexture(blitted, blit_data);
				bgfx::frame();
				bgfx::frame();
				readback_frame = current_frame + 1;
			} else {
				bgfx::readTexture(picking_rt, blit_data);
				bgfx::frame();
				bgfx::frame();
				readback_frame = current_frame + 1;
			}
		}

		current_frame++;
	}

public:
	Camera* getCamera() {
		return &camera;
	}
};

static RailCanyonApp app;

// todo: make Camera singleton so this awkward workaround isn't needed
Camera* getCamera() {
	return app.getCamera();
}

#include "io/ONEArchive.hh"
#include "chunk.hh"
int main( int argc, char** argv ) {
	BgfxCallback callback;
	return app.run( argc, argv, bgfx::RendererType::Count, 0, 0, &callback, nullptr );
}
