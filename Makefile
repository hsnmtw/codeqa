SRC=./src/codeqa.c
BIN=./bin/codeqa.out
INC=./include 
CC=clang
FLG=-O3 -Wuninitialized #-DMEMORY_DEBUG
# FLG=-Wall                           \
# 	-Wextra                         \
# 	-Wpedantic                      \
# 	-O3
# 	-fsanitize=memory -ggdb
# 	-Wformat=2                      \
# 	-march=native                   \
# 	-fstack-protector-strong        \
# 	-D_FORTIFY_SOURCE=2             \
# 	-fstack-protector-strong
# 	-Wshadow                        \
# 	-Wconversion                    \

#STD=-std=c2x
LIB=

main: ./src/main.c ./include/logger.h ./include/heap.h
	$(CC) ./src/main.c -o ./bin/main -I$(INC) $(FLG) $(LIB) $(STD)

# codeqa: ./src/codeqa.c
# 	$(CC) $(SRC) -o $(BIN) -I$(INC) $(FLG) $(LIB) $(STD)

# 	clang -fsanitize=memory -ggdb -Wall -Wextra ./src/codeqa.c -o ./bin/codeqa.out -I./include