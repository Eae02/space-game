#include "resources.hpp"
#include "utils.hpp"

Model res::shipModel;
Texture res::shipAlbedo;
Texture res::shipNormals;
Texture res::asteroidAlbedo;
Texture res::asteroidNormals;
GLuint res::skybox;

void res::load() {
	shipModel.loadObj(exeDirPath + "res/ship.obj");
	shipAlbedo.load(exeDirPath + "res/textures/shipDiffuse.png", true, true);
	shipNormals.load(exeDirPath + "res/textures/shipNormals.png", false, true);
	asteroidAlbedo.load(exeDirPath + "res/textures/asteroidDiffuse.jpg", true, true);
	asteroidNormals.load(exeDirPath + "res/textures/asteroidNormals.jpg", false, true);
	skybox = loadTextureCube(exeDirPath + "res/textures/skybox/", 1024);
}
