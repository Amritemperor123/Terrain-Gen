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

struct TerrainParams
{
    int m = 50;
    int n = 50;
    int numHills = 15;
    float maxHillRadius = 20.0f;
    float heightScale = 0.5f;
    int smoothingPasses = 2;
    unsigned int seed = 1337;
};

int main()
{
	if (!glfwInit())
	{
		std::cerr << "ERROR::MAIN::GLFW_INIT_FAILED\n";
		return -1;
	}

	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = primaryMonitor ? glfwGetVideoMode(primaryMonitor) : nullptr;

	int windowWidth = mode ? mode->width : 1280;
	int windowHeight = mode ? mode->height : 720;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Terrain & Cube Generator", primaryMonitor, nullptr);
	if (!window)
	{
		std::cerr << "WARNING::MAIN::FULLSCREEN_FAILED, falling back to windowed mode...\n";
		window = glfwCreateWindow(1280, 720, "Terrain & Cube Generator", nullptr, nullptr);
	}
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

	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glEnable(GL_DEPTH_TEST);

	Shader coreShader("shaders/vertex_core.glsl", "shaders/fragment_core.glsl");

	// Active terrain parameters being edited in UI
	TerrainParams activeParams;

	// Active cube generator parameters being edited in UI
	CubeParams activeCubeParams;

	// Render thread persistent terrain buffers
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;

	// Cube persistent buffers
	std::vector<Vertex> cubeVertices;
	std::vector<GLuint> cubeIndices;

	// Request synchronization state
	std::mutex requestMutex;
	TerrainParams targetParams = activeParams;
	uint64_t targetVersion = 0;
	bool stopRequested = false;

	// Async worker state
	std::future<void> terrainFuture;
	std::atomic<bool> isGenerating{ false };

	// Pending buffer handoff (protected by pendingMutex)
	std::mutex pendingMutex;
	std::vector<Vertex> pendingVertices;
	std::vector<GLuint> pendingIndices;
	bool pendingTopologyChanged = false;
	bool uploadPending = false;
	uint64_t pendingVersion = 0;

	// Terrain OpenGL Buffers
	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	// Configure Terrain VAO attribute pointers
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoord));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	// Cube OpenGL Buffers
	GLuint cubeVAO, cubeVBO, cubeEBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &cubeEBO);

	// Configure Cube VAO attribute pointers
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoord));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	// Function to generate and upload cube mesh to GPU
	auto updateCubeMesh = [&]() {
		generateCubes(activeCubeParams, cubeVertices, cubeIndices);

		glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(Vertex), cubeVertices.data(), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(GLuint), cubeIndices.data(), GL_DYNAMIC_DRAW);

		glBindVertexArray(0);
	};

	// Initial generation of cubes
	updateCubeMesh();

	// --- Worker function for terrain generation ---
	auto runWorker = [&]() {
		std::vector<float>  workerHeights;
		std::vector<float>  workerTemp;
		std::vector<Vertex> workerVertices;
		std::vector<GLuint> workerIndices;
		int workerCurrentM = 0;
		int workerCurrentN = 0;

		while (true)
		{
			TerrainParams paramsSnapshot;
			uint64_t versionSnapshot = 0;
			bool shouldStop = false;

			{
				std::lock_guard<std::mutex> lk(requestMutex);
				if (stopRequested)
				{
					shouldStop = true;
				}
				else
				{
					paramsSnapshot = targetParams;
					versionSnapshot = targetVersion;
				}
			}

			if (shouldStop)
				break;

			bool topologyChanged = (paramsSnapshot.m != workerCurrentM || paramsSnapshot.n != workerCurrentN);
			if (topologyChanged)
			{
				workerCurrentM = paramsSnapshot.m;
				workerCurrentN = paramsSnapshot.n;
				initStaticVertices(paramsSnapshot.m, paramsSnapshot.n, workerVertices);
				generateTerrainIndices(paramsSnapshot.m, paramsSnapshot.n, workerIndices);
			}

			generateTerrainHeights(
				paramsSnapshot.m, paramsSnapshot.n,
				paramsSnapshot.numHills, paramsSnapshot.maxHillRadius,
				paramsSnapshot.heightScale, paramsSnapshot.smoothingPasses,
				paramsSnapshot.seed,
				workerHeights, workerTemp
			);

			updateTerrainVertices(
				paramsSnapshot.m, paramsSnapshot.n,
				workerHeights, workerVertices
			);

			{
				std::lock_guard<std::mutex> lk(pendingMutex);
				if (versionSnapshot > pendingVersion)
				{
					pendingVersion = versionSnapshot;
					pendingVertices = workerVertices;
					if (topologyChanged)
					{
						pendingIndices = workerIndices;
						pendingTopologyChanged = true;
					}
					uploadPending = true;
				}
			}

			{
				std::lock_guard<std::mutex> lk(requestMutex);
				if (stopRequested || targetVersion == versionSnapshot)
				{
					isGenerating = false;
					break;
				}
			}
		}
	};

	// Launch worker if not already running, or update target version/params if running ("latest state wins")
	auto requestGeneration = [&]() {
		{
			std::lock_guard<std::mutex> lk(requestMutex);
			targetParams = activeParams;
			targetVersion++;
		}

		bool expected = false;
		if (isGenerating.compare_exchange_strong(expected, true))
		{
			if (terrainFuture.valid())
			{
				terrainFuture.get();
			}
			terrainFuture = std::async(std::launch::async, runWorker);
		}
	};

	// Kick off initial terrain generation
	requestGeneration();

	glm::mat4 ModelMatrix(1.f);
	bool wireframeMode = false;

	while (!glfwWindowShouldClose(window))
	{
		updateInput(window);
		glfwPollEvents();

		int displayW, displayH;
		glfwGetFramebufferSize(window, &displayW, &displayH);
		glViewport(0, 0, displayW, displayH);

		float aspect = (displayH > 0) ? (static_cast<float>(displayW) / static_cast<float>(displayH)) : 1.0f;
		glm::mat4 ProjectionMatrix = glm::perspective(glm::radians(45.f), aspect, 0.1f, 200.f);

		// --- Upload pending terrain results to GPU on main/render thread ---
		if (uploadPending)
		{
			std::lock_guard<std::mutex> lk(pendingMutex);
			if (uploadPending)
			{
				vertices = std::move(pendingVertices);

				if (pendingTopologyChanged)
				{
					indices = std::move(pendingIndices);

					glBindVertexArray(vao);
					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

					glBindVertexArray(0);
					pendingTopologyChanged = false;
				}
				else
				{
					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}

				uploadPending = false;
			}
		}

		// Start Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ----------------------------------------------------
		// UI Window 1: Terrain Settings (Snapped to Left)
		// ----------------------------------------------------
		{
			ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(340.0f, static_cast<float>(displayH) - 20.0f), ImGuiCond_Always);
			ImGui::Begin("Terrain Settings");

			// Spinner animation while generation is running in background
			if (isGenerating)
			{
				const char* spinFrames[] = { "| Generating...", "/ Generating...", "- Generating...", "\\ Generating..." };
				int frame = static_cast<int>(ImGui::GetTime() * 8.0) % 4;
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "%s", spinFrames[frame]);
				ImGui::Separator();
			}

			ImGui::Text("Grid Dimensions");
			bool changed = false;
			changed |= ImGui::SliderInt("Length", &activeParams.m, 2, 200);
			changed |= ImGui::SliderInt("Width", &activeParams.n, 2, 200);

			ImGui::Separator();
			ImGui::Text("Hill Settings");
			changed |= ImGui::SliderInt("Number of Hills", &activeParams.numHills, 1, 100);
			changed |= ImGui::SliderFloat("Max Hill Radius", &activeParams.maxHillRadius, 1.0f, 50.0f);
			changed |= ImGui::SliderFloat("Height Scale", &activeParams.heightScale, 0.0f, 2.0f);
			ImGui::Separator();
			ImGui::Text("Smoothing");
			ImGui::SetNextItemWidth(-1);
			changed |= ImGui::SliderInt("Blur Passes", &activeParams.smoothingPasses, 0, 10);
			ImGui::TextDisabled("0 = raw, 1-2 = smooth, 5+ = very smooth");

			ImGui::Separator();
			ImGui::Text("Random Seed");
			int terrainSeed = static_cast<int>(activeParams.seed);
			if (ImGui::InputInt("Terrain Seed", &terrainSeed))
			{
				if (terrainSeed < 0) terrainSeed = 0;
				activeParams.seed = static_cast<unsigned int>(terrainSeed);
				changed = true;
			}
			if (ImGui::Button("Randomize Seed"))
			{
				activeParams.seed += 1;
				changed = true;
			}

			// Auto-regen on any slider change or explicit button click ("latest state wins")
			if (ImGui::Button("Regenerate Terrain") || changed)
			{
				requestGeneration();
			}

			ImGui::Separator();
			ImGui::Text("Render Display");
			ImGui::Checkbox("Wireframe Terrain Mode", &wireframeMode);

			ImGui::Separator();
			ImGui::Text("Controls:");
			ImGui::BulletText("Left Mouse: Orbit");
			ImGui::BulletText("Right Mouse / WASD: Pan");
			ImGui::BulletText("Q/E: Up/Down");
			ImGui::BulletText("Scroll: Zoom");

			ImGui::End();
		}

		// ----------------------------------------------------
		// UI Window 2: Cube Generator Settings (Snapped to Right)
		// ----------------------------------------------------
		{
			ImGui::SetNextWindowPos(ImVec2(static_cast<float>(displayW) - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
			ImGui::SetNextWindowSize(ImVec2(340.0f, static_cast<float>(displayH) - 20.0f), ImGuiCond_Always);
			ImGui::Begin("Cube Generator Settings");

			bool cubeChanged = false;
			ImGui::Text("Cube Population");
			cubeChanged |= ImGui::SliderInt("Number of Cubes", &activeCubeParams.count, 0, 500);

			ImGui::Separator();
			ImGui::Text("Base Dimensions");
			cubeChanged |= ImGui::SliderFloat("Width", &activeCubeParams.width, 0.1f, 20.0f);
			cubeChanged |= ImGui::SliderFloat("Height", &activeCubeParams.height, 0.1f, 20.0f);
			cubeChanged |= ImGui::SliderFloat("Depth", &activeCubeParams.depth, 0.1f, 20.0f);

			ImGui::Separator();
			ImGui::Text("Randomness Parameters");
			cubeChanged |= ImGui::SliderFloat("Dimension Randomness", &activeCubeParams.dimensionRandomness, 0.0f, 2.0f);
			cubeChanged |= ImGui::SliderFloat("Scale of Randomness", &activeCubeParams.scaleOfRandomness, 1.0f, 100.0f);

			ImGui::Separator();
			ImGui::Text("Random Seed");
			int cubeSeed = static_cast<int>(activeCubeParams.seed);
			if (ImGui::InputInt("Cube Seed", &cubeSeed))
			{
				if (cubeSeed < 0) cubeSeed = 0;
				activeCubeParams.seed = static_cast<unsigned int>(cubeSeed);
				cubeChanged = true;
			}
			if (ImGui::Button("Randomize Cubes"))
			{
				activeCubeParams.seed += 1;
				cubeChanged = true;
			}

			if (cubeChanged)
			{
				updateCubeMesh();
			}

			ImGui::End();
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		coreShader.use();

		glm::mat4 ViewMatrix = glm::lookAt(getCameraPosition(), gOrbitTarget, glm::vec3(0.f, 1.f, 0.f));

		coreShader.setMat4fv(ModelMatrix, "ModelMatrix");
		coreShader.setMat4fv(ViewMatrix, "ViewMatrix");
		coreShader.setMat4fv(ProjectionMatrix, "ProjectionMatrix");

		// Draw terrain mesh (solid object mode by default)
		if (!indices.empty())
		{
			glBindVertexArray(vao);
			glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		// Draw cube generator mesh (solid filled mode alongside terrain)
		if (!cubeIndices.empty())
		{
			glBindVertexArray(cubeVAO);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cubeIndices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		// Rendering ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Signal worker thread to terminate and wait for completion
	{
		std::lock_guard<std::mutex> lk(requestMutex);
		stopRequested = true;
	}
	if (terrainFuture.valid())
	{
		terrainFuture.get();
	}

	// Cleanup OpenGL & GLFW
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &cubeEBO);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
