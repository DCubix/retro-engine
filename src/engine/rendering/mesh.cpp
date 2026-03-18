#include "mesh.h"

#include <sstream>

#include "../utils/physfs_wrapper.h"
#include "../utils/string_utils.h"
#include "../utils/logger.h"

Mesh::Mesh(const MeshDescription& desc)
{
	VertexFormat format{
		VertexAttribute{ DataType::float3Value, false }, // Position
		VertexAttribute{ DataType::float3Value, false }, // Normal
		VertexAttribute{ DataType::float3Value, false }, // Tangent
		VertexAttribute{ DataType::float2Value, false }, // TexCoord
		VertexAttribute{ DataType::float4Value, false }, // Color
		VertexAttribute{ DataType::int4Value, false },   // Bone IDs
		VertexAttribute{ DataType::float4Value, false }, // Bone Weights
	};

	mIsStatic = desc.isStatic;

	const auto usage = mIsStatic ? BufferUsage::staticDraw : BufferUsage::dynamicDraw;
	VertexBufferDescription<Vertex> vboDesc{};
	vboDesc.initialData = desc.vertices.data();
	vboDesc.initialDataLength = desc.vertices.size();
	vboDesc.format = format;
	vboDesc.usage = usage;

	mVAO = VertexArray();
	mVAO.Bind();

	mVBO = VertexBuffer<Vertex>(vboDesc);
	mVBO.Bind();

	if (desc.isInstanced) {
		UpdateInstances(desc.instanceData);

		const auto baseLocation = format.GetAttributeCount();

		// object ID
		glEnableVertexAttribArray(baseLocation + 0);
		glVertexAttribPointer(baseLocation + 0, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(InstanceData), nullptr);
		glVertexAttribDivisor(baseLocation + 0, 1);

		// transform matrix
		for (size_t i = 0; i < 4; ++i) {
			glEnableVertexAttribArray(baseLocation + 1 + i);
			glVertexAttribPointer(
				baseLocation + 1 + i,
				4,
				GL_FLOAT,
				GL_FALSE,
				sizeof(InstanceData),
				reinterpret_cast<void*>(sizeof(float) * i * 4)
			);
			glVertexAttribDivisor(baseLocation + 1 + i, 1);
		}
	}

	mEBO = Buffer<u32, BufferType::indexBuffer>(usage);
	mEBO.Bind();
	mEBO.Update(desc.indices.data(), desc.indices.size());

	mVAO.Unbind();

	ComputeAABB(desc.vertices);
}

void Mesh::Update(const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
{
	if (mIsStatic) return;

	mVBO.Bind();
	mVBO.Update(vertices.data(), vertices.size());

	mEBO.Bind();
	mEBO.Update(indices.data(), indices.size());

	ComputeAABB(vertices);
}

void Mesh::UpdateInstances(const std::vector<InstanceData>& instanceData)
{
	if (!mInstanceBuffer.IsValid()) {
		mInstanceBuffer = Buffer<InstanceData, BufferType::arrayBuffer>(BufferUsage::dynamicDraw);
	}

	mInstanceBuffer.Bind();
	mInstanceBuffer.Update(instanceData.data(), instanceData.size());

	mInstanceCount = static_cast<u32>(instanceData.size());
}

void Mesh::Draw(size_t offset)
{
	mVAO.Bind();
	glDrawElements(
		static_cast<GLenum>(mPrimitiveType),
		mEBO.GetElementCount(),
		GL_UNSIGNED_INT,
		reinterpret_cast<void*>(offset * sizeof(u32))
	);
	mVAO.Unbind();
}

void Mesh::DrawInstanced(u32 instanceCount, size_t offset)
{
	mVAO.Bind();
	glDrawElementsInstanced(
		static_cast<GLenum>(mPrimitiveType),
		mEBO.GetElementCount(),
		GL_UNSIGNED_INT,
		reinterpret_cast<void*>(offset * sizeof(u32)),
		instanceCount
	);
	mVAO.Unbind();
}


static void sCalculateTangents(std::vector<Vertex>& vertices, const std::vector<u32>& indices) {
	for (size_t i = 0; i < indices.size(); i += 3) {
		auto& v0 = vertices[indices[i]];
		auto& v1 = vertices[indices[i + 1]];
		auto& v2 = vertices[indices[i + 2]];
		
		Vector3 edge1 = v1.position - v0.position;
		Vector3 edge2 = v2.position - v0.position;
		Vector2 deltaUV1 = v1.texCoord - v0.texCoord;
		Vector2 deltaUV2 = v2.texCoord - v0.texCoord;

		f32 dividend = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
		f32 f = dividend != 0.0f ? 1.0f / dividend : 0.0f;

		Vector3 tangent{};
		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;
	}
	for (auto& v : vertices) {
		v.tangent = Vector3Normalize(v.tangent);
	}
}

static void sFlipUVsY(std::vector<Vertex>& vertices) {
	for (auto& v : vertices) {
		v.texCoord.y = 1.0f - v.texCoord.y;
	}
}

Mesh* Mesh::LoadOBJ(const str& fileName)
{
	File file(fileName);
	if (!file.IsOpen()) {
		utils::Fail("Failed to open OBJ file: {}", fileName);
	}

	std::vector<Vertex> vertices;
	std::vector<u32> indices;

	std::vector<Vector3> positions;
	std::vector<Vector4> colors;
	std::vector<Vector3> normals;
	std::vector<Vector2> texCoords;
	std::vector<std::tuple<int, int, int>> vertexIndices;

	std::vector<str> lines = file.ReadLines();
	for (str line : lines) {
		str type = line.substr(0, line.find(' '));

		if (type == "v") {
			Vector3 pos;
			Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f }; // Default color is white
			std::stringstream ss(line.substr(2));
			ss >> pos.x >> pos.y >> pos.z;

			if (!ss.eof()) {
				ss >> color.x >> color.y >> color.z;
			}
			positions.push_back(pos);
			colors.push_back(color);
		}
		else if (type == "vt") {
			Vector2 texCoord;
			std::stringstream ss(line.substr(3));
			ss >> texCoord.x >> texCoord.y;
			texCoords.push_back(texCoord);
		}
		else if (type == "vn") {
			Vector3 normal;
			std::stringstream ss(line.substr(3));
			ss >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (type == "f") {
			std::stringstream ss(line.substr(2));
			str vertex;
			u32 count = 0;

			while (ss >> vertex) {
				auto spl = utils::SplitString(vertex, "/");
				// there are some ways to define a vertex
				// 0. v
				// 1. v/vt/vn
				// 2. v/vt
				// 3. v//vn

				int posIndex = std::stoi(spl[0]) - 1;
				int texIndex = -1;
				int normIndex = -1;

				if (spl.size() == 3) {
					if (!spl[1].empty()) {
						texIndex = std::stoi(spl[1]) - 1;
					}
					normIndex = std::stoi(spl[2]) - 1;
				}
				else if (spl.size() == 2) {
					texIndex = std::stoi(spl[1]) - 1;
				}

				vertexIndices.push_back(std::make_tuple(posIndex, texIndex, normIndex));
				count++;
			}

			if (count != 3) {
				utils::Fail("Only triangles are supported in OBJ files");
			}
		}
	}

	for (const auto& [posIndex, texIndex, normIndex] : vertexIndices) {
		Vertex vertex{};
		vertex.position = positions[posIndex];
		vertex.texCoord = texCoords[texIndex];
		vertex.normal = normals[normIndex];
		vertex.tangent = Vector3Zeros;
		vertex.color = colors[posIndex];
		vertices.push_back(vertex);
		indices.push_back(static_cast<u32>(indices.size()));
	}

	// post-processing
	sCalculateTangents(vertices, indices);
	sFlipUVsY(vertices);

	MeshDescription meshDesc{
		.vertices = vertices,
		.indices = indices,
		.isInstanced = false,
		.isStatic = true,
	};
	return new Mesh(meshDesc);
}

void Mesh::ComputeAABB(const std::vector<Vertex>& vertices)
{
	// compute AABB
	mAABBMin = Vector3{ FLT_MAX, FLT_MAX, FLT_MAX };
	mAABBMax = Vector3{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& vertex : vertices) {
		mAABBMin = Vector3Min(mAABBMin, vertex.position);
		mAABBMax = Vector3Max(mAABBMax, vertex.position);
	}
}
