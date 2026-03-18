#include "material_shader.h"

#include "../utils/string_utils.h"

#include "shaders/generated/gbuffer_vert.h"
#include "shaders/generated/gbuffer_frag.h"

MaterialShader::MaterialShader(const str& src)
{
	std::vector<str> lines = utils::SplitString(src, "\n");

	std::vector<str> vertexLines;
	std::vector<str> fragmentLines;

#define vec std::vector<str>
#define popLine lines.front(); lines.erase(lines.begin())

	while (!lines.empty()) {
		str current = popLine;

		// find [[vertex_shader]]
		if (current.find("[[vertex_shader]]") != str::npos) {
			while (!lines.empty()) {
				current = popLine;
				if (current.find("[[") != str::npos) {
					lines.insert(lines.begin(), current);
					break;
				}
				vertexLines.push_back(current);
			}
		}
		// find [[fragment_shader]]
		else if (current.find("[[fragment_shader]]") != str::npos) {
			while (!lines.empty()) {
				current = popLine;
				if (current.find("[[") != str::npos) {
					lines.insert(lines.begin(), current);
					break;
				}
				fragmentLines.push_back(current);
			}
		}
	}

	str vertexShaderSrc = utils::JoinStrings(vertexLines, "\n");
	str fragmentShaderSrc = utils::JoinStrings(fragmentLines, "\n");

	ShaderDescription desc{
		.shaderSources = {
			{ShaderType::vertexShader, gbuffer_vert_src},
			{ShaderType::fragmentShader, gbuffer_frag_src},
		},
		.includes = {
			{"custom_vertex_shader", vertexShaderSrc},
			{"custom_fragment_shader", fragmentShaderSrc},
		},
		.defines = {
			{ ShaderType::vertexShader, vertexShaderSrc.empty() ? vec{} : vec{"CUSTOM_SHADER_VS"} },
			{ ShaderType::fragmentShader, fragmentShaderSrc.empty() ? vec{} : vec{"CUSTOM_SHADER_FS"} },
		}
	};
	Load(desc);
}
