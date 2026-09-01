void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;

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
