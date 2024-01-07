SRCPATHS = src/*.c src/deimos/*.c src/phobos/*.c
SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = mars

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = mars.exe
endif

CC = gcc

DEBUGFLAGS = -g -rdynamic -pg
ASANFLAGS = -fsanitize=undefined -fsanitize=address
DONTBEAFUCKINGIDIOT = -Werror -Wall -Wextra -pedantic -Wno-missing-field-initializers -Wno-unused-result
CFLAGS = -Isrc/ -O3
SHUTTHEFUCKUP = -Wno-unknown-warning-option -Wno-incompatible-pointer-types-discards-qualifiers -Wno-initializer-overrides -Wno-discarded-qualifiers

build/%.o: src/%.c
	@echo $<
	@$(CC) -c -o $@ $< $(CFLAGS) -MD $(SHUTTHEFUCKUP)

build: $(OBJECTS)
	@-cp build/deimos/* build/
	@-cp build/phobos/* build/
	@echo linking...
	@$(CC) $(OBJECTS) -o $(EXECUTABLE_NAME) $(CFLAGS) -MD
	

test: build
	./$(EXECUTABLE_NAME) ./mars_code test

test2: build
	./$(EXECUTABLE_NAME) ./test test

debug:
	$(DEBUGFLAGS) $(DONTBEAFUCKINGIDIOT)

clean:
	@echo cleaning build/
	@rm -rf build
	@mkdir build
	@mkdir build/deimos
	@mkdir build/phobos

printbuildinfo:
	@echo using $(CC) with flags $(CFLAGS) $(SHUTTHEFUCKUP)

new: clean printbuildinfo build

-include $(OBJECTS:.o=.d)