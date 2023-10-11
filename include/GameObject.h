#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

class GameObject
{
public:
    // Constructor
    GameObject(float x, float y, float width, float height);

    // Destructor
    ~GameObject();

    // Update the game object's state
    virtual void Update(float deltaTime);

    // Render the game object
    virtual void Render();

protected:
    // Position and size of the game object
    float x_;
    float y_;
    float width_;
    float height_;
};

#endif