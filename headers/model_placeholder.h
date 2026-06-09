#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/vec2.hpp>
#include<glm/vec3.hpp>
#include<glm/vec4.hpp>
#include<glm/mat4x4.hpp>
#include<glad/glad.h>
#include<glfw3.h>

struct ModelPlaceholder
{
	struct Material
	{
		glm::vec3 ambient = glm::vec3(1.f);
		glm::vec3 diffuse = glm::vec3(0.85f, 0.9f, 0.95f);
		glm::vec3 specular = glm::vec3(0.f);
		std::string diffuseTexturePath;
	};

	struct FaceVertex
	{
		int positionIndex = -1;
		int texCoordIndex = -1;
		int normalIndex = -1;

		bool operator==(const FaceVertex& other) const
		{
			return positionIndex == other.positionIndex
				&& texCoordIndex == other.texCoordIndex
				&& normalIndex == other.normalIndex;
		}
	};

	struct FaceVertexHasher
	{
		size_t operator()(const FaceVertex& value) const
		{
			size_t seed = static_cast<size_t>(value.positionIndex + 1);
			seed ^= static_cast<size_t>(value.texCoordIndex + 37) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= static_cast<size_t>(value.normalIndex + 73) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	std::string sourcePath = "assets/models/Plane.obj";
	std::string materialPath;
	std::string activeMaterialName;

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	std::unordered_map<std::string, Material> materials;

	glm::vec3 modelCenter = glm::vec3(0.f);
	glm::vec3 minBounds = glm::vec3(0.f);
	glm::vec3 maxBounds = glm::vec3(0.f);
	float boundingRadius = 1.f;

	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	bool gpuReady = false;

	~ModelPlaceholder()
	{
		destroy();
	}

	bool loadFromFile(const std::string& objPath = "assets/models/Plane.obj")
	{
		destroy();
		sourcePath = objPath;
		materialPath.clear();
		activeMaterialName.clear();
		vertices.clear();
		indices.clear();
		materials.clear();
		modelCenter = glm::vec3(0.f);
		minBounds = glm::vec3(0.f);
		maxBounds = glm::vec3(0.f);
		boundingRadius = 1.f;

		std::ifstream objFile(sourcePath);
		if (!objFile.is_open())
		{
			std::cout << "ERROR::MODEL::COULD_NOT_OPEN_OBJ " << sourcePath << "\n";
			return false;
		}

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
		std::unordered_map<FaceVertex, GLuint, FaceVertexHasher> uniqueVertices;

		std::string line;
		std::string currentMaterial;
		const std::string baseDirectory = getParentDirectory(sourcePath);

		while (std::getline(objFile, line))
		{
			line = trim(line);
			if (line.empty() || line[0] == '#')
			{
				continue;
			}

			std::istringstream lineStream(line);
			std::string keyword;
			lineStream >> keyword;

			if (keyword == "v")
			{
				glm::vec3 position(0.f);
				lineStream >> position.x >> position.y >> position.z;
				positions.push_back(position);
			}
			else if (keyword == "vt")
			{
				glm::vec2 texCoord(0.f);
				lineStream >> texCoord.x >> texCoord.y;
				texCoord.y = 1.f - texCoord.y;
				texCoords.push_back(texCoord);
			}
			else if (keyword == "vn")
			{
				glm::vec3 normal(0.f);
				lineStream >> normal.x >> normal.y >> normal.z;
				normals.push_back(glm::normalize(normal));
			}
			else if (keyword == "mtllib")
			{
				std::string mtllibName;
				lineStream >> mtllibName;
				materialPath = joinPath(baseDirectory, mtllibName);
				loadMaterials(materialPath);
			}
			else if (keyword == "usemtl")
			{
				lineStream >> currentMaterial;
				if (activeMaterialName.empty())
				{
					activeMaterialName = currentMaterial;
				}
			}
			else if (keyword == "f")
			{
				std::vector<FaceVertex> face;
				std::string faceToken;
				while (lineStream >> faceToken)
				{
					face.push_back(parseFaceVertex(faceToken, positions.size(), texCoords.size(), normals.size()));
				}

				if (face.size() < 3)
				{
					continue;
				}

				for (size_t i = 1; i + 1 < face.size(); ++i)
				{
					appendFaceVertex(face[0], positions, texCoords, normals, currentMaterial, uniqueVertices);
					appendFaceVertex(face[i], positions, texCoords, normals, currentMaterial, uniqueVertices);
					appendFaceVertex(face[i + 1], positions, texCoords, normals, currentMaterial, uniqueVertices);
				}
			}
		}

		if (vertices.empty() || indices.empty())
		{
			std::cout << "ERROR::MODEL::NO_GEOMETRY_LOADED " << sourcePath << "\n";
			return false;
		}

		if (normals.empty())
		{
			recalculateNormals();
		}

		computeBounds();
		return true;
	}

	bool uploadToGpu()
	{
		if (vertices.empty() || indices.empty())
		{
			std::cout << "ERROR::MODEL::UPLOAD_WITHOUT_DATA\n";
			return false;
		}

		if (gpuReady)
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
			glDeleteBuffers(1, &ebo);
			vao = 0;
			vbo = 0;
			ebo = 0;
			gpuReady = false;
		}

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, color));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoord));
		glEnableVertexAttribArray(3);

		glBindVertexArray(0);
		gpuReady = true;
		return true;
	}

	void draw() const
	{
		if (!gpuReady)
		{
			return;
		}

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void releaseGpu()
	{
		destroy();
	}

	void printStatus() const
	{
		std::cout << "Model path: " << sourcePath << "\n";
		std::cout << "Material path: " << (materialPath.empty() ? "none" : materialPath) << "\n";
		std::cout << "Vertices loaded: " << vertices.size() << "\n";
		std::cout << "Indices loaded: " << indices.size() << "\n";
		std::cout << "Bounds min: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ")\n";
		std::cout << "Bounds max: (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")\n";
	}

private:
	static std::string trim(const std::string& value)
	{
		const size_t start = value.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
		{
			return "";
		}

		const size_t end = value.find_last_not_of(" \t\r\n");
		return value.substr(start, end - start + 1);
	}

	static std::string getParentDirectory(const std::string& path)
	{
		const size_t slashPos = path.find_last_of("/\\");
		if (slashPos == std::string::npos)
		{
			return ".";
		}

		return path.substr(0, slashPos);
	}

	static std::string joinPath(const std::string& directory, const std::string& filename)
	{
		if (filename.empty())
		{
			return directory;
		}

		if (filename.size() > 1 && filename[1] == ':')
		{
			return filename;
		}

		if (!filename.empty() && (filename[0] == '/' || filename[0] == '\\'))
		{
			return filename;
		}

		if (directory.empty() || directory == ".")
		{
			return filename;
		}

		return directory + "/" + filename;
	}

	static int resolveIndex(int objIndex, size_t elementCount)
	{
		if (objIndex > 0)
		{
			return objIndex - 1;
		}

		if (objIndex < 0)
		{
			return static_cast<int>(elementCount) + objIndex;
		}

		return -1;
	}

	static FaceVertex parseFaceVertex(const std::string& token, size_t positionCount, size_t texCoordCount, size_t normalCount)
	{
		FaceVertex result;
		std::stringstream tokenStream(token);
		std::string part;
		int partIndex = 0;

		while (std::getline(tokenStream, part, '/'))
		{
			if (!part.empty())
			{
				const int value = std::stoi(part);
				if (partIndex == 0)
				{
					result.positionIndex = resolveIndex(value, positionCount);
				}
				else if (partIndex == 1)
				{
					result.texCoordIndex = resolveIndex(value, texCoordCount);
				}
				else if (partIndex == 2)
				{
					result.normalIndex = resolveIndex(value, normalCount);
				}
			}

			++partIndex;
		}

		return result;
	}

	void loadMaterials(const std::string& path)
	{
		std::ifstream materialFile(path);
		if (!materialFile.is_open())
		{
			std::cout << "WARNING::MODEL::COULD_NOT_OPEN_MTL " << path << "\n";
			return;
		}

		Material currentMaterial;
		std::string currentName;
		std::string line;
		const std::string baseDirectory = getParentDirectory(path);

		while (std::getline(materialFile, line))
		{
			line = trim(line);
			if (line.empty() || line[0] == '#')
			{
				continue;
			}

			std::istringstream lineStream(line);
			std::string keyword;
			lineStream >> keyword;

			if (keyword == "newmtl")
			{
				if (!currentName.empty())
				{
					materials[currentName] = currentMaterial;
				}

				currentMaterial = Material();
				lineStream >> currentName;
			}
			else if (keyword == "Ka")
			{
				lineStream >> currentMaterial.ambient.x >> currentMaterial.ambient.y >> currentMaterial.ambient.z;
			}
			else if (keyword == "Kd")
			{
				lineStream >> currentMaterial.diffuse.x >> currentMaterial.diffuse.y >> currentMaterial.diffuse.z;
			}
			else if (keyword == "Ks")
			{
				lineStream >> currentMaterial.specular.x >> currentMaterial.specular.y >> currentMaterial.specular.z;
			}
			else if (keyword == "map_Kd")
			{
				std::string texturePath;
				std::getline(lineStream, texturePath);
				texturePath = trim(texturePath);
				currentMaterial.diffuseTexturePath = joinPath(baseDirectory, texturePath);
			}
		}

		if (!currentName.empty())
		{
			materials[currentName] = currentMaterial;
		}
	}

	void appendFaceVertex(
		const FaceVertex& faceVertex,
		const std::vector<glm::vec3>& positions,
		const std::vector<glm::vec2>& texCoords,
		const std::vector<glm::vec3>& normals,
		const std::string& materialName,
		std::unordered_map<FaceVertex, GLuint, FaceVertexHasher>& uniqueVertices)
	{
		auto it = uniqueVertices.find(faceVertex);
		if (it != uniqueVertices.end())
		{
			indices.push_back(it->second);
			return;
		}

		if (faceVertex.positionIndex < 0 || faceVertex.positionIndex >= static_cast<int>(positions.size()))
		{
			return;
		}

		Vertex vertex{};
		vertex.position = positions[faceVertex.positionIndex];
		vertex.normal = (faceVertex.normalIndex >= 0 && faceVertex.normalIndex < static_cast<int>(normals.size()))
			? normals[faceVertex.normalIndex]
			: glm::vec3(0.f, 1.f, 0.f);
		vertex.texCoord = (faceVertex.texCoordIndex >= 0 && faceVertex.texCoordIndex < static_cast<int>(texCoords.size()))
			? texCoords[faceVertex.texCoordIndex]
			: glm::vec2(0.f);
		vertex.color = getMaterialColor(materialName);

		const GLuint newIndex = static_cast<GLuint>(vertices.size());
		vertices.push_back(vertex);
		indices.push_back(newIndex);
		uniqueVertices[faceVertex] = newIndex;
	}

	glm::vec3 getMaterialColor(const std::string& materialName) const
	{
		if (!materialName.empty())
		{
			const auto found = materials.find(materialName);
			if (found != materials.end())
			{
				return found->second.diffuse;
			}
		}

		if (!materials.empty())
		{
			return materials.begin()->second.diffuse;
		}

		return glm::vec3(0.85f, 0.9f, 0.95f);
	}

	void recalculateNormals()
	{
		for (Vertex& vertex : vertices)
		{
			vertex.normal = glm::vec3(0.f);
		}

		for (size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			Vertex& a = vertices[indices[i]];
			Vertex& b = vertices[indices[i + 1]];
			Vertex& c = vertices[indices[i + 2]];

			const glm::vec3 edge1 = b.position - a.position;
			const glm::vec3 edge2 = c.position - a.position;
			const glm::vec3 faceNormal = glm::cross(edge1, edge2);

			if (glm::length(faceNormal) > 0.f)
			{
				a.normal += faceNormal;
				b.normal += faceNormal;
				c.normal += faceNormal;
			}
		}

		for (Vertex& vertex : vertices)
		{
			if (glm::length(vertex.normal) > 0.f)
			{
				vertex.normal = glm::normalize(vertex.normal);
			}
			else
			{
				vertex.normal = glm::vec3(0.f, 1.f, 0.f);
			}
		}
	}

	void computeBounds()
	{
		minBounds = vertices.front().position;
		maxBounds = vertices.front().position;

		for (const Vertex& vertex : vertices)
		{
			minBounds = glm::min(minBounds, vertex.position);
			maxBounds = glm::max(maxBounds, vertex.position);
		}

		modelCenter = (minBounds + maxBounds) * 0.5f;
		boundingRadius = 0.f;

		for (const Vertex& vertex : vertices)
		{
			boundingRadius = std::max(boundingRadius, glm::length(vertex.position - modelCenter));
		}

		if (boundingRadius <= 0.f)
		{
			boundingRadius = 1.f;
		}
	}

	void destroy()
	{
		if (gpuReady)
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
			glDeleteBuffers(1, &ebo);
		}

		vao = 0;
		vbo = 0;
		ebo = 0;
		gpuReady = false;
	}
};
