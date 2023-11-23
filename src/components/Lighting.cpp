#include "components/Lighting.h"

#include <iostream>

void Lighting::SetUniforms(std::shared_ptr<Transform> transform)
{
    RenderComponent::AddUniform("lightPos", lightTrans->Pos, UniformTypeMap::vec3);
    RenderComponent::AddUniform("lightColor", lightTrans->Color, UniformTypeMap::vec3);
    RenderComponent::AddUniform("objectColor", *objectColor, UniformTypeMap::vec3);

    RenderComponent::SetUniforms(transform);
}