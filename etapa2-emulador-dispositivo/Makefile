# Nome do executável
TARGET = Main.exe

# Arquivos fonte
SRC = Main.c

# Compilador
CC = /c/msys64/mingw64/bin/gcc.exe

# Flags de compilação
CFLAGS = -fdiagnostics-color=always -g

# Diretórios de inclusão e bibliotecas
INCLUDES = -I/c/msys64/mingw64/include
LDFLAGS = -L/c/msys64/mingw64/lib

# Bibliotecas para linkagem
LIBS = -lmbedtls -lmbedcrypto -lmbedx509

# Regra padrão
all: $(TARGET)

# Regra de compilação
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Limpeza dos arquivos gerados
clean:
	rm -f $(TARGET)
