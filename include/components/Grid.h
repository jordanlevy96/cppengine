#pragma once

#include "components/Component.h"

struct Grid : public Component
{
    std::vector<std::vector<bool>> cells;
    Grid(int width, int height) : cells(height, std::vector<bool>(width, false)) {}
};