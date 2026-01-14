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

The flashlight state is passed to the shader as a uniform:
<img width="155" height="37" alt="flashlight" src="https://github.com/user-attachments/assets/7212cb85-50b7-4c9e-b987-6a47dbcf157f" />

The shader adds glinting, pulsing, glowing, a slight colour shift and a bloom boost to the crystal to ensure the player knows they are looking at a real crystal:

<img width="431" height="390" alt="crystalshader" src="https://github.com/user-attachments/assets/6ae667c6-2f8b-4c03-a6d2-4446edfe5f4c" />

Then when the crystal is drawn, the effects from the shader are applied to the real crystal:

<img width="445" height="315" alt="crystaldraw" src="https://github.com/user-attachments/assets/44f4cd7b-d960-4964-97cf-b2426fda9fd0" />

### Crystal Clicking Interaction System

The player interacts with crystals using a direction-based clicking system. When the player clicks, the game checks whether a "real" crystal is within a valid angle and distance from the camera's forward direction. If the crystal is real and part of the quest list, it is marked as found. If it is real but not on the list, the player is informed that crystal is not on their quest list. If all crystals are found, the Dev Crystal is spawned behind the player.

![20260114-1641-57 2609545](https://github.com/user-attachments/assets/198667ca-023f-4f23-8369-c899c28f21b7)

The interaction logic works by calculating the vector from the camera to each crystal, measuring the angle between that vector and the camera's forward direction, checking the distance, selecting the crystal the player is most directly looking at, updating the quest list or showing feedback.

<img width="389" height="322" alt="clicklogic" src="https://github.com/user-attachments/assets/b802aaf2-f715-4f34-be2e-e73ed884ebe6" />

### Quest List
The game maintains a quest list containing the names of the real crystals the player must find. This list is displayed on the UI and updates dynamically as crystals are discovered. When the player clicks a real crystal that is part of the quest list, it is removed from the list. The quest list is unique to each playthrough.

<img width="158" height="245" alt="questlist" src="https://github.com/user-attachments/assets/ac9660ca-0b58-439f-b817-aad348922810" />

The quest list is stored as a vector of strings. When a crystal is clicked, the game checks whether its type exists in the list and removes it if found:

<img width="719" height="251" alt="listlogic1" src="https://github.com/user-attachments/assets/b87b4e91-5e09-4374-a992-33aab3591f7f" />
<img width="728" height="148" alt="listlogic2" src="https://github.com/user-attachments/assets/e3bd420a-01bd-421e-831d-209459146cec" />

### Quest Completion and Dev Crystal Spawn
When the quest list becomes empty, the quest is marked as complete. A message is shown to the player, a completion sound plays, and a special Dev Crystal is spawned behind the player. This crystal acts as the final objective of the game.

![20260114-1723-46 6158525](https://github.com/user-attachments/assets/f59c1a4f-d1eb-4181-9295-b462fa63b80a)

The Dev Crystal’s position is calculated using the player’s current position and forward direction, then grounded to the terrain so it sits correctly on the surface.

<img width="401" height="374" alt="devlogic" src="https://github.com/user-attachments/assets/124b118b-37a0-4d4f-966a-9746d65b62ee" />

The Dev Crystal is the Signature and clicking on it shows the game credits.


### Audio Feedback
The audio is handled by the Irrklang library. There is background music playing continuously from the games launch:

<img width="397" height="98" alt="irrklanginit" src="https://github.com/user-attachments/assets/ae76a6fb-12d9-4714-b261-28591d13a7a6" />

Sounds play when a real crystal is found:

<img width="306" height="14" alt="foundNoise" src="https://github.com/user-attachments/assets/7e49e289-7717-4f62-833a-20e3f866eb55" />

and when the game is complete:

<img width="299" height="14" alt="complete" src="https://github.com/user-attachments/assets/56839c6b-b466-4b75-83f5-78f114bc961f" />

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

I also repeatedly tested crystal clicking, flashlight highlighting and quest progression through multiple playthroughs to ensure that crystals could only be clicked when looked at, fake crystals were ignored, real crystals updated the quest list correctly, incorrect clicks triggered the "not on quest list! message and the Dev Crystal spawned only once and in the correct location

This combination of incremental testing, diagnostic output and repeated in-game trials ensured that the project remained stable and that each mechanic functioned consistently throughout development.

## Evaluation and Reflection

Looking back at this project, I feel I managed to build a complete OpenGL game with procedural terrain, model loading, lighting, UI, audio, and a functional gameplay loop. There were many points in early development where I felt out of my depth, but I did enjoy learning how OpenGL works and gradually gained confidence as the different systems started coming together. I’m also glad that I was able to deliver the core idea from my proposal, even if I didn’t reach my stretch goal of turning the terrain into a cave system.

The crystal interaction system was the most challenging mechanic to implement. Since I wasn’t able to integrate PhysX or raycasting, I relied instead on vector maths, angle checks, and distance thresholds. It took a lot of trial and error to get right, but the final mechanic works reliably and feels consistent during gameplay.

If I were to continue working on this, I would like to expand the variety of crystal models and experiment with particle effects to make real crystals sparkle more convincingly. There are also things I would approach differently now that I understand the C++ language and libraries better, especially around structuring the code and planning features earlier in development.

Overall, even though I struggled at times, I’m satisfied with what I achieved. The project feels like a small but complete game, and the different systems work together in a way that I’m genuinely proud of.
