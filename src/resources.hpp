#pragma once

#include "graphics/texture.hpp"
#include "graphics/model.hpp"

namespace res {
	extern Model shipModel;
	extern Texture shipAlbedo;
	extern Texture shipNormals;
	
	void load();
	void destroy();
}
