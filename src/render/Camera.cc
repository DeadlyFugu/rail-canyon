// $DESC$
// © Sekien 2018
#include "Camera.hh"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "common.hh"

static const vec3 U(0, 1, 0);

Camera::Camera(vec3 position, vec3 target, float fov, float near, float far) {
	setPosition(position);
	lookAt(target);
	setFov(fov);
	setClipping(near, far);
	aspect = 1.0f;
}

mat4 Camera::getViewMatrix() {
	if (matViewDirty) {
		matView = glm::lookAt(pos, pos + dir, U);
		matViewDirty = false;
	}
	return matView;
}

mat4 Camera::getPerspMatrix() {
	if (matPerspDirty) {
		matPersp = bigg::perspective(glm::radians(fov), aspect, near, far);
		matPerspDirty = false;
	}
	return matPersp;
}

void Camera::setPosition (vec3 pos) {
	this->pos = pos;
	matViewDirty = true;
}

void Camera::lookAt(vec3 target) {
	this->dir = glm::normalize(target - pos);
	matViewDirty = true;
}

void Camera::setFov(float fov) {
	this->fov = fov;
	matPerspDirty = true;
}

void Camera::setClipping(float near, float far) {
	this->near = near;
	this->far = far;
	matPerspDirty = true;
}

void Camera::inputMoveAxis(vec3 axis, float dt) {
	float moveSensitivity = 3.0f; // todo: make (global?) param
	if (axis.x != 0 || axis.y != 0 || axis.z != 0) {
		axis *= moveSensitivity * (dt / 0.01667f);
		vec3 D = dir;
		vec3 R = cross(D, U);
		pos += R * axis.x + D * axis.z + U * axis.y;
		matViewDirty = true;
	}
}

inline float clamp(float x, float min, float max) {
	return (x < min ? min : (x > max ? max : x));
}

void Camera::inputLookAxis(vec2 axis, float dt) {
	float lookSensitivity = 1.4f;
	if (axis.x != 0 || axis.y != 0) {
		float pitch = asinf(dir.y);
		float yaw = acosf(clamp(dir.z / cosf(pitch), -1, 1));
		if (dir.x < 0) yaw = -yaw;

		axis *= lookSensitivity * dt;
		yaw -= axis.x;
		pitch = clamp(pitch + axis.y, -1.5f, 1.5f);

		dir.x = sinf(yaw) * cosf(pitch);
		dir.y = sinf(pitch);
		dir.z = cosf(yaw) * cosf(pitch);
		dir = glm::normalize(dir);
		matViewDirty = true;


		/*axis *= lookSensitivity;
		//axis *= axis;
		vec3 D = dir;
		vec3 R = cross(D, U);
		vec3 localU = cross(R, D);
		dir += R * axis.x + localU * axis.y;
		matViewDirty = true;*/
	}
}

vec3 Camera::getPosition() {
	return pos;
}

vec3 Camera::getDirection() {
	return dir;
}

void Camera::use(int view, float aspect) {
	if (this->aspect != aspect) {
		this->aspect = aspect;
		matPerspDirty = true;
	}
	glm::mat4 viewMatrix = getViewMatrix(); //
	glm::mat4 projMatrix = getPerspMatrix(); //
	bgfx::setViewTransform( view, &viewMatrix[0][0], &projMatrix[0][0] );
}
