TARGETS = bin/Gateway.exe bin/Main.exe

CC = /usr/bin/gcc

CFLAGS = -fdiagnostics-color=always -g -Wall -Wextra

INCLUDES = -Iinclude -I/usr/include

LDFLAGS = -L/usr/lib/x86_64-linux-gnu

LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcjson -lcurl

SRC_DIR = src
BIN_DIR = bin

GATEWAY_SRC = $(SRC_DIR)/Gateway.c
MAIN_SRC = $(SRC_DIR)/Main.c $(SRC_DIR)/lorawan.c $(SRC_DIR)/aes_cmac.c $(SRC_DIR)/Config.c

all: $(TARGETS)

bin/Gateway.exe: $(GATEWAY_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

bin/Main.exe: $(MAIN_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BIN_DIR)