CC = gcc
CFLAGS = -I../../ -Wall -Wextra -O2
TARGET = example
SRC = example.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) output.raw
