# Nome dos alvos finais
TARGETS = bin/Gateway.exe bin/Main.exe

# Caminho do compilador GCC
CC = /usr/bin/gcc

# Flags de compilação
CFLAGS = -fdiagnostics-color=always -g -Wall -Wextra

# Diretórios de inclusão para cabeçalhos
INCLUDES = -Iinclude -I/usr/include

# Diretórios de linkedição
LDFLAGS = -L/usr/lib/x86_64-linux-gnu

# Bibliotecas para linkedição
LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcjson -lcurl

# Diretórios de origem e binários
SRC_DIR = src
BIN_DIR = bin

# Arquivos fonte para cada alvo
GATEWAY_SRC = $(SRC_DIR)/Gateway.c
MAIN_SRC = $(SRC_DIR)/Main.c $(SRC_DIR)/lorawan.c $(SRC_DIR)/aes_cmac.c $(SRC_DIR)/Config.c

# Regra padrão para compilar todos os alvos
all: $(TARGETS)


# Regra para compilar o Gateway
bin/Gateway.exe: $(GATEWAY_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

# Regra para compilar o Main
bin/Main.exe: $(MAIN_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LIBS)

# Limpeza dos arquivos gerados
clean:
	rm -rf $(BIN_DIR)