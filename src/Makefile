include ../config.mk

SRC_FILES = $(wildcard *.cc)
OBJ_FILES = $(patsubst %.cc, %.o, $(SRC_FILES))
LIB_FILES = $(patsubst %.o, %.a, $(OBJ_FILES))
OBJ_FOLDER = ./objs
LIB_FOLDER = ../lib

INCLUDE = -I../include

.PHONY: build, clean

build: $(LIB_FILES)
	@mkdir -p $(OBJ_FOLDER)
	@mv $(OBJ_FILES) $(OBJ_FOLDER)
	@mkdir -p $(LIB_FOLDER)
	@mv $(LIB_FILES) $(LIB_FOLDER)

%.a: %.cc
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $(patsubst %.a, %.o, $@)
	@ar rvs $@ $(patsubst %.a, %.o, $@)

clean:
	@rm -rf $(OBJ_FILES)
	@rm -rf $(OBJ_FOLDER)/*
	@rm -rf $(LIB_FILES)
	@rm -rf $(LIB_FOLDER)/*
