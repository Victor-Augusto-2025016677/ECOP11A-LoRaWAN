TARGET = Gateway.exe

SRC = Gateway.c

CC = /usr/bin/gcc

CFLAGS = -fdiagnostics-color=always -g

INCLUDES = -I$(shell pwd) -I/usr/include

LDFLAGS = -L/usr/lib/x86_64-linux-gnu

LIBS = -lmbedtls -lmbedcrypto -lmbedx509 -lcjson

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
