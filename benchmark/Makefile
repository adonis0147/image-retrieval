include ../config.mk

SRC_FILES = $(wildcard *.cc)
OBJ_FILES = $(patsubst %.cc, %.o, $(SRC_FILES))
LIB_FOLDER = ../lib
LIB_FILES = $(wildcard $(LIB_FOLDER)/*.a)
BIN_FOLDER = ../bin

BIN_FILE = benchmark

INCLUDE = -I../include

.PHONY: build, clean

build: $(OBJ_FILES)
	$(CC) $(LFLAG) $(INCLUDE) $^ $(LIB_FILES) -o $(BIN_FILE)
	@mkdir -p $(BIN_FOLDER)
	@mv $(BIN_FILE) $(BIN_FOLDER)
	@mkdir -p ./objs
	@mv $(OBJ_FILES) ./objs

%.o: %.cc
	$(CC) $(CFLAG) $(INCLUDE) $< -o $@

clean:
	@rm -rf ./objs/*
	@rm -rf $(BIN_FOLDER)/$(BIN_FILE)
