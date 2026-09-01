void updateInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard) return;

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
