
all: disassemble

compile = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 -c
link = g++ -Wall -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -std=c++11 -O2 -o

includes = main.cpp ../../Common/*.h

disassemble: main.cpp ../../Common/*.h makefile
	$(compile) main.cpp -o disassemble.o \
	-I../../Common
	$(link) disassemble.out disassemble.o

