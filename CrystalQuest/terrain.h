#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "FastNoiseLite.h"

struct TerrainVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
};

struct Terrain {
    unsigned int VAO;
    unsigned int indexCount;
};
extern const int TERRAIN_SIZE;
extern const float TERRAIN_SCALE;

Terrain CreateTerrain();

float GetTerrainHeight(float x, float z);

const std::vector<TerrainVertex>& GetTerrainVertices();
