#pragma once

#include "core/Core.h"

class ShaderProgram;
class Frustum;
struct Ray;
enum FrustumPlane;

class Camera {
private:
	dvec3 position;
	quat orientation;
	dmat3 viewAxis;

	dvec3 frustumPosition;
	quat frustumOrientation;
	dmat3 frustumViewAxis;

	dmat4 viewMatrix;
	dmat4 projectionMatrix;
	dmat4 viewProjectionMatrix;
	dmat4 invViewMatrix;
	dmat4 invProjectionMatrix;
	dmat4 invViewProjectionMatrix;

	bool viewChanged;
	bool updateFrustum;

	float fov;
	float aspect;

	float frustumDistances[6];

	Frustum* frustum;

public:
	// TODO: find closest and furthest parts of the scene, and position near/far plane accordingle.
	// Maybe find distance to nearest and furthest pixels in postprocessing shader, and use these distances next frame
	Camera(float fov = HALF_PI, float aspect = 4.0 / 3.0, float near = 100.0, float far = 2000000.0);

	Camera(float left, float right, float bottom, float top, float near = 100.0, float far = 2000000.0);

	~Camera();

	void render(double partialTicks, double dt);

	void applyUniforms(ShaderProgram* shader) const;

	dvec3 getPosition(bool ignoreFrustum = false) const;

	void setPosition(dvec3 position);

	void move(dvec3 distance);

	dmat3 getAxis() const;

	quat getOrientation() const;

	void setOrientation(dmat3 orientation);

	void setOrientation(quat orientation);

	void setOrientation(dvec3 forward, dvec3 up);

	void rotate(quat rotation);

	void rotate(fvec3 axis, float angle);

	void lookAt(dvec3 center, dvec3 up = dvec3(0.0, 1.0, 0.0));

	float getFieldOfView() const;

	float getAspectRatio() const;

	float getNearPlane() const;

	float getFarPlane() const;

	void setFieldOfView(float fov);

	void setAspectRatio(float aspect);

	void setPerspective(float fov, float aspect, float near, float far);

	void setFrustum(float left, float right, float bottom, float top, float near, float far);

	Ray getPickingRay(fvec2 screenPos) const;

	dvec3 getLeftAxis() const;

	dvec3 getUpAxis() const;

	dvec3 getForwardAxis() const;

	dmat4 getViewMatrix() const;

	dmat4 getProjectionMatrix() const;

	dmat4 getViewProjectionMatrix() const;

	dmat4 getInvViewProjectionMatrix() const;

	Frustum* getFrustum() const;
};

