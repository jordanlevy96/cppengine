#include "Tetrimino.h"
#include "controllers/Registry.h"

#include <yaml-cpp/yaml.h>
#include "util/debug.h"

#include <iostream>

static Registry *registry = &Registry::GetInstance();

float spacingX = 2.0f;
float spacingY = 2.0f;

Tetrimino::Tetrimino(glm::mat4 matrix, std::shared_ptr<RenderComponent> rc)
{
    int cubeIndex = 0;
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

                std::shared_ptr<Transform> light = registry->TransformComponents[registry->GetEntityByName("light")];
                std::shared_ptr<Lighting> lightComp = std::make_shared<Lighting>(rc, &t->Color, light);
                registry->RegisterComponent(cube, lightComp, ComponentTypes::LightingType);

                cubes[cubeIndex++] = cube;
            }
        }
    }
}

static TetriminoShape MapStrToShape(std::string key)
{
    TetriminoShape shape;

    if (key == "I")
    {
        shape = TetriminoShape::I;
    }
    else if (key == "O")
    {
        shape = TetriminoShape::O;
    }
    else if (key == "T")
    {
        shape = TetriminoShape::T;
    }
    else if (key == "J")
    {
        shape = TetriminoShape::J;
    }
    else if (key == "L")
    {
        shape = TetriminoShape::L;
    }
    else if (key == "S")
    {
        shape = TetriminoShape::S;
    }
    else if (key == "Z")
    {
        shape = TetriminoShape::Z;
    }
    else
    {
        std::cerr << "Bad Tetrimino!" << std::endl;
    }

    return shape;
}

std::unordered_map<TetriminoShape, glm::mat4> Tetrimino::LoadTetriminos(const std::string &src)
{
    std::unordered_map<TetriminoShape, glm::mat4> tetriminos;

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
            tetriminos[MapStrToShape(key)] = tetrimino;
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "Error loading Tetriminos: " << e.what() << std::endl;
    }

    return tetriminos;
}