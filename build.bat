::codeqa: codeqa.c
	gcc -O3 ./src/codeqa.c -o ./bin/codeqa.exe -I./include
	::clang -fsanitize=memory -ggdb -Wall -Wextra ./src/codeqa.c -o ./bin/codeqa.out -I./include