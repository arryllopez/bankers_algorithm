CC     = gcc
CFLAGS = -pthread -Wall -Wextra

TARGET = group27_bankers
SRC    = group27_bankers.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
