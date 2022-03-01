
#include "shader.hpp"

Shader::Shader(const char *vertexPath, const char *fragmentPath)
{
	std::string vpath = vertexPath, fpath = fragmentPath;
	std::ifstream v_ifs(vpath);
	std::ifstream f_ifs(fpath);
	std::string vertexStr((std::istreambuf_iterator<char>(v_ifs)),
	                      (std::istreambuf_iterator<char>()   ));
	std::string fragmentStr((std::istreambuf_iterator<char>(f_ifs)),
	                        (std::istreambuf_iterator<char>()   ));
	const char *vertexCode = vertexStr.c_str();
	const char *fragmentCode = fragmentStr.c_str();

	GLuint vertex, fragment;
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexCode, nullptr);
	glCompileShader(vertex);
	checkCompileErrors(vertex);

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentCode, nullptr);
	glCompileShader(fragment);
	checkCompileErrors(fragment);

	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::setVec3(const glm::vec3 &value, const GLchar *name) const
{
	glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const glm::mat4 &value, const GLchar *name) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setInt(int value, const GLchar *name) const
{
	glUniform1i(glGetUniformLocation(ID, name), value);
}

void Shader::setBool(bool value, const GLchar *name) const
{
	glUniform1i(glGetUniformLocation(ID, name), (int) value);
}

void Shader::setFloat(float value, const GLchar *name) const
{
	glUniform1f(glGetUniformLocation(ID, name), value);
}

void Shader::use()
{
	glUseProgram(ID);
}

void Shader::checkCompileErrors(GLuint shader)
{
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "compilation failed\n" << infoLog << std::endl;
	}
}