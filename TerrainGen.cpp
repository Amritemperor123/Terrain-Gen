#include"lib.h"

struct OrbitCamera
{
	float yaw = -90.f;
	float pitch = -20.f;
	float distance = 18.f;
	float sensitivity = 0.12f;
	float minPitch = -80.f;
	float maxPitch = 80.f;
	float minDistance = 2.f;
	float maxDistance = 100.f;
	bool firstMouse = true;
	double lastMouseX = 0.0;
	double lastMouseY = 0.0;
};

OrbitCamera gCamera;
glm::vec3 gOrbitTarget(0.f);

glm::vec3 getCameraPosition()
{
	const float yawRadians = glm::radians(gCamera.yaw);
	const float pitchRadians = glm::radians(gCamera.pitch);

	glm::vec3 offset;
	offset.x = cos(pitchRadians) * cos(yawRadians);
	offset.y = sin(pitchRadians);
	offset.z = cos(pitchRadians) * sin(yawRadians);

	return gOrbitTarget - glm::normalize(offset) * gCamera.distance;
}

void frame_buffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (gCamera.firstMouse)
	{
		gCamera.lastMouseX = xpos;
		gCamera.lastMouseY = ypos;
		gCamera.firstMouse = false;
	}

	const float xoffset = static_cast<float>(xpos - gCamera.lastMouseX) * gCamera.sensitivity;
	const float yoffset = static_cast<float>(gCamera.lastMouseY - ypos) * gCamera.sensitivity;

	gCamera.lastMouseX = xpos;
	gCamera.lastMouseY = ypos;

	// Orbit rotation (Left Mouse)
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		gCamera.yaw += xoffset;
		gCamera.pitch += yoffset;
		gCamera.pitch = glm::clamp(gCamera.pitch, gCamera.minPitch, gCamera.maxPitch);
	}

	// Pan (Right Mouse)
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		const float yawRad = glm::radians(gCamera.yaw);
		glm::vec3 forward = glm::normalize(glm::vec3(cos(yawRad), 0.f, sin(yawRad)));
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));

		const float panSpeed = 0.05f;
		gOrbitTarget -= right * xoffset * panSpeed;
		gOrbitTarget -= forward * yoffset * panSpeed;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.distance -= static_cast<float>(yoffset) * 2.0f;
	gCamera.distance = glm::clamp(gCamera.distance, gCamera.minDistance, gCamera.maxDistance);
}

void updateInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	const float moveSpeed = 0.15f;
	const float yawRad = glm::radians(gCamera.yaw);
	
	// Horizontal movement vectors
	glm::vec3 forward = glm::normalize(glm::vec3(cos(yawRad), 0.f, sin(yawRad)));
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		gOrbitTarget += forward * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		gOrbitTarget -= forward * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		gOrbitTarget -= right * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		gOrbitTarget += right * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		gOrbitTarget.y += moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		gOrbitTarget.y -= moveSpeed;
	}
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "ERROR::MAIN::GLFW_INIT_FAILED\n";
		return -1;
	}

	const int WINDOW_WIDTH = 1280;
	const int WINDOW_HEIGHT = 720;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Terrain Generator", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "ERROR::MAIN::WINDOW_INIT_FAILED\n";
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "ERROR::MAIN::GLAD_INIT_FAILED\n";
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glEnable(GL_DEPTH_TEST);

	Shader coreShader("vertex_core.glsl", "fragment_core.glsl");

	// Hardcoded length and width
	int m = 10; // length
	int n = 10; // width

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			Vertex v;
			v.position = glm::vec3(i - m / 2.0f, 0.0f, j - n / 2.0f);
			v.color = glm::vec3(0.8f, 0.8f, 0.8f);
			v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			v.texCoord = glm::vec2(static_cast<float>(i) / m, static_cast<float>(j) / n);
			vertices.push_back(v);
		}
	}

	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			int topLeft = i * n + j;
			int topRight = topLeft + 1;
			int bottomLeft = (i + 1) * n + j;
			int bottomRight = bottomLeft + 1;

			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	// Color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	// Normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	// TexCoord
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoord));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	glm::mat4 ModelMatrix(1.f);
	glm::mat4 ProjectionMatrix = glm::perspective(glm::radians(45.f), static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 100.f);

	while (!glfwWindowShouldClose(window))
	{
		updateInput(window);
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		coreShader.use();

		glm::mat4 ViewMatrix = glm::lookAt(getCameraPosition(), gOrbitTarget, glm::vec3(0.f, 1.f, 0.f));

		coreShader.setMat4fv(ModelMatrix, "ModelMatrix");
		coreShader.setMat4fv(ViewMatrix, "ViewMatrix");
		coreShader.setMat4fv(ProjectionMatrix, "ProjectionMatrix");

		glBindVertexArray(vao);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
