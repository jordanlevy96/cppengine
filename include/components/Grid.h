#pragma once

#include <vector>

struct Grid
{
    std::vector<std::vector<bool>> cells;
    Grid(int width, int height) : cells(height, std::vector<bool>(width, false)) {}
};