#pragma once
#include<iostream>
#include<fstream>
#include<string>

#include<glad/glad.h>
#include<glfw3.h>

#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/vec2.hpp>
#include<glm/vec3.hpp>
#include<glm/vec4.hpp>
#include<glm/mat4x4.hpp>

class Shader
{
private:

	// member variable
	GLuint id;

	// private functions
	std::string loadShaderSource(char* fileName)
	{
		std::string temp = "";
		std::string src = "";

		std::ifstream in_file;

		// Vertex
		in_file.open(fileName);

		if (in_file.is_open())
		{
			while (std::getline(in_file, temp))
				src += temp + "\n";
		}
		else
		{
			std::cout << "ERROR::SHADER::COULD_NOT_OPEN_FILE:" << fileName << "\n";
		}
		in_file.close();

		return src;
	}

	GLuint loadShader(GLenum type, char* fileName)
	{
		char infoLog[512];
		GLint success;

		GLuint shader = glCreateShader(type);
		std::string str_src = this->loadShaderSource(fileName);
		const GLchar* src = str_src.c_str();
		glShaderSource(shader, 1, &src, NULL);
		glCompileShader(shader);

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::COULD_NOT_COMPILE_SHADER" << fileName << "\n";
			std::cout << infoLog << "\n";
		}

		return shader;
	}

	void linkProgram(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader)
	{
		char infoLog[512];
		GLint success;

		this->id = glCreateProgram();

		glAttachShader(this->id, vertexShader);

		if (geometryShader)
		{
			glAttachShader(this->id, geometryShader);
		}

		glAttachShader(this->id, fragmentShader);

		glLinkProgram(this->id);

		glGetProgramiv(this->id, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(this->id, 512, NULL, infoLog);
			std::cout << "ERROR::SHADERS::COULD_NOT_LINK_PROGRAM\n";
			std::cout << infoLog << "\n";
		}

		// End
		glUseProgram(0);
	}

public:
	// Constructor for vertex and fragment shaders only
	Shader(const char* vertexFile, const char* fragmentFile)
	{
		GLuint vertexShader = 0;
		GLuint geometryShader = 0;
		GLuint fragmentShader = 0;

		vertexShader = loadShader(GL_VERTEX_SHADER, const_cast<char*>(vertexFile));
		fragmentShader = loadShader(GL_FRAGMENT_SHADER, const_cast<char*>(fragmentFile));

		this->linkProgram(vertexShader, 0, fragmentShader);

		// End
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}

	// Existing constructor for vertex, fragment, and geometry shaders
	Shader(char* vertexFile, char* fragmentFile, char* geometryFile)
	{
		GLuint vertexShader = 0;
		GLuint geometryShader = 0;
		GLuint fragmentShader = 0;

		vertexShader = loadShader(GL_VERTEX_SHADER, vertexFile);

		if (geometryFile != nullptr && std::string(geometryFile) != "")
			geometryShader = loadShader(GL_GEOMETRY_SHADER, geometryFile);

		fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentFile);

		this->linkProgram(vertexShader, geometryShader, fragmentShader);

		// End
		glDeleteShader(vertexShader);
		glDeleteShader(geometryShader);
		glDeleteShader(fragmentShader);
	}

	// destructor
	~Shader()
	{
		glDeleteProgram(this->id);
	}

	// set uniform functions
	GLuint getID()
	{
		return this->id;
	}

	void use()
	{
		glUseProgram(this->id);
	}	

	void unUse()
	{
		glUseProgram(0);
	}

	void set1i(GLint value, const GLchar* name)
	{
		glUniform1i(glGetUniformLocation(this->id, name), value);
	}

	void set1f(GLfloat value, const GLchar* name)
	{
		glUniform1f(glGetUniformLocation(this->id, name), value);
	}

	void setVec2f(glm::fvec2 value, const GLchar* name)
	{
		glUniform2fv(glGetUniformLocation(this->id, name), 1, glm::value_ptr(value));
	}

	void setVec3f(glm::fvec3 value, const GLchar* name)
	{
		glUniform3fv(glGetUniformLocation(this->id, name), 1, glm::value_ptr(value));
	}

	void setVec4f(glm::fvec4 value, const GLchar* name)
	{
		glUniform4fv(glGetUniformLocation(this->id, name), 1, glm::value_ptr(value));
	}

	void setMat3fv(glm::mat3 value, const GLchar* name, GLboolean transpose = GL_FALSE)
	{
		glUniformMatrix3fv(glGetUniformLocation(this->id, name), 1, GL_FALSE, glm::value_ptr(value));
	}

	void setMat4fv(glm::mat4 value, const GLchar* name, GLboolean transpose = GL_FALSE)
	{
		glUniformMatrix4fv(glGetUniformLocation(this->id, name), 1, transpose, glm::value_ptr(value));
	}
};