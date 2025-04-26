TARGET = Main.exe

SRC = main.c lorawan.c aes_cmac.c config.c

CC = /c/msys64/mingw64/bin/gcc.exe

CFLAGS = -fdiagnostics-color=always -g

INCLUDES = -I/c/msys64/mingw64/include

LDFLAGS = -L/c/msys64/mingw64/lib

LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcurl -lcjson -lws2_32

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
