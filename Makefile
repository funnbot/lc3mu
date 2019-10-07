CC		:= g++
C_FLAGS := -std=c++17 -o0 -g

BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib

file    :=
LIBRARIES	:=

EXECUTABLE	:= main

all: $(BIN)/$(EXECUTABLE)

clean:
	$(RM) $(BIN)/$(EXECUTABLE)

run: all
	./$(BIN)/$(EXECUTABLE) $(file)

$(BIN)/$(EXECUTABLE): $(SRC)/*
	$(CC) $(C_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)