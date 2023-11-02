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
	GameObject3D(const char *shaderSrc, const char *modelSrc);
	~GameObject3D(){};
	void Rotate(float degrees, EulerAngles dir);
};
