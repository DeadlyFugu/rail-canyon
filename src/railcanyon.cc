
#include <bigg.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "stage.hh"

#include "render/Camera.hh"
#include "render/TexDictionary.hh"

class RailCanyonApp : public bigg::Application {
private:
	Camera camera;
	float mTime;
	Stage* stage;
	TexDictionary* txd;
public:
	RailCanyonApp() : camera(glm::vec3(0.f, 100.f, 350.f), glm::vec3(0,0,0), 60, 1.f, 960000.f) {}
private:
	void initialize( int _argc, char** _argv ) override {
		bgfx::setDebug( BGFX_DEBUG_TEXT );
		mTime = 0.0f;

#define STG_NAME "stg07"
		rc::util::FSPath dvdroot(_argc > 1 ? _argv[1] : "D:\\Heroes\\dvdroot");
		rc::util::FSPath onePath = dvdroot / STG_NAME ".one";
		rc::util::FSPath blkPath = dvdroot / STG_NAME "_blk.bin";
		rc::io::ONEArchive archive(onePath);

		// read TXD
		rc::util::FSPath txdPath = dvdroot / "textures/" STG_NAME ".txd";
		rc::util::FSFile txdFile(txdPath);
		auto x = txdFile.toBuffer();
		sk::Buffer sk_x(x.base_ptr(), x.size(), false); // convert buffer utility classes
		rw::Chunk* root = rw::readChunk(sk_x);
		root->dump(rw::util::DumpWriter());
		txd = new TexDictionary((rw::TextureDictionary*) root);

		// read BSPs
		stage = new Stage();
		stage->fromArchive(&archive, txd);
		stage->readVisibility(blkPath);
	}

	int shutdown() override {
		delete stage;
		delete txd;
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
		ImGui::Begin("Camera");
		ImGui::DragFloat3("eye", &eye[0]);
		ImGui::End();
		ImGui::ShowTestWindow(nullptr);
		txd->showWindow();

		stage->draw(camera.getPosition());
	}
};

#include "io/ONEArchive.hh"
#include "chunk.hh"
int main( int argc, char** argv ) {
	RailCanyonApp app;
	return app.run( argc, argv );
}
