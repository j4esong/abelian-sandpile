
#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <string>

class Shader
{
public:
	GLuint ID;
	Shader(const GLchar *vertexPath, const GLchar *fragmentPath);
	void setVec3(const glm::vec3 &value, const GLchar *name) const;
	void setMat4(const glm::mat4 &value, const GLchar *name) const;
	void setInt(const int value, const GLchar *name) const;
	void setBool(const bool value, const GLchar *name) const;
	void setFloat(const float value, const GLchar *name) const;
	void use();
private:
	void checkCompileErrors(GLuint shader);
};

#endif