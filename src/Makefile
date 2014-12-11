include ../config.mk

SRC_FILES = $(wildcard *.cc)
OBJ_FILES = $(patsubst %.cc, %.o, $(SRC_FILES))
OBJ_FOLDER = ../objs

INCLUDE = -I../include

.PHONY: build, clean

build: $(OBJ_FILES)
	@mkdir -p $(OBJ_FOLDER)
	@mv $(OBJ_FILES) $(OBJ_FOLDER)

%.o: %.cc
	$(CC) $(CFLAG) $(INCLUDE) $< -o $@

clean:
	@rm -rf $(OBJ_FILES)
	@rm -rf $(OBJ_FOLDER)/*
