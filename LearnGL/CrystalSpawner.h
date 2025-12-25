#pragma once
#include <glm/glm.hpp>
#include <vector>

struct CrystalInstance {
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
    int meshIndex;
};

class CrystalSpawner {
public:
    std::vector<CrystalInstance> crystals;

    void SpawnCrystals(
        int count,
        float terrainSize,
        float (*getHeight)(float, float),
        int meshCount
    );
};
