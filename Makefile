CC      = gcc
CFLAGS  = -I./include -O3 #-ggdb -fsanitize=memory
TARGET  = ./bin/codeqa.out
SRCS    = $(wildcard *.c) $(wildcard ./src/*.c)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)