#include "Planet.h"
#include "core/application/Application.h"
#include "core/engine/geometry/MeshData.h"
#include "core/engine/renderer/Camera.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/renderer/ScreenRenderer.h"
#include "core/engine/renderer/DebugRenderer.h"
#include "core/engine/renderer/postprocess/AtmosphereRenderer.h"
#include "core/engine/terrain/TerrainQuad.h"
#include "core/engine/terrain/TerrainRenderer.h"
#include "core/engine/terrain/Atmosphere.h"
#include "core/engine/terrain/MapGenerator.h"
#include "core/engine/terrain/TileSupplier.h"
#include "core/engine/scene/bounding/BoundingVolume.h"
#include "core/util/InputHandler.h"
#include "core/util/Time.h"
#include <GL/glew.h>

// Scale factor in kilometers per OpenGL unit
double Planet::minScaleFactor = 0.001;
double Planet::maxScaleFactor = 1000.0;
double Planet::scaleFactor = 1.0;
double Planet::currScaleFactor = 1.0;
double Planet::prevScaleFactor = 1.0;

Planet::Planet(dvec3 center, double radius, double splitThreshold, int32 maxSplitDepth) :
	GameObject(), center(center), radius(radius), invRadius(1.0 / radius), splitThreshold(splitThreshold), maxSplitDepth(maxSplitDepth) {

	this->tileSupplier = new TileSupplier(this);
	this->terrainRenderer = new TerrainRenderer(16);
	this->atmosphere = new Atmosphere(this);

	this->faceOrientations[X_NEG][0] = fvec3(0, 0, +1);
	this->faceOrientations[X_NEG][1] = fvec3(-1, 0, 0);
	this->faceOrientations[X_NEG][2] = fvec3(0, -1, 0);

	this->faceOrientations[X_POS][0] = fvec3(0, 0, -1);
	this->faceOrientations[X_POS][1] = fvec3(+1, 0, 0);
	this->faceOrientations[X_POS][2] = fvec3(0, -1, 0);

	this->faceOrientations[Y_NEG][0] = fvec3(+1, 0, 0);
	this->faceOrientations[Y_NEG][1] = fvec3(0, -1, 0);
	this->faceOrientations[Y_NEG][2] = fvec3(0, 0, -1);

	this->faceOrientations[Y_POS][0] = fvec3(+1, 0, 0);
	this->faceOrientations[Y_POS][1] = fvec3(0, +1, 0);
	this->faceOrientations[Y_POS][2] = fvec3(0, 0, +1);

	this->faceOrientations[Z_NEG][0] = fvec3(-1, 0, 0);
	this->faceOrientations[Z_NEG][1] = fvec3(0, 0, -1);
	this->faceOrientations[Z_NEG][2] = fvec3(0, -1, 0);

	this->faceOrientations[Z_POS][0] = fvec3(+1, 0, 0);
	this->faceOrientations[Z_POS][1] = fvec3(0, 0, +1);
	this->faceOrientations[Z_POS][2] = fvec3(0, -1, 0);

	this->faces[X_NEG] = new TerrainQuad(this, X_NEG);
	this->faces[X_POS] = new TerrainQuad(this, X_POS);
	this->faces[Y_NEG] = new TerrainQuad(this, Y_NEG);
	this->faces[Y_POS] = new TerrainQuad(this, Y_POS);
	this->faces[Z_NEG] = new TerrainQuad(this, Z_NEG);
	this->faces[Z_POS] = new TerrainQuad(this, Z_POS);

	// left, top, right, bottom
	this->faces[X_NEG]->updateNeighbours();
	this->faces[X_POS]->updateNeighbours();
	this->faces[Y_NEG]->updateNeighbours();
	this->faces[Y_POS]->updateNeighbours();
	this->faces[Z_NEG]->updateNeighbours();
	this->faces[Z_POS]->updateNeighbours();
	// this->faces[X_NEG]->setNeighbours(this->faces[Z_POS], this->faces[Y_POS], this->faces[Z_NEG], this->faces[Y_NEG]);
	// this->faces[X_POS]->setNeighbours(this->faces[Z_NEG], this->faces[Y_POS], this->faces[Z_POS], this->faces[Y_NEG]);
	// this->faces[Y_NEG]->setNeighbours(this->faces[X_NEG], this->faces[Z_NEG], this->faces[X_POS], this->faces[Z_POS]);
	// this->faces[Y_POS]->setNeighbours(this->faces[X_NEG], this->faces[Z_POS], this->faces[X_POS], this->faces[Z_NEG]);
	// this->faces[Z_NEG]->setNeighbours(this->faces[X_NEG], this->faces[Y_POS], this->faces[X_POS], this->faces[Y_NEG]);
	// this->faces[Z_POS]->setNeighbours(this->faces[X_POS], this->faces[Y_POS], this->faces[X_NEG], this->faces[Y_NEG]);

	this->renderDebugQuadBounds = false;
	this->tileSupplierDebugState = 0;

	this->elevationUnderCamera = 0.0;
	this->elevationScale = 24.0;
	this->horizonRadius = this->radius - this->elevationScale;
	this->invHorizonRadius = 1.0 / this->horizonRadius;

	this->mapGenerator = new MapGenerator(this);
}

Planet::~Planet() {
	delete this->terrainRenderer;
	delete this->tileSupplier;

	for (int i = 0; i < 6; i++) {
		delete this->faces[i];
	}
	delete[] this->faces;

}

void Planet::render(SceneGraph * sceneGraph, double partialTicks, double dt) {
	GameObject::render(sceneGraph, partialTicks, dt);

	uint64 a = Time::now();

	Planet::scaleFactor = Planet::prevScaleFactor * (1.0 - partialTicks) + Planet::currScaleFactor * partialTicks;

	dvec3 cameraPosition = SCENE_GRAPH.getCamera()->getPosition();
	this->localCameraPosition = this->worldToLocalPoint(cameraPosition);
	this->closestCameraFace = this->getClosestFaceLocal(this->localCameraPosition);
	this->faceCameraPosition = this->localToCubeFacePoint(this->localCameraPosition);

	this->elevationUnderCamera = this->faces[this->closestCameraFace]->getElevation(this->faceCameraPosition);
	//logInfo("Elevation at [%f, %f] = %f", this->faceCameraPosition.x, this->faceCameraPosition.z, this->elevationUnderCamera);

	if (INPUT_HANDLER.keyPressed(KEY_F2)) {
		this->debugRenderState = (this->debugRenderState + 1) % 3;
	}

	if (this->debugRenderState == 0) { // Render normal
		this->mapGenerator->setRenderDebugSurface(false);
		this->mapGenerator->setRenderDebugCurrents(false);
		SCREEN_RENDERER.setExposureEnabled(true);
		SCREEN_RENDERER.setGammaCorrectionEnabled(true);

		this->terrainRenderer->render(this, X_NEG, this->faces[X_NEG], partialTicks, dt);
		this->terrainRenderer->render(this, X_POS, this->faces[X_POS], partialTicks, dt);
		this->terrainRenderer->render(this, Y_NEG, this->faces[Y_NEG], partialTicks, dt);
		this->terrainRenderer->render(this, Y_POS, this->faces[Y_POS], partialTicks, dt);
		this->terrainRenderer->render(this, Z_NEG, this->faces[Z_NEG], partialTicks, dt);
		this->terrainRenderer->render(this, Z_POS, this->faces[Z_POS], partialTicks, dt);
		this->tileSupplier->update();

		SCREEN_RENDERER.getAtmosphereRenderer()->addAtmosphere(this->atmosphere);
	}
	else if (this->debugRenderState == 1) { // Render map surface
		this->mapGenerator->setRenderDebugSurface(true);
		this->mapGenerator->setRenderDebugCurrents(false);
		SCREEN_RENDERER.setExposureEnabled(false);
		SCREEN_RENDERER.setGammaCorrectionEnabled(false);
	}
	else if (this->debugRenderState == 2) {// Render map currents
		this->mapGenerator->setRenderDebugSurface(true);
		this->mapGenerator->setRenderDebugCurrents(true);
		SCREEN_RENDERER.setExposureEnabled(false);
		SCREEN_RENDERER.setGammaCorrectionEnabled(false);
	}


	this->mapGenerator->render(partialTicks, dt);

	uint64 b = Time::now();
	//logInfo("Took %f ms to render terrain", (b - a) / 1000000.0);
}

void Planet::update(SceneGraph * sceneGraph, double dt) {
	GameObject::update(sceneGraph, dt);

	uint64 a = Time::now();
	bool changed = false;

	this->closestCameraDistance = INFINITY;
	this->closestCameraTerrainQuad = NULL;

	for (int i = 0; i < 6; i++) {
		this->faces[i]->update(dt);
		changed |= this->faces[i]->didChange();
	}

	this->closestCameraDistance = sqrt(this->closestCameraDistance);

	// TODO: Update this elsewhere
	dvec3 cameraPosition = this->worldToLocalPoint(SCENE_GRAPH.getCamera()->getPosition(true));
	double altitude = this->closestCameraDistance;// this->getAltitude(cameraPosition);

	double base = 3.0F;
	double logDist = glm::log(glm::max(altitude, 0.000001)) / glm::log(base);

	double nextScaleFactor = glm::clamp(glm::pow(base, base - logDist), Planet::minScaleFactor, Planet::maxScaleFactor);
	double scaleRate = 0.1;

	Planet::prevScaleFactor = currScaleFactor;
	Planet::currScaleFactor = prevScaleFactor * (1.0 - scaleRate) + nextScaleFactor * scaleRate;
	uint64 b = Time::now();
	//logInfo("Took %f ms to update terrain", (b - a) / 1000000.0);

	if (INPUT_HANDLER.keyPressed(KEY_F5)) {
		this->tileSupplierDebugState = (this->tileSupplierDebugState + 1) % 3;

		if (this->tileSupplierDebugState == 1) {
			this->tileSupplier->setOverlayDebug(true);
			this->tileSupplier->setShowDebug(false);
		}
		else if (this->tileSupplierDebugState == 2) {
			this->tileSupplier->setOverlayDebug(false);
			this->tileSupplier->setShowDebug(true);
		}
		else {
			this->tileSupplier->setOverlayDebug(false);
			this->tileSupplier->setShowDebug(false);
		}
	}

	if (INPUT_HANDLER.keyPressed(KEY_F6)) {
		this->renderDebugQuadBounds = !this->renderDebugQuadBounds;
	}


	//double intersectDist;
	//int32 w, h; Application::getWindowSize(&w, &h);
	//Ray localRay = SCENE_GRAPH.getCamera()->getPickingRay((INPUT_HANDLER.getMousePosition() / fvec2(w, h)) * 2.0F - 1.0F);
	//if (IntersectionTests::raySphereIntersection(localRay, dvec3(0.0), this->radius, false, &intersectDist)) {
	//	dvec3 intersectionPoint = localRay.orig + localRay.dir * intersectDist;
	//	uint64 a = Time::now();
	//	closestNode = this->mapGenerator->getClosestMapNode(intersectionPoint, closestNode != NULL ? closestNode->index : -1);
	//	uint64 b = Time::now();
	//
	//	logInfo("Took %f ms to find closest point", (b - a) / 1000000.0);
	//}
}

void Planet::applyUniforms(ShaderProgram * program) {
	program->setUniform("localCameraPosition", this->localCameraPosition);
	program->setUniform("elevationScale", (float)this->elevationScale);
	program->setUniform("planetRadius", (float)this->radius);
	program->setUniform("renormalizeSphere", Planet::scaleFactor < 1.0);
}

IntersectionType Planet::getVisibility(CubeFace face, BoundingVolume * bound, bool horizonTest) {
	if (bound != NULL) {

		const Camera* camera = SCENE_GRAPH.getCamera();
		const Frustum* frustum = camera->getFrustum();
		const dvec3 cameraPosition = this->worldToLocalPoint(camera->getPosition());

		if (frustum != NULL) {

			if (bound->intersectsPoint(cameraPosition)) {
				return FULL_INTERSECTION;
			}

			if (AxisAlignedBB * bb = dynamic_cast<AxisAlignedBB*>(bound)) {
				dvec3 a = bb->getMin();
				dvec3 b = bb->getMax();

				// camera is not in local space.

				dvec3 v0 = this->cubeFaceToLocalPoint(face, dvec3(a.x, a.y, a.z));
				dvec3 v1 = this->cubeFaceToLocalPoint(face, dvec3(b.x, a.y, a.z));
				dvec3 v2 = this->cubeFaceToLocalPoint(face, dvec3(b.x, a.y, b.z));
				dvec3 v3 = this->cubeFaceToLocalPoint(face, dvec3(a.x, a.y, b.z));

				for (int i = 0; i < 4; i++) {
					Plane plane = frustum->getPlane((FrustumPlane)i);

					IntersectionType it = PARTIAL_INTERSECTION;
					bool flag = IntersectionTests::getSignedPlaneDistance(v0, plane) > 0.0;

					if (IntersectionTests::getSignedPlaneDistance(v1, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v2, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v3, plane) > 0.0 == flag) {
						it = (flag ? FULL_INTERSECTION : NO_INTERSECTION);
					}

					if (it == NO_INTERSECTION) {
						return NO_INTERSECTION;
					}
				}

				if (horizonTest) {
					const bool hc0 = this->horizonOcclusion(cameraPosition, v0);
					const bool hc1 = this->horizonOcclusion(cameraPosition, v1);
					const bool hc2 = this->horizonOcclusion(cameraPosition, v2);
					const bool hc3 = this->horizonOcclusion(cameraPosition, v3);

					if (hc0 && hc1 && hc2 && hc3) {
						return NO_INTERSECTION;
					}
				}

				return FULL_INTERSECTION;
			}

			if (Frustum * bb = dynamic_cast<Frustum*>(bound)) {

				const dvec3 v0 = bb->getCorner(FRUSTUM_LEFT_TOP_NEAR);
				const dvec3 v1 = bb->getCorner(FRUSTUM_RIGHT_TOP_NEAR);
				const dvec3 v2 = bb->getCorner(FRUSTUM_RIGHT_BOTTOM_NEAR);
				const dvec3 v3 = bb->getCorner(FRUSTUM_LEFT_BOTTOM_NEAR);

				const dvec3 v4 = bb->getCorner(FRUSTUM_LEFT_TOP_FAR);
				const dvec3 v5 = bb->getCorner(FRUSTUM_RIGHT_TOP_FAR);
				const dvec3 v6 = bb->getCorner(FRUSTUM_RIGHT_BOTTOM_FAR);
				const dvec3 v7 = bb->getCorner(FRUSTUM_LEFT_BOTTOM_FAR);

				for (int i = 0; i < 4; i++) {
					Plane plane = frustum->getPlane((FrustumPlane)i);

					IntersectionType it = PARTIAL_INTERSECTION;
					bool flag = IntersectionTests::getSignedPlaneDistance(v0, plane) > 0.0;

					if (IntersectionTests::getSignedPlaneDistance(v1, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v2, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v3, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v4, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v5, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v6, plane) > 0.0 == flag
						&& IntersectionTests::getSignedPlaneDistance(v7, plane) > 0.0 == flag) {
						it = (flag ? FULL_INTERSECTION : NO_INTERSECTION);
					}

					if (it == NO_INTERSECTION) {
						return NO_INTERSECTION;
					}
				}

				if (horizonTest) {
					//bool hc0 = this->horizonOcclusion(cameraPosition, v0);
					//bool hc1 = this->horizonOcclusion(cameraPosition, v1);
					//bool hc2 = this->horizonOcclusion(cameraPosition, v2);
					//bool hc3 = this->horizonOcclusion(cameraPosition, v3);
					bool hc4 = this->horizonOcclusion(cameraPosition, v4);
					bool hc5 = this->horizonOcclusion(cameraPosition, v5);
					bool hc6 = this->horizonOcclusion(cameraPosition, v6);
					bool hc7 = this->horizonOcclusion(cameraPosition, v7);
					// find furthest point along "up" vector away from planet center and test only that
					// SAT ? project all points onto the "up" ray and test the two extremes.  both above = NO_INTERSECT, both below = FULL_INTERSECT, else PARTIAL_INTERSECT

					if (/*hc0 && hc1 && hc2 && hc3 && */hc4 && hc5 && hc6 && hc7) {
						return NO_INTERSECTION;
					}

					if (this->renderDebugQuadBounds) {
						std::vector<Vertex> v = { Vertex(v0), Vertex(v1), Vertex(v2), Vertex(v3), Vertex(v4), Vertex(v5), Vertex(v6), Vertex(v7) };
						std::vector<int32> i = {
							0, 1, 1, 2, 2, 3, 3, 0,
							4, 5, 5, 6, 6, 7, 7, 4,
							0, 4, 1, 5, 2, 6, 3, 7
						};


						DEBUG_RENDERER.draw(v, i);
					}
				}

				return FULL_INTERSECTION;
			}
		}
	}

	return NO_INTERSECTION;
}

bool Planet::horizonOcclusion(dvec3 localViewer, dvec3 localPoint) {
	const dvec3 v = localViewer * this->invHorizonRadius;
	const dvec3 t = localPoint * this->invHorizonRadius;
	const dvec3 vt = t - v;
	const dvec3 vc = -v; // viewer to center - center is at (0,0,0) in local space, so this is just -v

	const double vtDotVc = dot(vt, vc);
	const double vtLenSq = length2(vt);
	const double vcLenSq = length2(vc);

	return (vtDotVc * vtDotVc) / vtLenSq > vcLenSq - 1.0  // point is inside the horizon frustum cone.
		&& vtDotVc > vcLenSq - 1.0; // point is behind the horizon plane
}

bool Planet::horizonOcclusion(CubeFace face, BoundingVolume * bound) {
	return false;
}

//TileData* Planet::getTileData(TerrainQuad* terrainQuad) {
//	return this->tileSupplier->consumeTileData(terrainQuad);
//}

TileSupplier* Planet::getTileSupplier() const {
	return this->tileSupplier;
}

TerrainQuad* Planet::getCubeFace(CubeFace face) {
	return this->faces[face];
}

CubeFace Planet::getClosestFaceWorld(dvec3 worldPoint) {
	return this->getClosestFaceLocal(this->worldToLocalPoint(worldPoint));
}

CubeFace Planet::getClosestFaceLocal(dvec3 localPoint) {
	return this->getClosestFaceSpherical(this->localToSphericalPoint(localPoint));
}

CubeFace Planet::getClosestFaceSpherical(dvec3 sphericalPoint) {

	if (sphericalPoint.y < 1.0 * QUARTER_PI) return Y_POS; // 0 deg = north pole. y < 45 deg, closest is top
	if (sphericalPoint.y > 3.0 * QUARTER_PI) return Y_NEG; // 180 deg = south pole. y > 135 deg, closest is bottom

	float theta = fmod(sphericalPoint.x + PI, TWO_PI) - PI + QUARTER_PI;

	constexpr double x0 = -PI;
	constexpr double x1 = -HALF_PI;
	constexpr double x2 = 0.0F;
	constexpr double x3 = HALF_PI;
	constexpr double x4 = PI;

	if (theta >= x0 && theta < x1) return X_NEG;
	if (theta >= x1 && theta < x2) return Z_NEG;
	if (theta >= x2 && theta < x3) return X_POS;
	if (theta >= x3 && theta < x4) return Z_POS;

	return X_NEG; // should never happen.
}

CubeFace Planet::getClosestSphereVector(dvec3 sphereVector) {
	int maxIndex;

	const double xAbs = fabs(sphereVector.x);
	const double yAbs = fabs(sphereVector.y);
	const double zAbs = fabs(sphereVector.z);

	if (xAbs > yAbs) {
		if (xAbs > zAbs) {
			maxIndex = 0; // x is largest
		}
		else {
			maxIndex = 2; // z is largest
		}
	}
	else { // y >= x
		if (yAbs > zAbs) {
			maxIndex = 1; // y is largest
		}
		else {
			maxIndex = 2; // z is largest
		}
	}

	if (maxIndex == 0) {
		if (sphereVector.x < 0) return X_NEG;
		else return X_POS;
	}
	else if (maxIndex == 1) {
		if (sphereVector.y < 0) return Y_NEG;
		else return Y_POS;
	}
	else if (maxIndex == 2) {
		if (sphereVector.z < 0) return Z_NEG;
		else return Z_POS;
	}
	else {
		// not possible ?
		return CubeFace(-1);
	}
}

dvec3 Planet::worldToLocalPoint(dvec3 worldPoint) {
	return worldPoint - this->center;
}

Ray Planet::worldToLocalRay(Ray worldRay) {
	return Ray(worldToLocalPoint(worldRay.orig), worldRay.dir); // TODO: ortate ray
}

dvec3 Planet::localToWorldPoint(dvec3 localPoint) {
	return localPoint + this->center;
}

Ray Planet::localToWorldRay(Ray localRay) {
	return Ray(localToWorldPoint(localRay.orig), localRay.dir);
}

dvec3 Planet::localToSphericalPoint(dvec3 localPoint, bool altitude) {
	double r = altitude ? length(localPoint) : this->radius;
	double u = atan2(localPoint.z, localPoint.x); // theta, horizontal angle, longitude, distance north or south
	double v = acos(localPoint.y / r); // phi, vertical angle, latitude, distance east or west of prime meridian
	return dvec3(u, v, r - this->radius);
}

dvec3 Planet::sphericalToLocalPoint(dvec3 sphericalPoint) {
	double x = cos(sphericalPoint.x) * cos(sphericalPoint.y) * sphericalPoint.z;
	double y = sin(sphericalPoint.x) * cos(sphericalPoint.y) * sphericalPoint.z;
	double z = sin(sphericalPoint.y) * sphericalPoint.z;

	return dvec3(x, y, z);
}

dvec3 Planet::cubeFaceToLocalPoint(CubeFace face, dvec3 facePoint) {
	dvec3 n = normalize(dvec3(this->getFaceTransformation(face) * dvec4(facePoint.x, 0.0, facePoint.z, 1.0)));
	return n * (this->radius + facePoint.y);
}

dvec3 Planet::localToCubeFacePoint(dvec3 localPoint, CubeFace * faceIndex) {

	double height = length(localPoint);
	localPoint /= height;

	dvec3 absPoint = abs(localPoint);
	double f;
	dvec2 uv;
	if (absPoint.z >= absPoint.x && absPoint.z >= absPoint.y) { // z
		if (faceIndex != NULL) * faceIndex = localPoint.z < 0.0 ? Z_NEG : Z_POS;
		f = 0.5 / absPoint.z;
		uv = dvec2(localPoint.z < 0.0 ? -localPoint.x : localPoint.x, -localPoint.y);
	}
	else if (absPoint.y >= absPoint.x) { // y
		if (faceIndex != NULL) * faceIndex = localPoint.y < 0.0 ? Y_NEG : Y_POS;
		f = 0.5 / absPoint.y;
		uv = dvec2(localPoint.x, localPoint.y < 0.0 ? -localPoint.z : localPoint.z);
	}
	else { // x
		if (faceIndex != NULL) * faceIndex = localPoint.x < 0.0 ? X_NEG : X_POS;
		f = 0.5 / absPoint.x;
		uv = dvec2(localPoint.x < 0.0 ? localPoint.z : -localPoint.z, -localPoint.y);
	}

	uv = (uv * f + 0.5);// *this->getDiameter() - this->getRadius();
	return dvec3(uv.x, height - this->radius, uv.y);
}

dmat3 Planet::getFaceOrientation(CubeFace face) {
	return this->faceOrientations[face];
}

dmat3 Planet::getLocalDeformation(dvec3 localPoint, dmat3 axis) {
	dmat3 m;
	m[1] = normalize(localPoint);
	if (fabs(dot(m[1], axis[2])) < 0.999) {
		m[0] = normalize(cross(m[1], axis[2]));
	}
	else {
		m[0] = normalize(cross(m[1], axis[0]));
	}

	m[2] = normalize(cross(m[0], m[1]));

	return m;
}

dmat4 Planet::getFaceTransformation(CubeFace face) {
	mat4 m;
	mat3 r = this->getFaceOrientation(face);

	dvec3 pos = dvec3(r[1]) * this->radius;
	m[0] = dvec4(r[0], 0.0F);
	m[1] = dvec4(r[1], 0.0F);
	m[2] = dvec4(r[2], 0.0F);
	m[3] = dvec4(pos, 1.0F);
	return m;
}

dmat4 Planet::getViewerTransformation(dvec3 viewerWorldPosition) {
	dmat4 m(1.0);
	m[3][0] = this->center.x - viewerWorldPosition.x * Planet::scaleFactor;
	m[3][1] = this->center.y - viewerWorldPosition.y * Planet::scaleFactor;
	m[3][2] = this->center.z - viewerWorldPosition.z * Planet::scaleFactor;

	return m;
}

dvec3 Planet::getCenter() const {
	return this->center;
}

dvec3 Planet::getLocalCameraPosition() const {
	return this->localCameraPosition;
}

dvec2 Planet::getFaceCameraPosition() const {
	return this->faceCameraPosition;
}

CubeFace Planet::getClosestCameraFace() const {
	return this->closestCameraFace;
}

TerrainQuad* Planet::getClosestCameraTerrainQuad() const {
	return this->closestCameraTerrainQuad;
}

TerrainQuad* Planet::getRayPickTerrainQuad(Ray localRay, double* distance) {

	double intersectDist;

	if (IntersectionTests::raySphereIntersection(localRay, dvec3(0.0), this->radius, false, &intersectDist)) {
		dvec3 intersectionPoint = localRay.orig + localRay.dir * intersectDist;

		CubeFace face;

		dvec3 facePoint = this->localToCubeFacePoint(intersectionPoint, &face);

		//logInfo("%s @ %f, %f, %f",
		//	face == X_NEG ? "X_NEG" :
		//	face == X_POS ? "X_POS" :
		//	face == Y_NEG ? "Y_NEG" :
		//	face == Y_POS ? "Y_POS" :
		//	face == Z_NEG ? "Z_NEG" :
		//	face == Z_POS ? "Z_POS" : "UNKNOWN_FACE",
		//	facePoint.x, facePoint.z, facePoint
		//);

		return this->faces[face]->getTerrainQuadUnder(dvec2(facePoint.x, facePoint.z));
	}


	return NULL;
}

double Planet::getClosestCameraDistance() const {
	return this->closestCameraDistance;
}

float Planet::getAltitude(dvec3 localPosition) const {
	float distance = length(localPosition);

	return (distance - this->radius);// *Planet::scaleFactor;
}

float Planet::getHeight(dvec3 localPosition) {
	double distance = length(localPosition);

	return (distance - this->radius + this->getElevation(localPosition));// *Planet::scaleFactor;
}

float Planet::getElevation(dvec3 sphereVector) {
	sphereVector = normalize(sphereVector);

	CubeFace cubeFace = this->getClosestSphereVector(sphereVector);
	TerrainQuad* terrainQuad = this->getCubeFace(cubeFace);
	dvec3 facePoint = this->localToCubeFacePoint(sphereVector * this->radius, &cubeFace);

	if (terrainQuad != NULL) {
		return terrainQuad->getElevation(dvec2(facePoint.x, facePoint.z));
	}

	return 0.0; // TODO
}

float Planet::getElevationUnderCamera() const {
	return this->elevationUnderCamera;
}

double Planet::getElevationScale() const {
	return this->elevationScale;
}

double Planet::getRadius() const {
	return this->radius;
}

double Planet::getDiameter() const {
	return this->radius* 2.0;
}

int32 Planet::getMaxSplitDepth() const {
	return this->maxSplitDepth;
}
