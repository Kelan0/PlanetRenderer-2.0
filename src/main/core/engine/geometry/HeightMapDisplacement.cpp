#include "Displacement.h"


HeightMapDisplacement::HeightMapDisplacement(int32 xSize, int32 ySize, float** heightMap) {
	if (xSize <= 0 || ySize <= 0) {
		throw "Invalid height map dimensions";
	}

	this->xSize = xSize;
	this->ySize = ySize;
	this->heightMap = new float*[xSize]; // array of columns

	for (int i = 0; i < xSize; i++) {
		this->heightMap[i] = new float[ySize]; // array of column values

		for (int j = 0; j < ySize; j++) {
			this->heightMap[i][j] = 10.0;

			if (heightMap != NULL && heightMap[i] != NULL) {
				this->heightMap[i][j] = heightMap[i][j];
			}
		}
	}
}

HeightMapDisplacement::~HeightMapDisplacement() {
	for (int i = 0; i < this->ySize; i++) {
		delete[] this->heightMap[i];
	}

	delete[] this->heightMap;
}

float HeightMapDisplacement::getHeight(int x, int y) const {
	x = glm::max(glm::min(x, xSize - 1), 0);
	y = glm::max(glm::min(y, ySize - 1), 0);

	return this->heightMap[x][y];
}

float HeightMapDisplacement::getHeight(float x, float y) const {
	float mx = x * (this->xSize - 1);
	float my = y * (this->ySize - 1);

	int x0 = floor(mx);
	int y0 = floor(my);
	int x1 = ceil(mx);
	int y1 = ceil(my);

	float tx = mx - x0;
	float ty = my - y0;

	// interpolate between the 4 height values that [x,y] lands on
	float h00 = this->getHeight(x0, y0);
	float h10 = this->getHeight(x1, y0);
	float h11 = this->getHeight(x1, y1);
	float h01 = this->getHeight(x0, y1);

	float hx0 = h00 + tx * (h10 - h00);
	float hx1 = h01 + tx * (h11 - h01);
	return hx0 + ty * (hx1 - hx0);
}

fvec2 HeightMapDisplacement::getDerivative(float x, float y, float distance) const {
	int w = this->xSize - 1;
	int h = this->ySize - 1;

	float hw = this->getHeight((x + distance), y);
	float he = this->getHeight((x - distance), y);
	float hn = this->getHeight(x, (y + distance));
	float hs = this->getHeight(x, (y - distance));

	return vec2(he - hw, hs - hn);
}

fvec3 HeightMapDisplacement::getNormal(float x, float y) const {
	float eps = 0.05;
	fvec2 d = this->getDerivative(x, y, eps) * eps;
	// v = [+0.1, +d.x, 0] - [-0.1, -d.x, 0]
	// u = [0, +d.y, +0.1] - [0, -d.y, -0.1]

	fvec3 v = normalize(vec3(-eps, d.x, 0.0F));
	fvec3 u = normalize(vec3(0.0F, d.y, -eps));
	return cross(u, v);
}

int32 HeightMapDisplacement::getXSize() const {
	return this->xSize;
}

int32 HeightMapDisplacement::getYSize() const {
	return this->ySize;
}
