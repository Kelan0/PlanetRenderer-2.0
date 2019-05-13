#pragma once

#include "core/Core.h"

struct Vertex;
struct VertexLayout;
struct VertexAttribute;
//class MeshData;

class Displacement;

#define DEFAULT_VERTEX_LAYOUT VertexLayout(32, {	\
	VertexAttribute(0, 3, 0),						\
	VertexAttribute(1, 3, 12),						\
	VertexAttribute(2, 2, 24)						\
})

struct Vertex {
	fvec3 position;
	fvec3 normal;
	fvec2 texture;

	Vertex(fvec3 position, fvec3 normal = fvec3(0.0), fvec2 texture = fvec2(0.0)) :
		position(position), normal(normal), texture(texture) {}
};

struct VertexLayout {
	int32 stride;
	std::vector<VertexAttribute> attributes;
	std::function<std::vector<float>(Vertex)> getFormattedVertexData;

	VertexLayout(int32 stride, std::vector<VertexAttribute> attributes):
		stride(stride), attributes(attributes), getFormattedVertexData([](Vertex v) -> std::vector<float> { return std::vector<float> {float(v.position.x), float(v.position.y), float(v.position.z), float(v.normal.x), float(v.normal.y), float(v.normal.z), float(v.texture.x), float(v.texture.y)}; }) {}

	VertexLayout(int32 stride, std::vector<VertexAttribute> attributes, std::function<std::vector<float>(Vertex)> formatVertexBytes):
		stride(stride), attributes(attributes), getFormattedVertexData(formatVertexBytes) {}
};

struct VertexAttribute {
	int32 index;
	int32 size;
	int32 offset;

	VertexAttribute(int32 index, int32 size, int32 offset) :
		index(index), size(size), offset(offset) {}
};

class MeshData {
private:
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	bool meshChanged;
	std::vector<float> vertexData;

	VertexLayout vertexLayout;
public:
	MeshData(std::vector <Vertex> vertices = {}, std::vector<uint32> indices = {}, VertexLayout vertexLayout = DEFAULT_VERTEX_LAYOUT);

	MeshData(int32 reservedVertices, int32 reservedIndices, VertexLayout vertexLayout = DEFAULT_VERTEX_LAYOUT);

	~MeshData() {}

	int32 addVertex(Vertex vertex, int32 index = -1);

	bool addIndex(uint32 index);

	bool addFace(uint32 i0, uint32 i1, uint32 i2);

	bool addFace(uint32 i0, uint32 i1, uint32 i2, uint32 i3);

	int32 getVertexCount() const;

	int32 getVertexBufferSize() const;

	void* getVertexBufferData();

	int32 getIndexCount() const;

	int32 getIndexBufferSize() const;

	void* getIndexBufferData();

	VertexLayout getVertexLayout() const;
};

namespace MeshHelper {
	// Create a cuboid. Defaults to a centered unit cube.
	MeshData* createCuboid(fvec3 min = fvec3(0.5), fvec3 max = fvec3(NAN), VertexLayout vertexLayout = DEFAULT_VERTEX_LAYOUT);

	MeshData* createPlane(fvec2 min, fvec2 max = fvec2(NAN), ivec2 divisions = ivec2(1, 1), mat3 orientation = mat3(1.0), bool regular = false, Displacement* displacement = NULL, VertexLayout vertexLayout = DEFAULT_VERTEX_LAYOUT);
};

inline Vertex operator*(const dmat4& matrix, const Vertex& vertex) {
	return Vertex(
		fvec3(matrix * dvec4(vertex.position, 1.0)),
		fvec3(matrix * dvec4(vertex.normal, 0.0)),
		fvec2(vertex.texture)
	);
}