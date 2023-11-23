#include "components/Lighting.h"

void Lighting::SetUniforms()
{
    RenderComponent::AddUniform("lightPos", lightTrans->Pos, UniformTypeMap::vec3);
    RenderComponent::AddUniform("lightColor", lightTrans->Color, UniformTypeMap::vec3);
    RenderComponent::AddUniform("objectColor", *objectColor, UniformTypeMap::vec3);

    RenderComponent::SetUniforms();
}