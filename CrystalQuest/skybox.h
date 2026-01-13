#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/shader_m.h>


class Skybox
{
public:
    Skybox(const std::vector<std::string>& faces);
    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    GLuint cubemapID;
    GLuint VAO, VBO;

    Shader shader;
    GLuint loadCubemap(const std::vector<std::string>& faces);
};
