
all: test create_normal_map

compile = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 -c
link = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 -o

# NOTE : test_x11 and test_sdl are not actively being maintained right now.

test_x11: xplatform.cpp vmath.cpp game.cpp makefile
	g++ -Wall -Wno-unused-function -std=c++11 -O2 -o test_x11 xplatform.cpp -I/usr/X11/include -L/usr/X11R6/lib -lX11

test_sdl: sdlplatform.cpp vmath.cpp game.cpp game.h makefile
	g++ -Wall -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 \
	-o test_sdl sdlplatform.cpp -I/usr/local/include/SDL2 -I/usr/local/include/GL -L/usr/local/lib -lSDL2 \
	-F OpenGl -framework OpenGL

stb_image.o : custom_stb_image.cpp custom_stb.h pixel.h
	$(compile) custom_stb_image.cpp -o stb_image.o

stb_truetype.o : custom_stb_truetype.cpp custom_stb.h pixel.h
	$(compile) custom_stb_truetype.cpp -o stb_truetype.o

create_normal_map : create_normal_map.cpp custom_stb.h pixel.h
	$(compile) create_normal_map.cpp -o create_normal_map.o \
	-I/usr/local/include/GL -I../Common
	$(link) create_normal_map create_normal_map.o stb_image.o


object_files = stb_image.o stb_truetype.o

engine_includes = gl_sdlplatform.cpp sdl_thread.cpp sdlgl_init.cpp glrenderer.cpp pixel.h vmath.cpp game.cpp entity.cpp assets.cpp game.h ../Common/* \
	blinn_phong.frag blinn_phong.vert debug_system.cpp custom_stb.h

test: $(engine_includes) $(object_files) makefile
	$(compile) gl_sdlplatform.cpp -o test.o \
	-I/usr/local/include/SDL2 -I/usr/local/include/GL -I../Common
	$(link) test test.o $(object_files) \
	-L/usr/local/lib -lSDL2 -framework OpenGL

