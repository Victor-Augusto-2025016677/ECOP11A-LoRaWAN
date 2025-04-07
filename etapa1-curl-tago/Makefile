# Nome do executável
TARGET = Main

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g -fdiagnostics-color=always

# Caminhos de includes e libs
INCLUDES = -I"C:/msys64/mingw64/include"
LIBS = -L"C:/msys64/mingw64/lib" -lcurl

# Fonte
SRCS = Main.c
OBJS = $(SRCS:.c=.o)

# Compilação
all: $(TARGET).exe

$(TARGET).exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES) $(LIBS)

# Limpar
clean:
	rm -f *.o *.exe

# Recompilar do zero
rebuild: clean all
