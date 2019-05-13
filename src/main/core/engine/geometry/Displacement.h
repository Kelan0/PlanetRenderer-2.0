#pragma once

#include "core/Core.h"

class Displacement {
public:
	Displacement() {}

	virtual ~Displacement() {}

	virtual float getHeight(float x, float y) const = 0;

	virtual fvec2 getDerivative(float x, float y, float distance = 1.0F) const = 0;

	virtual fvec3 getNormal(float x, float y) const = 0;
};

class HeightMapDisplacement : public Displacement {
private:
	float** heightMap;
	int32 xSize;
	int32 ySize;

public:
	HeightMapDisplacement(int32 width, int32 height, float** heightMap = NULL);

	~HeightMapDisplacement();

	float getHeight(int x, int y) const;

	float getHeight(float x, float y) const override;

	fvec2 getDerivative(float x, float y, float distance = 1.0F) const override;

	fvec3 getNormal(float x, float y) const override;

	int32 getXSize() const;

	int32 getYSize() const;
};