#define _USE_MATH_DEFINES
#include "GameplayScreen.h"

#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "PrimitivePrefabs.h"
#include "GLUtils.h"
#include "LevelLoader.h"
#include "BasicCameraMovementSystem.h"
#include "Utils.h"
#include "Terrain.h"
#include "TerrainFollowSystem.h"
#include "SimpleWorldSpaceMoveSystem.h"
#include "ClothSystem.h"
#include "MousePickingSystem.h"
#include "CollisionSystem.h"

#include <glm\gtc\constants.hpp>
#include <nanogui\nanogui.h>

#include <cmath>
#include <list>

enum test_enum {
	Item1 = 0,
	Item2,
	Item3
};

GameplayScreen::GameplayScreen()
	: Screen()
{
	bool bvar = true;
	int ivar = 12345678;
	double dvar = 3.1415926;
	float fvar = (float)dvar;
	std::string strval = "A string";
	test_enum enumval = Item2;
	nanogui::Color colval(0.5f, 0.5f, 0.7f, 1.f);

	// Create nanogui gui
	bool enabled = true;
	nanogui::FormHelper *gui = new nanogui::FormHelper(m_uiScreen);
	nanogui::ref<nanogui::Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Form helper example");
	gui->addGroup("Basic types");
	gui->addVariable("bool", bvar)->setTooltip("Test tooltip.");
	gui->addVariable("string", strval);

	gui->addGroup("Validating fields");
	gui->addVariable("int", ivar)->setSpinnable(true);
	gui->addVariable("float", fvar)->setTooltip("Test.");
	gui->addVariable("double", dvar)->setSpinnable(true);

	gui->addGroup("Complex types");
	gui->addVariable("Enumeration", enumval, enabled)->setItems({ "Item 1", "Item 2", "Item 3" });
	gui->addVariable("Color", colval)->setCallback([](const nanogui::Color &c) {
		std::cout << "ColorPicker Final Callback: ["
			<< c.r() << ", "
			<< c.g() << ", "
			<< c.b() << ", "
			<< c.w() << "]" << std::endl;
	});

	gui->addGroup("Other widgets");
	gui->addButton("A button", []() { std::cout << "Button pressed." << std::endl; })->setTooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");;

	m_uiScreen->setVisible(true);
	m_uiScreen->performLayout();
	nanoguiWindow->center();


	// Init systems
	m_activeSystems.push_back(std::make_unique<InputSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<TerrainFollowSystem>(m_scene));
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
	Entity& cameraEntity = Prefabs::createCamera(m_scene, { 0, 0, 3 }, { 0, 0, 0 }, { 0, 1, 0 });
	renderSystem->setCamera(&cameraEntity);
	basicCameraMovementSystem->setCameraToControl(&cameraEntity);

	//Prefabs::createTerrain(m_scene, "Assets/Textures/Heightmaps/heightmap_2.png", 100, 100);
	//Entity& terrain = Prefabs::createTerrain(m_scene, "Assets/Textures/Heightmaps/heightmap_2.png", 1000);

	//Entity& reflectiveSphere = Prefabs::createSphere(m_scene);
	//reflectiveSphere.transform.position += glm::vec3(0, 40, 0);
	//reflectiveSphere.model.materials[0].shader = &GLUtils::getDebugShader();
	//reflectiveSphere.model.materials[0].debugColor = glm::vec3(1, 1, 1);
	//reflectiveSphere.model.materials[0].shaderParams.glossiness = 1.0f;
	//reflectiveSphere.model.materials[0].shaderParams.metallicness = 1.0f;
	//reflectiveSphere.addComponents(COMPONENT_TERRAIN_FOLLOW, COMPONENT_SIMPLE_WORLD_SPACE_MOVE_COMPONENT,
	//                               COMPONENT_INPUT, COMPONENT_INPUT_MAP);
	//reflectiveSphere.terrainFollow.terrainToFollow = &terrain;
	//reflectiveSphere.inputMap.forwardBtnMap = GLFW_KEY_UP;
	//reflectiveSphere.inputMap.backwardBtnMap = GLFW_KEY_DOWN;
	//reflectiveSphere.inputMap.leftBtnMap = GLFW_KEY_LEFT;
	//reflectiveSphere.inputMap.rightBtnMap = GLFW_KEY_RIGHT;
	//reflectiveSphere.simpleWorldSpaceMovement.moveSpeed = 10;
	//reflectiveSphere.terrainFollow.followerHalfHeight = 1.0f;

	//Entity& diffuseSphere = Prefabs::createSphere(m_scene);
	//diffuseSphere.transform.position += glm::vec3(5, 40, 0);
	//diffuseSphere.model.materials[0].shader = &GLUtils::getDebugShader();
	//diffuseSphere.model.materials[0].debugColor = glm::vec3(1, 1, 1);
	//diffuseSphere.model.materials[0].shaderParams.glossiness = 0.0f;
	//diffuseSphere.model.materials[0].shaderParams.metallicness = 0.0f;
	//diffuseSphere.model.materials[0].shaderParams.specBias = -0.04f;
	//diffuseSphere.addComponents(COMPONENT_TERRAIN_FOLLOW, COMPONENT_SIMPLE_WORLD_SPACE_MOVE_COMPONENT,
	//	COMPONENT_INPUT, COMPONENT_INPUT_MAP);
	//diffuseSphere.terrainFollow.terrainToFollow = &terrain;
	//diffuseSphere.inputMap.forwardBtnMap = GLFW_KEY_UP;
	//diffuseSphere.inputMap.backwardBtnMap = GLFW_KEY_DOWN;
	//diffuseSphere.inputMap.leftBtnMap = GLFW_KEY_LEFT;
	//diffuseSphere.inputMap.rightBtnMap = GLFW_KEY_RIGHT;
	//diffuseSphere.simpleWorldSpaceMovement.moveSpeed = 10;
	//diffuseSphere.terrainFollow.followerHalfHeight = 1.0f;

	TransformComponent sphereTransform;
	sphereTransform.position.y = -1.2f;
	sphereTransform.position.x = 2.0f;
	Entity& sphere = Prefabs::createSphere(m_scene, sphereTransform);
	sphere.addComponents(COMPONENT_SPHERE_COLLISION | COMPONENT_SIMPLE_WORLD_SPACE_MOVE_COMPONENT | COMPONENT_INPUT | COMPONENT_INPUT_MAP);
	sphere.sphereCollision.radius = 1.0f;
	sphere.inputMap.backwardBtnMap = GLFW_KEY_DOWN;
	sphere.inputMap.forwardBtnMap = GLFW_KEY_UP;
	sphere.inputMap.leftBtnMap = GLFW_KEY_LEFT;
	sphere.inputMap.rightBtnMap = GLFW_KEY_RIGHT;
	sphere.simpleWorldSpaceMovement.moveSpeed = 1.0f;

	TransformComponent triangleTransform;
	triangleTransform.position.y = -1.4f;
	triangleTransform.position.x = -3.0f;
	Entity& pyramid = Prefabs::createPyramid(m_scene, triangleTransform);
	pyramid.addComponents(COMPONENT_PYRAMID_COLLISION | COMPONENT_SIMPLE_WORLD_SPACE_MOVE_COMPONENT | COMPONENT_INPUT | COMPONENT_INPUT_MAP);
	pyramid.sphereCollision.radius = 1.0f;
	pyramid.inputMap.backwardBtnMap = GLFW_KEY_DOWN;
	pyramid.inputMap.forwardBtnMap = GLFW_KEY_UP;
	pyramid.inputMap.leftBtnMap = GLFW_KEY_LEFT;
	pyramid.inputMap.rightBtnMap = GLFW_KEY_RIGHT;
	pyramid.simpleWorldSpaceMovement.moveSpeed = 1.0f;

	TransformComponent groundTransform;
	groundTransform.position.y = -3.0f;
	groundTransform.position.x = 0.0f;
	groundTransform.eulerAngles = {-glm::half_pi<float>(), 0.0f, 0.0f};
	groundTransform.scale = { 100.0f, 100.0f, 1.0f };
	Entity& ground = Prefabs::createQuad(m_scene, groundTransform);
	ground.addComponents(COMPONENT_GROUND_COLLISION);

	Entity& cloth = Prefabs::createCloth(m_scene, 10, 10, 2, 2, 1);

	m_activeSystems.push_back(std::move(basicCameraMovementSystem));
	m_activeSystems.push_back(std::move(renderSystem));
	m_activeSystems.push_back(std::make_unique<MousePickingSystem>(m_scene, cameraEntity));
	m_activeSystems.push_back(std::make_unique<PhysicsSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<SimpleWorldSpaceMoveSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<CollisionSystem>(m_scene));
	m_activeSystems.push_back(std::make_unique<ClothSystem>(m_scene));
}


GameplayScreen::~GameplayScreen()
{
}
