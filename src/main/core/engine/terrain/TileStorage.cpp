#include "TileStorage.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/terrain/TerrainQuad.h"
#include "core/engine/terrain/TerrainGenerator.h"
#include "core/util/Time.h"

TileStorage::TileStorage(TerrainGenerator* generator):
	generator(generator) {

	// Initialize textures
}


TileStorage::~TileStorage() {
	// deallocate textures
}

uint32 TileStorage::getNextIndex() {
	int32 textureIndex = 0;

	for (auto it = this->usedIndices.begin(); it != this->usedIndices.end(); it++) {
		if (*it == textureIndex) {
			textureIndex++;
		} else {
			break;
		}
	}

	this->usedIndices.insert(textureIndex);

	return textureIndex;
}

void TileStorage::releaseIndex(uint32 index) {
	this->usedIndices.erase(index);
}

bool TileStorage::containsTile(uvec3 id) {
	return false;
}

Tile* TileStorage::getTile(TerrainQuad* terrainQuad) {
	uvec3 id = terrainQuad->getTreePosition();

	for (auto it = this->cachedTiles.begin(); it != this->cachedTiles.end(); it++) {
		if ((*it)->id == id) {
			Tile* tile = *it;
			if (!tile->isActive()) {
				activeTiles.push_back(tile);
			}
			return tile;
		}
	}

	// generate the tile.
	uint32 textureIndex = this->getNextIndex();
	fvec2 texturePosition = fvec2(0.0, 0.0);
	fvec2 textureSize = fvec2(1.0, 1.0);
	
	Tile* tile = new Tile(this, id, textureIndex, texturePosition, textureSize);
	this->cachedTiles.push_back(tile);
	this->activeTiles.push_back(tile);
	tile->active = true;

	uint32 tileSize = 16;

	for (int i = 0; i < tileSize; i++) {
		double x = (double)i / tileSize;
	
		for (int j = 0; j < tileSize; j++) {
			double y = (double)j / tileSize;

			dvec3 sphereVec = terrainQuad->getSphereVector(dvec2(x, y));
			double h = this->generator->getElevation(sphereVec);

		}
	}

	return tile;
}

void TileStorage::releaseTile(Tile* tile) {
	for (auto it = this->activeTiles.begin(); it != this->activeTiles.end(); it++) {
		if (*it == tile) {
			this->activeTiles.erase(it);
			break;
		}
	}

	tile->active = false;
}

void TileStorage::deleteTile(Tile* tile) {
	this->releaseTile(tile); // Tile is no longer active
	this->releaseIndex(tile ->textureIndex); // Tiles index is free to overwrite

	for (auto it = this->cachedTiles.begin(); it != this->cachedTiles.end(); it++) {
		if (*it == tile) {
			this->cachedTiles.erase(it);
			break;
		}
	}

	tile->deleted = true;
}

Tile::Tile(TileStorage* storage, uvec3 id, int32 textureIndex, fvec2 texturePosition, fvec2 textureSize):
	storage(storage), id(id), textureIndex(textureIndex), texturePosition(texturePosition), textureSize(textureSize), deleted(false), active(false) {

	this->timeCreated = Time::now();
}

Tile::~Tile() {
	this->storage->deleteTile(this);
}

void Tile::applyUniforms(ShaderProgram* program) {
	// TODO
}

uvec3 Tile::getId() const {
	return this->id;
}

int32 Tile::getTextureIndex() const {
	return this->textureIndex;
}

fvec2 Tile::getTexturePosition() const {
	return this->texturePosition;
}

fvec2 Tile::getTextureSize() const {
	return this->textureSize;
}

bool Tile::isDeleted() const {
	return this->deleted;
}

bool Tile::isActive() const {
	return this->active;
}
