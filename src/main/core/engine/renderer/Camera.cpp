#include "Camera.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/scene/bounding/BoundingVolume.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/terrain/Planet.h"
#include "core/util/Logger.h"
#include "core/util/InputHandler.h"
#include "core/util/Time.h"
#include <GL/glew.h>


Camera::Camera(float fov, float aspect, float near, float far):
	position(), frustumPosition(), orientation(1, 0, 0, 0), viewMatrix(1), projectionMatrix(1), viewProjectionMatrix(1), viewAxis(1) {

	this->setPerspective(fov, aspect, near, far);
	this->viewChanged = true;
	this->updateFrustum = true;
}

Camera::Camera(float left, float right, float bottom, float top, float near, float far):
	position(), frustumPosition(), orientation(1, 0, 0, 0), viewMatrix(1), projectionMatrix(1), viewProjectionMatrix(1), viewAxis(1) {

	this->setFrustum(left, right, bottom, top, near, far);
	this->viewChanged = true;
	this->updateFrustum = true;
}

Camera::~Camera() {
	delete this->frustum;
}

void Camera::render(double partialTicks, double dt) {
	if (this->viewChanged) {

		float lenSq = dot(this->orientation, this->orientation);
		if (lenSq < 1e-6 || isnan(lenSq) || isinf(lenSq))
			this->orientation = quat(1, 0, 0, 0);

		this->viewAxis = mat3_cast(this->orientation);
		this->invViewMatrix[0] = dvec4(this->viewAxis[0], 0.0);
		this->invViewMatrix[1] = dvec4(this->viewAxis[1], 0.0);
		this->invViewMatrix[2] = dvec4(this->viewAxis[2], 0.0);
		this->invViewMatrix[3] = dvec4(vec3(0.0)/*this->position * double(Planet::scaleFactor)*/, 1.0);
		this->viewMatrix = inverse(this->invViewMatrix);
		this->viewProjectionMatrix = this->projectionMatrix * this->viewMatrix;
		this->invViewProjectionMatrix = inverse(this->viewProjectionMatrix);

		dmat4 viewerTransform(1.0);
		viewerTransform[3][0] = -this->position.x;
		viewerTransform[3][1] = -this->position.y;
		viewerTransform[3][2] = -this->position.z;

		if (this->frustum == NULL) {
			this->frustum = new Frustum(this->viewProjectionMatrix * viewerTransform);
		}

		if (INPUT_HANDLER.keyDown(KEY_LCTRL) || INPUT_HANDLER.keyDown(KEY_RCTRL)) {
			if (INPUT_HANDLER.keyPressed(KEY_F)) {
				this->position = this->frustumPosition;
				this->orientation = this->frustumOrientation;
				this->viewAxis = this->frustumViewAxis;
			}
		} else {
			if (INPUT_HANDLER.keyPressed(KEY_F)) {
				this->updateFrustum = !this->updateFrustum;
			}
		}

		if (updateFrustum) {
			this->frustumPosition = this->position;
			this->frustumOrientation = this->orientation;
			this->frustumViewAxis = this->viewAxis;
			this->frustum->set(this->viewProjectionMatrix * viewerTransform);
		} else {
			this->frustum->renderDebug(true);

			//Ray ray = Ray(this->position, this->getForwardAxis());
			//
			//double dist;
			//if (this->frustum->intersectsRay(ray, &dist)) {
			//	logInfo("Intersects ray");
			//
			//	dvec3 intersectionPoint = ray.orig + ray.dir * dist;
			//
			//	const int32 c = 12;
			//	dvec3 p[c];
			//	int32 i[c];
			//	dmat4 m(1.0);
			//	m[3] = dvec4(intersectionPoint, 1.0);
			//
			//	p[0] = dvec3(0.0);
			//	i[0] = 0;
			//
			//	for (int idx = 0; idx < c - 1; idx++) {
			//		double theta = (double)idx / (c - 2) * TWO_PI;
			//		p[idx + 1] = dvec3(cos(theta), sin(theta), 0.0);
			//		i[idx + 1] = idx + 1;
			//	}
			//
			//	glDisable(GL_CULL_FACE);
			//	SCENE_GRAPH.renderDebug(GL_TRIANGLE_FAN, c, p, i, m);
			//	glEnable(GL_CULL_FACE);
			//}
		}
	}
}

void Camera::applyUniforms(ShaderProgram* shader) const {
	if (shader != NULL) {
		float fp = this->getFarPlane();
		float np = this->getNearPlane();
		shader->setUniform("viewProjectionMatrix", fmat4(this->viewProjectionMatrix));
		shader->setUniform("invViewProjectionMatrix", fmat4(this->invViewProjectionMatrix));
		shader->setUniform("projectionMatrix", fmat4(this->projectionMatrix));
		shader->setUniform("invProjectionMatrix", fmat4(this->invProjectionMatrix));
		shader->setUniform("viewMatrix", fmat4(this->viewMatrix));
		shader->setUniform("invViewMatrix", fmat4(this->invViewMatrix));
		shader->setUniform("depthCoefficient", float(2.0 / log2(fp + 1.0)));
		shader->setUniform("nearPlane", fp);
		shader->setUniform("farPlane", np);
	}
}

dvec3 Camera::getPosition(bool ignoreFrustum) const {
	if (ignoreFrustum) {
		return this->position;
	} else {
		return this->frustumPosition;
	}
}

void Camera::setPosition(dvec3 position) {
	if (this->position != position) {
		this->viewChanged = true;
	}
	this->position = position;
}

void Camera::move(dvec3 distance) {
	this->setPosition(this->position + distance);
}

dmat3 Camera::getAxis() const {
	return this->viewAxis;
}

void Camera::setOrientation(dmat3 orientation) {
	this->orientation = quat_cast(orientation);
	this->viewAxis = dmat3(orientation);

	viewChanged = true;
}

void Camera::setOrientation(quat orientation) {
	this->orientation = normalize(orientation);
	this->viewAxis = mat3_cast(this->orientation);

	viewChanged = true;
}

quat Camera::getOrientation() const {
	return quat_cast(this->viewAxis);
}

void Camera::setOrientation(dvec3 forward, dvec3 up) {
	fvec3 z = forward;
	fvec3 y = up;

	float lenSq;

	if (fabs((lenSq = dot(z, z)) - 1.0) > 1e-6)
		z /= sqrt(lenSq);
	
	if (fabs((lenSq = dot(y, y)) - 1.0) > 1e-6)
		y /= sqrt(lenSq);

	this->setOrientation(glm::quatLookAtLH(z, y));
}

void Camera::rotate(quat rotation) {
	this->setOrientation(rotation * this->orientation);
}

void Camera::rotate(fvec3 axis, float angle) {
	this->rotate(glm::angleAxis(angle, axis));
}

void Camera::lookAt(dvec3 center, dvec3 up) {
	this->setOrientation(center - this->position, up);
}

float Camera::getFieldOfView() const {
	return this->fov;
}

float Camera::getAspectRatio() const {
	return this->aspect;
}

float Camera::getNearPlane() const {
	return this->frustumDistances[FRUSTUM_NEAR];
}

float Camera::getFarPlane() const {
	return this->frustumDistances[FRUSTUM_FAR];
}

void Camera::setFieldOfView(float fov) {
	this->setPerspective(fov, this->aspect, this->frustumDistances[FRUSTUM_NEAR], this->frustumDistances[FRUSTUM_FAR]);
}

void Camera::setAspectRatio(float aspect) {
	this->setPerspective(this->fov, aspect, this->frustumDistances[FRUSTUM_NEAR], this->frustumDistances[FRUSTUM_FAR]);
}

void Camera::setPerspective(float fov, float aspect, float near, float far) {
	float y = near * tan(fov * 0.5);
	float x = y * aspect;
	this->setFrustum(-x, +x, -y, +y, near, far);
}

void Camera::setFrustum(float left, float right, float bottom, float top, float near, float far) {
	this->frustumDistances[FRUSTUM_LEFT] = left;
	this->frustumDistances[FRUSTUM_RIGHT] = right;
	this->frustumDistances[FRUSTUM_BOTTOM] = bottom;
	this->frustumDistances[FRUSTUM_TOP] = top;
	this->frustumDistances[FRUSTUM_NEAR] = near;
	this->frustumDistances[FRUSTUM_FAR] = far;

	this->fov = fabs(2.0 * atan2(top, near));
	this->aspect = fabs(right - left) / fabs(top - bottom);

	this->projectionMatrix = glm::frustumLH(left, right, bottom, top, near, far);
	this->invProjectionMatrix = inverse(this->projectionMatrix);

	this->viewChanged = true;
}

Ray Camera::getPickingRay(fvec2 screenPos) const {
	fvec4 coord = fvec4(screenPos * fvec2(1.0, -1.0), -1.0, 1.0); // clip space
	coord = this->invProjectionMatrix * coord; // eye space
	coord = fvec4(coord.x, coord.y, 1.0, 0.0);
	coord = this->invViewMatrix * coord; // world space

	return Ray(this->position, dvec3(coord));
}

dvec3 Camera::getLeftAxis() const {
	return this->viewAxis[0];
}

dvec3 Camera::getUpAxis() const {
	return this->viewAxis[1];
}

dvec3 Camera::getForwardAxis() const {
	return this->viewAxis[2];
}

dmat4 Camera::getViewMatrix() const {
	return this->viewMatrix;
}

dmat4 Camera::getProjectionMatrix() const {
	return this->projectionMatrix;
}

dmat4 Camera::getViewProjectionMatrix() const {
	return this->viewProjectionMatrix;
}

dmat4 Camera::getInvViewProjectionMatrix() const {
	return this->invViewProjectionMatrix;
}

Frustum* Camera::getFrustum() const {
	return this->frustum;
}
