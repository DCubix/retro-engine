#pragma once

#include "shader.h"

class RENGINE_API MaterialShader : public Shader {
public:
	MaterialShader(const str& src);
	virtual void OnBeforeRender() {};
};