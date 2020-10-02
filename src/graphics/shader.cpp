#include "shader.hpp"

#include <sstream>
#include <fstream>

static void checkShaderStatus(std::string_view fileName, GLuint handle, GLenum statusName,
	PFNGLGETSHADERIVPROC glGetFunc, PFNGLGETSHADERINFOLOGPROC getInfoLogFunc) {

	GLint ok;
	glGetFunc(handle, statusName, &ok);
	if (!ok) {
		GLint infoLogLen = 0;
		glGetFunc(handle, GL_INFO_LOG_LENGTH, &infoLogLen);
		
		char* infoLog = (char*)alloca(infoLogLen + 1);
		getInfoLogFunc(handle, infoLogLen, nullptr, infoLog);
		
		std::cerr << "in " << fileName << "\n" << infoLog << std::endl;
		std::abort();
	}
}

static void loadShaderCode(std::ostringstream& sourceStream, std::string_view fileName) {
	std::string path("res/shaders/");
	path.append(fileName);
	std::ifstream fileStream(path, std::ios::binary | std::ios::in);
	if (!fileStream) {
		std::cerr << "error opening shader file for reading: '" << path << "'" << std::endl;
		std::abort();
	}
	
	std::string line;
	while (std::getline(fileStream, line)) {
		if (line.starts_with("#include ")) {
			loadShaderCode(sourceStream, line.substr(line.find(' ') + 1));
		} else {
			sourceStream << line << '\n';
		}
	}
}

void Shader::attachStage(GLenum stage, std::string_view fileName, std::string_view extraCode) {
	if (program == 0) {
		program = glCreateProgram();
	}
	
	GLuint shader = glCreateShader(stage);
	
	std::ostringstream sourceStream;
	sourceStream << "#version 450 core\n" << extraCode;
	loadShaderCode(sourceStream, fileName);
	
	std::string glslCode = sourceStream.str();
	const char* glslCodePtr = glslCode.c_str();
	glShaderSource(shader, 1, &glslCodePtr, nullptr);
	
	glCompileShader(shader);
	checkShaderStatus(fileName, shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog);
	
	glAttachShader(program, shader);
}

void Shader::link(const char* label) {
	glLinkProgram(program);
	
	checkShaderStatus(label, program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog);
	
	glObjectLabel(GL_PROGRAM, program, 0, label);
}
