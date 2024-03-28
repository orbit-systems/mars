SRCPATHS = src/*.c src/phobos/*.c src/deimos/*.c
SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = mars

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = mars.exe
endif

CC = gcc
LD = gcc

DEBUGFLAGS = -g -O0
ASANFLAGS = -fsanitize=undefined -fsanitize=address
DONTBEAFUCKINGIDIOT = -Wall -Wextra -pedantic -Wno-missing-field-initializers -Wno-unused-result
CFLAGS = -Wincompatible-pointer-types
OPT = -O0

all: build

build/%.o: src/%.c
	@echo compiling $<
	@$(CC) -c -o $@ $< -Isrc/ -MD $(CFLAGS) $(OPT)

build: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/

	@echo linking with $(LD)
	@$(LD) -lm $(OPT) $(OBJECTS) -o $(EXECUTABLE_NAME)

	@echo $(EXECUTABLE_NAME) built
	

dbgbuild/%.o: src/%.c
	@$(CC) -c -o $@ $< -Isrc/ -MD $(DEBUGFLAGS) -lm

dbgbuild: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/

	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME) $(DEBUGFLAGS) -lm
	
test: build
	./$(EXECUTABLE_NAME) ./mars_code

test2: build
	./$(EXECUTABLE_NAME) ./test

clean:
	@rm -rf build
	@mkdir build
	@mkdir build/deimos
	@mkdir build/phobos

printbuildinfo:
	@echo using $(CC) with flags $(CFLAGS) $(OPT)

new: clean printbuildinfo build

-include $(OBJECTS:.o=.d)