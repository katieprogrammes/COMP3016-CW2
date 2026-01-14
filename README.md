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
