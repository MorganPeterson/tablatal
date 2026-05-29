CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=

BIN := tablatal
SRC := \
	src/main.c \
	src/tbt_schema.c \
	src/tbt_init.c \
	src/tbt_file.c \
	src/tbt_pair.c \
	src/tbt_row.c \
	src/tbt_cmd.c

OBJ := $(SRC:.c=.o)

.PHONY: all clean install uninstall test

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c src/tbt.h
	$(CC) $(CFLAGS) -c $< -o $@

install: $(BIN)
	install -Dm755 $(BIN) /usr/local/bin/$(BIN)

uninstall:
	rm -f /usr/local/bin/$(BIN)

clean:
	rm -f $(OBJ) $(BIN)

test: $(BIN)
	SKIP_BUILD=1 ./test.sh
