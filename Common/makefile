
commands = g++ -Wall -Wno-writable-strings -Wno-unused-function \
	-Wno-missing-braces -std=c++11 -O2 -I../Common -I.. -o

all: swap_allocator_test dynamic_array_test heap_sort_test

swap_allocator_test: swap_allocator_test.cpp swap_allocator.h push_allocator.h common.h
	$(commands) swap_allocator_test swap_allocator_test.cpp

dynamic_array_test: dynamic_array_test.cpp dynamic_array.h common.h
	$(commands) dynamic_array_test dynamic_array_test.cpp

heap_sort_test: heap_sort_test.cpp heap_sort.h common.h
	$(commands) heap_sort_test heap_sort_test.cpp

