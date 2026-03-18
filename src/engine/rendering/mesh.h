#pragma once

#include "buffer.h"
#include "../external/raymath/raymath.h"
#include <vector>

struct RENGINE_API Vertex {
	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 texCoord;
	Vector4 color;

	u32 boneIDs[4];
	f32 boneWeights[4];
};

struct RENGINE_API InstanceData {
	u32 objectId;
	Matrix transform;
};

struct RENGINE_API MeshDescription {
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
	std::vector<InstanceData> instanceData;
	bool isInstanced{ false };
	bool isStatic{ true };
};

enum class PrimitiveType {
	points = GL_POINTS,
	lines = GL_LINES,
	lineStrip = GL_LINE_STRIP,
	lineLoop = GL_LINE_LOOP,
	triangles = GL_TRIANGLES,
	triangleStrip = GL_TRIANGLE_STRIP,
	triangleFan = GL_TRIANGLE_FAN
};

class RENGINE_API Mesh {
public:
	Mesh() = default;
	Mesh(const MeshDescription& desc);
	
	void Update(
		const std::vector<Vertex>& vertices,
		const std::vector<u32>& indices
	);

	void UpdateInstances(
		const std::vector<InstanceData>& instanceData
	);

	void Draw(size_t offset = 0);
	void DrawInstanced(u32 instanceCount, size_t offset = 0);

	bool IsInstanced() const { return mInstanceBuffer.IsValid(); }

	PrimitiveType GetPrimitiveType() const { return mPrimitiveType; }
	void SetPrimitiveType(PrimitiveType primitiveType) { mPrimitiveType = primitiveType; }

	Vector3 GetAABBMin() const { return mAABBMin; }
	Vector3 GetAABBMax() const { return mAABBMax; }

	u32 GetInstanceCount() const { return mInstanceCount; }

	static Mesh* LoadOBJ(const str& fileName);

private:
	Vector3 mAABBMin{ 0.0f, 0.0f, 0.0f }, mAABBMax{ 0.0f, 0.0f, 0.0f };
	VertexArray mVAO;
	VertexBuffer<Vertex> mVBO;
	Buffer<u32, BufferType::indexBuffer> mEBO;
	Buffer<InstanceData, BufferType::arrayBuffer> mInstanceBuffer;
	u32 mInstanceCount{ 0 };
	PrimitiveType mPrimitiveType{ PrimitiveType::triangles };
	bool mIsStatic{ true };

	void ComputeAABB(const std::vector<Vertex>& vertices);
};