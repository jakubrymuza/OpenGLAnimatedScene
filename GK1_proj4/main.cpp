#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "light.h"

#include <iostream>
#include <vector>
#include <algorithm>

#define PI 3.14159265358979323846

// screen info
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

// cameras info
#define CAMERAS_COUNT 3
#define STATIC_CAMERA 0 // static camera
#define STATIC_FOLLOWING_CAMERA 1 // static camera that follows the moving object
#define MOVING_CAMERA 2 // camera that moves alongside the moving object
#define DEFAULT_CAMERA STATIC_CAMERA

// shading algorithms info
#define FLAT_SHADING 0
#define GOURAUD_SHADING 1
#define PHONG_SHADING 2
#define DEFAULT_SHADING PHONG_SHADING

// specular component
#define BLINN_SPECULAR 0
#define PHONG_SPECULAR 1
#define DEFAULT_SPECULAR PHONG_SPECULAR

// lights info
#define STATIC_LIGHT 0
#define SUN_LIGHT 1
#define HEADLIGHTS1_LIGHT 2
#define HEADLIGHTS2_LIGHT 3
#define BEACON1_LIGHT 4
#define BEACON2_LIGHT 5

// fog
#define FOG_MAXDIST 3
#define FOG_MINDIST 2

// current state info
int currentCameraID = DEFAULT_CAMERA;
int currentShading = DEFAULT_SHADING;
int currentSpecular = DEFAULT_SPECULAR;
int currentWidth = SCREEN_WIDTH;
int currentHeight = SCREEN_HEIGHT;

// perspective info
#define FIELD_OF_VIEW 45.0f
#define ASPECT_RATIO (float)currentWidth / (float)currentHeight
#define NEAR 0.1f
#define FAR 100.0f

// other
#define R 0.4f // car's track radius

// functions
GLFWwindow* setupWindow();
void setDebugWindowTitle(GLFWwindow* window);
void drawBuilding(Shader shader, Model& buildingModel);
void drawBuilding2(Shader shader, Model& buildingModel);
void drawSphere(Shader shader, Model& sphereModel);
void drawGround(Shader shader, Model& groundModel);
void drawCar(Shader shader, Model& carModel, glm::vec3& carPosition);
void moveLights(std::vector<Light>& lights, glm::vec3& carPosition);
void setViewMatrix(Camera camera, Shader shader);
void setProjectionMatrix(Shader shader);
void moveCameras(std::vector<Camera>& cameras, const glm::vec3& carPosition);
void setFog(Shader shader);
void setConstants(Shader shader);
void bindLights(Shader shader, std::vector<Light>& lights);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
glm::vec3 getCircularPosition(float radius, float radiusShift = 0, float angleShift = 0, float angleMultiplier = 1);
std::vector<Camera> setupCameras();
std::vector<Shader> setupShaders();
std::vector<Light> setupLights();

int main()
{
	// --- setup
	GLFWwindow* window = setupWindow();

	if (window == NULL)
		return -1;

	std::vector<Camera> cameras = setupCameras();
	std::vector<Shader> shaders = setupShaders();
	std::vector<Light> lights = setupLights();

	// --- loading models
	Model carModel("models/police_car/Police_Vehicle.obj");
	Model groundModel("models/ground/ground.obj");
	Model sphereModel("models/sphere/sphere.obj");
	Model buildingModel("models/building/building.obj");

	// --- main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaders[currentShading].use();

		setDebugWindowTitle(window);

		// --- animations 	
		glm::vec3 carPosition = getCircularPosition(R);

		moveCameras(cameras, carPosition);

		moveLights(lights, carPosition);

		// --- bindings
		setProjectionMatrix(shaders[currentShading]);
		setViewMatrix(cameras[currentCameraID], shaders[currentShading]);	

		bindLights(shaders[currentShading], lights);

		setConstants(shaders[currentShading]);
		setFog(shaders[currentShading]);

		shaders[currentShading].setInt("specular_type", currentSpecular);
		shaders[currentShading].setVec3("cameraPosition", cameras[currentCameraID].Position);

		// --- moving & drawing models
		drawCar(shaders[currentShading], carModel, carPosition);
		drawGround(shaders[currentShading], groundModel);
		drawSphere(shaders[currentShading], sphereModel);
		drawBuilding(shaders[currentShading], buildingModel);
		drawBuilding2(shaders[currentShading], buildingModel);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void bindLights(Shader shader, std::vector<Light>& lights)
{
	shader.setInt("lights_count", lights.size());
	for (unsigned int i = 0; i < lights.size(); ++i)
		lights[i].bind(shader, i);
}

void setFog(Shader shader)
{
	shader.setFloat("fog_maxdist", FOG_MAXDIST);
	shader.setFloat("fog_mindist", FOG_MINDIST);
}

void setConstants(Shader shader)
{
	shader.setFloat("Ka", 0.15f);
	shader.setFloat("Kd", 0.8f);
	shader.setFloat("Ks", 0.5f);
	shader.setFloat("m", 100.0f);
}

void moveCameras(std::vector<Camera>& cameras, const glm::vec3& carPosition)
{
	cameras[STATIC_FOLLOWING_CAMERA].Target = carPosition;
	float cameraShift = 0.3f;
	cameras[MOVING_CAMERA].Position = getCircularPosition(R, 0, -cameraShift) + glm::vec3(0.0f, 0.3f, 0.0f);
	cameras[MOVING_CAMERA].Target = getCircularPosition(R, 0, cameraShift) + glm::vec3(0.0f, 0.1f, 0.0f);
}

void setProjectionMatrix(Shader shader)
{
	glm::mat4 projection = glm::perspective(FIELD_OF_VIEW, ASPECT_RATIO, NEAR, FAR);
	shader.setMat4("projection", projection);
}

void setViewMatrix(Camera camera, Shader shader)
{
	glm::mat4 view = camera.GetViewMatrix();
	shader.setMat4("view", view);
}

void moveLights(std::vector<Light>& lights, glm::vec3& carPosition)
{
	// sun
	double dayNightCycleSpeed = 0.2; // higher values mean faster day-night cycle
	lights[SUN_LIGHT].Color = glm::vec3(min(pow(cos(glfwGetTime() * dayNightCycleSpeed), 2), 1.0));

	// headlights
	float lightCameraShift = 0.02f;
	float lightsDist = 0.04f;
	lights[HEADLIGHTS1_LIGHT].Position = getCircularPosition(R, -lightsDist, lightCameraShift) + glm::vec3(0.0f, 0.05f, 0.0f);
	lights[HEADLIGHTS1_LIGHT].Direction = getCircularPosition(R, -lightsDist) - lights[HEADLIGHTS1_LIGHT].Position + glm::vec3(0.0f, 0.05f, 0.0f);

	lights[HEADLIGHTS2_LIGHT].Position = getCircularPosition(R, lightsDist, lightCameraShift) + glm::vec3(0.0f, 0.05f, 0.0f);
	lights[HEADLIGHTS2_LIGHT].Direction = getCircularPosition(R, lightsDist) - lights[HEADLIGHTS2_LIGHT].Position + glm::vec3(0.0f, 0.05f, 0.0f);

	// beacon
	double beaconSpeed = 5; // higher values indicate faster rotation of the beacon light
	lights[BEACON1_LIGHT].Position = carPosition + glm::vec3(0.0f, 0.2f, 0.0f);	
	lights[BEACON1_LIGHT].Direction = carPosition - getCircularPosition(R, 0, 0, beaconSpeed);

	lights[BEACON2_LIGHT].Position = lights[BEACON1_LIGHT].Position;
	lights[BEACON2_LIGHT].Direction = -lights[BEACON1_LIGHT].Direction;
}

glm::vec3 getCircularPosition(float radius, float radiusShift, float angleShift, float angleMultiplier)
{
	return glm::vec3((radius + radiusShift) * cos(glfwGetTime() * angleMultiplier + angleShift), 0.0, (radius + radiusShift) * sin(glfwGetTime() * angleMultiplier + angleShift));
}

void drawCar(Shader shader, Model& carModel, glm::vec3& carPosition)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, carPosition);
	model = glm::rotate(model, (glm::mediump_float)(-glfwGetTime() + PI / 2), glm::vec3(0.0, 1.0, 0.0));
	model = glm::scale(model, glm::vec3(0.3f));

	// vibrations
	float rx = rand() % 10;
	float ry = rand() % 10;
	float rz = rand() % 10;
	float vibrationStrength = 1.0 / 1500; // lower number means weaker vibrations
	model = glm::translate(model, glm::vec3(rx * vibrationStrength, ry * vibrationStrength, rz * vibrationStrength));

	shader.setMat4("model", model);

	shader.setBool("use_color", true);
	shader.setVec3("color", glm::vec3(0.05, 0.05, 0.05));

	carModel.Draw(shader);
}

void drawGround(Shader shader, Model& groundModel)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(2.0f));
	shader.setMat4("model", model);

	shader.setBool("use_color", true);
	shader.setVec3("color", glm::vec3(0.1, 0.3, 0.1));

	groundModel.Draw(shader);
}

void drawSphere(Shader shader, Model& sphereModel)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.2f));
	shader.setMat4("model", model);

	shader.setBool("use_color", true);
	shader.setVec3("color", glm::vec3(0.1, 0.1, 0.6));

	sphereModel.Draw(shader);
}

void drawBuilding(Shader shader, Model& buildingModel)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-0.7f, 0.0f, 0.7f));
	model = glm::scale(model, glm::vec3(0.04f));
	model = glm::rotate(model, (glm::mediump_float)(PI / 2), glm::vec3(0.0, 1.0, 0.0));
	shader.setMat4("model", model);

	shader.setBool("use_color", true);
	shader.setVec3("color", glm::vec3(0.2, 0.2, 0.2));

	buildingModel.Draw(shader);
}

void drawBuilding2(Shader shader, Model& buildingModel)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-0.7f, 0.0f, -0.7f));
	model = glm::scale(model, glm::vec3(0.04f));
	model = glm::rotate(model, (glm::mediump_float)(2 * PI), glm::vec3(0.0, 1.0, 0.0));
	shader.setMat4("model", model);

	shader.setBool("use_color", true);
	shader.setVec3("color", glm::vec3(0.2, 0.2, 0.2));

	buildingModel.Draw(shader);
}

GLFWwindow* setupWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GK_proj4", NULL, NULL);
	if (window == NULL)
	{
		glfwTerminate();
		return NULL;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		return NULL;

	glEnable(GL_DEPTH_TEST);

	return window;
}

void setDebugWindowTitle(GLFWwindow* window)
{
	string name = "GK_Proj4 ";
	string shading;

	switch (currentShading)
	{
	case FLAT_SHADING:
		shading = "flat";
		break;
	case GOURAUD_SHADING:
		shading = "Gouraud";
		break;
	case PHONG_SHADING:
		shading = "Phong";
		break;
	}

	string specular;
	switch (currentSpecular)
	{
	case BLINN_SPECULAR:
		specular = "Blinn";
		break;
	case PHONG_SPECULAR:
		specular = "Phong";
		break;
	}

	string finalName = name + "; shading: " + shading + "; specular model: " + specular;

	glfwSetWindowTitle(window, finalName.c_str());
}

void processInput(GLFWwindow* window)
{
	// exiting app
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// changing cameras
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		currentCameraID = STATIC_CAMERA;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		currentCameraID = STATIC_FOLLOWING_CAMERA;
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		currentCameraID = MOVING_CAMERA;

	// changing shading
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		currentShading = FLAT_SHADING;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		currentShading = GOURAUD_SHADING;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		currentShading = PHONG_SHADING;

	// changing specular model
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		currentSpecular = BLINN_SPECULAR;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		currentSpecular = PHONG_SPECULAR;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	currentWidth = width;
	currentHeight = height;
	glViewport(0, 0, width, height);
}

std::vector<Camera> setupCameras()
{
	std::vector<Camera> cameras = std::vector<Camera>();
	Camera staticCamera(glm::vec3(0.9f, 0.9f, 0.9f));
	Camera staticFollowingCamera(glm::vec3(0.0f, 0.5f, 0.0f));
	Camera movingCamera(glm::vec3(2.0f, 2.0f, 2.0f));

	cameras.push_back(staticCamera);
	cameras.push_back(staticFollowingCamera);
	cameras.push_back(movingCamera);

	return cameras;
}

std::vector<Shader> setupShaders()
{
	Shader ConstShader("ConstShader.vert", "ConstShader.frag");
	Shader GouraudShader("GouraudShader.vert", "GouraudShader.frag");
	Shader PhongShader("PhongShader.vert", "PhongShader.frag");

	std::vector<Shader> shaders;
	shaders.push_back(ConstShader);
	shaders.push_back(GouraudShader);
	shaders.push_back(PhongShader);
	
	return shaders;
}

std::vector<Light> setupLights()
{
	std::vector<Light> lights = std::vector<Light>();

	Light pl = Light();
	pl.makePoint(glm::vec3(1.0, 1.0, -1.0));
	pl.Color = glm::vec3(0.4f);
	lights.push_back(pl);

	Light sl = Light();
	sl.makeDirectional(glm::vec3(1.0, 2.0, 1.0));
	lights.push_back(sl);

	Light hl1 = Light();
	hl1.makeSpotlight(glm::vec3(1.0), glm::vec3(1.0));
	lights.push_back(hl1);
	Light hl2 = Light();
	hl2.makeSpotlight(glm::vec3(1.0), glm::vec3(1.0));
	lights.push_back(hl2);

	Light bl1 = Light();
	bl1.makeSpotlight(glm::vec3(1.0), glm::vec3(0.0));
	bl1.Color = glm::vec3(1.0f, 0.15f, 0.15f);
	bl1.setInnerAngle(40.0f);
	bl1.setOuterAngle(45.0f);
	lights.push_back(bl1);

	Light bl2 = Light();
	bl2.makeSpotlight(glm::vec3(1.0), glm::vec3(0.0));
	bl2.Color = glm::vec3(0.15f, 0.15f, 1.0f);
	bl2.setInnerAngle(40.0f);
	bl2.setOuterAngle(45.0f);
	lights.push_back(bl2);


	return lights;
}



