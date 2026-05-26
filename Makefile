CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
SRC_DIR = src
OBJ_DIR = obj
TARGET = minihttpd

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/server.c $(SRC_DIR)/http.c $(SRC_DIR)/mime.c $(SRC_DIR)/files.c
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/server.o $(OBJ_DIR)/http.o $(OBJ_DIR)/mime.o $(OBJ_DIR)/files.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean