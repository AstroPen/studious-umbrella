
all: json_gen test prod

compile = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -Wno-multichar -std=c++11 -O2 -c
asm = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -Wno-multichar -std=c++11 -O2 -S
link = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 -o

json_gen: json_gen.cpp haversine.h ../../Common/*.h makefile
	$(compile) json_gen.cpp -o json_gen.o \
	-I../../Common
	$(link) json_gen.out json_gen.o


prod: test.cpp haversine.h ../../Common/*.h makefile
	$(compile) test.cpp -D NO_PROFILE -o prod.o \
	-I../../Common
	$(link) prod.out prod.o

test: test.cpp haversine.h ../../Common/*.h makefile
	$(compile) test.cpp -o test.o \
	-I../../Common
	$(link) test.out test.o

test_asm: test.cpp haversine.h ../../Common/*.h makefile
	$(asm) test.cpp -o test.asm \
	-I../../Common

