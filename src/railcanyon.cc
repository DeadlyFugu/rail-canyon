
#include <bigg.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "stage.hh"

#include "render/Camera.hh"
#include "render/TexDictionary.hh"

const char* stageDisplayNames[] = {
		"<none>",
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

class RailCanyonApp : public bigg::Application {
private:
	Camera camera;
	float mTime = 0.0f;
	Stage* stage = nullptr;
	TexDictionary* txd = nullptr;
	int stageSelect = 0;
	char dvdroot[512] = "D:\\Heroes\\dvdroot\0";
public:
	RailCanyonApp() : camera(glm::vec3(0.f, 100.f, 350.f), glm::vec3(0,0,0), 60, 1.f, 960000.f) {}
private:
	void openTXD(const rc::util::FSPath& path) {
		rc::util::FSPath txdPath(path);
		rc::util::FSFile txdFile(txdPath);
		auto x = txdFile.toBuffer();
		sk::Buffer sk_x(x.base_ptr(), x.size(), false); // convert buffer utility classes
		rw::Chunk* root = rw::readChunk(sk_x);
		root->dump(rw::util::DumpWriter());
		txd = new TexDictionary((rw::TextureDictionary*) root);
	}

	void openBSPWorld(rc::util::FSPath& onePath, rc::util::FSPath& blkPath) {
		// read BSPs
		rc::io::ONEArchive archive(onePath);
		stage = new Stage();
		stage->fromArchive(&archive, txd);
		stage->readVisibility(blkPath);
	}

	void openStage(const char* name) {
		char buffer[512];
		sprintf(buffer, "%s/textures/%s.txd", dvdroot, name);
		rc::util::FSPath txdPath(buffer);
		sprintf(buffer, "%s/%s.one", dvdroot, name);
		rc::util::FSPath onePath(buffer);
		sprintf(buffer, "%s/%s_blk.bin", dvdroot, name);
		rc::util::FSPath blkPath(buffer);

		if (!txdPath.exists()) {
			rw::util::logger.error("missing .txd");
		} else if (!onePath.exists()) {
			rw::util::logger.error("missing .one");
		} else if (!blkPath.exists()) {
			rw::util::logger.error("missing _blk.bin");
		} else {
			openTXD(txdPath);
			openBSPWorld(onePath, blkPath);
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
	}
	void initialize( int _argc, char** _argv ) override {
		bgfx::setDebug( BGFX_DEBUG_TEXT );
		mTime = 0.0f;
		reset(BGFX_RESET_VSYNC);
	}

	int shutdown() override {
		closeStage();
		return 0;
	}

	void onReset() override {
		bgfx::setViewClear( 0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0 );
		bgfx::setViewRect( 0, 0, 0, uint16_t( getWidth() ), uint16_t( getHeight() ) );
	}

	void update( float dt ) override {
		mTime += dt;
		static auto eye = glm::vec3(0.0f, 10.0f, 35.0f );
//		glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f ), glm::vec3(0.0f, 1.0f, 0.0f ) );
//		glm::mat4 proj = bigg::perspective( glm::radians( 60.0f ), float( getWidth() ) / getHeight(), 0.1f, 16000.0f );
//		bgfx::setViewTransform( 0, &view[0][0], &proj[0][0] );
		glm::vec3 inputMove(0,0,0);
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
		camera.inputMoveAxis(inputMove, dt);

		glm::vec2 inputLook(0,0);
		if (glfwGetKey(this->mWindow, GLFW_KEY_UP))
			inputLook.y += 1;
		if (glfwGetKey(this->mWindow, GLFW_KEY_DOWN))
			inputLook.y -= 1;
		if (glfwGetKey(this->mWindow, GLFW_KEY_RIGHT))
			inputLook.x += 1;
		if (glfwGetKey(this->mWindow, GLFW_KEY_LEFT))
			inputLook.x -= 1;
		camera.inputLookAxis(inputLook, dt);


		if (glfwGetKey(this->mWindow, GLFW_KEY_SPACE))
			camera.lookAt(vec3(0,0,0));
		//camera.setPosition(eye);
		//camera.lookAt(glm::vec3(0,0,0));
		camera.use(0, (float) getWidth() / getHeight());
		bgfx::setViewRect( 0, 0, 0, uint16_t( getWidth() ), uint16_t( getHeight() ) );
		bgfx::touch( 0 );
		ImGui::Begin("RailCanyon v0.1");

		static bool dvdrootExistsDirty = true;
		static bool dvdrootExists = false;
		if (ImGui::InputText("dvdroot", dvdroot, 512)) {
			dvdrootExistsDirty = true;
		}
		if (dvdrootExistsDirty) {
			rc::util::FSPath dvdrootPath(dvdroot);
			dvdrootExists = dvdrootPath.exists();
		}
		if (!dvdrootExists) {
			ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.0f, 1.0f), "Path does not exist");
		}

		auto campos = camera.getPosition();
		if (ImGui::Combo("stage", &stageSelect, stageDisplayNames, sizeof(stageDisplayNames) / sizeof(*stageDisplayNames))) {
			closeStage();
			if (stageSelect) {
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
		ImGui::LabelText("campos", "%f, %f, %f", campos.x, campos.y, campos.z);
		static bool vsync = true;
		if (ImGui::Checkbox("vsync", &vsync)) {
			reset(vsync ? BGFX_RESET_VSYNC : 0);
		}
		ImGui::End();

		static bool showOverlay = true;
		ImGui::SetNextWindowPos(ImVec2(10,10));
		if (ImGui::Begin("dispoverlay", &showOverlay, ImVec2(240,0), 0.3f, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::Text("Stage: %s", stageDisplayNames[stageSelect]);
			ImGui::Separator();
			ImGui::Text("Cam: (%.0f, %.0f, %.0f)", campos.x, campos.y, campos.z);
			ImGui::Text("FPS: %0.3f", 1.0f / dt);
		}
		ImGui::End();

		ImGui::ShowTestWindow(nullptr);

		if (txd)
			txd->showWindow();

		if (stage)
			stage->draw(campos);
	}
};

#include "io/ONEArchive.hh"
#include "chunk.hh"
int main( int argc, char** argv ) {
	RailCanyonApp app;
	return app.run( argc, argv );
}
