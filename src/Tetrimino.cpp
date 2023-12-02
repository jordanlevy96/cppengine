#include "Tetrimino.h"
#include "controllers/Registry.h"

#include <yaml-cpp/yaml.h>
#include "util/debug.h"

#include <iostream>

static Registry *registry = &Registry::GetInstance();

static float spacingX = 2.0f;
static float spacingY = 2.0f;
std::shared_ptr<RenderComponent> Tetrimino::cubeComp = nullptr;

unsigned int Tetrimino::RegisterTetrimino(glm::mat4 matrix)
{
    unsigned int id = registry->RegisterEntity();
    std::shared_ptr<Transform> parentTransform = std::make_shared<Transform>();
    std::shared_ptr<CompositeEntity> wrapper = std::make_shared<CompositeEntity>();
    registry->RegisterComponent(id, parentTransform, ComponentTypes::TransformType);
    registry->RegisterComponent(id, wrapper, ComponentTypes::CompositeType);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (matrix[i][j] == 1)
            {
                unsigned int cube = registry->RegisterEntity();
                std::shared_ptr<Transform> t = std::make_shared<Transform>();
                t->Pos.x = j * spacingX;
                t->Pos.y = i * spacingY;
                registry->RegisterComponent(cube, t, ComponentTypes::TransformType);

                std::shared_ptr<Transform> light = registry->GetComponent<Transform>(registry->GetEntityByName("light"));
                std::shared_ptr<Lighting> lightComp = std::make_shared<Lighting>(Tetrimino::cubeComp, &t->Color, light);
                registry->RegisterComponent(cube, lightComp, ComponentTypes::LightingType);

                wrapper->AddChild(cube);
            }
        }
    }
    return id;
}

std::unordered_map<Tetrimino::TetriminoShape, glm::mat4> Tetrimino::LoadTetriminos(const std::string &src)
{
    std::unordered_map<Tetrimino::TetriminoShape, glm::mat4> tetriminos;

    try
    {
        YAML::Node tetriminosNode = YAML::LoadFile(src);

        for (const auto &tetriminoNode : tetriminosNode["tetriminos"])
        {
            std::string key = tetriminoNode.first.as<std::string>();
            glm::mat4 tetrimino(0.0f);

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    tetrimino[i][j] = tetriminoNode.second[i][j].as<int>();
                }
            }
            tetriminos[TetriminoMap[key]] = tetrimino;
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "Error loading Tetriminos: " << e.what() << std::endl;
    }

    return tetriminos;
}
