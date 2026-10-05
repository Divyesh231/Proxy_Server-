CC ?= gcc
CFLAGS ?= -std=gnu11 -Wall -Wextra -pthread

TARGET := proxy_server
SOURCES := src/server.c src/http.c src/forwarding.c src/cache.c src/accesscontrol.c src/logger.c

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SOURCES) src/proxy.h

	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)

	./$(TARGET)

test: $(TARGET)

	bash test/local_smoke_test.sh

clean:

	rm -f $(TARGET)
