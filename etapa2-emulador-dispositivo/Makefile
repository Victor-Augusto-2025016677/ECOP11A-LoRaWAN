# Nome do executável
TARGET = Main.exe

# Arquivos fonte
SRC = main.c lorawan.c aes_cmac.c config.c

# Compilador (ajustado para seu caminho no MSYS2)
CC = /c/msys64/mingw64/bin/gcc.exe

# Flags de compilação
CFLAGS = -fdiagnostics-color=always -g

# Diretórios de inclusão e bibliotecas
INCLUDES = -I/c/msys64/mingw64/include

# Diretórios de linkagem
LDFLAGS = -L/c/msys64/mingw64/lib

# Bibliotecas para linkagem (🚨 aqui adicionamos -lcjson)
LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcurl -lcjson

# Regra padrão
all: $(TARGET)

# Regra de compilação
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Limpeza dos arquivos gerados
clean:
	rm -f $(TARGET)
