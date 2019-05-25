#pragma once

#include "core/Core.h"

class AxisAlignedBB;
class Frustum;
class Planet;
class TileData;
enum CubeFace;

typedef enum QuadIndex {
	TOP_LEFT = 0,
	TOP_RIGHT = 1,
	BOTTOM_LEFT = 2,
	BOTTOM_RIGHT = 3,
} QuadIndex;

typedef enum NeighbourIndex {
	LEFT = 0,
	TOP = 1,
	RIGHT = 2,
	BOTTOM = 3,
} NeighbourIndex;

class TerrainQuad {
private:
	friend class Planet;

	static const QuadIndex faceRotations[6][4][4];

	Planet* planet; // The planet that this terrain quad belongs to.
	CubeFace face; // The face of the planet that this quad is on. planet->getCubeFace(face) should return the root quad for this node.
	TerrainQuad* parent; // The parent of this terrain quad, or null if this is the root.
	TerrainQuad** children; // The 4 child nodes of this quad.
	TerrainQuad** neighbours; // The 4 neighbours of this quad.

	QuadIndex quadIndex; // The quad index within its parent.
	AxisAlignedBB* faceBounds; // The bounding box around this quad in un-deformed face space.
	Frustum* deformedBounds; // The bounding frustum around this quad after it has been deformed to the surface of a sphere.

	dmat4 worldNormals; // The 4 normals of this quad stored in matrix columns, in world space.
	dmat4 worldCorners; // The 4 corners of this quad stored in matrix columns, in world space.

	dmat4 faceTransformation; // The transformation from the face space to local planet space.

	dvec2 distortion; // The cube edge/corner distortion in the x and y direction
	dvec2 facePosition; // The position of this quad, on the surface of its face, before deformation
	dvec3 localPosition; // The position of this quad in local planet space, after deformation.
	uvec2 treePosition; // The unique index in the entire quad tree.

	TileData* tileData; // The data associated with this terrain quad, height/normal maps or other data.

	double minHeight; // The lowest elevation point within this terrain quad.
	double maxHeight; // The highest elevation point within this terrain quad.
	double size; // The size, or side length of this quad relative to the root node.
	int32 depth; // The depth of this quad into the tree. This is the number of nodes below the root node.

	bool occluded; // True if this quad is occluded or behind the horizon.
	bool changed; // True if this quad changed in the previous frame, and needs to be re-rendered
	bool neighbourChanged;


	void setNeighbours(TerrainQuad* left, TerrainQuad* top, TerrainQuad* right, TerrainQuad* bottom);

	TerrainQuad(Planet* planet, CubeFace face, TerrainQuad* parent, QuadIndex quadIndex, dvec2 facePosition, uvec2 treePosition, float minHeight, float maxHeight, bool occluded);

public:
	TerrainQuad(Planet* planet, CubeFace face);

	~TerrainQuad();

	void init();

	void update(double dt);

	bool split();

	void merge();

	void updateNeighbours();

	void deleteNeighbours();

	void notifyNeighbours();

	bool isLeaf() const;

	bool isRenderLeaf();

	bool isRenderable();

	AxisAlignedBB getBoundingBox() const;

	Frustum getDeformedBoundingBox() const;

	Planet* getPlanet() const;

	CubeFace getCubeFace() const;

	TerrainQuad* getParent() const;

	TerrainQuad* getChild(QuadIndex index) const;

	TerrainQuad* getNeighbour(NeighbourIndex index) const;

	TerrainQuad* getTerrainQuadUnder(dvec2 facePoint);

	QuadIndex getQuadIndex() const;

	void getNearFarOrdering(dvec2 facePosition, QuadIndex indices[4]);

	dmat4 getWorldNormals() const;

	dmat4 getWorldCorners() const;

	dmat4 getFaceTransformation() const;

	dvec3 getSphereVector(dvec2 quadPosition) const;

	dvec2 getDistortion() const;

	dvec2 getFacePosition() const;

	dvec3 getLocalPosition() const;

	uvec3 getTreePosition(bool planetaryUnique = false) const;

	TileData* getTileData(fvec2* tilePosition = NULL, fvec2* tileSize = NULL, bool useParent = true);

	double getMinHeight() const;

	double getMaxHeight() const;

	double getSize() const;

	double getElevation(dvec2 position);

	int32 getDepth() const;

	bool isOccluded() const;

	bool didChange() const;
};

