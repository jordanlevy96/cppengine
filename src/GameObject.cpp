#include <GameObject.h>
#include <iostream>
#include <GLFW/glfw3.h>

GameObject::GameObject(float x, float y, float width, float height)
    : x_(x), y_(y), width_(width), height_(height)
{
    // Constructor code, if any
    std::cout << "Init GameObject" << std::endl;
}

GameObject::~GameObject()
{
    // Destructor code, if any
}

void GameObject::Update(float deltaTime)
{
    // Update logic, if any
}

void GameObject::Render()
{
    // Set color (red in this example)
    glColor3f(1.0f, 0.0f, 0.0f);

    // Draw a red square (box)
    glBegin(GL_QUADS);
    glVertex2f(x_, y_);
    glVertex2f(x_ + width_, y_);
    glVertex2f(x_ + width_, y_ + height_);
    glVertex2f(x_, y_ + height_);
    glEnd();
}
