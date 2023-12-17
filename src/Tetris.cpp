#include "Tetris.h"

#include "util/SceneTraversal.h"
#include "util/TransformUtils.h"

#include <yaml-cpp/yaml.h>

#include <iostream>

static float spacingX = 1.0f;
static float spacingY = 1.0f;
std::unordered_map<char, Tetris::Tetrimino> Tetris::TetriminoMap;

EntityID Tetris::RegisterTetrimino(RenderComponent rc, Tetrimino &tetriminoData)
{
    unsigned int id = registry->RegisterEntity();
    Transform parentTransform = registry->GetComponent<Transform>(id);

    // initialize to all -1
    glm::mat4 cubeIDMap = glm::mat4(-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (tetriminoData.shape[i][j] == 1)
            {
                EntityID cube = registry->RegisterEntity();
                cubeIDMap[i][j] = cube;
                Transform &t = registry->GetComponent<Transform>(cube);
                t.Pos.x = j * spacingX;
                t.Pos.y = i * spacingY;
                t.Color = tetriminoData.color;

                registry->RegisterComponent<RenderComponent>(cube, rc);

                EntityID lightID = registry->GetEntityByName("light");
                Lighting lightComp = Lighting(lightID);
                registry->RegisterComponent<Lighting>(cube, lightComp);

                AddChild(id, cube);
            }
        }
    }
    ActiveTetriminoChildMap = cubeIDMap;
    Tween tween = Tween(&TransformUtils::move_to, parentTransform.Pos, parentTransform.Pos + glm::vec3(0, -2, 0), 1000, TransitionType::TRANS_LINEAR);
    registry->RegisterComponent<Tween>(id, tween);

    return id;
}

glm::mat4 Tetris::GetTetriminoChildMap()
{
    return ActiveTetriminoChildMap;
}

EntityID Tetris::CreateTetrimino(RenderComponent rc, const std::string &in)
{
    char key = in[0];
    Tetrimino tetrimino = TetriminoMap[key];
    EntityID id = Tetris::RegisterTetrimino(rc, tetrimino);
    return id;
}

void Tetris::LoadTetriminos(const std::string &src)
{
    try
    {
        YAML::Node root = YAML::LoadFile(src);

        for (const auto &tetriminoNode : root["tetriminos"])
        {
            Tetrimino tetrimino;
            char key = tetriminoNode.first.as<char>();

            YAML::Node colorNode = tetriminoNode.second["color"];
            tetrimino.color.r = colorNode["r"].as<float>();
            tetrimino.color.g = colorNode["g"].as<float>();
            tetrimino.color.b = colorNode["b"].as<float>();

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    tetrimino.shape[i][j] = tetriminoNode.second["shape"][i][j].as<int>();
                }
            }

            TetriminoMap[key] = tetrimino;
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "Error loading Tetriminos: " << e.what() << std::endl;
    }
}

void Tetris::RotateTetrimino(EntityID id, Rotations rotation)
{
    float angle = RotationMap[rotation];
    glm::mat4 &orientation = ActiveTetriminoChildMap;
    orientation = glm::rotate(orientation, glm::radians(angle), EulerAngles::Pitch);
    TransformUtils::rotate(id, angle, EulerAngles::Pitch);
}

void Tetris::MoveTetrimino(EntityID id, glm::vec2 dir)
{
    TransformUtils::translate(id, glm::vec3(dir, 0));
}

void Tetris::TweenTetrimino(EntityID id, glm::vec3 dir, float duration)
{
    Transform transform = registry->GetComponent<Transform>(id);
    Tween tween = registry->GetComponent<Tween>(id);
    tween.elapsed = 0;
    tween.Start = transform.Pos;
    tween.End = transform.Pos + dir;
    tween.Duration = duration;

    tween.isActive = true;
}