#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include <future>
#include <atomic>
#include <mutex>
#include <random>

#include "../Linking/include/glad/glad.h"
#include<glfw3.h>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/vec2.hpp>
#include<glm/vec3.hpp>
#include<glm/vec4.hpp>
#include<glm/mat4x4.hpp>

#include"shader.h"

#include <ctime>
#include <cstdlib>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../objects/vertex.h"

#include"model_placeholder.h"
#include "../objects/orbitCamera.h"
#include "../objects/mouseCallback.h"
#include "../objects/scrollCallback.h"
#include "../objects/updateInput.h"
#include "../objects/frameBufferSizeCallback.h"

#include "../mesh/terrainGenerator.h"
#include "../mesh/cubeGenerator.h"
