TARGET = Main.exe

SRC = Main.c lorawan.c aes_cmac.c Config.c

CC = /usr/bin/gcc

CFLAGS = -fdiagnostics-color=always -g

INCLUDES = -I$(shell pwd) -I/usr/include

LDFLAGS = -L/usr/lib/x86_64-linux-gnu

LIBS =

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)