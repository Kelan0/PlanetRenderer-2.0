#include "TileSupplier.h"
#include "core/application/Application.h"
#include "core/engine/renderer/ShaderProgram.h"
#include "core/engine/terrain/TerrainQuad.h"
#include "core/engine/terrain/Planet.h"
#include "core/util/Logger.h"
#include "core/util/Time.h"
#include "core/util/InputHandler.h"
#include <thread>
#include <GL/glew.h>

////////////////// TileData \\\\\\\\\\\\\\\\\\ 

TileData::TileData(TileSupplier* supplier, uint32 textureIndex) :
	supplier(supplier), textureIndex(textureIndex) {

	uint64 now = Time::now();

	this->id = uvec3(0, 0, -1); // invalid id.
	this->quadNormals = dmat4(0.0);
	this->quadCorners = dmat4(0.0);
	this->cameraDistance = INFINITY;
	this->timeLastUsed = now;
	this->timeLastRetrieved = now;
	this->timeCreated = now;
	this->timeGenerated = 0;
	this->generated = false;
	this->awaitingGeneration = false;
	this->awaitingReadbackResponse = false;
	this->awaitingReadbackRequest = false;
	this->textureData = NULL;
	this->minHeight = 0.0;
	this->maxHeight = 0.0;
	this->textureData = new float[supplier->tileSize * supplier->tileSize * 4];
}

TileData::~TileData() {
	uint32 invalidReferences = 0;

	for (auto it = this->references.begin(); it != this->references.end(); it++) {
		TileData** store =*it;
		if (store != NULL &&*store == this) {
			*store = NULL;
		}
		else {
			invalidReferences++;
		}
	}

	if (invalidReferences > 0) { // This hsouldn't happen if the references list is properly maintained.
		logWarn("Deleted TileData with %d invalid references", invalidReferences);
	}

	if (this->textureData != NULL) {
		delete[] this->textureData;
	}

	// free the texture slot.
}

void TileData::onReadbackReceived() {
	this->timeReadback = Time::now();

	uint32 invalidPixels = 0;
	this->minHeight = +INFINITY;
	this->maxHeight = -INFINITY;
	for (int i = 0; i < supplier->getTileSize(); i++) {
		for (int j = 0; j < supplier->getTileSize(); j++) {
			double h = this->getTextureData(uvec2(i, j)).w;
	
			if (isnan(h)) {
				invalidPixels++;
			} else {
				this->minHeight = glm::min(this->minHeight, h);
				this->maxHeight = glm::max(this->maxHeight, h);
			}
		}
	}
	
	if (invalidPixels > 0) {
		logError("Invalid readback result (%d of %d pixels)", invalidPixels, supplier->getTileSize() * supplier->getTileSize());
	}

	//logInfo("Tile minHeight = %f, maxHeight = %f, height range = %f",
	//	this->minHeight,
	//	this->maxHeight,
	//	this->maxHeight - this->minHeight
	//);
}

void TileData::onUsed() {
	this->timeLastUsed = Time::now();
}

void TileData::regenerate() {
	this->supplier->markForGeneration(this);
}

void TileData::requestAsyncReadback() {
	this->awaitingReadbackResponse = true;
	this->awaitingReadbackRequest = true;
}

void TileData::setCameraDistance(double cameraDistance) {
	this->cameraDistance = cameraDistance;
}

dvec3 TileData::getSphereVector(dvec2 pos) const {
	dvec4 interp = dvec4(
		(0 + pos.x) * (0 + pos.y), // (0+x) * (0+y) // x * y
		(1 - pos.x) * (0 + pos.y), // (1-x) * (0+y) // z * y
		(1 - pos.x) * (1 - pos.y), // (1-x) * (1-y) // z * w
		(0 + pos.x) * (1 - pos.y)  // (0+x) * (1-y) // x * w
	);


	return normalize(dvec3(this->quadNormals * interp));
}

dmat4 TileData::getQuadNormals() const {
	return this->quadNormals;
}

dmat4 TileData::getQuadCorners() const {
	return this->quadCorners;
}

uint32 TileData::getTextureIndex() const {
	return this->textureIndex;
}

uint64 TileData::getTimeLastUsed() const {
	return this->timeLastUsed;
}

uint64 TileData::getTimeLastRetrieved() const {
	return this->timeLastRetrieved;
}

uint64 TileData::getTimeCreated() const {
	return this->timeCreated;
}

uint64 TileData::getTimeGenerated() const {
	return this->timeGenerated;
}

uint64 TileData::getTimeReadback() const {
	return this->timeReadback;
}

dvec4 TileData::getTextureData(uvec2 pos) {
	const uint32 textureSize = this->supplier->tileSize;

	if (pos.x >= 0 && pos.x < textureSize && pos.y >= 0 && pos.y < textureSize) {
		if (this->textureData != NULL) {
			uint32 index = pos.x + pos.y * textureSize;
			const double x = this->textureData[index * 4 + 0];
			const double y = this->textureData[index * 4 + 1];
			const double z = this->textureData[index * 4 + 2];
			const double w = this->textureData[index * 4 + 3];
			return dvec4(x, y, z, w);
		} else {
			this->requestAsyncReadback();
		}
	} else {
		logError("Tile texture coords [%d, %d] out of bounds", pos.x, pos.y);
	}

	return dvec4(NAN);
}

dvec4 TileData::getInterpolatedTextureData(dvec2 pos) {
	if (this->textureData != NULL) {
		const ivec2 p0 = ivec2(pos);
		if (fabs(pos.x - p0.x) < 1e-9 && fabs(pos.y - p0.y) < 1e-9) {
			return this->getTextureData(p0);
		} else {
			const ivec2 p1 = ivec2(pos + 1.0);

			const dvec4 d00 = this->getTextureData(ivec2(p0.x, p0.y));
			const dvec4 d10 = this->getTextureData(ivec2(p1.x, p0.y));
			const dvec4 d11 = this->getTextureData(ivec2(p1.x, p1.y));
			const dvec4 d01 = this->getTextureData(ivec2(p0.x, p1.y));

			const double xInterp = (pos.x - p0.x) / (p1.x - p0.x);
			const double yInterp = (pos.y - p0.y) / (p1.y - p0.y);
			const dvec4 id0 = d00 * xInterp + d10 * (1.0 - xInterp);
			const dvec4 id1 = d01 * xInterp + d11 * (1.0 - xInterp);
			return id0 * yInterp + id1 * (1.0 - yInterp);
		}
	} else {
		this->requestAsyncReadback();
		return fvec4(NAN);
	}
}

ivec2 TileData::getTextureSize() const {
	return ivec2(this->supplier->tileSize);
}

bool TileData::isGenerated() const {
	return this->generated;
}

bool TileData::isAwaitingGeneration() const {
	return this->awaitingGeneration;
}

bool TileData::isAwaitingReadback() const {
	return this->awaitingReadbackResponse;
}

bool TileData::isActive() const {
	return !this->references.empty(); // Active for as long as there are any references to this tile.
}

double TileData::getMaxHeight() const {
	return this->maxHeight;
}

double TileData::getMinHeight() const {
	return this->minHeight;
}



////////////////// TileData \\\\\\\\\\\\\\\\\\ 

//uint32 timerQuery;
TileSupplier::TileSupplier(Planet* planet, uint32 seed, uint32 textureCapacity, uint32 textureSize) {
	this->planet = planet;

	if (textureCapacity == 0) {
		int32 layers;
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &layers);
		textureCapacity = layers; // This is assuming that layers is not zero as well...

		logInfo("%d OpenGL texture array layers are available for terrain texture cache.", layers);
	}

	this->capacity = textureCapacity;
	this->tileSize = textureSize;

	this->timeLastCleanupCycle = Time::now();
	this->timeLastGenerationCheck = Time::now();
	this->cleanupFrequency = 1.0; // Every X second, we go through and cleanup the active tiles list.
	this->generationCheckFrequency = 0.5; // Every X seconds we check if we need to generate textures, and sort tiles by statance to the viewer.

	this->overlayDebug = false;
	this->showDebug = false;

	this->maxGenerationTime = 10.0;

	this->availableTextures = new bool[this->capacity];
	for (int i = 0; i < this->capacity; i++) {
		this->availableTextures[i] = true;
	}

	this->maxAsyncReadbacks = 10;
	this->asyncReadbackSlots = new AsyncReadbackRequest*[this->maxAsyncReadbacks];
	this->asyncReadbackTimeout = 3.0; // Timeout after 3 second.

	for (int i = 0; i < this->maxAsyncReadbacks; i++) {
		this->asyncReadbackSlots[i] = NULL;
	}

	// Initialize texture array
	this->textureArray = 0;
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &this->textureArray);

	if (this->textureArray == 0) {
		logError("Failed to allocate OpenGL texture array for terrain tile cache");
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, this->tileSize);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTextureUnit(0, this->textureArray);
	glTextureStorage3D(this->textureArray, 1, GL_RGBA32F, this->tileSize, this->tileSize, this->capacity);
	glTextureParameteri(this->textureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(this->textureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(this->textureArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(this->textureArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Initialize pixel transfer buffer

	glGenBuffers(1, &this->pixelTransferBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pixelTransferBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, this->tileSize * this->tileSize * this->maxAsyncReadbacks * sizeof(float) * 4, NULL, GL_STREAM_COPY);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);


	// Initialize compute shader.
	//glGenQueries(1, &timerQuery);
	this->tileGeneratorProgram = new ShaderProgram();
	this->tileGeneratorProgram->addShader(GL_COMPUTE_SHADER, "simpleTerrain/heightComp.glsl");
	this->tileGeneratorProgram->completeProgram();
}

TileSupplier::~TileSupplier() {
	// Nullify all references to all tiles, and delete all tiles.
	// destroy OpenGL texture array

	//TODO
	// (this isn't really a problematic memory leak, since planets are only being created once at the application start and never again... but this should still be implemented)
}


void TileSupplier::generateTexture(TileData* tile) {

	this->numTexturesGenerated++;

	this->tileGeneratorProgram->useProgram(true);
	glBindImageTexture(0, this->textureArray, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	this->tileGeneratorProgram->setUniform("computePointBuffers", false);
	this->tileGeneratorProgram->setUniform("tileTexture", 0);
	this->tileGeneratorProgram->setUniform("textureIndex", (int32)tile->textureIndex);
	this->tileGeneratorProgram->setUniform("planetRadius", (float)this->planet->getRadius());
	this->tileGeneratorProgram->setUniform("elevationScale", (float)this->planet->getElevationScale());
	this->tileGeneratorProgram->setUniform("quadNormals", tile->getQuadNormals());
	this->tileGeneratorProgram->setUniform("quadCorners", tile->getQuadCorners());
	this->tileGeneratorProgram->setUniform("textureSize", (int32)this->tileSize);
	
	int textureSize = this->tileSize;
	int localSizeX = 16;
	int localSizeY = 16;
	int xGroups = ceil((float)textureSize / localSizeX);
	int yGroups = ceil((float)textureSize / localSizeY);

	//glBeginQuery(GL_TIME_ELAPSED, timerQuery);
	glDispatchCompute(xGroups, yGroups, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	//glEndQuery(GL_TIME_ELAPSED);

	//uint64 elapsed = 0;
	//glGetQueryObjectui64v(timerQuery, GL_QUERY_RESULT, &elapsed);

	//logInfo("Took %f ms to execute compute shader", elapsed / 1000000.0);

	tile->requestAsyncReadback();

	this->tileGeneratorProgram->useProgram(false);

	tile->generated = true;
	tile->awaitingGeneration = false;
	tile->timeGenerated = Time::now();
}

void TileSupplier::markForGeneration(TileData* tile) {
	tile->awaitingGeneration = true;
	this->textureGenerationQueue.push_back(tile);
}

void TileSupplier::markIdle(TileData* tile) {
	constexpr bool debug = true;

	if (!tile->references.empty()) { // If the tile has references, invalidate them
		for (auto rit = tile->references.begin(); rit != tile->references.end(); rit++) {
			TileData** store =*rit;

			if (store != NULL) {
				assert(*store == tile); // Tile store must reference the tile.
				*store = NULL; // Nullify the external reference to the tile.
			}
		}

		tile->references.clear();
	}

	// TODO: pass iterators if they are known, since they are known before calling this functin in some cases.

	// Find the tile in the active cache, and make sure this is the correct tile.
	ActiveCacheIterator aci = this->activeTiles.find(tile->id);
	//if (aci != this->activeTiles.end()) {
		assert(aci != this->activeTiles.end());
		assert(aci->second == tile);

		if (debug) {
			// Make sure the tile is not in the idle cache.
			IdleCacheIterator ici = this->idleTiles.find(tile->id);
			assert(ici == this->idleTiles.end());
		}

		assert(tile->references.empty());

		this->activeTiles.erase(aci); // Tile is no longer active, remove it from the active cache.
		this->idleTiles[tile->id] = this->lruList.insert(this->lruList.end(), tile); // Add the tile to the idle cache, and to the end of the LRU list.
	//}
}

void TileSupplier::update() {
	uint64 now = Time::now();

	const double elapsedSinceLastCleanup = Time::time_cast<Time::time_unit, Time::seconds, double>(now - this->timeLastCleanupCycle);
	const double elapsedSinceLastGenerationCheck = Time::time_cast<Time::time_unit, Time::seconds, double>(now - this->timeLastGenerationCheck);

	// Either perform a cleanup pass, a generation check / sorting pass, or a texture generation pass.
	// Don't perform multiple in the same frame to avoid a larger than usual frame spike.

	if (elapsedSinceLastCleanup >= this->cleanupFrequency) { // cleanup pass
		this->timeLastCleanupCycle = now;

		int32 pendingReadbacks = 0;

		for (int i = 0; i < this->maxAsyncReadbacks; i++) {
			if (this->asyncReadbackSlots[i] != NULL) {
				pendingReadbacks++;
			}
		}

		//logInfo("%d tiles exist, %d active tiles, %d idle tiles. %d textures generated, %d textures remaining, %d tiles expired, %d async readbacks, %d pending readbacks",
		//	this->activeTiles.size() + this->idleTiles.size(),
		//	this->activeTiles.size(),
		//	this->idleTiles.size(),
		//	this->numTexturesGenerated,
		//	this->textureGenerationQueue.size(),
		//	this->numTilesExpired,
		//	this->textureReadbackQueue.size(),
		//	pendingReadbacks
		//);

		this->numTexturesGenerated = 0;
		this->numTilesExpired = 0;

		// TODO: tileTimeout should be dynamically determined. Lower framerates for example would need larger timeout
		// period, otherwise it is consievable that the frame time could drop so low that the tiles timeout every frame.
		// Furthermore, if the player is moving extremely quickly, tiles should possibly have a shorter timeout, so that
		// the tile supplier can keep up better with the vast number of requested tiles.
		double tileTimeout = 1.0; // 1 second

		std::vector<TileData*> expiredTiles; // Need separete list because deactivating the tile would modify the activeTiles cache while iterating it.

		for (ActiveCacheIterator aci = this->activeTiles.begin(); aci != this->activeTiles.end();) {
			uvec3 id = aci->first;
			TileData* tile = aci->second;

			bool flag = false; // true if we need to erase this iterator.

			if (tile != NULL) {
				assert(tile->id == id);

				const double elapsedSinceLastUsed = Time::time_cast<Time::time_unit, Time::seconds, double>(now - tile->timeLastUsed);
				if (elapsedSinceLastUsed > tileTimeout) { // Tile has been unused for too long, so will be automatically freed.
					expiredTiles.push_back(tile);
					this->numTilesExpired++;
				}
			}
			else {
				flag = true; // Null tile... somehow... erase it. Maybe this should be an assertion
			}

			if (flag) {
				aci = this->activeTiles.erase(aci);
			}
			else {
				aci++;
			}
		}

		for (auto it = expiredTiles.begin(); it != expiredTiles.end(); it++) {
			this->markIdle(*it);
		}
	} else if (elapsedSinceLastGenerationCheck >= this->generationCheckFrequency) { // generation check / sorting pass
		this->timeLastGenerationCheck = now;
		this->textureGenerationQueue.clear();
		this->textureReadbackQueue.clear();

		struct TileProcessor {
			static void process(std::vector<TileData*>& textureGenerationQueue, std::vector<TileData*>& textureReadbackQueue, uvec3 id, TileData* tile) {
				if (tile != NULL) {
					assert(tile->id == id);

					if (tile->awaitingGeneration) {
						textureGenerationQueue.push_back(tile);
					}

					if (tile->awaitingReadbackRequest) {
						textureReadbackQueue.push_back(tile);
					}
				}
			}
		};

		for (IdleCacheIterator ici = this->idleTiles.begin(); ici != this->idleTiles.end(); ici++) {
			uvec3 id = ici->first;
			TileData* tile = *ici->second;
			TileProcessor::process(this->textureGenerationQueue, this->textureReadbackQueue, id, tile);
		}

		const uint32 idleEndGenerationIndex = this->textureGenerationQueue.size();
		const uint32 idleEndReadbackIndex = this->textureReadbackQueue.size();

		for (ActiveCacheIterator aci = this->activeTiles.begin(); aci != this->activeTiles.end(); aci++) {
			uvec3 id = aci->first;
			TileData* tile = aci->second;
			TileProcessor::process(this->textureGenerationQueue, this->textureReadbackQueue, id, tile);
		}

		auto comparator = [](const TileData* t0, const TileData* t1) {
			return t1->cameraDistance < t0->cameraDistance;
		};

		std::sort(this->textureGenerationQueue.begin(), this->textureGenerationQueue.begin() + idleEndGenerationIndex, comparator);
		std::sort(this->textureGenerationQueue.begin() + idleEndGenerationIndex, this->textureGenerationQueue.end(), comparator);

		std::sort(this->textureReadbackQueue.begin(), this->textureReadbackQueue.begin() + idleEndReadbackIndex, comparator);
		std::sort(this->textureReadbackQueue.begin() + idleEndReadbackIndex, this->textureReadbackQueue.end(), comparator);

		if (!this->textureGenerationQueue.empty()) {
			logInfo("Found %d tiles that need generation", this->textureGenerationQueue.size());
		}
	} else { // actual generation pass
		if (!this->textureGenerationQueue.empty()) {
			uint64 start = Time::now();

			while (!this->textureGenerationQueue.empty()) {
				now = Time::now();
				TileData* tile = this->textureGenerationQueue.back();
				this->textureGenerationQueue.pop_back();

				if (tile != NULL) {
					this->generateTexture(tile); // Generate the current closest tile to the viewer.

					const double elapsedGenerationTime = Time::time_cast<Time::time_unit, Time::milliseconds, double>(now - start);
					if (elapsedGenerationTime >= this->maxGenerationTime) {
						break; // If the time spent generating textures exceeds the time limit per frame, break and let the next frame pick up more textures.
					}
				}
			}
		}

		// process asynchronous texture readback requests
		if (!this->textureReadbackQueue.empty()) {
			while (!this->textureReadbackQueue.empty()) {
				int readbackIndex = -1;
				for (int i = 0; i < this->maxAsyncReadbacks; i++) {
					if (this->asyncReadbackSlots[i] == NULL) {
						readbackIndex = i;
						break;
					}
				}
		
				if (readbackIndex == -1) {
					break;
				}
		
				TileData* tile = this->textureReadbackQueue.back();
				this->textureReadbackQueue.pop_back();
				if (tile != NULL && tile->awaitingReadbackRequest) {
					tile->awaitingReadbackRequest = false;
		
					int32 err = 0;
		
					uint32 readSize = this->tileSize * this->tileSize * sizeof(float) * 4;
					uint32 offset = readbackIndex * readSize;
					glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pixelTransferBuffer);
					glBindTextureUnit(0, this->textureArray);
					glGetTextureSubImage(this->textureArray, 0, 0, 0, tile->textureIndex, this->tileSize, this->tileSize, 1, GL_RGBA, GL_FLOAT, readSize, (char*) offset);
		
					AsyncReadbackRequest* request = new AsyncReadbackRequest();

					request->requestTime = Time::now();
					request->tile = tile;
					request->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	
					this->asyncReadbackSlots[readbackIndex] = request;
				}
			}
		}
		
		for (int i = 0; i < this->maxAsyncReadbacks; i++) {
			AsyncReadbackRequest* request = this->asyncReadbackSlots[i];
			if (request != NULL) {
				int32 result;
				glGetSynciv(request->sync, GL_SYNC_STATUS, sizeof(result), NULL, &result);
		
				if (result == GL_SIGNALED) {
					glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pixelTransferBuffer);
					uint32 readSize = this->tileSize * this->tileSize * sizeof(float) * 4;
					uint32 offset = i * readSize;
					float* data = static_cast<float*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, offset, readSize, GL_MAP_READ_BIT));
		
					if (request->tile->textureData == NULL) {
						request->tile->textureData = new float[this->tileSize * this->tileSize * 4];
					}
					
					std::memcpy(request->tile->textureData, data, readSize);
					
					// ================= REMOVE THIS =================
					glTextureSubImage3D(this->textureArray, 0, 0, 0, request->tile->textureIndex, this->getTileSize(), this->getTileSize(), 1, GL_RGBA, GL_FLOAT, data); // debug test
					// ===============================================

					request->tile->onReadbackReceived();
					glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		
					glDeleteSync(request->sync);
					delete request;
					this->asyncReadbackSlots[i] = NULL;
				}  else {
					if (Time::time_cast<Time::time_unit, Time::seconds, double>(Time::now() - request->requestTime) > this->asyncReadbackTimeout) {
						glDeleteSync(request->sync);
						delete request;
						this->asyncReadbackSlots[i] = NULL;
		
						logInfo("Async texture readback timed out");
						// maybe notify the tile of the cancelled request?
					}
				}
			}
		}
	}
}

void TileSupplier::computePointData(int32 count, fvec3* points, fvec4* data) {

	this->tileGeneratorProgram->useProgram(true);

	fvec4* p = new fvec4[count];
	std::transform(points, points + count, p, [](fvec3 point) { return fvec4(point, 0.0F); });

	uint32* ssbo = new uint32[2];
	glGenBuffers(2, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(fvec4), p, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[0]);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(fvec4), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[1]);

	delete[] p;

	this->tileGeneratorProgram->setUniform("computePointBuffers", true);
	this->tileGeneratorProgram->setUniform("pointBufferSize", count);
	this->tileGeneratorProgram->setUniform("tileTexture", 0);
	this->tileGeneratorProgram->setUniform("planetRadius", (float)this->planet->getRadius());
	this->tileGeneratorProgram->setUniform("elevationScale", (float)this->planet->getElevationScale());
	this->tileGeneratorProgram->setUniform("textureSize", (int32)this->tileSize);
	
	int localSizeX = 16;
	int localSizeY = 16;
	int xGroups = (count + localSizeX * localSizeY) / (localSizeX * localSizeY);
	int yGroups = 1;


	glDispatchCompute(xGroups, yGroups, 1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	
	this->tileGeneratorProgram->useProgram(false);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
	fvec4* mappedData = static_cast<fvec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(fvec4), GL_MAP_READ_BIT));
	memcpy(data, mappedData, count * sizeof(fvec4));

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glDeleteBuffers(2, ssbo);
}

void TileSupplier::getTileData(TerrainQuad* terrainQuad, TileData** store) {
	uvec3 id = terrainQuad->getTreePosition(true);
	TileData* tile = NULL;

	ActiveCacheIterator aci = this->activeTiles.find(id);
	if (aci == this->activeTiles.end()) { // Tile is not in the active cache

		IdleCacheIterator ici = this->idleTiles.find(id);
		if (ici == this->idleTiles.end()) { // The tile is not in the idle cache
			// The tile is not in either cache, so must be created or reallocated from an existing tile.

			// TODO: dont iterate this every time... find faster method.
			int nextAvailableTextureIndex = -1;
			for (int i = 0; i < this->capacity; i++) {
				if (this->availableTextures[i]) {
					nextAvailableTextureIndex = i;
					break;
				}
			}

			if (nextAvailableTextureIndex != -1) { // A texture slot is available and unused.
				tile = new TileData(this, nextAvailableTextureIndex);
				this->availableTextures[nextAvailableTextureIndex] = false;
			} else if (!this->idleTiles.empty()) { // No textures are available. Find least recently used tile to overwrite.
				LRUIterator lit = this->lruList.begin();
				tile = *lit;
				assert(tile != NULL); // Should not be in the unused list if it is null or has references.
				assert(tile->references.empty());

				this->lruList.erase(lit);
				this->idleTiles.erase(tile->id);
			} else { // No idle tiles are available to overwrite.
				tile = NULL;
			}

			if (tile != NULL) {
				tile->id = id;

				tile->quadNormals = terrainQuad->getWorldNormals();
				tile->quadCorners = terrainQuad->getWorldCorners();
				tile->generated = false; // Texture should not be read if this is false.
				this->markForGeneration(tile); // Mark the tile for texture generation for the new ID
			}

		} else { // Tile was found in the idle cache. It is no longer idle, so remove it from the cache.
			LRUIterator lit = ici->second;
			tile =*lit;

			assert(tile != NULL);
			assert(tile->id == id);

			this->idleTiles.erase(ici);
			this->lruList.erase(lit);
		}

		if (tile != NULL) { // Tile is now active.
			this->activeTiles[id] = tile;
		}
	} else { // The tile was found in the active cache. Just return it.
		tile = aci->second;

		assert(tile != NULL);
		assert(tile->id == id);
	}

	if (tile != NULL) { // Tile may be null if it could not be generated
		tile->references.insert(store);
		tile->timeLastRetrieved = Time::now();
		//tile->sphereVectors = terrainQuad->getWorldNormals();
	}

	*store = tile;
}

bool TileSupplier::putTileData(TileData** store) {
	bool flag = false;

	if (store != NULL) {
		TileData* tile = *store;

		if (tile != NULL) {
			auto it = std::find(tile->references.begin(), tile->references.end(), store);

			if (it != tile->references.end()) {
				tile->references.erase(it);
				flag = true;
			} else {
				logWarn("Tile reference was not found when freeing the TileData");
			}

			if (tile->references.empty()) { // no more references... mark the tile as idle.
				this->markIdle(tile);
			}

			*store = NULL; // Nullify the external reference
		}
	}

	return flag; // Returns true if the tile store was erased from the tiles references list.
}

void TileSupplier::applyUniforms(ShaderProgram* program) {
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureArray);
	glBindTextureUnit(0, this->textureArray);
	program->setUniform("heightSampler", 0);
	program->setUniform("overlayDebug", this->overlayDebug);
	program->setUniform("showDebug", this->showDebug);
	program->setUniform("textureSize", (int32) this->tileSize);
}

uint32 TileSupplier::getTileSize() const {
	return this->tileSize;
}

bool TileSupplier::isShowDebug() const {
	return this->showDebug;
}

bool TileSupplier::isOverlayDebug() const {
	return this->overlayDebug;
}

void TileSupplier::setShowDebug(bool show) {
	this->showDebug = show;
}

void TileSupplier::setOverlayDebug(bool show) {
	this->overlayDebug = show;
}
