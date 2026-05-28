codeqa: ./src/codeqa.c
#	clang -fsanitize=memory -ggdb -Wall -Wextra ./src/codeqa.c -o ./bin/codeqa.out -I./include
#
#_clang:	
	gcc -std=c23                        \
	    -ggdb                           \
		-Wall                           \
		-Wextra                         \
		-Wpedantic                      \
		-Wconversion                    \
		-Wformat=2                      \
		-Wshadow                        \
		-march=native                   \
		-fstack-protector-strong        \
		-D_FORTIFY_SOURCE=2             \
		-fstack-protector-strong        \
		./src/codeqa.c                  \
		-o ./bin/codeqa.out             \
		-I./include 
#		-O3                             \
# 		-fsanitize=leak                 \
# ./bin/codeqa.out
