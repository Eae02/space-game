#include "opengl.hpp"

namespace glfunc {
#define GL_FUNC(name, type) type name;
#include "glfunctionslist.inl"
#undef GL_FUNC
}
using namespace glfunc;
