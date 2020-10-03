#pragma once

#include "graphics/texture.hpp"
#include "graphics/model.hpp"

namespace res {
	extern Model shipModel;
	extern Texture shipAlbedo;
	extern Texture shipNormals;
	extern Texture asteroidAlbedo;
	extern Texture asteroidNormals;
	
	void load();
	void destroy();
}
