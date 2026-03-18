#pragma once

#include "opengl.hpp"
#include "../external/glad/glad.h"
#include "../external/raymath/raymath.h"
#include <map>
#include <vector>
#include <variant>

using ShaderUniformType = std::variant<
	bool, Vector2, Vector3, Vector4, Matrix,
	i8, i16, i32, u8, u16, u32, f32, GLuint64,
	std::monostate
>;

enum class ShaderType {
	vertexShader = GL_VERTEX_SHADER,
	fragmentShader = GL_FRAGMENT_SHADER,
	geometryShader = GL_GEOMETRY_SHADER
};

struct RENGINE_API ShaderDescription {
	std::map<ShaderType, str> shaderSources;
	std::map<str, str> includes;
	std::map<ShaderType, std::vector<str>> defines;
};

class RENGINE_API Shader : public IGPUResource {
public:
	Shader() = default;
	Shader(const ShaderDescription& descriptor);

	void Load(ShaderDescription descriptor);

	void SetUniform1u(const str& name, u32 value);
	void SetUniform1i(const str& name, i32 value);
	void SetUniform1f(const str& name, f32 value);
	void SetUniform2f(const str& name, const Vector2& v);
	void SetUniform3f(const str& name, const Vector3& v);
	void SetUniform4f(const str& name, const Vector4& v);
	void SetUniformMatrix4f(const str& name, const Matrix& m);
	void SetUniformTextureHandle(const str& name, GLuint64 handle);

	void SetUniform(const str& name, ShaderUniformType value);

	void SetUniformBlockBinding(const str& name, u32 bindingPoint);

	GLint GetUniformLocation(const str& name, bool silent = false);
	GLint GetAttributeLocation(const str& name);
	GLint GetUniformBlockIndex(const str& name);

	GLuint GetID() const override { return mID; }

	bool HasUniform(const str& name);

	void Bind() override;
	void Unbind() override;

	void Destroy() override;
private:
	GLuint mID{ 0 };

	std::map<str, GLint> mUniformLocations;
	std::map<str, GLint> mAttributeLocations;
	std::map<str, GLint> mUniformBlockIndices;

	str ProcessShaderIncludes(
		const str& src,
		const std::map<str, str>& includes,
		const std::vector<str>& defines
	);
};