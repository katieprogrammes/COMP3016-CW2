# COMP3016-Coursework 2 - Crystal Quest

## Overview
Crystal Quest is a calm, exploration-focused OpenGL game set on a procedurally generated rocky terrain beneath a night sky. The terrain is scattered with crystals, some of which are “real” while others are decoys. The player carries a torch, which they must use to distinguish genuine crystals from fake ones through dynamic lighting and visual effects.

At the start of the game, the player receives a quest list containing specific crystal types they must locate. To progress, they explore the terrain, use the torch to identify real crystals and click on those that match their quest list. Each correct discovery triggers positive audio feedback which reinforces the relaxed and rewarding atmosphere.

The game is designed to feel peaceful and atmospheric, using low-intensity lighting, gentle background music and rewarding audio cues to create to calming mood. The player wins the game by finding all the crystals on their quest list, completing the experience at their own pace.

## Dependencies

The dependencies used were:
- GLAD (OpenGL 4.6 Core Loader) 
- GLFW Version 3.3
- ASSIMP Version 5 to import and handle models and meshes
- IrrKlang Version 1.6 to handle background music and sound effects
- stb_easy_font for lightweight text rendering within the UI
- stb_image for loading texture files used on models and terrain

## Use of AI Description

These were the Acceptable Use Categories that my AI usage fell under:

- A7 - I initially followed the LearnOpenGL tutorials to understand the fundamentals of OpenGL and what could be achieved with it. When the tutorials moved in directions that no longer aligned with my project, I used AI to help reinterpret the tutorial code and header files so I could adapt the concepts to my own implementation.
- A8 - When Visual Studio produced error messages that were unclear or difficult to interpret, I used AI to reformat and explain the errors in a more understandable way. This allowed me to identify the underlying issues and fix them myself.
- Partnered Work - I used Copilot to generate a texture for my Dev Crystal by providing my existing emissive textures and requesting a rainbow variant in the same style. I also used AI as a code assistant when I was unsure how to structure and implement my crystal‑clicking logic. No AI‑generated code was directly copied into the project with all gameplay logic, rendering systems, and shader implementations being written and edited manually by me.

## Game Patterns I Used

Sequencing Patterns - For my project, I used a traditional Update–Render Loop to sequence all game behaviour. Each frame follows the same pattern: input is processed, game logic is updated, and then the scene is rendered. This sequencing pattern ensures that crystal interactions, camera movement, and shader updates occur in a predictable order every frame.
Within the Update stage, I implemented a spatial‑query‑based interaction system that checks whether the player is looking at a crystal and clicking within a valid angle and distance. This logic determines whether a crystal is selected, whether it contributes to the quest, and whether the Dev Crystal should appear after the quest is completed.

State Patterns - I used the State Pattern to manage both crystal behaviour and overall quest progression. Each crystal instance stores state flags such as isReal, found, and whether it is the Dev Crystal, and these states determine how the crystal behaves when clicked or rendered.
The quest system also functions as a state machine: the game tracks whether the quest is active, completed, or waiting for the Dev Crystal to spawn. Once all required crystals are found, the state changes and triggers new behaviour, such as spawning the Dev Crystal and playing completion audio.
These states allow the game to switch cleanly between different behaviours without rewriting logic, ensuring that rendering, clicking, and quest progression all respond correctly to the current game state.

Component‑Style Entity Pattern - Although not using a full ECS, the project uses a component‑style approach for crystals with each CrystalInstance storing all the data relevant for that entity (position, scale, rotation, model reference, radius, type, state flags (found, isReal, etc.)). This mirrors the Component pattern, where entities are simple data containers and behaviour is applied externally through update and render functions. This makes it easy to add, remove, or modify crystals without affecting other parts of the code.

Singleton-style Systems - Several systems in the project behave like singletons, including the camera, shader programs, and sound engine. These are created once and accessed globally throughout the program. This pattern simplifies access to core systems that must remain consistent across the entire game, such as camera position for interaction checks or shader state for rendering.

Spatial Query Pattern - The crystal‑clicking system uses a spatial query to determine whether the player is looking at a crystal. This involves calculating the angle between the camera direction and the vector to each crystal, and comparing it to a threshold that includes the crystal’s angular radius.
This pattern is commonly used in games for object selection, targeting, and interaction cones, and it allows the player to interact with crystals naturally without requiring a physics engine or raycasting system which unfortunately would not work when I implemented them.

## Game Mechanics and How They Are Coded

The main game mechanic I implemented for this project is the Crystal Collection System, where the player must explore the terrain, locate a set of hidden crystals, and interact with them using a directional clicking mechanic. Once all required crystals have been found, a special Dev Crystal spawns behind the player, acting as the final objective of the quest.

### Terrain Generation (Procedural Content Generation)
The terrain in my project is generated procedurally using a heightmap-based approach. A grid of vertices is created and each vertex's height is sampled from a noise function. This produces a natural-looking landscape with height variation.

<img width="1905" height="961" alt="terrainingame" src="https://github.com/user-attachments/assets/9e6ca05c-33f7-45af-9e55-ae03688a2af1" />

The terrain is generated by looping through a 2D grid and assigning each vertex a height value based on a noise function:

<img width="940" height="648" alt="terrainvertices" src="https://github.com/user-attachments/assets/31e151b9-7f99-48d1-9a4b-e39dbad55081" />

The resulting vertices and indices are uploaded to OpenGL buffers:

<img width="940" height="511" alt="terrainbuffers" src="https://github.com/user-attachments/assets/02c30727-5208-4a17-a08a-f48f4d4dd5d3" />

The terrain is rendered using a custom vertex and fragment shader.
In the render loop, the shader is activated, transformation matrices are passed in, the terrain texture is bound, and the terrain VAO is drawn using glDrawElements:

<img width="882" height="402" alt="terrainrender" src="https://github.com/user-attachments/assets/155db5a3-9c56-4610-aa35-bf00908aadf7" />

### Crystal Spawning

Crystals are spawned across the terrain at randomised positions. Each crystal is grounded to the terrain by sampling the terrain height at its XZ position and applying a model‑specific minimum Y offset to ensure it sits naturally on the surface.

<img width="1900" height="958" alt="crystalingame" src="https://github.com/user-attachments/assets/62a9e2de-2ec2-44d3-9528-069c394dc03a" />

Each crystal is represented by a CrystalInstance struct containing its position, scale, rotation, model reference, and state flags:

<img width="227" height="172" alt="crystalstruct" src="https://github.com/user-attachments/assets/ca2f1a66-b883-496e-aef3-176b409f9844" />

The models are loaded in using ASSIMP:
<img width="314" height="209" alt="modelload" src="https://github.com/user-attachments/assets/eeffcb92-4166-4d4b-8620-dc059ff78262" />

Then, a random model is selected from the available crystal models and an attempt is made to place it at a random XZ coordinate within the terrain bounds. To avoid overlapping crystals, the code checks for collisions with existing crystals by comparing distances and radii. If a suitable position is found within a limited number of attempts, the crystal is spawned at that location:

<img width="424" height="376" alt="spawnlogic" src="https://github.com/user-attachments/assets/7af1f164-2a52-4ba1-b93a-fe9341b9a8c9" />

### Flashlight Highlighting Real Crystals

The flashlight mechanic helps the player identify real crystals. Real crystals glow and pulse while fake crystals remain unchanged. This provides visual feedback and supports exploration:

![20260114-1620-24 4210765](https://github.com/user-attachments/assets/17801c75-eebf-45cb-ad3c-e8f3e2c87efc)

