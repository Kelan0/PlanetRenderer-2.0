#pragma once

#include "core/Core.h"

class Tile;
class TileStorage;
class ShaderProgram;
class TerrainQuad;
class TerrainGenerator;

class Tile {
private:
	friend class TileStorage;
	
	TileStorage* storage; // The tile storage this tile belongs to.
	uvec3 id; // The id of this tile in storage.

	int32 textureIndex; // The index in the texture array of this tile.
	fvec2 texturePosition; // The normalized position of this texture in range [0, 1]
	fvec2 textureSize; // The normalized size of this texture, in the range [0, 1]

	uint64 timeCreated;
	bool deleted;
	bool active;

	Tile(TileStorage* storage, uvec3 id, int32 textureIndex, fvec2 texturePosition, fvec2 textureSize);
public:
	~Tile();

	void applyUniforms(ShaderProgram* program);

	uvec3 getId() const;

	int32 getTextureIndex() const;

	fvec2 getTexturePosition() const;
	
	fvec2 getTextureSize() const;

	bool isDeleted() const;

	bool isActive() const;
};

class TileStorage
{
private:
	friend class Tile;

	uint32 textureArray; // 256x256x2048 texture array.
	
	std::set<uint32> usedIndices; // List of texture indexes that are used.
	std::vector<Tile*> activeTiles; // List of tiles currently in use.
	std::vector<Tile*> cachedTiles; // List of tiles that are cached.

	TerrainGenerator* generator;

	uint32 getNextIndex();

	void releaseIndex(uint32 index);
public:
	TileStorage(TerrainGenerator* generator);

	~TileStorage();

	bool containsTile(uvec3 id);

	Tile* getTile(TerrainQuad* terrainQuad);

	void releaseTile(Tile* tile);

	void deleteTile(Tile* tile);
};

