CC		:= g++
C_FLAGS := -std=c++17 -Wall -Wextra -g

BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib

file    :=
LIBRARIES	:=

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= lc3mu.exe
else
EXECUTABLE	:= lc3mu
endif

all: $(BIN)/$(EXECUTABLE)

clean:
	$(RM) $(BIN)/$(EXECUTABLE)

run: all
	./$(BIN)/$(EXECUTABLE) $(file)

$(BIN)/$(EXECUTABLE): $(SRC)/*
	$(CC) $(C_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)