#pragma once

#include "../utils/custom_types.h"
#include "../external/glad/glad.h"

class IGPUResource {
public:
	virtual void Bind() = 0;
	virtual void Unbind() = 0;
	virtual GLuint GetID() const = 0;
	virtual void Destroy() {}
};
