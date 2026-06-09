#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<vector>

#include<glad/glad.h>
#include<glfw3.h>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/vec2.hpp>
#include<glm/vec3.hpp>
#include<glm/vec4.hpp>
#include<glm/mat4x4.hpp>

#include"shader.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

#include"model_placeholder.h"
