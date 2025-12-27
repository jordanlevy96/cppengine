#pragma once

#include "controllers/Registry.h"

// Scene hierarchy utility functions
void AddChild(EntityID parent, EntityID child);
EntityID GetParent(EntityID child);
