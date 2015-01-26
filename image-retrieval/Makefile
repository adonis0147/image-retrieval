include ../config.mk

SRC_FILES = $(wildcard *.cc)
OBJ_FILES = $(patsubst %.cc, %.o, $(SRC_FILES))
LIB_FOLDER = ../lib
LIB_FILES = $(wildcard $(LIB_FOLDER)/*.a)
BIN_FOLDER = ../bin

BIN_FILE = ir

INCLUDE = -I../include \
		  -I../include/libevent
LIB = -L../lib/libevent \
	  -levent

.PHONY: build, clean

build: $(OBJ_FILES)
	$(CC) $(LDFLAGS) $< $(LIB_FILES) $(LIB) -o $(BIN_FILE)
	@mkdir -p $(BIN_FOLDER)
	@mv $(BIN_FILE) $(BIN_FOLDER)
	@mkdir -p ./objs
	@mv $(OBJ_FILES) ./objs

%.o: %.cc
	$(CC) $(INCLUDE) $(CFLAGS) $< -o $@

clean:
	@rm -f ./objs/*
	@rm -f $(BIN_FOLDER)/$(BIN_FILE)
