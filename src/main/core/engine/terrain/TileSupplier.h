#pragma once

#include "core/Core.h"

class TerrainQuad;
class Planet;
class TileData;
class TileSupplier;
class ShaderProgram;

class TileData {
private:
	friend class TileSupplier;

	TileSupplier* supplier; // The TileSupplier which owns this TileData.
	uint32 textureIndex; // The index within the OpenGL texture array that this tile uses.
	std::set<TileData**> references; // The tracked references to this tile.

	uvec3 id; // The id of this tile, corrsponding to its quadtree index.
	dmat4 quadNormals; // The four corner vectors of this tile on the surface of a sphere. Needed for texture generation.
	dmat4 quadCorners; // The four corner points of this tile on the surface of a sphere. Needed for texture generation.
	double cameraDistance; // The distance between the TerrainQuad using this tile, and the world camera. This is updated externally, and initialized as infinity.

	uint64 timeLastUsed; // The time that this tile was last used. If the tile remains unused for too long, it will be automatically freed.
	uint64 timeLastRetrieved; // The time that this tile was last retrieved from the TileSupplier via getTileData. (debug purposes)
	uint64 timeCreated; // The time that this tile was initially created. (debug purposes)
	uint64 timeGenerated; // The time that this tiles texture was generated. (debug purposes)
	uint64 timeReadback; // The time that this tiles texture data was read back from video memory.

	bool generated; // True if the texture for this tile has been generated, and the tile may be used for rendering.
	bool awaitingGeneration; // True if the texture of this tile is currently waiting in the texture generation queue.
	bool awaitingReadbackResponse; // True if the tile has requested an asynchronous texture readback from video memory, but is still waiting for the response.
	bool awaitingReadbackRequest; // Flag to let the TileSupplier update pass know if an asynchronous request is needed.

	float* textureData; // The texture data read back to the CPU after generation.
	double maxHeight; // The maximum elevation value within this tile data.
	double minHeight; // The minimum elevation value within this tile data.

	TileData(TileSupplier* supplier, uint32 textureIndex);

	~TileData();

	// Deleted copy constructor and assignment function
	TileData(const TileData&) = delete;
	TileData& operator=(const TileData&) = delete;

	void onReadbackReceived();

public:
	/**
	 * Function to call for every frame that this tile is used for rendering. If the tile has remained
	 * unused for a certain length of time, it will be automatically deactivated, and its references
	 * will be nullified.
	 */
	void onUsed();

	/**
	 * Mark this tile to be re-generated. It's texture may continue being used for the period of time
	 * between now and the time that the texture actually gets generated, it will simply remain with
	 * the old (current) texture data until then.
	 */
	void regenerate();

	/**
	 * Request for the tiles texture data to be asynchronously read back from video memory. The
	 * 'awaitingReadback' flag will be set to true until the async request is fufilled.
	 */
	void requestAsyncReadback();

	/**
	 * Set the distance of this tile to the camera. This is down to the object using this tile to update.
	 * The default value is infinity, resulting in this tile having the lowest priority for texture generation.
	 */
	void setCameraDistance(double cameraDistance);

	dvec3 getSphereVector(dvec2 pos) const;

	dmat4 getQuadNormals() const;

	dmat4 getQuadCorners() const;

	uint32 getTextureIndex() const;

	uint64 getTimeLastUsed() const;

	uint64 getTimeLastRetrieved() const;

	uint64 getTimeCreated() const;

	uint64 getTimeGenerated() const;

	uint64 getTimeReadback() const;

	dvec4 getTextureData(uvec2 pos);

	dvec4 getInterpolatedTextureData(dvec2 pos);

	ivec2 getTextureSize() const;

	bool isGenerated() const;

	bool isAwaitingGeneration() const;

	bool isAwaitingReadback() const;

	bool isActive() const;

	double getMaxHeight() const;

	double getMinHeight() const;
};

typedef struct __GLsync* GLsync;

struct AsyncReadbackRequest {
	uint64 requestTime;
	TileData* tile;
	GLsync sync;
};

class TileSupplier {
private:
	friend class TileData;

	using LRUIterator = std::list<TileData*>::iterator;
	using IdleCacheIterator = std::unordered_map<uvec3, LRUIterator>::iterator;
	using ActiveCacheIterator = std::unordered_map<uvec3, TileData*>::iterator;

	std::unordered_map<uvec3, TileData*> activeTiles; // Map of currenrly active tiles. These tiles cannot be reallocated.
	std::unordered_map<uvec3, LRUIterator> idleTiles; // Map of idle tiles in the LRU list, which may be reallocated or reactivated
	std::list<TileData*> lruList; // "Least recently used" sorted list of tiles. Tiles at the beginning of this list are the first to be reallocated.
	std::vector<TileData*> textureGenerationQueue; // List of textures to be generated, sorted by distance to the viewer. Closest textures get generated first.
	std::vector<TileData*> textureReadbackQueue; // List of textures to be read back to system memory, sorted by distance to the viewer. Closest textures get requested first.

	Planet* planet; // The planet that this tile supplier is for.
	uint32 capacity; // The capacity, or number of tiles, which can be stored.
	uint32 tileSize; // The width and height of each tile texture in the array.
	uint32 textureArray; // The OpenGL texture array handle.
	uint32 pixelTransferBuffer; // The OpenGL pixel buffer object used to transfer texture data asynchronously back to system memory.
	bool* availableTextures; // Array to identify which textures are available and unused. The first true index in this array identifies the first available texture.

	uint32 maxAsyncReadbacks; // The maximum number of concurrent asynchronous texture readbacks.
	AsyncReadbackRequest** asyncReadbackSlots; // The available asynchronous readback slots are NULL. Used ones contain the OpenGL sync object.
	double asyncReadbackTimeout; // The maximum amount of time an asynchronous readback request is allowed to remain unresolved for. The request wil be cancelled after this.

	ShaderProgram* tileGeneratorProgram; // The compute shader used to generate terrain tiles on the GPU.

	uint32 numTexturesGenerated; // Debug info
	uint32 numTilesExpired; // Debug info

	bool overlayDebug; // Overlay the texture generation time
	bool showDebug; // Show texture generation, retrieve and use time as the texture RGB components.

	uint64 timeLastCleanupCycle; // The time that we last went through and removed expired tiles that have been unused for too long.
	uint64 timeLastGenerationCheck; // The time at which we last went through and checked if we need to initiate a generation burst.
	double cleanupFrequency; // The frequency at which we go through and remove expired tiles.
	double generationCheckFrequency; // The frequency at which we go through and mark textures that need generation, and sort tiles in order of distance to the viewer.

	double maxGenerationTime; // The maximum amount of time in milliseconds that is allowed to be spent in a single frame generating textures.

	/**
	 * Generate the texture data for the specified tile. This will not happen asynchronously,
	 * and will cause slowdowns if too many textures are generated at once. After this function
	 * returns, the tiles texture will be fully generated.
	 */
	void generateTexture(TileData* tile);

	/**
	 * Add the tile to the texture generation queue. The tile texture will be generated at some
	 * point in the future, generally depending on how close the tile is to the viewer. The tile
	 * will be marked as 'awaitingGeneration = true' and 'generated = false' by this function,
	 * and will remain in the queue until it is generated.
	 */
	void markForGeneration(TileData* tile);

	/**
	 * Mark the tile as idle, invalidating all references if they exist.
	 */
	void markIdle(TileData* tile);

public:
	TileSupplier(Planet* planet, uint32 seed = 1337, uint32 textureCapacity = 0, uint32 textureSize = 64);

	~TileSupplier();

	/**
	 * Update function called every tick to perform periodic cleanup routines, primarily automatically
	 * freeing active tiles which have not been used for too long.
	 */
	void update();

	void computePointData(int32 count, fvec3* points, fvec4* data);

	/**
	 * Gets the TileData corresponding to the specified TerrainQuad, and stores it in the storage
	 * pointer. The active tile cache will be searched first, and the tile will be returned if it
	 * was found, with nothing extra to do. If it was not found in the active cache, the cache of
	 * idle tiles will be searched, and if a tile was found, it is moved to the active cache and
	 * returned. If neither of these caches contained the tile, then a tile either has to be created
	 * or reallocated from the least recently used idle tile, and marked for texture generation. If
	 * no tiles were available to reallocate, then NULL is returned.
	 *
	 * If the returned tile is not NULL, the storage pointer will be kept as a reference within the tile,
	 * and will be automatically nullified when the tile is deleted or reallocated. The returned tile also
	 * may not be deleted externally. When it is no longer needed, it should be returned to the TileSupplier
	 * via the putTileData function.
	 *
	 * Note that the returned tile may not yet have a generated texture associated with it, and should not
	 * be used if it is awaiting generation.
	 */
	void getTileData(TerrainQuad* terrainQuad, TileData** store);

	/**
	 * Removes the tiles reference to the storage pointer, and nullifies the value of the pointer.
	 * The tile internally keeps track of its references, and as long as there is more than one
	 * reference, the tile will not be reallocated.
	 *
	 * Calling this function is the formal way of releasing the TileData, but this may end up happening
	 * automatically if the tile has remained unused for too long. The tiles 'onUsed' function should be
	 * called to constantly renew the tiles used status, and to keep it active.
	 *
	 * Returns true if the store pointer was successfully removed from the tiles references list.
	 */
	bool putTileData(TileData** store);

	/**
	 * Apply the uniforms necessary to render the currently active tiles to the specified shader program.
	 */
	void applyUniforms(ShaderProgram* program);

	uint32 getTileSize() const;

	bool isShowDebug() const;

	bool isOverlayDebug() const;

	void setShowDebug(bool show);

	void setOverlayDebug(bool show);
};