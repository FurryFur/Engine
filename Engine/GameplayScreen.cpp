//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2018 Media Design School
//
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#define _USE_MATH_DEFINES
#include "GameplayScreen.h"

#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "PrimitivePrefabs.h"
#include "GLUtils.h"
#include "BasicCameraMovementSystem.h"
#include "Utils.h"
#include "Terrain.h"
#include "TerrainFollowSystem.h"
#include "SimpleWorldSpaceMoveSystem.h"
#include "ModelUtils.h"
#include "ParticleEmitterComponent.h"
#include "ParticleSystem.h"
#include "ssAnimatedModel.h"
#include "AnimationSystem.h"

#include <glm\gtc\constants.hpp>
#include <nanogui\nanogui.h>

#include <cmath>
#include <list>
#include <memory>

GameplayScreen::GameplayScreen()
	: Screen()
{
	setupGui();

	// Init systems
	m_activeSystems.push_back(std::make_unique<InputSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<TerrainFollowSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<AnimationSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<SimpleWorldSpaceMoveSystem>(m_scene));
	auto basicCameraMovementSystem = std::make_unique<BasicCameraMovementSystem>(m_scene);
	auto renderSystem = std::make_unique<RenderSystem>(m_scene);

	// Create environment map / skybox
	Entity& skybox = Prefabs::createSkybox(m_scene, {
		"Assets/Textures/envmap_violentdays/violentdays_rt.tga",
		"Assets/Textures/envmap_violentdays/violentdays_lf.tga",
		"Assets/Textures/envmap_violentdays/violentdays_up.tga",
		"Assets/Textures/envmap_violentdays/violentdays_dn.tga",
		"Assets/Textures/envmap_violentdays/violentdays_bk.tga",
		"Assets/Textures/envmap_violentdays/violentdays_ft.tga",
	});
	Texture irradianceMap = GLUtils::loadDDSTexture("Assets/Textures/envmap_violentdays/violentdays_iem.dds");
	Texture radianceMap = GLUtils::loadDDSTexture("Assets/Textures/envmap_violentdays/violentdays_pmrem.dds");
	renderSystem->setRadianceMap(radianceMap.id);
	renderSystem->setIrradianceMap(irradianceMap.id);

	// Setup the camera
	Entity& cameraEntity = Prefabs::createCamera(m_scene, { 0, 45, 25 }, { 0, 40, 0 }, { 0, 1, 0 });
	renderSystem->setCamera(&cameraEntity);
	basicCameraMovementSystem->setCameraToControl(&cameraEntity);

	Entity& terrain = Prefabs::createTerrainPerlin(m_scene, 1000);

	Entity& reflectiveSphere = Prefabs::createSphere(m_scene);
	reflectiveSphere.transform.position += glm::vec3(0, 40, 0);
	reflectiveSphere.model.materials[0].shader = &GLUtils::getDebugShader();
	reflectiveSphere.model.materials[0].debugColor = glm::vec3(1, 1, 1);
	reflectiveSphere.model.materials[0].shaderParams.glossiness = 1.0f;
	reflectiveSphere.model.materials[0].shaderParams.metallicness = 1.0f;
	reflectiveSphere.addComponents(COMPONENT_TERRAIN_FOLLOW, COMPONENT_SIMPLE_WORLD_SPACE_MOVE_COMPONENT,
	                               COMPONENT_INPUT, COMPONENT_INPUT_MAP, COMPONENT_PHYSICS);
	reflectiveSphere.terrainFollow.terrainToFollow = &terrain;
	reflectiveSphere.inputMap.forwardBtnMap = GLFW_KEY_UP;
	reflectiveSphere.inputMap.backwardBtnMap = GLFW_KEY_DOWN;
	reflectiveSphere.inputMap.leftBtnMap = GLFW_KEY_LEFT;
	reflectiveSphere.inputMap.rightBtnMap = GLFW_KEY_RIGHT;
	reflectiveSphere.inputMap.upBtnMap = GLFW_KEY_SPACE;
	reflectiveSphere.simpleWorldSpaceMovement.moveSpeed = 10;
	reflectiveSphere.terrainFollow.jumpEnabled = true;
	reflectiveSphere.terrainFollow.followerHalfHeight = 1.0f;

	Entity& character = m_scene.createEntity(COMPONENT_ANIMATED_MODEL, COMPONENT_TRANSFORM);
	character.transform.position += glm::vec3(5, 40, 0);
	//diffuseSphere.model.materials[0].shader = &GLUtils::getDebugShader();
	//diffuseSphere.model.materials[0].debugColor = glm::vec3(1, 1, 1);
	//diffuseSphere.model.materials[0].shaderParams.glossiness = 0.0f;
	//diffuseSphere.model.materials[0].shaderParams.metallicness = 0.0f;
	//diffuseSphere.model.materials[0].shaderParams.specBias = -0.04f;
	character.animatedModel.model = std::make_unique<ssAnimatedModel>("Assets/Models/theDude/theDude.DAE", "Assets/Models/theDude/theDude.png", GLUtils::getDebugShader().getGPUHandle());
	character.animatedModel.model->setCurrentAnimation(0, 30); // Idle
	character.addComponents(COMPONENT_TERRAIN_FOLLOW, COMPONENT_SIMPLE_WORLD_SPACE_MOVE_COMPONENT,
	                        COMPONENT_INPUT, COMPONENT_INPUT_MAP, COMPONENT_PHYSICS);
	character.terrainFollow.terrainToFollow = &terrain;
	character.transform.eulerAngles.y = glm::pi<float>();
	character.transform.scale *= 0.1f;
	character.inputMap.forwardBtnMap = GLFW_KEY_UP;
	character.inputMap.backwardBtnMap = GLFW_KEY_DOWN;
	character.inputMap.leftBtnMap = GLFW_KEY_LEFT;
	character.inputMap.rightBtnMap = GLFW_KEY_RIGHT;
	character.inputMap.upBtnMap = GLFW_KEY_SPACE;
	character.simpleWorldSpaceMovement.moveSpeed = 10;
	character.terrainFollow.jumpEnabled = true;
	character.terrainFollow.followerHalfHeight = 0.0f;

	Entity& directionalLight = m_scene.createEntity(COMPONENT_DIRECTIONAL_LIGHT);
	directionalLight.directionalLight.color = { 0.64, 0.39, 0.31 };
	directionalLight.directionalLight.direction = { 1, 1, -1 };

	Entity& particleEmitter = Prefabs::createParticleEmitter(m_scene, 
		128 * 1000, 
		1, 5,
		{ -10, -10, -10 }, { 10, 10, 10 },
		{ -1, -1, -1 }, { 1, 1, 1 }
	);
	particleEmitter.transform.position = glm::vec3{ 0, 40, 0 };

	m_activeSystems.push_back(std::move(renderSystem));
	m_activeSystems.push_back(std::move(basicCameraMovementSystem));
	m_activeSystems.push_back(std::make_unique<PhysicsSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<ParticleSystem>(m_scene));
}


GameplayScreen::~GameplayScreen()
{
}

// TODO: divide this into functions
void GameplayScreen::setupGui()
{

}
