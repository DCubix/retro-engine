#include "shader.h"

#include <stdexcept>
#include <vector>
#include <regex>
#include <fmt/format.h>

#include "../utils/string_utils.h"
#include "../utils/logger.h"

#include "shaders/generated/utils_glsl.h"

Shader::Shader(const ShaderDescription& descriptor)
{
	Load(descriptor);
}

void Shader::Load(ShaderDescription descriptor)
{
	Destroy();

	std::map<str, str> includes;
	for (const auto& [name, src] : descriptor.includes) {
		includes[name] = src;
	}

	// Default includes
	includes["utils"] = utils_glsl_src;

	mID = glCreateProgram();
	for (const auto& [type, source] : descriptor.shaderSources) {
		GLuint shader = glCreateShader(static_cast<GLenum>(type));
		str sourceCode = ProcessShaderIncludes(
			source,
			descriptor.includes,
			descriptor.defines[type]
		);

		const char* src = sourceCode.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);
		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
#ifdef IS_DEBUG
			std::vector<str> sourceLines = utils::SplitLines(source);

			GLint logLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
			std::string log(logLength, '\0');
			glGetShaderInfoLog(shader, logLength, nullptr, log.data());

			std::string logText = "";
			std::string shaderType = "??";

			switch (type) {
				case ShaderType::vertexShader: shaderType = "Vertex Shader"; break;
				case ShaderType::fragmentShader: shaderType = "Fragment Shader"; break;
				case ShaderType::geometryShader: shaderType = "Geometry Shader"; break;
			}

			std::smatch match;
			std::regex lineMsgRegex(R"(ERROR: (\d+):(\d+):(.*))");

			// get line number, string index and error message
			if (std::regex_search(log, match, lineMsgRegex)) {
				if (match.size() > 1) {
					int stringIndex = std::stoi(match[1].str());
					int lineNumber = std::stoi(match[2].str());
					std::string errorMessage = match[3].str();

					logText = fmt::format("{} error at line {}:{}: {}\nOffending code:\n", shaderType, lineNumber, stringIndex, errorMessage);
					if (lineNumber < sourceLines.size()) {
						std::string line = sourceLines[lineNumber - 1];

						// add a ^ at string index in a new line
						std::string arrow(line.size(), ' ');
						arrow[stringIndex] = '^';

						if (lineNumber - 1 > 0) {
							logText += fmt::format("{}\n", sourceLines[lineNumber - 2]);
						}
						logText += fmt::format("{}\n{}", line, arrow);
						if (lineNumber < sourceLines.size()) {
							logText += fmt::format("\n{}", sourceLines[lineNumber]);
						}
					}
				}
			}

			glDebugMessageInsert(
				GL_DEBUG_SOURCE_APPLICATION,
				GL_DEBUG_TYPE_ERROR,
				0,
				GL_DEBUG_SEVERITY_HIGH,
				-1,
				logText.c_str()
			);
#endif
			glDeleteShader(shader);
			Destroy();
			return;
		}
		glAttachShader(mID, shader);
		glDeleteShader(shader);
	}
	glLinkProgram(mID);
}

bool Shader::HasUniform(const str& name)
{
	return GetUniformLocation(name, true) != -1;
}

void Shader::Bind()
{
	glUseProgram(mID);
}

void Shader::Unbind()
{
	glUseProgram(0);
}

void Shader::Destroy()
{
	if (mID != 0) {
		glDeleteProgram(mID);
		mID = 0;
	}
	mUniformLocations.clear();
	mAttributeLocations.clear();
}

str Shader::ProcessShaderIncludes(
	const str& src,
	const std::map<str, str>& includes,
	const std::vector<str>& defines
)
{
	if (includes.empty()) {
		return src;
	}

	std::vector<str> lines = utils::SplitLines(src);
	std::string result;

	for (const auto& line : lines) {
		std::smatch match;
		std::regex includeRegex(R"#(^\s*#include\s*["<](.*)[">]\s*$)#");
		if (std::regex_search(line, match, includeRegex)) {
			if (match.size() > 1) {
				str includeName = match[1].str();
				auto it = includes.find(includeName);
				if (it != includes.end()) {
					result += ProcessShaderIncludes(it->second, includes, {});
				}
				else {
					utils::Fail("Include not found: {}", includeName);
				}
			}
		}
		else if (line.find("#version") != str::npos) {
			result += line + "\n";
			for (const auto& define : defines) {
				result += fmt::format("#define {}\n", define);
			}
		}
		else {
			result += line + "\n";
		}
	}

	if (result.back() == '\n' || result.back() == '\r') {
		result.pop_back();
	}

	return result;
}

GLint Shader::GetUniformLocation(const str& name, bool silent)
{
	auto it = mUniformLocations.find(name);
	if (it != mUniformLocations.end()) {
		return it->second;
	}
	GLint location = glGetUniformLocation(mID, name.c_str());
	if (location == -1 && !silent) {
		utils::LogE("Uniform not found: {}", name);
	}
	mUniformLocations[name] = location;
	return location;
}

GLint Shader::GetAttributeLocation(const str& name)
{
	auto it = mAttributeLocations.find(name);
	if (it != mAttributeLocations.end()) {
		return it->second;
	}
	GLint location = glGetAttribLocation(mID, name.c_str());
	if (location == -1) {
		utils::LogE("Attribute not found: {}", name);
	}
	mAttributeLocations[name] = location;
	return location;
}

GLint Shader::GetUniformBlockIndex(const str& name)
{
	auto it = mUniformBlockIndices.find(name);
	if (it != mUniformBlockIndices.end()) {
		return it->second;
	}
	GLint location = glGetUniformBlockIndex(mID, name.c_str());
	if (location == -1) {
		utils::LogE("Uniform block not found: {}", name);
	}
	mUniformBlockIndices[name] = location;
	return location;
}

void Shader::SetUniform1u(const str& name, u32 value)
{
	GLint location = GetUniformLocation(name);
	glUniform1ui(location, value);
}

void Shader::SetUniform1i(const str& name, i32 value)
{
	GLint location = GetUniformLocation(name);
	glUniform1i(location, value);
}

void Shader::SetUniform1f(const str& name, f32 value)
{
	GLint location = GetUniformLocation(name);
	glUniform1f(location, value);
}

void Shader::SetUniform2f(const str& name, const Vector2& v)
{
	GLint location = GetUniformLocation(name);
	glUniform2f(location, v.x, v.y);
}

void Shader::SetUniform3f(const str& name, const Vector3& v) {
	GLint location = GetUniformLocation(name);
	glUniform3f(location, v.x, v.y, v.z);
}

void Shader::SetUniform4f(const str& name, const Vector4& v) {
	GLint location = GetUniformLocation(name);
	glUniform4f(location, v.x, v.y, v.z, v.w);
}

void Shader::SetUniformMatrix4f(const str& name, const Matrix& m) {
	GLint location = GetUniformLocation(name);
	glUniformMatrix4fv(location, 1, GL_TRUE, (const GLfloat*)&m);
}

void Shader::SetUniformTextureHandle(const str& name, GLuint64 handle)
{
	GLint location = GetUniformLocation(name);
	glUniformHandleui64ARB(location, handle);
}

void Shader::SetUniform(const str& name, ShaderUniformType value)
{
	std::visit(
		[this, &name](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, bool>) {
				SetUniform1i(name, arg ? 1 : 0);
			}
			else if constexpr (std::is_same_v<T, Vector2>) {
				SetUniform2f(name, arg);
			}
			else if constexpr (std::is_same_v<T, Vector3>) {
				SetUniform3f(name, arg);
			}
			else if constexpr (std::is_same_v<T, Vector4>) {
				SetUniform4f(name, arg);
			}
			else if constexpr (std::is_same_v<T, Matrix>) {
				SetUniformMatrix4f(name, arg);
			}
			else if constexpr (std::is_same_v<T, i8> || std::is_same_v<T, i16> || std::is_same_v<T, i32>) {
				SetUniform1i(name, static_cast<i32>(arg));
			}
			else if constexpr (std::is_same_v<T, u8> || std::is_same_v<T, u16> || std::is_same_v<T, u32>) {
				SetUniform1u(name, static_cast<u32>(arg));
			}
			else if constexpr (std::is_same_v<T, GLuint64>) {
				SetUniformTextureHandle(name, arg);
			}
			else if constexpr (std::is_same_v<T, f32>) {
				SetUniform1f(name, arg);
			}
		},
		value
	);	
}

void Shader::SetUniformBlockBinding(const str& name, u32 bindingPoint)
{
	GLint location = GetUniformBlockIndex(name);
	if (location == -1) {
		utils::Fail("Uniform block not found: {}", name);
	}
	glUniformBlockBinding(mID, location, bindingPoint);
}
