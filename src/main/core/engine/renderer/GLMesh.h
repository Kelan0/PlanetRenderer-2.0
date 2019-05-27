#pragma once
#include "core/Core.h"
#include "core/engine/geometry/MeshData.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

class GLMesh;
class InstanceBuffer;
struct InstanceAttribute;

class InstanceBuffer {
	friend class GLMesh;

private:
	uint32 instanceBuffer;
	int32 instanceSizeBytes;
	int32 instanceCount;
	int32 divisor;
	std::vector<InstanceAttribute> attributes;

public:
	InstanceBuffer(int32 instanceSizeBytes, int32 instanceCount, int32 divisor, std::vector<InstanceAttribute> attributes);

	void uploadInstanceData(uint32 offset, uint32 size, void* data);

	void bind(bool bind);

	void enableAttributes(bool enabled);
};

struct InstanceAttribute {
	int32 index;
	int32 size;
	int32 type;
	int32 offset;

	InstanceAttribute(int32 index, int32 size, int32 type, int32 offset):
		index(index), size(size), type(type), offset(offset) {}
};

class GLMesh {
private:
	uint32 vertexArray;
	uint32 vertexBuffer;
	uint32 indexBuffer;

	uint32 primitive;

	VertexLayout attributes;

	int32 vertexCount;
	int32 indexCount;
	int32 allocatedVertexBufferSize;
	int32 allocatedIndexBufferSize;
public:
	GLMesh(MeshData* meshData = NULL, VertexLayout attributes = DEFAULT_VERTEX_LAYOUT);

	~GLMesh();

	void uploadMeshData(MeshData* meshData);

	void reserveBuffers(int32 vertexBufferSize, int32 indexBufferSize);

	void draw(int32 instances = 1, int32 offset = 0, int32 count = 0, InstanceBuffer* instanceBuffer = NULL);

	void setPrimitive(uint32 primitive);

	uint32 getVertexArray();

	uint32 getVertexBuffer();

	uint32 getIndexBuffer();

	uint32 getPrimitive() const;

	static void bindVertexAttribLayout(VertexLayout layout);

	static void enableVertexAttribLayout(VertexLayout layout, bool enabled);
};