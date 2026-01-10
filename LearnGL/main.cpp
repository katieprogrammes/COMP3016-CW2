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

#include <unordered_map>
#include <cstdlib>
#include <ctime>

#include "stb_easy_font.h"
#include "skybox.h"

#include <sound/irrKlang.h>


using namespace irrklang;
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
std::unordered_map<Model*, std::string> modelNames;


//crystal model structure
struct CrystalInstance {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    Model* model;
    float radius;
    bool isReal = false; // default fake
    bool found = false; 
    std::string type;

    void onClick()
    {
        
    }
};
std::vector<CrystalInstance> crystals;

void SpawnCrystalOnTerrain(Model* model, float x, float z) 
{
    float y = GetTerrainHeight(x, z);
    bool real = (rand() % 5 == 0); // 20% chance real

    //Normalise height
    float desiredRadius = 1.5f;
    float scaleFactor = desiredRadius / model->boundingRadius;
    glm::vec3 scale(scaleFactor);

    float radius = model->GetWorldRadius(glm::vec3(scaleFactor));


    crystals.push_back({ 
        glm::vec3(x,y,z), 
        scale,
        glm::vec3(0.0f),
        model,
        radius,
        real,
        false,
        modelNames[model]
    });

}

//Quest List
std::vector<std::string> questList;
void GenerateQuestList(int count)
{
    questList.clear();

    // pick from the modelNames map
    std::vector<std::string> allTypes;
    for (auto& pair : modelNames)
        allTypes.push_back(pair.second);

    std::random_shuffle(allTypes.begin(), allTypes.end());

    for (int i = 0; i < count; i++)
        questList.push_back(allTypes[i]);

    std::cout << "Quest List:\n";
    for (auto& t : questList)
        std::cout << " - " << t << "\n";
}
//showing the quest list
GLuint textVAO = 0;
GLuint textVBO = 0;

//Quest completion
bool questJustCompleted = false;
float questCompleteTimer = 0.0f;


void DrawText(Shader& shader, float x, float y, const char* text)
{
    static char buffer[99999]; // stb_easy_font output buffer

    // Generate quads from text (each quad = 4 vertices, each vertex = 4 floats)
    int num_quads = stb_easy_font_print(
        x, y,
        (char*)text,
        NULL,
        buffer,
        sizeof(buffer)
    );

    if (num_quads <= 0)
        return;

    // Convert quads (4 verts) -> triangles (6 verts) for GL_TRIANGLES
    float* src = (float*)buffer; // x,y,z,w but we only use x,y
    std::vector<float> vertices;
    vertices.reserve(num_quads * 6 * 2); // 6 vertices * 2 floats (x,y)

    for (int i = 0; i < num_quads; ++i)
    {
        float x0 = src[i * 16 + 0];
        float y0 = src[i * 16 + 1];

        float x1 = src[i * 16 + 4];
        float y1 = src[i * 16 + 5];

        float x2 = src[i * 16 + 8];
        float y2 = src[i * 16 + 9];

        float x3 = src[i * 16 + 12];
        float y3 = src[i * 16 + 13];

        // Two triangles: (0,1,2) and (0,2,3)

        // Triangle 1
        vertices.push_back(x0); vertices.push_back(y0);
        vertices.push_back(x1); vertices.push_back(y1);
        vertices.push_back(x2); vertices.push_back(y2);

        // Triangle 2
        vertices.push_back(x0); vertices.push_back(y0);
        vertices.push_back(x2); vertices.push_back(y2);
        vertices.push_back(x3); vertices.push_back(y3);
    }

    if (textVAO == 0)
    {
        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);
    }

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);

    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,              // layout(location = 0)
        2,              // vec2
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 2));

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
float GetTextWidth(const char* text)
{
    // stb_easy_font prints each character as a 4‑vertex quad
    // Each quad is 8 pixels wide by default
    int len = strlen(text);
    return len * 8.0f;
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

//logic to make the crystals clickable

bool mouseClicked = false;

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mouseClicked = true;
    }
}

int main()
{
    //randomising crystals
    srand(static_cast<unsigned>(time(nullptr)));

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
    glfwSetMouseButtonCallback(window, mouse_click_callback);
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
    Shader crystalShader("shaders/crystal.vs", "shaders/crystal.fs");
    Shader uiShader("shaders/ui.vs", "shaders/ui.fs");
    Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.fs");


    //skybox textures
    std::vector<std::string> faces = {
    "media/skybox/stars1.png",
    "media/skybox/stars1.png",
    "media/skybox/stars1.png",
    "media/skybox/stars1.png",
    "media/skybox/stars1.png",
    "media/skybox/stars1.png"
    };


    Skybox skybox(faces);

    //randomise crystals for each launch
    crystals.clear();

    //load models
    Model blueCrystal("media/gems/blue.gltf");
    Model redCrystal("media/gems/red.gltf"); 
    Model greenCrystal("media/gems/green.gltf");
    Model orangeCrystal("media/gems/orange.gltf");
    Model purpleCrystal("media/gems/purple.gltf");
    Model yellowCrystal("media/gems/yellow.gltf");
    Model lgPurpCrystal("media/gems/lgPurp.gltf");
    Model lgBlueCrystal("media/gems/lgBlue.gltf");
    Model lgRedCrystal("media/gems/lgRed.gltf");
    Model lgOrangeCrystal("media/gems/lgOrange.gltf");
    Model lilGreenCrystal("media/gems/lilgreen.gltf");
    Model lilPurpCrystal("media/gems/lilpurp.gltf");
    Model lilOrangeCrystal("media/gems/lilorange.gltf");
    Model lilRedCrystal("media/gems/lilred.gltf");

    std::vector<Model*> crystalModels = { 
        &redCrystal, 
        &blueCrystal, 
        &greenCrystal, 
        &orangeCrystal, 
        &purpleCrystal, 
        &yellowCrystal,
        &lgPurpCrystal,
        &lgBlueCrystal,
        &lgOrangeCrystal,
        &lilGreenCrystal,
        &lilPurpCrystal,
        &lilOrangeCrystal,
        &lilRedCrystal,
    };
    modelNames[&blueCrystal] = "Blue";
    modelNames[&redCrystal] = "Red";
    modelNames[&greenCrystal] = "Green";
    modelNames[&orangeCrystal] = "Orange";
    modelNames[&purpleCrystal] = "Purple";
    modelNames[&yellowCrystal] = "Yellow";
    modelNames[&lgPurpCrystal] = "Large Purple";
    modelNames[&lgBlueCrystal] = "Large Blue";
    modelNames[&lgRedCrystal] = "Large Red";
    modelNames[&lgOrangeCrystal] = "Large Orange";
    modelNames[&lilGreenCrystal] = "Small Green";
    modelNames[&lilPurpCrystal] = "Small Purple";
    modelNames[&lilOrangeCrystal] = "Small Orange";
    modelNames[&lilRedCrystal] = "Small Red";

    for (int i = 0; i < 75; i++)
    {
        Model* model = crystalModels[rand() % crystalModels.size()];

        bool placed = false;
        for (int attempt = 0; attempt < 20; attempt++)
        {
            float x = rand() % TERRAIN_SIZE;
            float z = rand() % TERRAIN_SIZE;

            float desiredHeight = 3.0f;
            float scaleFactor = desiredHeight / model->height;
            glm::vec3 scale(scaleFactor);
            float radius = model->GetWorldRadius(scale);

            bool overlaps = false;
            for (auto& c : crystals)
            {
                float dist = glm::distance(glm::vec2(x, z), glm::vec2(c.position.x, c.position.z));
                if (dist < radius + c.radius)
                {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps)
            {
                SpawnCrystalOnTerrain(model, x, z);
                placed = true;
                break;
            }
        }

        if (!placed)
        {
            float x = rand() % TERRAIN_SIZE;
            float z = rand() % TERRAIN_SIZE;
            SpawnCrystalOnTerrain(model, x, z);
        }
    }


    glm::vec3 pointLightPosition(0.0f, 20.0f, 0.0f);

    // shader configuration
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    crystalShader.use();
    crystalShader.setInt("texture_diffuse1", 0);
    crystalShader.setInt("texture_normal1", 1);
    crystalShader.setInt("texture_emissive1", 2);

    GenerateQuestList(10);




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

        //Camera collision with crystals
        float cameraRadius = 0.4f;
        for (auto& c : crystals)
        {
            glm::vec3 center = c.model->GetWorldCenter(c.position, c.scale);
            float minDist = cameraRadius + c.radius;
            float dist = glm::distance(camera.Position, center);

            if (dist < minDist && dist > 0.0001f)
            {
                glm::vec3 pushDir = glm::normalize(camera.Position - center);
                camera.Position = center + pushDir * minDist;
            }
        }


        if (mouseClicked)
        {
            mouseClicked = false;

            CrystalInstance* best = nullptr;
            float bestScore = -1.0f;

            float maxAngleDeg = 6.0f;                 // how tight the cone is
            float maxAngleRad = glm::radians(maxAngleDeg);
            float maxDistance = 30.0f;                // max click distance

            glm::vec3 camDir = glm::normalize(camera.Front);

            for (auto& c : crystals)
            {
                glm::vec3 center = c.model->GetWorldCenter(c.position, c.scale);
                glm::vec3 toCrystal = center - camera.Position;

                float dist = glm::length(toCrystal);
                if (dist > maxDistance)
                    continue;

                glm::vec3 dir = toCrystal / dist;
                float alignment = glm::dot(dir, camDir);

                // angle between camera direction and crystal center
                float angle = acos(glm::clamp(alignment, -1.0f, 1.0f));

                // sphere radius in world space
                float radius = c.model->GetWorldRadius(c.scale);

                // angular radius of the crystal (how big it appears from the camera)
                float angularRadius = asin(glm::clamp(radius / dist, -1.0f, 1.0f));

                // hit if the sphere overlaps the cone
                if (angle < maxAngleRad + angularRadius)
                {
                    // choose the most aligned crystal
                    if (alignment > bestScore)
                    {
                        bestScore = alignment;
                        best = &c;
                    }
                }
            }

            if (best)
            {
                std::string type = best->type;

                std::cout
                    << "Clicked (by look): "
                    << type
                    << " | isReal=" << best->isReal
                    << "\n";

                if (best->isReal && !best->found)
                {
                    // is this type in the quest list?
                    auto it = std::find(questList.begin(), questList.end(), type);

                    if (it != questList.end())
                    {
                        best->found = true;
                        std::cout << "You found a quest crystal: " << type << "!\n";

                        // remove from quest list
                        questList.erase(it);

                        if (questList.empty())
                            std::cout << "Quest complete!\n";
                    }
                    else
                    {
                        std::cout << "This crystal is real, but not on your list.\n";
                    }
                }

                best->onClick();
            }

            else
            {
                std::cout << "Clicked nothing.\n";
            }
        }



        // render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        // directional light
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("dirLight.diffuse", 0.2f, 0.2f, 0.2f);
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
        lightingShader.setVec3("spotLight.diffuse", 3.0f, 3.0f, 3.0f);
        lightingShader.setVec3("spotLight.specular", 3.0f, 3.0f, 3.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.06f);
        lightingShader.setFloat("spotLight.quadratic", 0.020f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(11.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(14.0f)));

        //crystal reaction to spotLight
        crystalShader.use();
        crystalShader.setVec3("lightPos", pointLightPosition);

        // view/projection transformations
        lightingShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        
        skybox.Draw(view, projection);

        //draw crystal
        crystalShader.use();
        crystalShader.setMat4("projection", projection);
        crystalShader.setMat4("view", view);
        crystalShader.setFloat("time", currentFrame);
        crystalShader.setVec3("viewPos", camera.Position);


        for (auto& c : crystals)
        {
            glm::mat4 modelMat = glm::mat4(1.0f);

            //position
            modelMat = glm::translate(modelMat, c.position);

            //ground the model so it sits on terrain
            modelMat = glm::translate(modelMat, glm::vec3(0.0f, -c.model->minY * c.scale.y, 0.0f));

            //scale
            modelMat = glm::scale(modelMat, c.scale);

            crystalShader.setMat4("model", modelMat);

            //detecting if the torch is on the crystal
            glm::vec3 center = c.model->GetWorldCenter(c.position, c.scale);
            glm::vec3 toCrystal = glm::normalize(center - camera.Position);
            glm::vec3 torchDir = glm::normalize(camera.Front);

            float alignment = glm::dot(toCrystal, torchDir);

            float cutOff = glm::cos(glm::radians(12.5f));
            float outerCutOff = glm::cos(glm::radians(15.0f));

            float intensity = (alignment - outerCutOff) / (cutOff - outerCutOff);
            intensity = glm::clamp(intensity, 0.0f, 1.0f);

            float dist = glm::distance(camera.Position, center);
            float falloff = 1.0f - glm::clamp(dist / 12.0f, 0.0f, 1.0f);

            float sparkle = (c.isReal) ? (intensity * falloff) : 0.0f;
            crystalShader.setFloat("sparkleStrength", sparkle);
            crystalShader.setBool("isRealCrystal", c.isReal);


            // draw the crystal
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


        //UI render
        glDisable(GL_DEPTH_TEST);

        // Build orthographic projection
        glm::mat4 uiProjection = glm::ortho(
            0.0f, (float)SCR_WIDTH,
            (float)SCR_HEIGHT, 0.0f,
            -1.0f, 1.0f
        );

        uiShader.use();
        uiShader.setMat4("projection", uiProjection);

        // Draw quest list
        float y = 20.0f;
        DrawText(uiShader, 20, y, "QUEST LIST:");
        y += 20;

        for (auto& type : questList)
        {
            std::string line = " - " + type;
            DrawText(uiShader, 20, y, line.c_str());
            y += 20;
        }

        if (questJustCompleted)
        {
            const char* msg = "QUEST COMPLETE!";
            float textWidth = GetTextWidth(msg);

            float x = (SCR_WIDTH - textWidth) * 0.5f;  // center horizontally
            float yText = 200.0f;                      // vertical position

            DrawText(uiShader, x, yText, msg);

            questCompleteTimer -= deltaTime;
            if (questCompleteTimer <= 0.0f)
                questJustCompleted = false;
        }

        glEnable(GL_DEPTH_TEST);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


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