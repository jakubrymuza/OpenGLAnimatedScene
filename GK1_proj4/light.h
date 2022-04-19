#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "shader.h"

#define DIRECTIONAL_LIGHT_TYPE 0
#define POINT_LIGHT_TYPE 1
#define SPOTLIGHT_TYPE 2

// one light struct for all light types, type of light determined by field 'type', fields that are incompatible with the light type might contain garbage
// valid light is to be constructed using one of the methods make***
struct Light
{
private:
	int type; 
	float InnerAngle;
	float OuterAngle;
public:
	glm::vec3 Color;
	glm::vec3 Direction;
	glm::vec3 Position;
	float Attentuation;

	void setInnerAngle(float angle)
	{
		InnerAngle = glm::cos(glm::radians(angle));
	}

	void setOuterAngle(float angle)
	{
		OuterAngle = glm::cos(glm::radians(angle));
	}

	void makeDirectional(glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f))
	{
		type = DIRECTIONAL_LIGHT_TYPE;
		Color = color;
		Direction = direction;
	}

	void makePoint(glm::vec3 position, glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f), float attentuation = 0.3f)
	{
		type = POINT_LIGHT_TYPE;
		Color = color;
		Position = position;
		Color = color;
		Attentuation = attentuation;
	}

	void makeSpotlight(glm::vec3 position, glm::vec3 direction, float innerAngle = 13.0f, float outerAngle = 16.0f, glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f), float attentuation = 0.2f)
	{
		type = SPOTLIGHT_TYPE;
		Color = color;
		Position = position;
		Color = color;
		Attentuation = attentuation;
		Direction = direction;
		setInnerAngle(innerAngle);
		setOuterAngle(outerAngle);
	}

	void bind(Shader& shader, unsigned int nr)
	{
		std::string name = "lights[" + std::to_string(nr) + "]";

		shader.setInt(name + ".type", type);
		shader.setVec3(name + ".position", Position);
		shader.setVec3(name + ".color", Color);
		shader.setFloat(name + ".attentuation", Attentuation);
		shader.setVec3(name + ".direction", Direction);
		shader.setFloat(name + ".innerAngle", InnerAngle);
		shader.setFloat(name + ".outerAngle", OuterAngle);
	}
};