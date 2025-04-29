# Nome do alvo final
TARGET = Main.exe

# Arquivos fonte
SRC = Main.c lorawan.c aes_cmac.c Config.c

# Caminho do compilador GCC
CC = /usr/bin/gcc

# Flags de compilação
CFLAGS = -fdiagnostics-color=always -g

# Diretórios de inclusão para cabeçalhos (diretórios onde o compilador busca os arquivos .h)
INCLUDES = -I$(shell pwd) -I/usr/include

# Diretórios de linkedição (onde o linker busca as bibliotecas .so ou .a)
# Caminho para as bibliotecas mbedtls
LDFLAGS = -L/usr/lib/x86_64-linux-gnu


# Bibliotecas para linkedição
LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcurl -lcjson


# Regra padrão para compilar o programa
all: $(TARGET)

# Como criar o executável a partir dos arquivos fonte
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Limpeza dos arquivos gerados (executáveis e objetos)
clean:
	rm -f $(TARGET)
