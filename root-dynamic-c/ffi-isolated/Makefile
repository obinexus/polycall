CC = gcc
CFLAGS = -Wall -Werror -fPIC -O2
TARGET = libffi_isolated.so

all: $(TARGET)

$(TARGET): src/*.c
	$(CC) $(CFLAGS) -shared -o $@ $^
