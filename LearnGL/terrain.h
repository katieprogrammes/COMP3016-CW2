#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "FastNoiseLite.h"

struct Terrain {
    unsigned int VAO;
    unsigned int indexCount;
};
extern const int TERRAIN_SIZE;

Terrain CreateTerrain();

float GetTerrainHeight(float x, float z);
