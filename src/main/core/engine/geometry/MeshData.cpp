#include "MeshData.h"
#include "core/application/Application.h"
#include "core/engine/geometry/Displacement.h"
#include "core/util/Logger.h"

MeshData::MeshData(std::vector<Vertex> vertices, std::vector<uint32> indices, VertexLayout vertexLayout):
	vertices(vertices), indices(indices), vertexLayout(vertexLayout), meshChanged(true) {}

MeshData::MeshData(int32 reservedVertices, int32 reservedIndices, VertexLayout vertexLayout):
	vertices({}), indices({}), vertexLayout(vertexLayout), meshChanged(true) {
	this->vertices.reserve(reservedVertices);
	this->indices.reserve(reservedIndices);
}

int32 MeshData::addVertex(Vertex vertex, int32 index) {
	if (index > 0 && index < this->vertices.size()) {
		this->vertices.insert(this->vertices.begin() + index, vertex);
	} else {
		index = this->vertices.size();
		this->vertices.push_back(vertex);
	}

	this->meshChanged = true;

	return index;
}

bool MeshData::addIndex(uint32 index) {
	this->indices.push_back(index);
	this->meshChanged = true;
	return true;
}

bool MeshData::addFace(uint32 i0, uint32 i1, uint32 i2) {
	this->addIndex(i0);
	this->addIndex(i1);
	this->addIndex(i2);

	return true;
}

bool MeshData::addFace(uint32 i0, uint32 i1, uint32 i2, uint32 i3) {
	// TODO: auto winding order
	this->addFace(i0, i1, i2);
	this->addFace(i0, i2, i3);

	return true;
}

void MeshData::clear() {
	this->vertices.clear();
	this->indices.clear();
	this->vertexData.clear();
	this->meshChanged = true;
}

int32 MeshData::getVertexCount() const {
	return this->vertices.size();
}

int32 MeshData::getVertexBufferSize() const {
	return this->vertexLayout.stride * this->vertices.size();
}

void* MeshData::getVertexBufferData() {
	if (this->meshChanged) {
		this->meshChanged = false;

		bool flag = true;
		this->vertexData.clear();

		for (int i = 0; i < this->vertices.size(); i++) {
			std::vector<float> vertex = this->vertexLayout.getFormattedVertexData(this->vertices[i]);

			if (flag) {
				this->vertexData.reserve(this->vertices.size() * vertex.size());
				flag = false;
			}

			this->vertexData.insert(this->vertexData.end(), vertex.begin(), vertex.end());
		}
	}

	return static_cast<void*>(&this->vertexData[0]);
}

int32 MeshData::getIndexCount() const {
	return this->indices.size();
}

int32 MeshData::getIndexBufferSize() const {
	return sizeof(uint32) * this->indices.size();
}

void* MeshData::getIndexBufferData() {
	return static_cast<void*>(&this->indices[0]);
}

VertexLayout MeshData::getVertexLayout() const {
	return this->vertexLayout;
}

namespace MeshHelper {
	MeshData* MeshHelper::createCuboid(fvec3 min, fvec3 max, VertexLayout vertexLayout) {
		max.x = isnan(max.x) ? -min.x : max.x;
		max.y = isnan(max.y) ? -min.y : max.y;
		max.z = isnan(max.z) ? -min.z : max.z;

		if (isnan(max.x + max.y + max.z)) { // any components are still NaN, means both min and max are NaN, so this is an invalid geometry.
			return NULL;
		}

		fvec3 temp = min;
		min = glm::min(temp, max);
		max = glm::max(temp, max);

		MeshData* meshData = new MeshData(24, 36, vertexLayout);

		// negative x
		int32 v00 = meshData->addVertex(Vertex(vec3(min.x, min.y, min.z), fvec3(-1.0F, 0.0F, 0.0F), fvec2(0.0, 0.0))); // ---
		int32 v01 = meshData->addVertex(Vertex(vec3(min.x, max.y, min.z), fvec3(-1.0F, 0.0F, 0.0F), fvec2(1.0, 0.0))); // -+-
		int32 v02 = meshData->addVertex(Vertex(vec3(min.x, max.y, max.z), fvec3(-1.0F, 0.0F, 0.0F), fvec2(1.0, 1.0))); // -++
		int32 v03 = meshData->addVertex(Vertex(vec3(min.x, min.y, max.z), fvec3(-1.0F, 0.0F, 0.0F), fvec2(0.0, 1.0))); // --+
		// positive x
		int32 v04 = meshData->addVertex(Vertex(vec3(max.x, min.y, min.z), fvec3(+1.0, 0.0F, 0.0F), fvec2(0.0, 0.0))); // +--
		int32 v05 = meshData->addVertex(Vertex(vec3(max.x, max.y, min.z), fvec3(+1.0, 0.0F, 0.0F), fvec2(1.0, 0.0))); // ++-
		int32 v06 = meshData->addVertex(Vertex(vec3(max.x, max.y, max.z), fvec3(+1.0, 0.0F, 0.0F), fvec2(1.0, 1.0))); // +++
		int32 v07 = meshData->addVertex(Vertex(vec3(max.x, min.y, max.z), fvec3(+1.0, 0.0F, 0.0F), fvec2(0.0, 1.0))); // +-+
		// negative y
		int32 v08 = meshData->addVertex(Vertex(vec3(min.x, min.y, min.z), fvec3(0.0F, -1.0F, 0.0F), fvec2(0.0, 0.0))); // ---
		int32 v09 = meshData->addVertex(Vertex(vec3(max.x, min.y, min.z), fvec3(0.0F, -1.0F, 0.0F), fvec2(1.0, 0.0))); // +--
		int32 v10 = meshData->addVertex(Vertex(vec3(max.x, min.y, max.z), fvec3(0.0F, -1.0F, 0.0F), fvec2(1.0, 1.0))); // +-+
		int32 v11 = meshData->addVertex(Vertex(vec3(min.x, min.y, max.z), fvec3(0.0F, -1.0F, 0.0F), fvec2(0.0, 1.0))); // --+
		// positive y
		int32 v12 = meshData->addVertex(Vertex(vec3(min.x, max.y, min.z), fvec3(0.0F, +1.0F, 0.0F), fvec2(0.0, 0.0))); // -+-
		int32 v13 = meshData->addVertex(Vertex(vec3(max.x, max.y, min.z), fvec3(0.0F, +1.0F, 0.0F), fvec2(1.0, 0.0))); // ++-
		int32 v14 = meshData->addVertex(Vertex(vec3(max.x, max.y, max.z), fvec3(0.0F, +1.0F, 0.0F), fvec2(1.0, 1.0))); // +++
		int32 v15 = meshData->addVertex(Vertex(vec3(min.x, max.y, max.z), fvec3(0.0F, +1.0F, 0.0F), fvec2(0.0, 1.0))); // -++
		// negative z
		int32 v16 = meshData->addVertex(Vertex(vec3(min.x, min.y, min.z), fvec3(0.0F, 0.0F, -1.0F), fvec2(0.0, 0.0))); // ---
		int32 v17 = meshData->addVertex(Vertex(vec3(max.x, min.y, min.z), fvec3(0.0F, 0.0F, -1.0F), fvec2(1.0, 0.0))); // +--
		int32 v18 = meshData->addVertex(Vertex(vec3(max.x, max.y, min.z), fvec3(0.0F, 0.0F, -1.0F), fvec2(1.0, 1.0))); // ++-
		int32 v19 = meshData->addVertex(Vertex(vec3(min.x, max.y, min.z), fvec3(0.0F, 0.0F, -1.0F), fvec2(0.0, 1.0))); // -+-
		// positive z
		int32 v20 = meshData->addVertex(Vertex(vec3(min.x, min.y, max.z), fvec3(0.0F, 0.0F, +1.0F), fvec2(0.0, 0.0))); // --+
		int32 v21 = meshData->addVertex(Vertex(vec3(max.x, min.y, max.z), fvec3(0.0F, 0.0F, +1.0F), fvec2(1.0, 0.0))); // +-+
		int32 v22 = meshData->addVertex(Vertex(vec3(max.x, max.y, max.z), fvec3(0.0F, 0.0F, +1.0F), fvec2(1.0, 1.0))); // +++
		int32 v23 = meshData->addVertex(Vertex(vec3(min.x, max.y, max.z), fvec3(0.0F, 0.0F, +1.0F), fvec2(0.0, 1.0))); // -++

		meshData->addFace(v00, v01, v02, v03);
		meshData->addFace(v04, v05, v06, v07);
		meshData->addFace(v08, v09, v10, v11);
		meshData->addFace(v12, v13, v14, v15);
		meshData->addFace(v16, v17, v18, v19);
		meshData->addFace(v20, v21, v22, v23);

		return meshData;
	}

	MeshData* MeshHelper::createPlane(fvec2 min, fvec2 max, ivec2 divisions, mat3 orientation, bool regular, Displacement* displacement, VertexLayout vertexLayout) {

		max.x = isnan(max.x) ? -min.x : max.x;
		max.y = isnan(max.y) ? -min.y : max.y;

		if (isnan(max.x + max.y)) { // any components are still NaN, means both min and max are NaN, so this is an invalid geometry.
			return NULL;
		}

		int32 xDiv = glm::max(1, divisions.x);
		int32 yDiv = glm::max(1, divisions.y);

		const int32 vertexCount = (xDiv + 1) * (yDiv + 1) + (regular ? (xDiv * yDiv) : 0);
		const int32 indexCount = xDiv * yDiv * (regular ? 12 : 6);

		MeshData* meshData = new MeshData(vertexCount, indexCount, vertexLayout);

		int32* indices = new int32[vertexCount];

		int32 i, j;
		float u, v, x, y, uc, vc, xc, yc;
		fvec3 p, n;
		fvec2 t;

		for (i = 0; i < xDiv + 1; i++) {
			u = (float)i / xDiv;
			x = min.x + u * (max.x - min.x);

			if (regular && i < xDiv) {
				uc = (float)(i + 0.5F) / xDiv;
				xc = min.x + uc * (max.x - min.x);
			}

			for (j = 0; j < yDiv + 1; j++) {
				v = (float)j / yDiv;
				y = min.y + v * (max.y - min.y);

				if (regular && i < xDiv && j < yDiv) {
					vc = (float)(j + 0.5F) / yDiv;
					yc = min.y + vc * (max.y - min.y);

					p = orientation[0] * xc + orientation[2] * yc;
					n = orientation[1];
					t = vec2(u, v);

					if (displacement != NULL) {
						fvec2 d = displacement->getDerivative(uc, vc, 2.0F);
						p += orientation[1] * displacement->getHeight(uc, vc);
						//n += orientation[0] * d.x + orientation[1] * d.y;
						n = displacement->getNormal(uc, vc);
					}
					indices[(xDiv + 1) * (yDiv + 1) + i * yDiv + j] = meshData->addVertex(Vertex(p, n, t));
				}

				p = orientation[0] * x + orientation[2] * y;
				n = orientation[1];
				t = vec2(u, v);

				if (displacement != NULL) {
					fvec2 d = displacement->getDerivative(u, v, 2.0F);
					p += orientation[1] * displacement->getHeight(u, v);
					//n += orientation[0] * d.x + orientation[1] * d.y;
					n = displacement->getNormal(u, v);
				}
				indices[i * (yDiv + 1) + j] = meshData->addVertex(Vertex(p, n, t));
			}
		}

		int32 v0, v1, v2, v3, v4;

		for (i = 0; i < xDiv; i++) {
			for (j = 0; j < yDiv; j++) {
				v0 = indices[(i + 0) * (yDiv + 1) + (j + 0)];
				v1 = indices[(i + 1) * (yDiv + 1) + (j + 0)];
				v2 = indices[(i + 1) * (yDiv + 1) + (j + 1)];
				v3 = indices[(i + 0) * (yDiv + 1) + (j + 1)];

				if (regular) {
					v4 = indices[(xDiv + 1) * (yDiv + 1) + i * yDiv + j];
					meshData->addFace(v0, v1, v4);
					meshData->addFace(v1, v2, v4);
					meshData->addFace(v2, v3, v4);
					meshData->addFace(v3, v0, v4);
				} else {
					meshData->addFace(v0, v1, v2, v3);
				}
			}
		}

		delete[] indices;

		return meshData;
	}
};

