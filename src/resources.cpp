#include "resources.hpp"
#include "utils.hpp"

Model res::shipModel;
Texture res::shipAlbedo;
Texture res::shipNormals;
Texture res::asteroidAlbedo;
Texture res::asteroidNormals;

void res::load() {
	shipModel.loadObj(exeDirPath + "res/ship.obj");
	shipAlbedo.load(exeDirPath + "res/textures/shipDiffuse.png", true, true);
	shipNormals.load(exeDirPath + "res/textures/shipNormals.png", false, true);
	asteroidAlbedo.load(exeDirPath + "res/textures/asteroidDiffuse.png", true, true);
	asteroidNormals.load(exeDirPath + "res/textures/asteroidNormals.png", false, true);
}

void res::destroy() {
	shipModel.destroy();
	shipAlbedo.destroy();
	shipNormals.destroy();
	asteroidAlbedo.destroy();
	asteroidNormals.destroy();
}
