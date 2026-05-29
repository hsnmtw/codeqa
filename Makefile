SRC=./src/codeqa.c
BIN=./bin/codeqa.out
INC=./include 
CC=clang
FLG=-O3                             \
	-Wall                           \
	-Wextra                         #\
# 	-Wpedantic                      \
# 	-Wformat=2                      \
# 	-march=native                   \
# 	-fstack-protector-strong        \
# 	-D_FORTIFY_SOURCE=2             \
# 	-fstack-protector-strong
# 	-Wshadow                        \
# 	-Wconversion                    \

#STD=-std=c2x
LIB=

codeqa: ./src/codeqa.c
	$(CC) $(SRC) -o $(BIN) -I$(INC) $(FLG) $(LIB) $(STD)

# 	clang -fsanitize=memory -ggdb -Wall -Wextra ./src/codeqa.c -o ./bin/codeqa.out -I./include