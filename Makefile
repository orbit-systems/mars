SRCPATHS = src/*.c src/deimos/*.c src/phobos/*.c
SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = mars

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = mars.exe
endif

CC = gcc
LD = gcc

DEBUGFLAGS = -g -rdynamic -pg
ASANFLAGS = -fsanitize=undefined -fsanitize=address
DONTBEAFUCKINGIDIOT = -Wall -Wextra -pedantic -Wno-missing-field-initializers -Wno-unused-result
CFLAGS = -O3 -Wincompatible-pointer-types

build/%.o: src/%.c
	@echo compiling $<
	@$(CC) -c -o $@ $< -Isrc/ -MD $(CFLAGS)

build: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/

	@echo linking with $(LD)
	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME)

	@echo $(EXECUTABLE_NAME) built
	

dbgbuild/%.o: src/%.c
	@$(CC) -c -o $@ $< -Isrc/ -MD $(DEBUGFLAGS)

dbgbuild: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/

	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME)
	

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
	@echo using $(CC) with flags $(CFLAGS)

new: clean printbuildinfo build

-include $(OBJECTS:.o=.d)