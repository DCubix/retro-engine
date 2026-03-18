#pragma once

#include "opengl.hpp"
#include "../external/glad/glad.h"

#include <vector>
#include <initializer_list>

enum class BufferType {
	arrayBuffer = GL_ARRAY_BUFFER,
	indexBuffer = GL_ELEMENT_ARRAY_BUFFER,
	uniformBuffer = GL_UNIFORM_BUFFER,
	shaderStorageBuffer = GL_SHADER_STORAGE_BUFFER,
};

enum class BufferUsage {
	staticDraw = GL_STATIC_DRAW,
	dynamicDraw = GL_DYNAMIC_DRAW,
	streamDraw = GL_STREAM_DRAW,
};

enum class BufferAccess {
	readOnly = GL_READ_ONLY,
	writeOnly = GL_WRITE_ONLY,
	readWrite = GL_READ_WRITE
};

template <typename T, BufferType Type>
class RENGINE_API Buffer : public IGPUResource {
public:
	Buffer() = default;

	Buffer(BufferUsage usage) : mUsage(usage)
	{
		glCreateBuffers(1, &mID);
	}

	void Update(const T* data, size_t length, size_t offset = 0)
	{
		const size_t size = length * sizeof(T);
		if (mSize != size) {
			mSize = size;
			glNamedBufferData(mID, mSize, data, static_cast<GLenum>(mUsage));
		}
		else if (mUsage == BufferUsage::dynamicDraw) {
			glNamedBufferSubData(mID, offset * sizeof(T), mSize, data);
		}

		mElementCount = length;
	}

	T* Map(BufferAccess access = BufferAccess::readWrite)
	{
		return static_cast<T*>(glMapNamedBuffer(mID, static_cast<GLenum>(access)));
	}

	void Unmap()
	{
		glUnmapNamedBuffer(mID);
	}

	virtual void Bind() override
	{
		glBindBuffer(static_cast<GLenum>(Type), mID);
	}

	void BindBase(u32 bindingPoint) const
	{
		glBindBufferBase(static_cast<GLenum>(Type), bindingPoint, mID);
	}

	void Unbind() override
	{
		glBindBuffer(static_cast<GLenum>(Type), 0);
	}

	GLuint GetID() const override { return mID; }
	size_t GetElementCount() const {
		return mElementCount;
	}

	bool IsValid() const {
		return mID != 0;
	}

	void Destroy() override {
		if (mID) {
			glDeleteBuffers(1, &mID);
			mID = 0;
		}
	}

protected:
	GLuint mID{ 0 };
	size_t mElementCount{ 0 };
	size_t mSize{ 0 };
	BufferUsage mUsage{ BufferUsage::staticDraw };
};

enum class DataType {
	floatValue = 0,
	float2Value,
	float3Value,
	float4Value,
	intValue,
	int2Value,
	int3Value,
	int4Value,
	mat3Value,
	mat4Value,
	boolValue
};

struct RENGINE_API VertexAttribute {
	DataType type;
	bool normalized{ false };
};

class RENGINE_API VertexFormat {
public:
	VertexFormat() = default;
	VertexFormat(const std::initializer_list<VertexAttribute>& attributes);

	size_t GetStride() const;
	size_t GetAttributeCount() const { return mAttributes.size(); }

	void Apply();

private:
	struct VertexAttributeInfo {
		GLenum type;
		size_t size;
		size_t components;
		size_t offset;
		bool normalized;
	};

	size_t GetDateTypeComponents(DataType type) const {
		switch (type) {
			case DataType::floatValue: return 1;
			case DataType::float2Value: return 2;
			case DataType::float3Value: return 3;
			case DataType::float4Value: return 4;
			case DataType::intValue: return 1;
			case DataType::int2Value: return 2;
			case DataType::int3Value: return 3;
			case DataType::int4Value: return 4;
			case DataType::mat3Value: return 9;
			case DataType::mat4Value: return 16;
			case DataType::boolValue: return 1;
			default: return 0;
		}
	}

	size_t GetDataTypeSize(DataType type) const {
		const size_t components = GetDateTypeComponents(type);
		switch (type) {
			case DataType::floatValue: return sizeof(float) * components;
			case DataType::float2Value: return sizeof(float) * components;
			case DataType::float3Value: return sizeof(float) * components;
			case DataType::float4Value: return sizeof(float) * components;
			case DataType::intValue: return sizeof(int) * components;
			case DataType::int2Value: return sizeof(int) * components;
			case DataType::int3Value: return sizeof(int) * components;
			case DataType::int4Value: return sizeof(int) * components;
			case DataType::mat3Value: return sizeof(float) * components;
			case DataType::mat4Value: return sizeof(float) * components;
			case DataType::boolValue: return 1;
			default: return 0;
		}
	}

	GLenum GetDataType(DataType type) const {
		switch (type) {
			case DataType::floatValue: return GL_FLOAT;
			case DataType::float2Value: return GL_FLOAT;
			case DataType::float3Value: return GL_FLOAT;
			case DataType::float4Value: return GL_FLOAT;
			case DataType::intValue: return GL_INT;
			case DataType::int2Value: return GL_INT;
			case DataType::int3Value: return GL_INT;
			case DataType::int4Value: return GL_INT;
			case DataType::mat3Value: return GL_FLOAT;
			case DataType::mat4Value: return GL_FLOAT;
			case DataType::boolValue: return GL_BOOL;
			default: return 0;
		}
	}

	std::vector<VertexAttributeInfo> mAttributes;
};

template <typename V>
struct RENGINE_API VertexBufferDescription {
	VertexFormat format;
	BufferUsage usage{ BufferUsage::staticDraw };
	const V* initialData{ nullptr };
	size_t initialDataLength{ 0 };
};

template <typename V>
class RENGINE_API VertexBuffer : public Buffer<V, BufferType::arrayBuffer> {
public:
	VertexBuffer() {}

	VertexBuffer(const VertexBufferDescription<V>& description) : Buffer<V, BufferType::arrayBuffer>(description.usage)
	{
		if (description.initialDataLength > 0) {
			this->Update(description.initialData, description.initialDataLength);
		}
		mFormat = description.format;
	}

	void Bind() override {
		Buffer<V, BufferType::arrayBuffer>::Bind();
		mFormat.Apply();
	}

	VertexFormat GetFormat() const { return mFormat; }

private:
	VertexFormat mFormat;
};

class RENGINE_API VertexArray : public IGPUResource {
public:
	VertexArray() = default;

	void Bind() override {
		if (mID == 0) {
			glGenVertexArrays(1, &mID);
		}
		glBindVertexArray(mID);
	}

	void Unbind() override {
		glBindVertexArray(0);
	}

	GLuint GetID() const override { return mID; }

	void Destroy() override {
		if (mID) {
			glDeleteVertexArrays(1, &mID);
			mID = 0;
		}
	}

protected:
	GLuint mID{ 0 };
};