#include "resources.hpp"

Model res::shipModel;
Texture res::shipAlbedo;
Texture res::shipNormals;
Texture res::asteroidAlbedo;
Texture res::asteroidNormals;

void res::load() {
	shipModel.loadObj("res/ship.obj");
	shipAlbedo.load("res/textures/shipDiffuse.png", true, true);
	shipNormals.load("res/textures/shipNormals.png", false, true);
	asteroidAlbedo.load("res/textures/asteroidDiffuse.png", true, true);
	asteroidNormals.load("res/textures/asteroidNormals.png", false, true);
}

void res::destroy() {
	shipModel.destroy();
	shipAlbedo.destroy();
	shipNormals.destroy();
	asteroidAlbedo.destroy();
	asteroidNormals.destroy();
}
