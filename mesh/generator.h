void generateTerrain(int m, int n, int numHills, float maxRadius, float heightScale, int smoothingPasses, std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
	vertices.clear();
	indices.clear();

	// Thread-safe RNG: each call gets its own generator so the worker thread
	// never races with main() or any other thread.
	std::mt19937 rng(static_cast<unsigned int>(
		std::random_device{}() ^ static_cast<unsigned int>(std::time(nullptr))));
	std::uniform_real_distribution<float> distPos01(0.0f, 1.0f);

	std::vector<float> heights(m * n, 0.0f);

	for (int k = 0; k < numHills; ++k)
	{
		float cx     = distPos01(rng) * static_cast<float>(m - 1);
		float cz     = distPos01(rng) * static_cast<float>(n - 1);
		float radius = distPos01(rng) * maxRadius + 2.0f;
		float h      = (distPos01(rng) * 2.0f - 1.0f) * heightScale * 15.0f;

		for (int i = 0; i < m; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				float dx   = static_cast<float>(i) - cx;
				float dz   = static_cast<float>(j) - cz;
				float dist = std::sqrt(dx * dx + dz * dz);
				if (dist < radius)
				{
					// Cosine falloff
					float falloff = 0.5f * (std::cos(3.14159265f * dist / radius) + 1.0f);
					heights[i * n + j] += h * falloff;
				}
			}
		}
	}

	// --- Box-blur smoothing passes ---
	// Each pass replaces every height with the weighted average of its 3x3
	// neighbourhood. Multiple passes approximate a Gaussian blur, which
	// kills isolated spikes while leaving broad terrain features intact.
	for (int pass = 0; pass < smoothingPasses; ++pass)
	{
		std::vector<float> smoothed(m * n, 0.0f);
		for (int i = 0; i < m; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				float sum  = 0.0f;
				int   count = 0;
				for (int di = -1; di <= 1; ++di)
				{
					for (int dj = -1; dj <= 1; ++dj)
					{
						int ni = i + di, nj = j + dj;
						if (ni >= 0 && ni < m && nj >= 0 && nj < n)
						{
							sum += heights[ni * n + nj];
							++count;
						}
					}
				}
				smoothed[i * n + j] = sum / static_cast<float>(count);
			}
		}
		heights = std::move(smoothed);
	}

	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			float height = heights[i * n + j];
			Vertex v;
			v.position = glm::vec3(i - m / 2.0f, height, j - n / 2.0f);

			float colorFactor = (height + 5.0f) / 10.0f;
			colorFactor = glm::clamp(colorFactor, 0.0f, 1.0f);
			v.color = glm::vec3(0.2f, 0.5f + colorFactor * 0.5f, 0.2f + (1.0f - colorFactor) * 0.3f);

			v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			v.texCoord = glm::vec2(static_cast<float>(i) / m, static_cast<float>(j) / n);
			vertices.push_back(v);
		}
	}

	// Recalculate normals for lighting
	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			float hL = (i > 0) ? heights[(i - 1) * n + j] : heights[i * n + j];
			float hR = (i < m - 1) ? heights[(i + 1) * n + j] : heights[i * n + j];
			float hD = (j > 0) ? heights[i * n + (j - 1)] : heights[i * n + j];
			float hU = (j < n - 1) ? heights[i * n + (j + 1)] : heights[i * n + j];

			glm::vec3 normal;
			normal.x = hL - hR;
			normal.y = 2.0f;
			normal.z = hD - hU;
			vertices[i * n + j].normal = glm::normalize(normal);
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
}
