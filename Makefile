CC     = gcc
CFLAGS = -pthread -Wall -Wextra

TARGET = group0_bankers
SRC    = group0_bankers.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
