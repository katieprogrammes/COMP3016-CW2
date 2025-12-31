#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <learnopengl/mesh.h>

#include "terrain.h"

#include <PxPhysicsAPI.h> 
#include <cooking/PxCooking.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace physx;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);

//physX
PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxScene* gScene = nullptr;

PxDefaultAllocator gAllocator;

//crystal model structure
struct CrystalInstance {
    glm::vec3 position;
    glm::vec3 scale;
    Model* model;
    bool isReal = false; // default fake
};
std::vector<CrystalInstance> crystals;

void SpawnCrystalOnTerrain(
    Model* model,
    float x,
    float z,
    float scale = 0.005f
) {
    float y = GetTerrainHeight(x, z);
    bool real = (rand() % 5 == 0); // 20% chance real
    crystals.push_back({ glm::vec3(x,y,z), glm::vec3(scale), model, real });

}




class MyErrorCallback : public PxErrorCallback
{
public:
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        printf("PhysX Error (%d): %s at %s:%d\n", code, message, file, line);
    }
};

MyErrorCallback gErrorCallback;


class MyCpuDispatcher : public physx::PxCpuDispatcher
{
public:
    void submitTask(PxBaseTask& task) override
    {
        task.run();
        task.release();
    }

    uint32_t getWorkerCount() const override
    {
        return 1;
    }
};

MyCpuDispatcher gDispatcher;

PxFilterFlags MyFilterShader(
    PxFilterObjectAttributes a0, PxFilterData d0,
    PxFilterObjectAttributes a1, PxFilterData d1,
    PxPairFlags& pairFlags, const void*, PxU32)
{
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    return PxFilterFlag::eDEFAULT;
}


void InitPhysX()
{
    PxTolerancesScale scale;
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, scale);

    PxSceneDesc sceneDesc(scale);
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = &gDispatcher;
    sceneDesc.filterShader = MyFilterShader;

    gScene = gPhysics->createScene(sceneDesc);
}


PxRigidStatic* CreatePhysXHeightfield()
{
    const auto& verts = GetTerrainVertices();

    PxHeightFieldDesc hfDesc;
    hfDesc.nbRows = TERRAIN_SIZE;
    hfDesc.nbColumns = TERRAIN_SIZE;
    hfDesc.format = PxHeightFieldFormat::eS16_TM;

    std::vector<PxHeightFieldSample> samples;
    samples.resize(TERRAIN_SIZE * TERRAIN_SIZE);

    // Simple row/col mapping that matches terrain vertices (x = column, z = row)
    for (int z = 0; z < TERRAIN_SIZE; z++)
    {
        for (int x = 0; x < TERRAIN_SIZE; x++)
        {
            int terrainIndex = z * TERRAIN_SIZE + x;
            int physxIndex = z * TERRAIN_SIZE + x;

            float h = verts[terrainIndex].pos.y;

            PxHeightFieldSample& s = samples[physxIndex];
            s.height = (PxI16)h;
            s.materialIndex0 = 0;
            s.materialIndex1 = 0;
        }
    }

    hfDesc.samples.data = samples.data();
    hfDesc.samples.stride = sizeof(PxHeightFieldSample);

    PxHeightField* heightField = PxCreateHeightField(hfDesc);
    if (!heightField) {
        std::cout << "Failed to create PhysX heightfield" << std::endl;
        return nullptr;
    }

    PxHeightFieldGeometry hfGeom(
        heightField,
        PxMeshGeometryFlags(),
        1.0f,          // height scale
        TERRAIN_SCALE, // row scale (X)
        TERRAIN_SCALE  // column scale (Z)
    );

    PxMaterial* mat = gPhysics->createMaterial(0.5f, 0.5f, 0.5f);

    PxTransform pose(PxVec3(
        0.5f * TERRAIN_SCALE,
        0.0f,
        0.5f * TERRAIN_SCALE
    ));

    PxRigidStatic* terrainActor = gPhysics->createRigidStatic(pose);
    PxShape* shape = gPhysics->createShape(hfGeom, *mat);
    terrainActor->attachShape(*shape);

    gScene->addActor(*terrainActor);

    return terrainActor;
}


float GetPhysicsTerrainHeight(float x, float z) {
    PxVec3 origin(x, 500.0f, z);
    PxRaycastBuffer hit;
    bool ok = gScene->raycast(PxVec3(0, 500, 0), PxVec3(0, -1, 0), 500, hit);
    std::cout << ok << " " << hit.block.position.y << std::endl;



    if (gScene->raycast(origin, PxVec3(0, -1, 0), 500.0f, hit))
        return hit.block.position.y;

    return 0.0f;
}

// window settings
const unsigned int SCR_WIDTH = 1080;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(100.0f, 50.0f, 100.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


int main()
{

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    //terrain
    unsigned int terrainTexture = loadTexture("media/rockytext.jpg");
    Terrain terrain = CreateTerrain();


    //physx
    InitPhysX();
    CreatePhysXHeightfield();

    // build and compile shader programs
    Shader lightingShader("textures/colors.vs", "textures/colors.fs");
    Shader lightCubeShader("shaders/light_cube.vs", "shaders/light_cube.fs");
    Shader crystalShader("shaders/crystal.vs", "shaders/crystal.fs");


    //load models
    Model blueCrystal("media/gems/blue.gltf"); 
    Model redCrystal("media/gems/red.gltf"); 
    Model greenCrystal("media/gems/green.gltf");
    Model orangeCrystal("media/gems/orange.gltf");
    Model purpleCrystal("media/gems/purple.gltf");
    Model yellowCrystal("media/gems/yellow.gltf");

    std::vector<Model*> crystalModels = { 
        &redCrystal, 
        &blueCrystal, 
        &greenCrystal, 
        &orangeCrystal, 
        &purpleCrystal, 
        &yellowCrystal };

    for (int i = 0; i < 50; i++)
    {
        float x = rand() % TERRAIN_SIZE;
        float z = rand() % TERRAIN_SIZE;

        Model* model = crystalModels[rand() % crystalModels.size()];

        SpawnCrystalOnTerrain(model, x, z);
    }

    

    //lightcube
    unsigned int lightCubeVAO, VBO;
    float cubeVerts[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,0.5f,-0.5f,
         0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,0.5f,   0.5f,-0.5f,0.5f,   0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,  -0.5f,0.5f,0.5f,  -0.5f,-0.5f,0.5f
    };

    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glm::vec3 pointLightPosition(0.0f, 20.0f, 0.0f);

    // shader configuration
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    crystalShader.use();
    crystalShader.setInt("texture_diffuse1", 0);
    crystalShader.setInt("texture_normal1", 1);
    crystalShader.setInt("texture_emissive1", 2);







    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // Step PhysX scene (for future dynamic actors)
        if (gScene && deltaTime > 0.0f) {
            gScene->simulate(deltaTime);
            gScene->fetchResults(true);
        }

        // CAMERA FOLLOWS TERRAIN (CPU height, from terrain.cpp)
        float terrainY = GetTerrainHeight(camera.Position.x, camera.Position.z);
        camera.Position.y = glm::mix(camera.Position.y, terrainY + 2.0f, 10.0f * deltaTime);

        // render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        // directional light
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        // point light 1
        lightingShader.setVec3("pointLights[0].position", pointLightPosition);
        lightingShader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.09f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.032f);

        // spotLight
        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        
        
        //draw crystal
        crystalShader.use();
        crystalShader.setMat4("projection", projection);
        crystalShader.setMat4("view", view);
        crystalShader.setFloat("time", currentFrame);

        for (auto& c : crystals)
        {
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::translate(modelMat, c.position);
            modelMat = glm::scale(modelMat, c.scale);

            crystalShader.setMat4("model", modelMat);
            crystalShader.setFloat("sparkleStrength", c.isReal ? 1.0f : 0.0f);

            c.model->Draw(crystalShader);
        }

        // world transformation
        lightingShader.use();

        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        glDisableVertexAttribArray(3); 
        glDisableVertexAttribArray(4); 
        glDisableVertexAttribArray(5); 
        glDisableVertexAttribArray(6);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, terrainTexture); 
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, terrainTexture);

        glBindVertexArray(terrain.VAO);
        glDrawElements(GL_TRIANGLES, terrain.indexCount, GL_UNSIGNED_INT, 0);

        // also draw the lamp object
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        
        glBindVertexArray(lightCubeVAO);
        model = glm::translate(glm::mat4(1.0f), pointLightPosition);
        model = glm::scale(model, glm::vec3(0.2f));
        lightCubeShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.    
    glfwTerminate();
    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = camera.MovementSpeed * deltaTime;

    glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 right = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.Position += forward * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.Position -= forward * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.Position -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.Position += right * speed;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;

    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, STBI_rgb);

    if (data)
    {
        GLenum format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

