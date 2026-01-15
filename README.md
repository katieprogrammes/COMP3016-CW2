# COMP3016-Coursework 2 - Crystal Quest

## Overview
Crystal Quest is a calm, exploration‑focused OpenGL game set on a procedurally generated rocky terrain beneath a night sky. The landscape is scattered with crystals, some of which are “real” while others are decoys. The player carries a torch, which they use to distinguish genuine crystals from fake ones through dynamic lighting and visual effects.

At the start of the game, the player receives a quest list containing specific crystal types they must locate. To progress, they explore the terrain, use the torch to identify real crystals, and click on those that match their quest list. Each correct discovery triggers positive audio feedback, reinforcing the relaxed and rewarding atmosphere.

The game is designed to feel peaceful and atmospheric, using low-intensity lighting, gentle background music and rewarding audio cues to create to calming mood. The player wins the game by finding all the crystals on their quest list, completing the experience at their own pace.

## Dependencies

The dependencies used were:
- GLAD - OpenGL 4.6 Core Loader
- GLFW 3.3 - Window creation, input handling, and context management
- ASSIMP 5 – Importing and handling 3D models and meshes
- IrrKlang 1.6 – Background music and sound effects
- stb_easy_font – Lightweight text rendering for the UI
- stb_image – Loading texture files for models and terrain

## How To Compile
You can clone from github into Visual Studio then open the .sln (solution) file and you can then either run through the Debug or Release configuration (x64 version).

The .exe file provided in Release/x64 should run without any additional setup.

## How To Play
Controls are listed in the top right of the screen and are as follows:
WASD for Movement
Mouse for Camera
Left Click for Crystal Detection

## Youtube Link
https://youtu.be/46T4UNBQjXY

## Use of AI Description

My AI usage fell under the following Acceptable Use Categories:

- A7 - I initially followed the LearnOpenGL tutorials to understand the fundamentals of OpenGL and what could be achieved with it. When the tutorials moved in directions that no longer aligned with my project, I used AI to help reinterpret the tutorial code and header files so I could adapt the concepts to my own implementation.
- A8 - When Visual Studio produced error messages that were unclear or difficult to interpret, I used AI to reformat and explain them in a more understandable way. This helped me identify the underlying issues and fix them myself.
- Partnered Work - I used Copilot to generate a texture for my Dev Crystal by providing my existing emissive textures and requesting a rainbow variant in the same style. I also used AI as a code assistant when I was unsure how to structure and implement my crystal‑clicking logic. No AI‑generated code was directly copied into the project; all gameplay logic, rendering systems, and shader implementations were written and edited manually by me.

## Game Patterns I Used

Sequencing Patterns - For my project, I used a traditional Update–Render Loop to sequence all game behaviour. Each frame follows the same pattern: input is processed, game logic is updated, and then the scene is rendered. This ensures that crystal interactions, camera movement, and shader updates occur predictably every frame.
Within the Update stage, I implemented a spatial‑query interaction system that checks whether the player is looking at a crystal and clicking within a valid angle and distance. This determines whether a crystal is selected, whether it contributes to the quest, and whether the Dev Crystal should appear once the quest is complete.

State Patterns - I used the State Pattern to manage both crystal behaviour and overall quest progression. Each crystal instance stores state flags such as isReal, found, and whether it is the Dev Crystal. These states determine how the crystal behaves when clicked or rendered.
The quest system also functions as a state machine: the game tracks whether the quest is active, completed, or waiting for the Dev Crystal to spawn. Once all required crystals are found, the state changes and triggers new behaviour, such as spawning the Dev Crystal and playing completion audio.
These states allow the game to switch cleanly between different behaviours without rewriting logic, ensuring that rendering, clicking, and quest progression all respond correctly to the current game state.

Component‑Style Entity Pattern - Although not using a full ECS, the project uses a component‑style approach for crystals. Each CrystalInstance stores all data relevant to that entity (position, scale, rotation, model reference, radius, type, and state flags). This mirrors the Component pattern, where entities act as simple data containers and behaviour is applied externally through update and render functions. This makes it easy to add, remove, or modify crystals without affecting other parts of the code.

Singleton-Style Systems - Several systems in the project behave like singletons, including the camera, shader programs, and sound engine. These are created once and accessed globally throughout the program. This pattern simplifies access to core systems that must remain consistent across the entire game, such as camera position for interaction checks or shader state for rendering.

Spatial Query Pattern - The crystal‑clicking system uses a spatial query to determine whether the player is looking at a crystal. This involves calculating the angle between the camera direction and the vector to each crystal, and comparing it to a threshold that includes the crystal’s angular radius.
This pattern is commonly used in games for object selection, targeting, and interaction cones, and it allows the player to interact with crystals naturally without requiring a physics engine or raycasting system, which I was unable to integrate successfully.

## Game Mechanics and How They Are Coded

The core mechanic of this project is the Crystal Collection System. The player explores the terrain, identifies real crystals using a directional clicking mechanic and a flashlight, and collects the crystals listed in their quest. Once all required crystals are found, a special Dev Crystal appears behind the player as the final objective.

### Terrain Generation (Procedural Content Generation)
The terrain is generated procedurally using a heightmap‑based approach. A grid of vertices is created, and each vertex’s height is sampled from a noise function to produce a natural‑looking rocky landscape.

<img width="1905" height="961" alt="terrainingame" src="https://github.com/user-attachments/assets/9e6ca05c-33f7-45af-9e55-ae03688a2af1" />

Each vertex is assigned a height value during a loop over a 2D grid:

<img width="940" height="648" alt="terrainvertices" src="https://github.com/user-attachments/assets/31e151b9-7f99-48d1-9a4b-e39dbad55081" />

The vertices and indices are then uploaded to OpenGL buffers:

<img width="940" height="511" alt="terrainbuffers" src="https://github.com/user-attachments/assets/02c30727-5208-4a17-a08a-f48f4d4dd5d3" />

Rendering is handled by a custom vertex and fragment shader. In the render loop, the shader is activated, transformation matrices are passed in, the terrain texture is bound, and the VAO is drawn using glDrawElements:

<img width="882" height="402" alt="terrainrender" src="https://github.com/user-attachments/assets/155db5a3-9c56-4610-aa35-bf00908aadf7" />

### Skybox
For the night sky I generated a skybox 

<img width="940" height="477" alt="image" src="https://github.com/user-attachments/assets/f6e38444-63f9-4fe9-812b-7e9131b76ab1" />

I achieved this by making a simple cube of 36 vertices:

<img width="207" height="745" alt="image" src="https://github.com/user-attachments/assets/ec61fcd1-447b-491a-8d1c-42446710706c" />

And loading 6 images into a cubemap texture:

<img width="406" height="231" alt="image" src="https://github.com/user-attachments/assets/0e800b4e-6346-4c74-94de-e7b6b51bf646" />

I then draw the cube with the camera’s translation removed so it always stays centred around the player:

<img width="366" height="205" alt="image" src="https://github.com/user-attachments/assets/e873a72c-3fa9-4f73-a7ed-797f6ae8327b" />


### Crystal Spawning

Crystals are spawned at randomised positions across the terrain. Each crystal is grounded by sampling the terrain height at its XZ position and applying a model‑specific offset so it sits naturally on the surface.

<img width="1900" height="958" alt="crystalingame" src="https://github.com/user-attachments/assets/62a9e2de-2ec2-44d3-9528-069c394dc03a" />

Each crystal is represented by a CrystalInstance struct containing its transform, model reference, and state flags:

<img width="227" height="172" alt="crystalstruct" src="https://github.com/user-attachments/assets/ca2f1a66-b883-496e-aef3-176b409f9844" />

Models are loaded using ASSIMP:

<img width="314" height="209" alt="modelload" src="https://github.com/user-attachments/assets/eeffcb92-4166-4d4b-8620-dc059ff78262" />

A random model is selected, and the system attempts to place it at a valid location. To avoid overlapping crystals, the code checks distances and radii against existing crystals. If a valid position is found, the crystal is spawned:

<img width="424" height="376" alt="spawnlogic" src="https://github.com/user-attachments/assets/7af1f164-2a52-4ba1-b93a-fe9341b9a8c9" />

### Flashlight Highlighting Real Crystals

The flashlight helps the player identify real crystals. Real crystals glow, pulse, and shift colour slightly, while fake crystals remain unchanged:

![20260114-1620-24 4210765](https://github.com/user-attachments/assets/17801c75-eebf-45cb-ad3c-e8f3e2c87efc)

The flashlight state is passed to the shader as a uniform:

<img width="155" height="37" alt="flashlight" src="https://github.com/user-attachments/assets/7212cb85-50b7-4c9e-b987-6a47dbcf157f" />

The shader applies glinting, pulsing, glowing, a colour shift, and a bloom boost to real crystals:

<img width="431" height="390" alt="crystalshader" src="https://github.com/user-attachments/assets/6ae667c6-2f8b-4c03-a6d2-4446edfe5f4c" />

These effects are applied during the crystal’s draw call:

<img width="445" height="315" alt="crystaldraw" src="https://github.com/user-attachments/assets/44f4cd7b-d960-4964-97cf-b2426fda9fd0" />

### Crystal Clicking Interaction System

The player interacts with crystals using a directional clicking system. When the player clicks, the game checks whether a real crystal is within a valid angle and distance from the camera’s forward direction. If the crystal is real and on the quest list, it is marked as found. If it is real but not on the list, a message explaining this is shown. When all required crystals are found, the Dev Crystal spawns behind the player.

![20260114-1641-57 2609545](https://github.com/user-attachments/assets/198667ca-023f-4f23-8369-c899c28f21b7)

The interaction logic calculates the vector from the camera to each crystal, measures the angle between that vector and the camera’s forward direction, checks distance, and selects the crystal the player is most directly looking at:

<img width="389" height="322" alt="clicklogic" src="https://github.com/user-attachments/assets/b802aaf2-f715-4f34-be2e-e73ed884ebe6" />

### Quest List
The quest list contains the names of the real crystals the player must find. It is displayed on the UI and updates dynamically as crystals are discovered. Each playthrough generates a unique list.

<img width="288" height="376" alt="image" src="https://github.com/user-attachments/assets/fdd7e35e-b1a3-4024-a9f2-faa98a4203b1" />

The quest list is stored as a vector of strings. When a crystal is clicked, the game checks whether its type is in the list and removes it if found:

<img width="719" height="251" alt="listlogic1" src="https://github.com/user-attachments/assets/b87b4e91-5e09-4374-a992-33aab3591f7f" />
<img width="728" height="148" alt="listlogic2" src="https://github.com/user-attachments/assets/e3bd420a-01bd-421e-831d-209459146cec" />

### Quest Completion and Dev Crystal Spawn
When the quest list becomes empty, the quest is marked as complete. A message appears, a completion sound plays, and the Dev Crystal spawns behind the player as the final objective.

![20260114-1723-46 6158525](https://github.com/user-attachments/assets/f59c1a4f-d1eb-4181-9295-b462fa63b80a)

The Dev Crystal’s position is calculated using the player’s current position and forward direction, then grounded to the terrain:

<img width="401" height="374" alt="devlogic" src="https://github.com/user-attachments/assets/124b118b-37a0-4d4f-966a-9746d65b62ee" />

Clicking the Dev Crystal displays the game credits.

### Audio Feedback
Audio is handled by the IrrKlang library. Background music plays continuously from the start of the game:

<img width="397" height="98" alt="irrklanginit" src="https://github.com/user-attachments/assets/ae76a6fb-12d9-4714-b261-28591d13a7a6" />

A sound plays when a real crystal is found:

<img width="306" height="14" alt="foundNoise" src="https://github.com/user-attachments/assets/7e49e289-7717-4f62-833a-20e3f866eb55" />

Another sound plays when the quest is complete:

<img width="299" height="14" alt="complete" src="https://github.com/user-attachments/assets/56839c6b-b466-4b75-83f5-78f114bc961f" />

The final sound plays when the Dev Crystal is clicked:

<img width="518" height="26" alt="image" src="https://github.com/user-attachments/assets/d6cb2f56-e681-490f-81d0-1a84f12c91d6" />



## Exception Handling and Test Cases

Even though this project is primarily graphical and interactive, I implemented several small but important safety checks to prevent crashes and undefined behaviour during gameplay.

### Terrain Bounds Checks
When sampling terrain height, the code checks that the XZ coordinates are within valid bounds before accessing the vertex array. This prevents out‑of‑range memory access:

<img width="389" height="31" alt="terrainboundchecking" src="https://github.com/user-attachments/assets/258f89d7-5f23-4a38-a7ac-201a36e3567b" />

### Model Pointer Validation
Before interacting with crystals, the code checks that the model pointer exists:

<img width="157" height="46" alt="modelcheck" src="https://github.com/user-attachments/assets/d85a4a61-a6be-4779-97f2-fb12e923eb19" />

### Dev Crystal spawn safety 
The Dev Crystal only spawns once, and only when the quest list is empty, preventing duplicate spawns or logic conflicts.

### Test Cases
Throughout development, I used an incremental testing strategy to ensure each feature worked correctly before moving on to the next. I made small, isolated changes and tested them immediately, which made it easier to identify the source of errors and revert to a stable version when necessary.

Because OpenGL provides very limited runtime error feedback, I relied heavily on std::cout diagnostic messages to verify that each subsystem was behaving as expected. These were used extensively during:

Startup and initialisation - I added checks for GLFW window creation, GLAD loading, and IrrKlang audio initialisation. Each subsystem printed a clear error message if it failed to load, preventing the program from running in an invalid state.

<img width="527" height="377" alt="initcout" src="https://github.com/user-attachments/assets/352284ba-25b6-44f6-a450-38a9683a1070" />

Asset Loading - When loading models and textures, I printed out file paths and success/failure messages. This made it easy to detect incorrect paths, missing files, or unsupported formats.

<img width="440" height="64" alt="assetcout" src="https://github.com/user-attachments/assets/b4d3f32f-a0c0-4457-907c-28ddf576c9a0" />

Gameplay Logic - I used cout statements to monitor crystal spawning, collision checks, click detection, quest list updates, and Dev Crystal spawning. These logs helped confirm that each mechanic was triggering at the correct time and behaving as intended.

<img width="362" height="197" alt="logicout" src="https://github.com/user-attachments/assets/32576379-d345-41cd-a992-41421a6f8112" />

I also repeatedly tested crystal clicking, flashlight highlighting, and quest progression through multiple playthroughs. This ensured that crystals could only be clicked when looked at, fake crystals were ignored, real crystals updated the quest list correctly, incorrect clicks triggered the “not on quest list!” message, and the Dev Crystal spawned only once and in the correct location.

This combination of incremental testing, diagnostic output and repeated in-game trials ensured that the project remained stable and that each mechanic functioned consistently throughout development.

## Evaluation and Reflection

Looking back at this project, I feel I managed to build a complete OpenGL game with procedural terrain, model loading, lighting, UI, audio, and a functional gameplay loop. There were many points in early development where I felt out of my depth, but I did enjoy learning how OpenGL works and gradually gained confidence as the different systems started coming together. I’m also glad that I was able to deliver the core idea from my proposal, even if I didn’t reach my stretch goal of turning the terrain into a cave system.

The crystal interaction system was the most challenging mechanic to implement. Since I wasn’t able to integrate PhysX or raycasting, I relied instead on vector maths, angle checks, and distance thresholds. It took a lot of trial and error to get right, but the final mechanic works reliably and feels consistent during gameplay.

If I were to continue working on this, I would like to expand the variety of crystal models and experiment with particle effects to make real crystals sparkle more convincingly. There are also things I would approach differently now that I understand the C++ language and libraries better, especially around structuring the code and planning features earlier in development.

Overall, even though I struggled at times, I’m satisfied with what I achieved. The project feels like a small but complete game, and the different systems work together in a way that I’m genuinely proud of.

## Asset Credits

### Models
Small Crystals:
"Stylised Crystal Texture Tests" (https://sketchfab.com/3d-models/stylised-crystal-texture-tests-b3a1b42e51f042ef9a96546c42d77bec) by LukeThePunk666 (https://sketchfab.com/LukeThePunk666) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)

Regular Crystals:
"Crystal Pack Stylized" (https://sketchfab.com/3d-models/crystal-pack-stylized-6e4fb0784b264c62858763f394b0f169) by Batuhan13 (https://sketchfab.com/Batuhan13) licensed under CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)

Large Crystals:
"Stylized Crystals" (https://skfb.ly/oJEDu) by Uğur Yakışık is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

### Audio

Background Music:
Serene Ambient Soundscape for Relaxation by Matio888 -- https://freesound.org/s/797847/ -- License: Attribution 4.0

Found Noise:
13819 star twinkle ding by Robinhood76 -- https://freesound.org/s/819418/ -- License: Attribution NonCommercial 4.0

Success Noise:
Victory (short sting) by xkeril -- https://freesound.org/s/706753/ -- License: Creative Commons 0

Dev Crystal Found:
Completed.wav by Kenneth_Cooney -- https://freesound.org/s/609336/ -- License: Creative Commons 0
