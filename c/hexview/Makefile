CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -pedantic -std=c99 -O2
TARGET   = hexview
SRC_DIR  = src
BIN_DIR  = bin
SRCS     = $(SRC_DIR)/hexview.c
OBJS     = $(SRCS:.c=.o)

.PHONY: all clean install

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(SRC_DIR)/*.o $(BIN_DIR)

install: $(BIN_DIR)/$(TARGET)
	install -d /usr/local/bin
	install -m 755 $(BIN_DIR)/$(TARGET) /usr/local/bin/
