
all: swap_allocator_test

swap_allocator_test: swap_allocator_test.cpp swap_allocator.h push_allocator.h
	g++ -Wall -Wno-writable-strings -Wno-unused-function \
	-Wno-missing-braces -std=c++11 -O2 \
	-I../Common -I.. \
	-o swap_allocator_test swap_allocator_test.cpp

