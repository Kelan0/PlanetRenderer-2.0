#pragma once

#include "core/Core.h"

class TileSupplier;
class TerrainRenderer;
class AtmosphereRenderer;
class ShaderProgram;
class TerrainQuad;
class TileData;
class BoundingVolume;
struct Ray;
enum IntersectionType;

typedef enum CubeFace {
	X_NEG = 0, X_POS = 1,
	Y_NEG = 2, Y_POS = 3,
	Z_NEG = 4, Z_POS = 5,
} CubeFace;

class Planet {
private:
	friend class TerrainRenderer;
	friend class TerrainQuad;

	TileSupplier* tileSupplier;
	TerrainRenderer* terrainRenderer;
	AtmosphereRenderer* atmosphereRenderer;

	TerrainQuad* faces[6]; // cube faces.
	mat3 faceOrientations[6]; // face orientations.

	dvec3 localCameraPosition; // The position of the camera in local space.
	dvec2 faceCameraPosition; // The position of the camera in undeformed face space, for the closest face.
	CubeFace closestCameraFace; // The closest cube face to the camera.

	dvec3 center;
	double radius;
	double horizonRadius;
	double invRadius;
	double invHorizonRadius;

	double closestCameraDistance;
	TerrainQuad* closestCameraTerrainQuad;

	double elevationUnderCamera;
	double elevationScale; // Elevation scale in kilometers.
	double splitThreshold;
	int32 maxSplitDepth;

	bool renderDebugQuadBounds;
	int tileSupplierDebugState;


public:
	static double minScaleFactor;
	static double maxScaleFactor;
	static double scaleFactor;

	Planet(dvec3 center, double radius, double splitThreshold, int32 maxSplitDepth);
	~Planet();

	void render(double partialTicks, double dt);

	void update(double dt);

	void applyUniforms(ShaderProgram* program);

	IntersectionType getVisibility(CubeFace face, BoundingVolume* bound, bool horizonTest = true);

	bool horizonOcclusion(dvec3 localViewer, dvec3 localPoint);

	bool horizonOcclusion(CubeFace face, BoundingVolume* bound);

	//TileData* getTileData(TerrainQuad* terrainQuad);

	TerrainQuad* getCubeFace(CubeFace face);

	CubeFace getClosestFaceWorld(dvec3 worldPoint);

	CubeFace getClosestFaceLocal(dvec3 localPoint);

	CubeFace getClosestFaceSpherical(dvec3 sphericalPoint);

	CubeFace getClosestSphereVector(dvec3 sphereVector);

	dvec3 worldToLocalPoint(dvec3 worldPoint);

	Ray worldToLocalRay(Ray worldRay);

	dvec3 localToWorldPoint(dvec3 localPoint);

	Ray localToWorldRay(Ray localRay);

	dvec3 localToSphericalPoint(dvec3 localPoint, bool altitude = true);

	dvec3 sphericalToLocalPoint(dvec3 sphericalPoint);

	dvec3 cubeFaceToLocalPoint(CubeFace face, dvec3 facePoint);

	dvec3 localToCubeFacePoint(dvec3 localPoint, CubeFace* face = NULL);

	dmat3 getFaceOrientation(CubeFace face);

	dmat3 getLocalDeformation(dvec3 localPoint, dmat3 axis);

	dmat4 getFaceTransformation(CubeFace face);

	dmat4 getViewerTransformation(dvec3 viewerWorldPosition);

	dvec3 getCenter() const;

	dvec3 getLocalCameraPosition() const;

	dvec2 getFaceCameraPosition() const;

	CubeFace getClosestCameraFace() const;

	TerrainQuad* getClosestCameraTerrainQuad() const;

	TerrainQuad* getRayPickTerrainQuad(Ray localRay, double* distance = NULL);

	double getClosestCameraDistance() const;

	float getAltitude(dvec3 localPosition) const;

	float getHeight(dvec3 localPosition);

	float getElevation(dvec3 sphereVector, bool normalized = false);

	float getElevationUnderCamera() const;

	double getElevationScale() const;

	double getRadius() const;

	double getDiameter() const;

	int32 getMaxSplitDepth() const;
};

