CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -std=c11
SRC = src/main.c src/neural_net.c src/utils.c
OBJ = $(SRC:.c=.o)
TARGET = ocr_demo

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) -lm

ocr_test: src/ocr_test.o src/neural_net.o src/utils.o
	$(CC) -Iinclude -Wall -Wextra -std=c11 -o ocr_test src/ocr_test.o src/neural_net.o src/utils.o -lm

poc:
	gcc -Iinclude -Wall -Wextra -std=c11 src/poc_xor.c src/neural_net.c src/utils.c -lm -o poc_xor

clean:
	rm -f $(OBJ) $(TARGET)

