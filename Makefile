codeqa: ./src/codeqa.c
	gcc -O3 ./src/codeqa.c -o ./bin/codeqa.out -I./include

_clang:	
	clang -fsanitize=memory -ggdb -Wall -Wextra ./src/codeqa.c -o ./bin/codeqa.out -I./include