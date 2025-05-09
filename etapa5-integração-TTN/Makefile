TARGETS = bin/Gateway.exe bin/Main.exe bin/Controlador.exe bin/Envio.exe

CC = /usr/bin/gcc

CFLAGS = -fdiagnostics-color=always -g -Wall -Wextra

INCLUDES = -Iinclude -I/usr/include

LDFLAGS = -L/usr/lib/x86_64-linux-gnu

LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcjson -lcurl

SRC_DIR = src
BIN_DIR = bin
log_DIR = logs
out_DIR = out

GATEWAY_SRC = $(SRC_DIR)/Gateway.c
ENVIO_SRC = $(SRC_DIR)/Envio.c
CONTROLADOR_SRC = $(SRC_DIR)/Controlador.c
MAIN_SRC = $(SRC_DIR)/Main.c $(SRC_DIR)/lorawan.c $(SRC_DIR)/aes_cmac.c $(SRC_DIR)/Config.c

all: $(TARGETS)

bin/Gateway.exe: $(GATEWAY_SRC)
	mkdir -p $(log_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(out_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

bin/Main.exe: $(MAIN_SRC)
	mkdir -p $(log_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(out_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

bin/Envio.exe: $(ENVIO_SRC)
	mkdir -p $(log_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(out_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

bin/Controlador.exe: $(CONTROLADOR_SRC)
	mkdir -p $(log_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(out_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BIN_DIR)

clean2:
	rm -rf $(log_DIR)

clean3:
	rm -rf $(out_DIR)