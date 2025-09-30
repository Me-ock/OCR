CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c
OBJ = $(SRC:.c=.o)
TARGET = ocr_app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)
