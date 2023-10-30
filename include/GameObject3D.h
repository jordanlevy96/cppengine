#pragma once
#include <GameObject.h>

enum EulerAngles
{
	PITCH,
	YAW,
	ROLL
};

class GameObject3D : public GameObject
{
public:
	GameObject3D(float x, float y, float width, float height, float *vertices, int numVertices, unsigned int *indices, int numIndices, char *shaderSrc);
	~GameObject3D(){};
	void Rotate(float degrees, EulerAngles dir);
};
