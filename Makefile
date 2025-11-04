CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

SRC_DIR = src

SRC = $(SRC_DIR)/main.c \
      $(SRC_DIR)/image_loader.c \
      $(SRC_DIR)/preprocessing.c

OBJ = $(SRC:.c=.o)

TARGET = ocr

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -lm -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET) $(ARG)

clean:
	rm -f $(OBJ) $(TARGET)

re: clean all
