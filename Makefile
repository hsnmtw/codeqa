codeqa: ./src/codeqa.c
	clang -fsanitize=memory -ggdb -Wall -Wextra ./src/codeqa.c -o ./bin/codeqa.out -I./include

_clang:	
	gcc -O3 ./src/codeqa.c -o ./bin/codeqa.out -I./include