#pragma once

#include <GL/gl.h>
#include <GL/glext.h>

namespace glfunc {
#define GL_FUNC(name, type) extern type name;
#include "glfunctionslist.inl"
#undef GL_FUNC
}
using namespace glfunc;
