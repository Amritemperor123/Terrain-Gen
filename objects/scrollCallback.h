void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;

	gCamera.distance -= static_cast<float>(yoffset) * 2.0f;
	gCamera.distance = glm::clamp(gCamera.distance, gCamera.minDistance, gCamera.maxDistance);
}
