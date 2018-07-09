
commands = g++ -Wall -Wno-writable-strings -Wno-unused-function \
	-Wno-missing-braces -std=c++11 -O2 -I../Common -I.. -o

all: swap_allocator_test dynamic_array_test sort_test

common_includes = common.h scalar_math.h

swap_allocator_test: swap_allocator_test.cpp swap_allocator.h push_allocator.h $(common_includes)
	$(commands) swap_allocator_test swap_allocator_test.cpp

dynamic_array_test: dynamic_array_test.cpp dynamic_array.h $(common_includes)
	$(commands) dynamic_array_test dynamic_array_test.cpp

sort_test: sort_test.cpp heap_sort.h quick_sort.h $(common_includes)
	$(commands) sort_test sort_test.cpp

