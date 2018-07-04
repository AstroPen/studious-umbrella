# Game Engine WIP

This is a game engine I am building using SDL and OpenGL.

## Structure

The project main is in gl\_sdlplatorm.cpp. 
All other files are currently just includes to reduce the need for header files and keep everything in a single translation unit.

After some initialization to create the window, OpenGL context, and worker threads, 
the main loop parses keyboard events through SDL and calls update\_and\_render with a large block of memory, 
a buffer for rendering commands, a multithreaded work queue, and the "controller" (keyboard and mouse) state.
Afterwards, the render buffer is processed and sent to OpenGL to be drawn.

In game.cpp, update\_and\_render initializes the GameState if necessary, then applies the controller input to it.
The physics loop, unlike the main loop, has a set frame time. This is necessary in order for the physics to behave consistently at 
different framerates. update\_physics is still very primative and mostly just forwards the call to process\_entity in entity.cpp.

The render code can be found in glrenderer.cpp. Entities are pushed to the render buffer as Quads, then later are processed to be put
into a large vertex buffer that is sent to OpenGL. The shaders can be found in blinn\_phong.frag and blinn\_phong.vert.

The remaining files are 
- game.h which includes definitions for various game types
- assets.cpp which handles asset processing
- sdl\_thread.cpp which contains code for the WorkQueue and atomics
- vmath.cpp which contains math code for collision detection
- debug\_system.cpp which contains macros that I use for measuring performance
- utilities.cpp which includes various allocators and general programming helpers
- custom\_stb\_image.h which contains functions interacting with the stb\_image library
- sdlgl\_init.cpp which just handles some initialization for the platform layer
- xplatform.cpp which was an old platform layer for X11 that is not being maintained.

There are also various include from ../Common, in particular push\_allocator.h, scalar\_math.h, vector\_math.h and various others.

## Setup

This project is not guaranteed to work on any other machine, but it might be possible if you are on Mac.
It uses the stb libraries (in particular stb\_image.h and stb\_truetype.h) which are simply header files that must be saved in this directory.
It also uses SDL which is more complicated to install. Since the assets I use are not on source control, you will need to provide your own.
I will add more details later.


