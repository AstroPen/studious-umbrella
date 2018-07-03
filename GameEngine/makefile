
all: test

commands = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 -o

test_x11: xplatform.cpp vmath.cpp game.cpp makefile
	g++ -Wall -Wno-unused-function -std=c++11 -O2 -o test_x11 xplatform.cpp -I/usr/X11/include -L/usr/X11R6/lib -lX11

test_sdl: sdlplatform.cpp vmath.cpp game.cpp game.h makefile
	g++ -Wall -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 \
	-o test_sdl sdlplatform.cpp -I/usr/local/include/SDL2 -I/usr/local/include/GL -L/usr/local/lib -lSDL2 \
	-F OpenGl -framework OpenGL

test: gl_sdlplatform.cpp sdl_thread.cpp sdlgl_init.cpp glrenderer.cpp utilities.cpp ../Common/* vmath.cpp game.cpp entity.cpp assets.cpp game.h custom_stb_image.h blinn_phong.frag blinn_phong.vert makefile debug_system.cpp
	$(commands) test gl_sdlplatform.cpp \
	-I/usr/local/include/SDL2 -I/usr/local/include/GL -I../Common \
	-L/usr/local/lib -lSDL2 -framework OpenGL
