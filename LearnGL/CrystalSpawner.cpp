#include "CrystalSpawner.h"
#include <cstdlib>

void CrystalSpawner::SpawnCrystals(
    int count,
    float terrainSize,
    float (*getHeight)(float, float),
    int meshCount
) {
    crystals.clear();
    crystals.reserve(count);

    for (int i = 0; i < count; i++) {
        float x = (float)(rand() % (int)terrainSize);
        float z = (float)(rand() % (int)terrainSize);
        float y = getHeight(x, z);

        CrystalInstance c;
        c.position = glm::vec3(x, y, z);

        float s = 0.5f + (rand() % 100) / 200.0f; // 0.5–1.0
        c.scale = glm::vec3(s);

        c.rotation = (float)(rand() % 360);

        c.meshIndex = rand() % meshCount;

        crystals.push_back(c);
    }
}
