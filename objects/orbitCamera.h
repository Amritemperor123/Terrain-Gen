struct OrbitCamera
{
	float yaw = -90.f;
	float pitch = -30.f;
	float distance = 25.f;
	float sensitivity = 0.12f;
	float minPitch = -80.f;
	float maxPitch = 80.f;
	float minDistance = 2.f;
	float maxDistance = 100000.f;
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
