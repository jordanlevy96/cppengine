#pragma once

#include "util/globals.h"

#include <string>

class Component
{
public:
    Component(){};
    virtual ~Component(){};
    virtual const std::string GetName() = 0;
};