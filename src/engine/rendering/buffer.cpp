#include "buffer.h"

VertexFormat::VertexFormat(const std::initializer_list<VertexAttribute>& attributes)
{
	size_t offset = 0;
	for (const auto& attribute : attributes) {
		VertexAttributeInfo info{};
		info.type = GetDataType(attribute.type);
		info.size = GetDataTypeSize(attribute.type);
		info.components = GetDateTypeComponents(attribute.type);
		info.offset = offset;
		info.normalized = attribute.normalized;
		mAttributes.push_back(info);
		offset += info.size;
	}
}

size_t VertexFormat::GetStride() const
{
	size_t stride = 0;
	for (const auto& attribute : mAttributes) {
		stride += attribute.size;
	}
	return stride;
}

void VertexFormat::Apply()
{
	const size_t stride = GetStride();
	for (size_t i = 0; i < mAttributes.size(); ++i) {
		const auto& attribute = mAttributes[i];
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(
			i, attribute.size / sizeof(float),
			attribute.type, attribute.normalized,
			stride,
			(void*)attribute.offset
		);
	}
}