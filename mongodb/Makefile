include ../config.mk

OBJ_FOLDER = ./objs
BIN_FOLDER = ../bin

INCLUDE = -I../include \
		  -I../include/libbson \
		  -I../include/libmongoc

LIB = -L../lib \
	  -L../lib/mongo-c-driver \
	  -lbson-1.0 \
	  -lmongoc-1.0

.PHONY: build, clean

build: init_mongodb

init_mongodb: init_mongodb.o
	$(CC) $(LDFLAGS) $< $(LIB) -o $@
	@mkdir -p $(OBJ_FOLDER)
	@mv $< $(OBJ_FOLDER)
	@mv $@ $(BIN_FOLDER)

init_mongodb.o: init_mongodb.cc
	$(CC) $(INCLUDE) $(CFLAGS) $< -o $@

clean:
	@rm -f $(OBJ_FOLDER)/*
	@rm -f $(BIN_FOLDER)/init_mongodb
