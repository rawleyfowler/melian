.DEFAULT_GOAL := all

OS := $(shell uname -s)

CC = cc
LD = cc

AFLAGS += -std=c11
AFLAGS += -g
# AFLAGS += -O

ifeq ($(OS),Darwin)
# Linux also has sanitizers, but valgrind is more valuable there.
# Note: thread and address cannot be used together
# Note: memory is not supported on M1
# AFLAGS += -fsanitize=undefined
# AFLAGS += -fsanitize=address
# AFLAGS += -fsanitize=thread
# AFLAGS += -fsanitize=memory
endif

BREW_DIR = /opt/homebrew
TAP_DIR = $(HOME)/src/gonzo/libtap

CFLAGS += $(AFLAGS)
CFLAGS += -c
CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -I.
CFLAGS += -I/usr/local/include
CFLAGS += -I$(TAP_DIR)
ifeq ($(OS),Linux)
CFLAGS += -D_GNU_SOURCE
CFLAGS += -D_XOPEN_SOURCE
endif
ifeq ($(OS),Darwin)
CFLAGS += -I$(BREW_DIR)/opt/mysql-client/include
endif

LDFLAGS += $(AFLAGS)
LDFLAGS += -L.
LDFLAGS += -L/usr/local/lib
LDFLAGS += -L$(TAP_DIR)
ifeq ($(OS),Darwin)
LIBS_SERVER += -L$(BREW_DIR)/lib
LIBS_CLIENT += -L$(BREW_DIR)/lib
LIBS_SERVER += -L$(BREW_DIR)/opt/mysql-client/lib
endif
LIBS_SERVER += -levent -lmysqlclient
LIBS_CLIENT += -ljansson -lm

C_SRC_SERVER = \
	server/util.c \
	server/log.c \
	server/json.c \
	server/arena.c \
	server/xxhash.c \
	server/hash.c \
	server/server.c \
	server/config.c \
	server/status.c \
	server/data.c \
	server/db.c \
	server/cron.c \
	server/melian-server.c \

C_SRC_CLIENT = \
	clients/c/client.c \
	clients/c/melian-client.c \

C_OBJ_SERVER = $(C_SRC_SERVER:.c=.o)
C_OBJ_CLIENT = $(C_SRC_CLIENT:.c=.o)

C_SRC_TEST = $(wildcard t/*.c)
C_OBJ_TEST = $(patsubst %.c, %.o, $(C_SRC_TEST))
C_EXE_TEST = $(patsubst %.c, %, $(C_SRC_TEST))

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

server/melian-server: $(C_OBJ_SERVER) ## build melian server
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS_SERVER)

clients/c/melian-client: $(C_OBJ_CLIENT) ## build melian client
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS_CLIENT)

$(C_EXE_TEST): %: %.o
	$(CC) $(LDFLAGS) -o $@ $^ -ltap

tests: $(C_EXE_TEST) ## build all tests

test: tests ## run all tests
	@for t in $(C_EXE_TEST); do ./$$t; done

ifeq ($(OS),Linux)
# Linux has valgrind!
valgrind: tests ## run all tests under valgrind (Linux only)
	@for t in $(C_EXE_TEST); do valgrind --leak-check=full ./$$t; done
endif

all: server/melian-server clients/c/melian-client  ## (re)build everything

clean:  ## clean everything
	rm -fr server/melian-server server/melian-server.dSYM $(C_OBJ_SERVER)
	rm -fr clients/c/melian-client clients/c/melian-client.dSYM $(C_OBJ_CLIENT)
	rm -f $(C_OBJ_TEST) $(C_EXE_TEST)

help: ## display this help
	@grep -E '^[-_/a-zA-Z ]+:.*?## .*$$' /dev/null $(MAKEFILE_LIST) | sort | awk -F: '{ sub(/.*##/, "", $$3); printf("\033[36;1m%-30s\033[0m %s\n", $$2, $$3); }'

.PHONY: all clean help test tests valgrind
