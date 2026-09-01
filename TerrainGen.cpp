#include "headers/lib.h"

// Force discrete GPU on hybrid (Optimus/PowerXpress) laptops.
// NVIDIA's OpenGL driver scans the EXE export table for NvOptimusEnablement;
// if set to 1, Optimus routes the OpenGL context to the dGPU instead of the iGPU.
// The AMD equivalent achieves the same for AMD hybrid setups.
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
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

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glEnable(GL_DEPTH_TEST);

	Shader coreShader("shaders/vertex_core.glsl", "shaders/fragment_core.glsl");

	// std::srand removed — generateTerrain now seeds its own mt19937 per call.

	int m = 50;
	int n = 50;
	float heightScale = 0.5f;
	int numHills = 15;
	float maxHillRadius = 20.0f;
	int smoothingPasses = 2;   // box-blur passes to remove height outliers

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	// --- Async terrain generation state ---
	std::future<void>   terrainFuture;
	std::vector<Vertex> pendingVertices;
	std::vector<GLuint> pendingIndices;
	std::atomic<bool>   isGenerating{ false };
	std::mutex          pendingMutex;
	bool                uploadPending{ false };


	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	auto uploadToGPU = [&]() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, color));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoord));
		glEnableVertexAttribArray(3);
		glBindVertexArray(0);
	};

	// --- Helper: launch terrain generation on a worker thread ---
	// Captures all terrain parameters by VALUE (snapshot) so slider drags
	// mid-generation cannot cause a data race.
	auto launchGeneration = [&]() {
		if (isGenerating) return;          // ignore if already running
		isGenerating = true;
		int    sm = m, sn = n, sHills = numHills, sPasses = smoothingPasses;
		float  sRadius = maxHillRadius, sScale = heightScale;
		terrainFuture = std::async(std::launch::async, [&, sm, sn, sHills, sRadius, sScale, sPasses]() {
			std::vector<Vertex>  v;
			std::vector<GLuint>  idx;
			generateTerrain(sm, sn, sHills, sRadius, sScale, sPasses, v, idx);
			{
				std::lock_guard<std::mutex> lk(pendingMutex);
				pendingVertices = std::move(v);
				pendingIndices  = std::move(idx);
				uploadPending   = true;
			}
			isGenerating = false;
		});
	};

	// Kick off the initial terrain asynchronously
	launchGeneration();

	glm::mat4 ModelMatrix(1.f);
	glm::mat4 ProjectionMatrix = glm::perspective(glm::radians(45.f), static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 200.f);

	while (!glfwWindowShouldClose(window))
	{
		updateInput(window);
		glfwPollEvents();

		// --- Check if the worker thread finished; swap buffers and upload on the GL thread ---
		if (uploadPending)
		{
			std::lock_guard<std::mutex> lk(pendingMutex);
			if (uploadPending)
			{
				vertices      = std::move(pendingVertices);
				indices       = std::move(pendingIndices);
				uploadToGPU();
				uploadPending = false;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("Terrain Settings");

			// Spinner animation while the worker thread is busy
			if (isGenerating)
			{
				const char* spinFrames[] = { "| Generating...", "/ Generating...", "- Generating...", "\\ Generating..." };
				int frame = static_cast<int>(ImGui::GetTime() * 8.0) % 4;
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "%s", spinFrames[frame]);
				ImGui::Separator();
			}

			// Disable all controls while a generation is in flight
			ImGui::BeginDisabled(isGenerating.load());

			ImGui::Text("Grid Dimensions");
			bool changed = false;
			changed |= ImGui::SliderInt("Length", &m, 2, 200);
			changed |= ImGui::SliderInt("Width", &n, 2, 200);

			ImGui::Separator();
			ImGui::Text("Hill Settings");
			changed |= ImGui::SliderInt("Number of Hills", &numHills, 1, 100);
			changed |= ImGui::SliderFloat("Max Hill Radius", &maxHillRadius, 1.0f, 50.0f);
			changed |= ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 2.0f);
			ImGui::Separator();
			ImGui::Text("Smoothing");
			ImGui::SetNextItemWidth(-1);
			changed |= ImGui::SliderInt("Blur Passes", &smoothingPasses, 0, 10);
			ImGui::TextDisabled("0 = raw, 1-2 = smooth, 5+ = very smooth");

			// Auto-regen on any slider change (no size cap — generation is non-blocking now)
			if (ImGui::Button("Regenerate Terrain") || changed)
			{
				launchGeneration();
			}

			ImGui::EndDisabled();

			ImGui::Separator();
			ImGui::Text("Controls:");
			ImGui::BulletText("Left Mouse: Orbit");
			ImGui::BulletText("Right Mouse / WASD: Pan");
			ImGui::BulletText("Q/E: Up/Down");
			ImGui::BulletText("Scroll: Zoom");

			ImGui::End();
		}

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

		// Rendering ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
