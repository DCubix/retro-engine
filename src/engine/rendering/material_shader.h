#pragma once

#include "shader.h"

class MaterialShader : public Shader {
public:
	MaterialShader(const str& src);
	virtual void OnBeforeRender() {};
};