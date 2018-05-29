
all: hash_table_test

hash_table_test_templated: hash_table_test_templated.cpp hash_table_templated.h
	g++ -Wall -Wno-writable-strings -Wno-unused-function \
	-Wno-missing-braces -std=c++11 -O2 \
	-o test hash_table_test_templated.cpp

hash_table_test: hash_table_test.cpp hash_set.h hash_table.h
	g++ -Wall -Wno-writable-strings -Wno-unused-function \
	-Wno-missing-braces -std=c++11 -O2 \
	-o test hash_table_test.cpp
