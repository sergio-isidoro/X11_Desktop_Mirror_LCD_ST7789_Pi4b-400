# Executable name
TARGET = mirror_screen

# Compiler
CC = gcc

# Compilation options (O3 for performance optimization)
CFLAGS = -Wall -O3

# Required libraries
LIBS = -lX11 -lXext -lbcm2835

# The source file (adjust if the name is different)
SRC = mirror_screen.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)
