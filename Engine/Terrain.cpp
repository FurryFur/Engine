//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2018 Media Design School
//
//                scene.
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#include "Terrain.h"

#include "Entity.h"
#include "VertexFormat.h"
#include "GLUtils.h"
#include "Scene.h"
#include "GLMUtils.h"

#include "stb_image.h"

using namespace glm;

bool TerrainUtils::castPosToTerrainHeight(const Entity& terrainEntity, const vec3& entityPos, float& outHeight)
{
	vec2 texCoord;
	if (!castPosToHeightMapTexCoord(terrainEntity, entityPos, texCoord))
		return false;

	// Bilinear interpolation
	int numPixelsX = terrainEntity.terrain.heightMapDimensions.x;
	float xBlend = glm::fract(texCoord.x);
	float yBlend = glm::fract(texCoord.y);

	// Blend top left and top right corners of the 2 by 2 pixel square
	float heightTopLeft = terrainEntity.terrain.heightMap[static_cast<GLuint>(glm::floor(texCoord.y)) * numPixelsX + static_cast<GLuint>(glm::floor(texCoord.x))];
	float heightTopRight = terrainEntity.terrain.heightMap[static_cast<GLuint>(glm::floor(texCoord.y)) * numPixelsX + static_cast<GLuint>(glm::ceil(texCoord.x))];
	float heightTop = (1 - xBlend) * heightTopLeft + xBlend * heightTopRight;

	// Blend bottom left and bottom right corners of the 2 by 2 pixel square
	float heightBottomLeft = terrainEntity.terrain.heightMap[static_cast<GLuint>(glm::ceil(texCoord.y)) * numPixelsX + static_cast<GLuint>(glm::floor(texCoord.x))];
	float heightBottomRight = terrainEntity.terrain.heightMap[static_cast<GLuint>(glm::ceil(texCoord.y)) * numPixelsX + static_cast<GLuint>(glm::ceil(texCoord.x))];
	float heightBottom = (1 - xBlend) * heightBottomLeft + xBlend * heightBottomRight;

	// Blend the two above results across the y component for final blend
	float height = (1 - yBlend) * heightTop + yBlend * heightBottom;

	// Scale and offset the height by the terrains offset and height scale
	float heightScale = terrainEntity.terrain.heightScale;
	float yOffset = terrainEntity.transform.position.y;
	outHeight = height * heightScale + yOffset;

	return true;
}

bool TerrainUtils::castPosToHeightMapTexCoord(const Entity& terrainEntity, const vec3& entityPos, vec2& outTexCoord)
{
	float terrainExtent = terrainEntity.terrain.size / 2.0f;
	float terrainSize = terrainEntity.terrain.size;
	vec2 terrainPos = vec2{ terrainEntity.transform.position.x, terrainEntity.transform.position.z };
	vec2 terrainTopLeft = terrainPos - terrainExtent;
	ivec2 numHeightMapPixels = terrainEntity.terrain.heightMapDimensions;
	vec2 entityXZPos = vec2{ entityPos.x, entityPos.z };
	vec2 normalizedTexCoords = (entityXZPos - terrainTopLeft) / terrainSize;

	if (normalizedTexCoords.x < 0 || normalizedTexCoords.y < 0
	 || normalizedTexCoords.x > 1 || normalizedTexCoords.y > 1)
		return false;

	outTexCoord = normalizedTexCoords * vec2(numHeightMapPixels - 1);

	return true;
}

glm::vec3 TerrainUtils::castHeightMapTexCoordToWorldPos(const Entity& terrainEntity, const glm::vec2& texCoord)
{
	const float size = terrainEntity.terrain.size;
	const int numPixelsX = terrainEntity.terrain.heightMapDimensions.x;
	const int numPixelsY = terrainEntity.terrain.heightMapDimensions.y;
	const float heightScale = terrainEntity.terrain.heightScale;
	const float extent = size / 2;
	const float xSpacing = size / (numPixelsX - 1);
	const float zSpacing = size / (numPixelsY - 1);
	const auto& heightMapData = terrainEntity.terrain.heightMap;

	vec3 worldPos;
	worldPos.x = texCoord.x * xSpacing - extent;
	worldPos.y = heightMapData[static_cast<size_t>(glm::round(texCoord.y * numPixelsX + texCoord.x))] * heightScale;
	worldPos.z = texCoord.y * zSpacing - extent;
	worldPos += terrainEntity.transform.position; // Offset by overall terrain position
	return worldPos;
}

void _fillTerrain(Scene& scene, Entity& terrain, float size, GLsizei baseTessellation, const glm::vec3& position) {
	std::vector<float>& heightMapData = terrain.terrain.heightMap;
	int numPixelsX = terrain.terrain.heightMapDimensions.x;
	int numPixelsY = terrain.terrain.heightMapDimensions.y;

	// Create CPU tesselated quad for base tesselation level
	GLsizei numVertsX, numVertsZ;
	numVertsX = numVertsZ = baseTessellation + 1;
	std::vector<VertexFormat> meshVertices;
	std::vector<GLuint> meshIndices;
	GLUtils::createTessellatedQuadData(numVertsX, numVertsZ, size, size, meshVertices, meshIndices);

	// Compute normal map
	std::vector<vec3> normalMapData(numPixelsX * numPixelsY);
	for (GLsizei r = 0; r < numPixelsY; ++r) {
		for (GLsizei c = 0; c < numPixelsX; ++c) {

			// Compute and accumulate tri normals surrounding vertex
			vec3 normalAccumulator = { 0, 0, 0 };
			GLsizei numSurroundingTris = 0;
			for (int i = -1; i < 1; ++i) {
				for (int j = -1; j < 1; ++j) {
					// Get corresponding cell verts
					std::array<vec3, 4> cellVerts;
					for (GLsizei k = 0; k < cellVerts.size(); ++k) {
						GLsizei pixelR = r + i + (k / 2);
						GLsizei pixelC = c + j + (k % 2);
						pixelR = pixelR >= 0 && pixelR < numPixelsY ? pixelR : r;
						pixelC = pixelC >= 0 && pixelC < numPixelsX ? pixelC : c;
						cellVerts[k] = TerrainUtils::castHeightMapTexCoordToWorldPos(terrain, { pixelC, pixelR });
					}

					// Get vertices for each tri in cell
					// 0--1
					// |\ |
					// | \| 
					// 2--3
					std::array<std::array<vec3, 3>, 2> tris = { { { cellVerts[0], cellVerts[2], cellVerts[3] },
					{ cellVerts[0], cellVerts[3], cellVerts[1] } } };

					// Compute and accumulate normals for each tri in cell
					for (auto& triVerts : tris) {
						vec3 edge1 = triVerts[1] - triVerts[0];
						vec3 edge2 = triVerts[2] - triVerts[0];

						vec3 normal = cross(edge1, edge2);
						if (normal.x != 0 || normal.y != 0 || normal.z != 0) { // Can't normalize zero vectors generated at edges
							normalAccumulator += normalize(normal);
							++numSurroundingTris;
						}
					}
				}
			}
			normalMapData[r * numPixelsX + c] = normalize(normalAccumulator / static_cast<float>(numSurroundingTris));
		}
	}
	// Create GPU normalMap Texture
	Texture normalMap = Texture::Texture2D(numPixelsX, numPixelsY, GL_RGB, GL_FLOAT, GL_RGB, normalMapData.data());

	// Create GPU mesh
	Mesh mesh = GLUtils::bufferMeshData(meshVertices, meshIndices);

	// Fill model component with mesh data
	terrain.model.rootNode.meshIDs.push_back(0);
	terrain.model.rootNode.meshIDs.push_back(1);
	terrain.model.meshes.push_back(mesh);
	terrain.model.meshes.push_back(mesh);
	terrain.model.meshes[1].materialIndex = 1;

	// Create terrain material component
	Material terrainMaterial;
	terrainMaterial.shader = &GLUtils::getTerrainShader();
	terrainMaterial.colorMaps.push_back(GLUtils::loadTexture("Assets/Textures/dessert-floor.png"));
	terrainMaterial.normalMaps.push_back(normalMap);
	Texture heightMap = Texture::Texture2D(numPixelsX, numPixelsY, GL_RED, GL_FLOAT, GL_RED, heightMapData.data());
	terrainMaterial.heightMaps.push_back(heightMap);
	terrainMaterial.heightMapScale = terrain.terrain.heightScale;
	terrainMaterial.willDrawDepth = true;
	terrainMaterial.shaderParams.metallicness = 0;
	terrainMaterial.shaderParams.glossiness = 0;
	terrainMaterial.shaderParams.specBias = 0;
	terrain.model.materials.push_back(std::move(terrainMaterial));

	// Create grass material component
	Material grassMaterial;
	grassMaterial.shader = &GLUtils::getTerrainGrassGeoShader();
	grassMaterial.colorMaps.push_back(GLUtils::loadTexture("Assets/Textures/weedy_grass.png"));
	grassMaterial.colorMaps.push_back(GLUtils::loadTexture("Assets/Textures/vegetation_map.png", false, false));
	grassMaterial.heightMaps.push_back(heightMap);
	grassMaterial.heightMapScale = terrain.terrain.heightScale;
	grassMaterial.willDrawDepth = true;
	grassMaterial.shaderParams.metallicness = 0;
	grassMaterial.shaderParams.glossiness = 0;
	grassMaterial.shaderParams.specBias = 0;
	grassMaterial.shaderParams.discardTransparent = true;
	terrain.model.materials.push_back(std::move(grassMaterial));
}

Entity& Prefabs::createTerrain(Scene& scene, const std::string& heightMapFile, float size, GLsizei baseTessellation, const glm::vec3& position)
{
	const float heightScale = size * 0.1f;

	// Read height map from file
	int numPixelsX, numPixelsY, numChannels;
	unsigned char* heightMapImg = stbi_load(heightMapFile.c_str(), &numPixelsX, &numPixelsY, &numChannels, 0);

	// Create the terrain entity
	Entity& terrain = scene.createEntity(COMPONENT_MODEL, COMPONENT_TRANSFORM);
	terrain.terrain.heightMap = std::vector<float>(numPixelsX * numPixelsY);
	terrain.terrain.heightMapDimensions = { numPixelsX, numPixelsY };

	// Fill terrain variables
	terrain.transform.position = position;
	terrain.terrain.heightScale = heightScale;
	terrain.terrain.size = size;

	// Convert to array of floats
	std::vector<float>& heightMapData = terrain.terrain.heightMap;
	for (GLsizei r = 0; r < numPixelsY; ++r) {
		for (GLsizei c = 0; c < numPixelsX; ++c) {
			heightMapData[r * numPixelsX + c] = heightMapImg[(r * numPixelsX + c) * numChannels] / 255.0f; // Ignore any extra channels by skipping over them
		}
	}

	stbi_image_free(heightMapImg);

	_fillTerrain(scene, terrain, size, baseTessellation, position);

	return terrain;
}

Entity& Prefabs::createTerrainPerlin(Scene& scene, float size, GLuint heightMapResolution, GLsizei baseTessellation, const glm::vec3& position)
{
	const float heightScale = size * 0.1f;

	// Create the terrain entity
	Entity& terrain = scene.createEntity(COMPONENT_MODEL, COMPONENT_TRANSFORM);
	terrain.terrain.heightMap = std::vector<float>(heightMapResolution * heightMapResolution);
	terrain.terrain.heightMapDimensions = { heightMapResolution, heightMapResolution };

	// Fill terrain variables
	terrain.transform.position = position;
	terrain.terrain.heightScale = heightScale;
	terrain.terrain.size = size;

	// Fill height map with perlin noise
	std::vector<float>& heightMapData = terrain.terrain.heightMap;
	for (GLuint y = 0; y < heightMapResolution; ++y) {
		for (GLuint x = 0; x < heightMapResolution; ++x) {
			heightMapData[y * heightMapResolution + x] = GLMUtils::octavePerlin(
				glm::vec3{ x / 250.0f, y / 250.0f, 0 },
				6,
				2
			);
		}
	}

	_fillTerrain(scene, terrain, size, baseTessellation, position);

	return terrain;
}
