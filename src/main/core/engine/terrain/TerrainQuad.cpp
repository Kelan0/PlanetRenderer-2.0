#include "TerrainQuad.h"
#include "core/application/Application.h"
#include "core/engine/scene/SceneGraph.h"
#include "core/engine/renderer/Camera.h"
#include "core/engine/terrain/Planet.h"
#include "core/engine/terrain/TileSupplier.h"
#include "core/engine/scene/bounding/BoundingVolume.h"
#include "core/util/Time.h"

TerrainQuad::TerrainQuad(Planet* planet, CubeFace face, TerrainQuad* parent, QuadIndex quadIndex, dvec2 facePosition, uvec2 treePosition, float minHeight, float maxHeight, bool occluded) :
	planet(planet),
	face(face),
	parent(parent),
	children(NULL),
	neighbours(NULL),
	quadIndex(quadIndex),
	faceBounds(),
	deformedBounds(),
	worldNormals(0.0),
	faceTransformation(1.0),
	facePosition(facePosition),
	treePosition(treePosition),
	tileData(NULL),
	minHeight(minHeight),
	maxHeight(maxHeight),
	occluded(occluded),
	changed(true) {
	this->size = parent->size * 0.5;
	this->depth = parent->depth + 1;
	this->init();
}

TerrainQuad::TerrainQuad(Planet* planet, CubeFace face):
	planet(planet),
	face(face),
	parent(NULL),
	children(NULL),
	neighbours(NULL),
	quadIndex(),
	faceBounds(),
	deformedBounds(),
	worldNormals(0.0),
	faceTransformation(1.0),
	facePosition(0.0F, 0.0F),
	treePosition(0, 0),
	tileData(NULL),
	minHeight(0.0F),
	maxHeight(0.0F),
	size(planet->getDiameter()),
	depth(0),
	occluded(false),
	changed(true) {
	this->init();
}

TerrainQuad::~TerrainQuad() {
	if (this->tileData != NULL && !this->planet->tileSupplier->putTileData(&this->tileData)) {
		logError("Failed to release reference to tile data when quad was deleted");
	}

	this->merge();
	delete this->faceBounds;
	delete this->deformedBounds;
}

void TerrainQuad::setNeighbours(TerrainQuad* left, TerrainQuad* top, TerrainQuad* right, TerrainQuad* bottom) {
	//this->neighbours[LEFT] = left;
	//left->neighbours[RIGHT] = this;
	//
	//this->neighbours[TOP] = top;
	//top->neighbours[BOTTOM] = this;
	//
	//this->neighbours[RIGHT] = right;
	//right->neighbours[LEFT] = this;
	//
	//this->neighbours[BOTTOM] = bottom;
	//bottom->neighbours[TOP] = this;
}

void TerrainQuad::init() {

	const double xmin = this->facePosition.x - this->size * 0.5;
	const double xmax = this->facePosition.x + this->size * 0.5;
	const double ymin = this->facePosition.y - this->size * 0.5;
	const double ymax = this->facePosition.y + this->size * 0.5;

	this->neighbours = new TerrainQuad*[4]{ NULL, NULL, NULL, NULL };

	if (this->faceBounds == NULL) {
		this->faceBounds = new AxisAlignedBB(dvec3(xmin, 0.0, ymin), dvec3(xmax, 0.0, ymax));
	} else {
		this->faceBounds->setMinMax(dvec3(xmin, 0.0, ymin), dvec3(xmax, 0.0, ymax));
	}

	double hmin = this->minHeight;
	double hmax = glm::max(0.0, this->maxHeight);

	double d = this->size * 0.325;
	const dvec3 ltn = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmin, this->planet->elevationScale * hmin, ymin));
	const dvec3 rtn = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmax, this->planet->elevationScale * hmin, ymin));
	const dvec3 rbn = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmax, this->planet->elevationScale * hmin, ymax));
	const dvec3 lbn = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmin, this->planet->elevationScale * hmin, ymax));
	const dvec3 ltf = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmin, this->planet->elevationScale * hmax + d, ymin));
	const dvec3 rtf = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmax, this->planet->elevationScale * hmax + d, ymin));
	const dvec3 rbf = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmax, this->planet->elevationScale * hmax + d, ymax));
	const dvec3 lbf = this->planet->cubeFaceToLocalPoint(this->face, dvec3(xmin, this->planet->elevationScale * hmax + d, ymax));

	if (this->deformedBounds == NULL) {
		this->deformedBounds = new Frustum(ltn, rtn, rbn, lbn, ltf, rtf, rbf, lbf);
	} else {
		this->deformedBounds->set(ltn, rtn, rbn, lbn, ltf, rtf, rbf, lbf);
	}

	const dvec2 p = this->getFacePosition();
	const double r = this->planet->getRadius();

	this->faceTransformation = this->planet->getFaceTransformation(this->face);
	this->localPosition = this->deformedBounds->getCenter();// this->planet->cubeFaceToLocalPoint(this->face, dvec3(this->facePosition.x, 0.0, this->facePosition.y));

	const double is = 0.5 * this->getSize();// *1.02;

	dvec3 n0, n1, n2, n3;

	n0 = normalize(dvec3(this->faceTransformation * dvec4(p.x - is, 0.0, p.y - is, 1.0)));
	n1 = normalize(dvec3(this->faceTransformation * dvec4(p.x + is, 0.0, p.y - is, 1.0)));
	n2 = normalize(dvec3(this->faceTransformation * dvec4(p.x + is, 0.0, p.y + is, 1.0)));
	n3 = normalize(dvec3(this->faceTransformation * dvec4(p.x - is, 0.0, p.y + is, 1.0)));

	dvec3 c0 = n0 * this->planet->getRadius();
	dvec3 c1 = n1 * this->planet->getRadius();
	dvec3 c2 = n2 * this->planet->getRadius();
	dvec3 c3 = n3 * this->planet->getRadius();

	this->distortion = dvec2(1.0);
	this->worldNormals = dmat4(dvec4(n0, 0.0), dvec4(n1, 0.0), dvec4(n2, 0.0), dvec4(n3, 0.0));
	this->worldCorners = dmat4(dvec4(c0, 1.0), dvec4(c1, 1.0), dvec4(c2, 1.0), dvec4(c3, 1.0));
}

void TerrainQuad::update(double dt) {

	dvec3 cameraPosition = this->planet->getLocalCameraPosition();
	double distanceSq = INFINITY;

	for (int i = 0; i < 8; i++) {
		distanceSq = glm::min(distanceSq, glm::distance2(this->deformedBounds->getCorner((FrustumCorner)i), cameraPosition));
	}

	double threshold = this->size * 3.0;

	if (this->face == this->planet->closestCameraFace) {
		if (distanceSq < this->planet->closestCameraDistance) {
			this->planet->closestCameraDistance = distanceSq;
			this->planet->closestCameraTerrainQuad = this;
		}
	}

	if (this->tileData != NULL) {
		this->tileData->setCameraDistance(distanceSq);
	}

	this->changed = false;
	if (distanceSq < threshold * threshold) {
		this->split();
	} else {
		this->merge();
	}

	if (this->children != NULL) {
		QuadIndex order[4];

		this->getNearFarOrdering(this->planet->getFaceCameraPosition(), order);
		this->children[order[0]]->update(dt);
		this->children[order[1]]->update(dt);
		this->children[order[2]]->update(dt);
		this->children[order[3]]->update(dt);

		this->changed |= this->children[TOP_LEFT]->changed;
		this->changed |= this->children[TOP_RIGHT]->changed;
		this->changed |= this->children[BOTTOM_LEFT]->changed;
		this->changed |= this->children[BOTTOM_RIGHT]->changed;
	}

	if (this->tileData != NULL) {
		// If the maximum or minimum height of the tile data does not match the current saved values, reinitialize the bounds.
		if (glm::max(abs(this->tileData->getMaxHeight() - this->maxHeight), abs(this->tileData->getMinHeight() - this->minHeight)) > 1e-12) {
			this->maxHeight = tileData->getMaxHeight();
			this->minHeight = tileData->getMinHeight();
			this->init();
		}
	}
}

bool TerrainQuad::split() {
	if (this->depth < this->planet->getMaxSplitDepth() && this->children == NULL) {
		
		if (this->tileData != NULL && !this->planet->tileSupplier->putTileData(&this->tileData)) {
			logError("Failed to release reference to tile data when quad was subdivided");
		}

		this->children = new TerrainQuad*[4];
		double h = this->size * 0.25;
		uvec3 p = this->getTreePosition();

		this->children[TOP_LEFT] = new TerrainQuad(this->planet, this->face, this, TOP_LEFT, this->facePosition + dvec2(-h, -h), (uint32(2) * this->treePosition) + uvec2(0, 0), this->minHeight, this->maxHeight, this->occluded);
		this->children[TOP_RIGHT] = new TerrainQuad(this->planet, this->face, this, TOP_RIGHT, this->facePosition + dvec2(+h, -h), (uint32(2) * this->treePosition) + uvec2(1, 0), this->minHeight, this->maxHeight, this->occluded);
		this->children[BOTTOM_LEFT] = new TerrainQuad(this->planet, this->face, this, BOTTOM_LEFT, this->facePosition + dvec2(-h, +h), (uint32(2) * this->treePosition) + uvec2(0, 1), this->minHeight, this->maxHeight, this->occluded);
		this->children[BOTTOM_RIGHT] = new TerrainQuad(this->planet, this->face, this, BOTTOM_RIGHT, this->facePosition + dvec2(+h, +h), (uint32(2) * this->treePosition) + uvec2(1, 1), this->minHeight, this->maxHeight, this->occluded);

		this->notifyNeighbours();
		this->children[TOP_LEFT]->updateNeighbours();
		this->children[TOP_RIGHT]->updateNeighbours();
		this->children[BOTTOM_LEFT]->updateNeighbours();
		this->children[BOTTOM_RIGHT]->updateNeighbours();

		this->changed = true;

		return true;
	}
	return false;
}

void TerrainQuad::merge() {
	if (this->children != NULL) {
		this->children[TOP_LEFT]->merge();
		this->children[TOP_RIGHT]->merge();
		this->children[BOTTOM_LEFT]->merge();
		this->children[BOTTOM_RIGHT]->merge();

		delete this->children[TOP_LEFT];
		delete this->children[TOP_RIGHT];
		delete this->children[BOTTOM_LEFT];
		delete this->children[BOTTOM_RIGHT];
		delete[] this->children;
		this->children = NULL;

		this->notifyNeighbours();

		this->changed = true;
	}
}

void TerrainQuad::updateNeighbours() {
	static const QuadIndex faceRotations[6][4][4] = {
		{
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [X_NEG][LEFT]
			{ TOP_RIGHT, BOTTOM_RIGHT, TOP_LEFT, BOTTOM_LEFT }, // [X_NEG][TOP]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [X_NEG][RIGHT]
			{ BOTTOM_LEFT, TOP_LEFT, BOTTOM_RIGHT, TOP_RIGHT }, // [X_NEG][BOTTOM]
		}, {
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [X_POS][LEFT]
			{ TOP_LEFT, BOTTOM_LEFT, BOTTOM_RIGHT, TOP_RIGHT }, // [X_POS][TOP]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [X_POS][RIGHT]
			{ TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT, TOP_LEFT }, // [X_POS][BOTTOM]
		}, {
			{ TOP_RIGHT, BOTTOM_RIGHT, TOP_LEFT, BOTTOM_LEFT }, // [Y_NEG][LEFT]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Y_NEG][TOP]
			{ BOTTOM_LEFT, TOP_LEFT, BOTTOM_RIGHT, TOP_RIGHT }, // [Y_NEG][RIGHT]
			{ BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT }, // [Y_NEG][BOTTOM]
		}, {
			{ BOTTOM_LEFT, TOP_LEFT, BOTTOM_RIGHT, TOP_RIGHT }, // [Y_POS][LEFT]
			{ BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT }, // [Y_POS][TOP]
			{ TOP_RIGHT, BOTTOM_RIGHT, TOP_LEFT, BOTTOM_LEFT }, // [Y_POS][RIGHT]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Y_POS][BOTTOM]
		}, {
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Z_NEG][LEFT]
			{ BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT }, // [Z_NEG][TOP]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Z_NEG][RIGHT]
			{ BOTTOM_RIGHT, BOTTOM_LEFT, TOP_RIGHT, TOP_LEFT }, // [Z_NEG][BOTTOM]
		}, {
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Z_POS][LEFT]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Z_POS][TOP]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Z_POS][RIGHT]
			{ TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT }, // [Z_POS][BOTTOM]
		},
	};

	struct Neighbour {
		TerrainQuad* terrainQuad;
		QuadIndex rotation[4];

		Neighbour(TerrainQuad* terrainQuad, int face = -1, int dir = -1) :
			terrainQuad(terrainQuad) {

			if (face == -1 || dir == -1) {
				this->rotation[0] = TOP_LEFT;
				this->rotation[1] = TOP_RIGHT;
				this->rotation[2] = BOTTOM_LEFT;
				this->rotation[3] = BOTTOM_RIGHT;
			}
			else {
				this->rotation[0] = faceRotations[face][dir][0];
				this->rotation[1] = faceRotations[face][dir][1];
				this->rotation[2] = faceRotations[face][dir][2];
				this->rotation[3] = faceRotations[face][dir][3];
			}
		}

		TerrainQuad* getTerrainQuad(int treeDepth) {
			//TerrainQuad* quad = this->terrainQuad;
			//
			//if (quad != NULL) {
			//	while (quad->getParent() != NULL && quad->getDepth() > treeDepth + 1) {
			//		quad = quad->getParent();
			//	}
			//}
			//
			//return quad;
			return this->terrainQuad;
		}

		static Neighbour getNeighbour(TerrainQuad* quad, NeighbourIndex index) {
			TerrainQuad* parent = quad->getParent();
			CubeFace face = quad->getCubeFace();
			Planet* planet = quad->getPlanet();

			if (parent == NULL) {
				if (face == X_NEG) {
					TerrainQuad* cubeNeighbour = NULL;
					if (index == LEFT) cubeNeighbour = planet->getCubeFace(Z_NEG);
					else if (index == TOP) cubeNeighbour = planet->getCubeFace(Y_POS);
					else if (index == RIGHT) cubeNeighbour = planet->getCubeFace(Z_POS);
					else if (index == BOTTOM) cubeNeighbour = planet->getCubeFace(Y_NEG);
					return Neighbour(cubeNeighbour, X_NEG, index);
				}
				else if (face == X_POS) {
					TerrainQuad* cubeNeighbour = NULL;
					if (index == LEFT) cubeNeighbour = planet->getCubeFace(Z_POS);
					else if (index == TOP) cubeNeighbour = planet->getCubeFace(Y_POS);
					else if (index == RIGHT) cubeNeighbour = planet->getCubeFace(Z_NEG);
					else if (index == BOTTOM) cubeNeighbour = planet->getCubeFace(Y_NEG);
					return Neighbour(cubeNeighbour, X_POS, index);
				}
				else if (face == Y_NEG) {
					TerrainQuad* cubeNeighbour = NULL;
					if (index == LEFT) cubeNeighbour = planet->getCubeFace(X_NEG);
					else if (index == TOP) cubeNeighbour = planet->getCubeFace(Z_POS);
					else if (index == RIGHT) cubeNeighbour = planet->getCubeFace(X_POS);
					else if (index == BOTTOM) cubeNeighbour = planet->getCubeFace(Z_NEG);
					return Neighbour(cubeNeighbour, Y_NEG, index);
				}
				else if (face == Y_POS) {
					TerrainQuad* cubeNeighbour = NULL;
					if (index == LEFT) cubeNeighbour = planet->getCubeFace(X_NEG);
					else if (index == TOP) cubeNeighbour = planet->getCubeFace(Z_NEG);
					else if (index == RIGHT) cubeNeighbour = planet->getCubeFace(X_POS);
					else if (index == BOTTOM) cubeNeighbour = planet->getCubeFace(Z_POS);
					return Neighbour(cubeNeighbour, Y_POS, index);
				}
				else if (face == Z_NEG) {
					TerrainQuad* cubeNeighbour = NULL;
					if (index == LEFT) cubeNeighbour = planet->getCubeFace(X_POS);
					else if (index == TOP) cubeNeighbour = planet->getCubeFace(Y_POS);
					else if (index == RIGHT) cubeNeighbour = planet->getCubeFace(X_NEG);
					else if (index == BOTTOM) cubeNeighbour = planet->getCubeFace(Y_NEG);
					return Neighbour(cubeNeighbour, Z_NEG, index);
				}
				else if (face == Z_POS) {
					TerrainQuad* cubeNeighbour = NULL;
					if (index == LEFT) cubeNeighbour = planet->getCubeFace(X_NEG);
					else if (index == TOP) cubeNeighbour = planet->getCubeFace(Y_POS);
					else if (index == RIGHT) cubeNeighbour = planet->getCubeFace(X_POS);
					else if (index == BOTTOM) cubeNeighbour = planet->getCubeFace(Y_NEG);
					return Neighbour(cubeNeighbour, Z_POS, index);
				}

				return Neighbour(NULL); // invalid
			}

			QuadIndex qi = quad->getQuadIndex();

			if (index == LEFT) {
				// If the requested neighbour is within the same parent as this node...
				if (qi == TOP_RIGHT) return Neighbour(parent->getChild(TOP_LEFT));
				if (qi == BOTTOM_RIGHT) return Neighbour(parent->getChild(BOTTOM_LEFT));

				// Else, the requested neighbour is outside this parent node, traverse up the tree.
				Neighbour node = Neighbour::getNeighbour(parent, LEFT);
				// If the parents neighbour is a leaf node, it is also the neighbour of this node.
				if (node.terrainQuad == NULL || node.terrainQuad->isLeaf()) {
					return node;
				}

				// Move back down the tree to the appropriate neighbour. TODO: traverse back down to the same level as the original node?
				if (qi == TOP_LEFT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[TOP_RIGHT]);
				if (qi == BOTTOM_LEFT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[BOTTOM_RIGHT]);
				return node;
			}
			else if (index == RIGHT) {
				if (qi == TOP_LEFT) return Neighbour(parent->getChild(TOP_RIGHT));
				if (qi == BOTTOM_LEFT) return Neighbour(parent->getChild(BOTTOM_RIGHT));

				Neighbour node = Neighbour::getNeighbour(parent, RIGHT);
				if (node.terrainQuad == NULL || node.terrainQuad->isLeaf()) {
					return node;
				}

				if (qi == TOP_RIGHT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[TOP_LEFT]);
				if (qi == BOTTOM_RIGHT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[BOTTOM_LEFT]);
				return node;
			}
			else if (index == BOTTOM) {
				if (qi == TOP_LEFT) return Neighbour(parent->getChild(BOTTOM_LEFT));
				if (qi == TOP_RIGHT) return Neighbour(parent->getChild(BOTTOM_RIGHT));

				Neighbour node = Neighbour::getNeighbour(parent, BOTTOM);
				if (node.terrainQuad == NULL || node.terrainQuad->isLeaf()) {
					return node;
				}

				if (qi == BOTTOM_LEFT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[TOP_LEFT]);
				if (qi == BOTTOM_RIGHT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[TOP_RIGHT]);
				return node;
			}
			else if (index == TOP) {
				if (qi == BOTTOM_LEFT) return Neighbour(parent->getChild(TOP_LEFT));
				if (qi == BOTTOM_RIGHT) return Neighbour(parent->getChild(TOP_RIGHT));

				Neighbour node = Neighbour::getNeighbour(parent, TOP);
				if (node.terrainQuad == NULL || node.terrainQuad->isLeaf()) {
					return node;
				}

				if (qi == TOP_LEFT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[BOTTOM_LEFT]);
				if (qi == TOP_RIGHT) node.terrainQuad = node.terrainQuad->getChild(node.rotation[BOTTOM_RIGHT]);
				return node;
			}
			else {
				return Neighbour(NULL); // invalid
			}
		}
	};

	constexpr int top_left = 0;
	constexpr int top_right = 1;
	constexpr int bottom_left = 3;
	constexpr int bottom_right = 2;

	TerrainQuad* left = Neighbour::getNeighbour(this, LEFT).getTerrainQuad(this->depth);
	this->neighbours[LEFT] = left;
	if (left != NULL) left->neighbours[RIGHT] = this;

	TerrainQuad* right = Neighbour::getNeighbour(this, RIGHT).getTerrainQuad(this->depth);
	this->neighbours[RIGHT] = right;
	if (right != NULL) right->neighbours[LEFT] = this;

	TerrainQuad* bottom = Neighbour::getNeighbour(this, BOTTOM).getTerrainQuad(this->depth);
	this->neighbours[BOTTOM] = bottom;
	if (bottom != NULL) bottom->neighbours[TOP] = this;

	TerrainQuad* top = Neighbour::getNeighbour(this, TOP).getTerrainQuad(this->depth);
	this->neighbours[TOP] = top;
	if (top != NULL) top->neighbours[BOTTOM] = this;
}

void TerrainQuad::deleteNeighbours() {
	constexpr bool allowInternal = true;
	if (this->neighbours != NULL) {
		if (allowInternal || (this->quadIndex == TOP_LEFT || this->quadIndex == BOTTOM_LEFT)) {
			if (this->neighbours[LEFT] != NULL && this->neighbours[LEFT]->neighbours != NULL) {
				this->neighbours[LEFT]->neighbours[RIGHT] = NULL;
				this->neighbours[LEFT]->neighbourChanged = true;
			}
		}

		if (allowInternal || (this->quadIndex == TOP_LEFT || this->quadIndex == TOP_RIGHT)) {
			if (this->neighbours[TOP] != NULL && this->neighbours[TOP]->neighbours != NULL) {
				this->neighbours[TOP]->neighbours[BOTTOM] = NULL;
				this->neighbours[TOP]->neighbourChanged = true;
			}
		}

		if (allowInternal || (this->quadIndex == TOP_RIGHT || this->quadIndex == BOTTOM_RIGHT)) {
			if (this->neighbours[RIGHT] != NULL && this->neighbours[RIGHT]->neighbours != NULL) {
				this->neighbours[RIGHT]->neighbours[LEFT] = NULL;
				this->neighbours[RIGHT]->neighbourChanged = true;
			}
		}

		if (allowInternal || (this->quadIndex == BOTTOM_LEFT || this->quadIndex == BOTTOM_RIGHT)) {
			if (this->neighbours[BOTTOM] != NULL && this->neighbours[BOTTOM]->neighbours != NULL) {
				this->neighbours[BOTTOM]->neighbours[TOP] = NULL;
				this->neighbours[BOTTOM]->neighbourChanged = true;
			}
		}

		//delete[] this->neighbours;
		this->neighbours = NULL;
	}
}

void TerrainQuad::notifyNeighbours() {
	if (this->neighbours != NULL) {
		this->updateNeighbours();

		if (this->neighbours[LEFT] != NULL) {
			this->neighbours[LEFT]->updateNeighbours();
		}
		if (this->neighbours[TOP] != NULL) {
			this->neighbours[TOP]->updateNeighbours();
		}
		if (this->neighbours[RIGHT] != NULL) {
			this->neighbours[RIGHT]->updateNeighbours();
		}
		if (this->neighbours[BOTTOM] != NULL) {
			this->neighbours[BOTTOM]->updateNeighbours();
		}
	}
}

void TerrainQuad::onHeightRangeChanged() {
}

bool TerrainQuad::isLeaf() const {
	return this->children == NULL;
}

bool TerrainQuad::isRenderLeaf() {
	return this->isLeaf();
	////	If all children are renderable,
	////		return false. This is not a render leaf, as all children can be rendered.
	////	Else
	////		If this is renderable,
	////			return true, and render this until all children can be rendered.
	////		Else
	////			return false
	//
	//if (this->isLeaf()) {
	//	return this->isRenderable();
	//}
	//
	//return false;
	//
	////// If all children are renderable
	////if (this->children[TOP_LEFT]->isRenderLeaf() &&
	////	this->children[TOP_RIGHT]->isRenderLeaf() &&
	////	this->children[BOTTOM_LEFT]->isRenderLeaf() &&
	////	this->children[BOTTOM_RIGHT]->isRenderLeaf()) {
	////	return false;
	////} else { // Any child cannot be rendered, and would cause a world hole.
	////	return this->isRenderable();
	////}
}

bool TerrainQuad::isRenderable() {
	return this->getTileData() != NULL && this->tileData->isGenerated() && !this->tileData->isAwaitingReadback();
}

AxisAlignedBB TerrainQuad::getBoundingBox() const {
	return *this->faceBounds;
}

Frustum TerrainQuad::getDeformedBoundingBox() const {
	return *this->deformedBounds;
}

Planet* TerrainQuad::getPlanet() const {
	return this->planet;
}

CubeFace TerrainQuad::getCubeFace() const {
	return this->face;
}

TerrainQuad* TerrainQuad::getParent() const {
	return this->parent;
}

TerrainQuad* TerrainQuad::getChild(QuadIndex index) const {
	if (this->children != NULL) {
		return this->children[index];
	}

	return NULL;
}

TerrainQuad* TerrainQuad::getNeighbour(NeighbourIndex index) const {
	if (this->neighbours != NULL) {
		return this->neighbours[index];
	}

	return NULL;
}

TerrainQuad* TerrainQuad::getTerrainQuadUnder(dvec2 facePoint) {
	if (this->isLeaf()) {
		return this;
	}

	if (facePoint.x < this->facePosition.x) {
		if (facePoint.y < this->facePosition.y) {
			return this->getChild(TOP_LEFT)->getTerrainQuadUnder(facePoint);
		} else {
			return this->getChild(BOTTOM_LEFT)->getTerrainQuadUnder(facePoint);
		}
	} else {
		if (facePoint.y < this->facePosition.y) {
			return this->getChild(TOP_RIGHT)->getTerrainQuadUnder(facePoint);
		} else {
			return this->getChild(BOTTOM_RIGHT)->getTerrainQuadUnder(facePoint);
		}
	}

	return NULL;
}

QuadIndex TerrainQuad::getQuadIndex() const {
	return this->quadIndex;
}

void TerrainQuad::getNearFarOrdering(dvec2 facePosition, QuadIndex indices[4]) {
	double px = facePosition.x;
	double py = facePosition.y;
	double tx = this->facePosition.x;
	double ty = this->facePosition.y;

	if (px < tx) { // if p is to the *left* of this quad
		if (py < ty) { // if p is to the *top left* of this quad
			indices[0] = TOP_LEFT;
			indices[1] = BOTTOM_LEFT;
			indices[2] = TOP_RIGHT;
			indices[3] = BOTTOM_RIGHT;
		} else { // if p is to the *bottom left* of this quad
			indices[0] = BOTTOM_LEFT;
			indices[1] = TOP_LEFT;
			indices[2] = BOTTOM_RIGHT;
			indices[3] = TOP_RIGHT;
		}
	} else { // if p is to the *right* of this quad.
		if (py < ty) { // if p is to the *top right* of this quad
			indices[0] = TOP_RIGHT;
			indices[1] = BOTTOM_RIGHT;
			indices[2] = TOP_LEFT;
			indices[3] = BOTTOM_LEFT;
		} else { // if p is to the *bottom right* of this quad
			indices[0] = BOTTOM_RIGHT;
			indices[1] = TOP_RIGHT;
			indices[2] = BOTTOM_LEFT;
			indices[3] = TOP_LEFT;
		}
	}
}

dmat4 TerrainQuad::getWorldNormals() const {
	return this->worldNormals;
}

dmat4 TerrainQuad::getWorldCorners() const {
	return this->worldCorners;
}

dmat4 TerrainQuad::getFaceTransformation() const {
	return this->faceTransformation;
}

dvec3 TerrainQuad::getSphereVector(dvec2 quadPosition) const {
	dvec4 interp = dvec4(
		(0 + quadPosition.x) * (0 + quadPosition.y), // (0+x) * (0+y) // x * y
		(1 - quadPosition.x) * (0 + quadPosition.y), // (1-x) * (0+y) // z * y
		(1 - quadPosition.x) * (1 - quadPosition.y), // (1-x) * (1-y) // z * w
		(0 + quadPosition.x) * (1 - quadPosition.y)  // (0+x) * (1-y) // x * w
	);
	
	return normalize(this->worldNormals * interp);
}

dvec2 TerrainQuad::getDistortion() const {
	return this->distortion;
}

dvec2 TerrainQuad::getFacePosition() const {
	return this->facePosition;
}

dvec3 TerrainQuad::getLocalPosition() const {
	return this->localPosition;
}

uvec3 TerrainQuad::getTreePosition(bool planetaryUnique) const {
	return uvec3(this->treePosition, this->depth + (planetaryUnique ? this->face * this->planet->getMaxSplitDepth() : 0));
}

TileData* TerrainQuad::getTileData(fvec2* tilePosition, fvec2* tileSize, bool useParent) {
	TileData* tile = this->tileData;

	if (this->tileData == NULL) {
		this->planet->tileSupplier->getTileData(this, &this->tileData);
	}

	if (this->tileData != NULL) {
		this->tileData->onUsed();
	}

	if (useParent) {
		if ((this->tileData == NULL || !this->tileData->isGenerated()) && this->parent != NULL) {
			if (tilePosition != NULL && tileSize != NULL) {
				*tileSize *= 0.5;
				if (this->quadIndex == BOTTOM_RIGHT) * tilePosition += fvec2(0.0, 0.0) * (*tileSize);
				if (this->quadIndex == BOTTOM_LEFT) * tilePosition += fvec2(1.0, 0.0) * (*tileSize);
				if (this->quadIndex == TOP_RIGHT) * tilePosition += fvec2(0.0, 1.0) * (*tileSize);
				if (this->quadIndex == TOP_LEFT) * tilePosition += fvec2(1.0, 1.0) * (*tileSize);
			}
			tile = this->parent->getTileData(tilePosition, tileSize, true);
		}
	}

	return tile;
}

double TerrainQuad::getMinHeight() const {
	return this->minHeight;
}

double TerrainQuad::getMaxHeight() const {
	return this->maxHeight;
}

double TerrainQuad::getSize() const {
	return this->size;
}

double TerrainQuad::getElevation(dvec2 position) {
	if (position.x >= 0.0 && position.y >= 0.0 && position.x < 1.0 && position.y < 1.0) {
		if (this->children != NULL) {
			if (position.x < 0.5) {
				if (position.y < 0.5) {
					return this->children[TOP_LEFT]->getElevation(position * 2.0 - dvec2(0.0, 0.0)); // top left
				} else {
					return this->children[BOTTOM_LEFT]->getElevation(position * 2.0 - dvec2(0.0, 1.0)); // bottom left
				}
			} else {
				if (position.y < 0.5) { 
					return this->children[TOP_RIGHT]->getElevation(position * 2.0 - dvec2(1.0, 0.0)); // top right
				} else {
					return this->children[BOTTOM_RIGHT]->getElevation(position * 2.0 - dvec2(1.0, 1.0)); // bottom right
				}
			}
		} else {
			TileData* tile = this->getTileData();

			if (tile != NULL) {
				return tile->getInterpolatedTextureData(position).w;
			}
		}
	}
	return 0.0;
}

int32 TerrainQuad::getDepth() const {
		return this->depth;
}

bool TerrainQuad::isOccluded() const {
	return this->occluded;
}

bool TerrainQuad::didChange() const {
	return this->changed;
}
