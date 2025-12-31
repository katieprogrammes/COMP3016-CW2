#include "terrain.h"
#include <glad/glad.h>

const int TERRAIN_SIZE = 200;
const float TERRAIN_SCALE = 1.0f;
const float HEIGHT_SCALE = 10.0f;



static int idx(int x, int z) {
    return z * TERRAIN_SIZE + x;
}
static std::vector<TerrainVertex> vertices;


Terrain CreateTerrain() {

    
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(4);
    noise.SetFrequency(0.01f);

   // vertices
    vertices.resize(TERRAIN_SIZE * TERRAIN_SIZE);

    for (int z = 0; z < TERRAIN_SIZE; z++) {
        for (int x = 0; x < TERRAIN_SIZE; x++) {

            float h = noise.GetNoise((float)x, (float)z) * HEIGHT_SCALE;

            TerrainVertex v;
            v.pos = glm::vec3(x * TERRAIN_SCALE, h - 15.0f, z * TERRAIN_SCALE);


            float c = glm::clamp(h / HEIGHT_SCALE, 0.0f, 1.0f);
            v.color = glm::vec3(0.3f + c * 0.4f, 0.3f + c * 0.3f, 0.3f);

            float tile = 20.0f;
            v.uv = glm::vec2( (float)x / tile, (float)z / tile );
            vertices[idx(x, z)] = v;

        }

    }

    // normals
    for (int z = 1; z < TERRAIN_SIZE - 1; z++) {
        for (int x = 1; x < TERRAIN_SIZE - 1; x++) {

            glm::vec3 left = vertices[idx(x - 1, z)].pos;
            glm::vec3 right = vertices[idx(x + 1, z)].pos;
            glm::vec3 down = vertices[idx(x, z - 1)].pos;
            glm::vec3 up = vertices[idx(x, z + 1)].pos;

            glm::vec3 dx = right - left;
            glm::vec3 dz = up - down;

            vertices[idx(x, z)].normal = glm::normalize(glm::cross(dz, dx));
        }
    }

    // indices
    std::vector<unsigned int> indices;

    for (int z = 0; z < TERRAIN_SIZE - 1; z++) {
        for (int x = 0; x < TERRAIN_SIZE - 1; x++) {

            unsigned int i0 = idx(x, z);
            unsigned int i1 = idx(x + 1, z);
            unsigned int i2 = idx(x, z + 1);
            unsigned int i3 = idx(x + 1, z + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    
    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TerrainVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, normal));
    glEnableVertexAttribArray(1);

    // color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, color));
    glEnableVertexAttribArray(3);


    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, uv)); 
    glEnableVertexAttribArray(2);


    glBindVertexArray(0);

    return { VAO, (unsigned int)indices.size() };
}
float GetTerrainHeight(float x, float z)
{
    int ix = (int)x;
    int iz = (int)z;

    if (ix < 0 || iz < 0 || ix >= TERRAIN_SIZE || iz >= TERRAIN_SIZE)
        return 0;

    return vertices[idx(ix, iz)].pos.y;
}
const std::vector<TerrainVertex>& GetTerrainVertices() {
    return vertices;
}

