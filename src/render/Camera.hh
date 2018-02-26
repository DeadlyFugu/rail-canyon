// Basic camera class

#pragma once
#include <bigg.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

class Camera {
	vec3 pos;
	vec3 dir;
	float fov;
	float near;
	float far;
	float aspect;

	mat4 matView;
	mat4 matPersp;
	bool matViewDirty;
	bool matPerspDirty;
public:
	Camera(vec3 position, vec3 target, float fov, float near, float far);

	mat4 getViewMatrix();
	mat4 getPerspMatrix();
	void use(int view, float aspect);

	void setPosition(vec3 pos);
	void lookAt(vec3 target);
	void setFov(float fov);
	void setClipping(float near, float far);

	void inputMoveAxis(vec3 axis, float dt);
	void inputLookAxis(vec2 axis, float dt);

	vec3 getPosition();
	vec3 getDirection();
};

