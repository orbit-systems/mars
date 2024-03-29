SRCPATHS = src/*.c src/phobos/*.c src/deimos/*.c
SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = mars

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = mars.exe
endif

CC = gcc
LD = gcc

DEBUGFLAGS = -lm -rdynamic -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
<<<<<<< HEAD
DONTBEAFUCKINGIDIOT = -Wall -Wextra -pedantic -Wno-missing-field-initializers -Wno-unused-result
CFLAGS = -Wincompatible-pointer-types
OPT = -O0
=======
DONTBEAFUCKINGIDIOT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-result
CFLAGS = -O0 -Wincompatible-pointer-types -lm
>>>>>>> 5f8ffca (pushing changes because im not gonna be at my pc for a bit)

all: build

build/%.o: src/%.c
	@echo compiling $<
	@$(CC) -c -o $@ $< -Isrc/ -MD $(CFLAGS) $(OPT)

build: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/

	@echo linking with $(LD)
<<<<<<< HEAD
	@$(LD) -lm $(OPT) $(OBJECTS) -o $(EXECUTABLE_NAME)
=======
	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME) $(CFLAGS)
>>>>>>> 5f8ffca (pushing changes because im not gonna be at my pc for a bit)

	@echo $(EXECUTABLE_NAME) built
	

dbgbuild/%.o: src/%.c
	@$(CC) -c -o $@ $< -Isrc/ -MD $(DEBUGFLAGS)

dbgbuild: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/

	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME) $(DEBUGFLAGS)
	
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