#pragma once

struct Shader {
	GLuint program = 0;
	
	void attachStage(GLenum stage, std::string_view fileName, std::string_view extraCode = {});
	void link(const char* label);
	
	void use() const {
		glUseProgram(program);
	}
};
