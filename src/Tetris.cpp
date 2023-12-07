#include "Tetris.h"

#include "controllers/Registry.h"
#include "util/TransformUtils.h"

#include <yaml-cpp/yaml.h>

#include <iostream>

static Registry *registry = &Registry::GetInstance();

static float spacingX = 2.0f;
static float spacingY = 2.0f;
std::unordered_map<char, Tetris::Tetrimino> Tetris::TetriminoMap;

unsigned int Tetris::RegisterTetrimino(std::shared_ptr<RenderComponent> rc, Tetrimino tetriminoData)
{
    unsigned int id = registry->RegisterEntity();
    std::shared_ptr<Transform> parentTransform = registry->GetComponent<Transform>(id);
    std::shared_ptr<TetriminoComponent> wrapper = std::make_shared<TetriminoComponent>();
    registry->RegisterComponent<CompositeEntity>(id, wrapper);

    glm::mat4 cubeIDMap = glm::mat4(-1.0f);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (tetriminoData.shape[i][j] == 1)
            {
                unsigned int cube = registry->RegisterEntity();
                cubeIDMap[i][j] = cube;
                std::shared_ptr<Transform> t = registry->GetComponent<Transform>(cube);
                t->Pos.x = j * spacingX;
                t->Pos.y = i * spacingY;
                t->Color = tetriminoData.color;

                std::shared_ptr<Transform> light = registry->GetComponent<Transform>(registry->GetEntityByName("light"));
                std::shared_ptr<Lighting> lightComp = std::make_shared<Lighting>(rc, &t->Color, light);
                registry->RegisterComponent<Lighting>(cube, lightComp);

                wrapper->AddChild(cube);
            }
        }
    }
    wrapper->cubeIDMap = cubeIDMap;

    return id;
}

glm::mat4 Tetris::GetChildMap(unsigned int parentID)
{
    std::shared_ptr<CompositeEntity> ptr = registry->GetComponent<CompositeEntity>(parentID);
    std::shared_ptr<TetriminoComponent> tetrimino = std::static_pointer_cast<TetriminoComponent>(ptr);
    return tetrimino->cubeIDMap;
}

unsigned int Tetris::CreateTetrimino(std::shared_ptr<RenderComponent> rc, const std::string &in)
{
    char key = in[0];
    Tetrimino tetrimino = TetriminoMap[key];
    unsigned int id = Tetris::RegisterTetrimino(rc, tetrimino);
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

void Tetris::RotateTetrimino(unsigned int id, Rotations rotation)
{
    float angle = RotationMap[rotation];

    std::shared_ptr<CompositeEntity> ptr = registry->GetComponent<CompositeEntity>(id);
    std::shared_ptr<TetriminoComponent> tetrimino = std::static_pointer_cast<TetriminoComponent>(ptr);

    glm::mat4 &orientation = tetrimino->cubeIDMap;

    orientation = glm::rotate(orientation, glm::radians(angle), EulerAngles::Pitch);

    TransformUtils::rotate(id, angle, EulerAngles::Pitch);
}

void Tetris::MoveTetrimino(unsigned int id, glm::vec2 dir)
{
    TransformUtils::translate(id, glm::vec3(dir, 0));
}

void Tetris::TweenTetrimino(unsigned int id, glm::vec3 dir, float duration)
{
    std::shared_ptr<Transform> transform = registry->GetComponent<Transform>(id);
    std::shared_ptr<Tween> tween;
    tween = registry->GetComponent<Tween>(id);
    if (!tween)
    {
        tween = std::make_shared<Tween>(&TransformUtils::move_to, transform->Pos, transform->Pos + dir, duration, TransitionType::TRANS_LINEAR);
        registry->RegisterComponent<Tween>(id, tween);
    }
    else
    {
        // reset
        tween->elapsed = 0;
        tween->Start = transform->Pos;
        tween->End = transform->Pos + dir;
        tween->Duration = duration;
    }

    tween->isActive = true;
}