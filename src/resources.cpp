#include "resources.hpp"

Model res::shipModel;
Texture res::shipAlbedo;
Texture res::shipNormals;

void res::load() {
	shipModel.loadObj("res/ship.obj");
	shipAlbedo.load("res/textures/shipDiffuse.png", true, true);
	shipNormals.load("res/textures/shipNormals.png", false, true);
}

void res::destroy() {
	shipModel.destroy();
	shipAlbedo.destroy();
	shipNormals.destroy();
}
