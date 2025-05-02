TARGET = Main.exe

SRC = Main.c

CC = /usr/bin/gcc

CFLAGS = -fdiagnostics-color=always -g

INCLUDES = -I/c/msys64/mingw64/include
LDFLAGS = -L/c/msys64/mingw64/lib

LIBS = -lcurl

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
