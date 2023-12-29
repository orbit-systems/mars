SRCPATHS = src/*.c
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
CFLAGS = -O3
SHUTTHEFUCKUP = -Wno-unknown-warning-option -Wno-incompatible-pointer-types-discards-qualifiers -Wno-initializer-overrides -Wno-discarded-qualifiers

build/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -MD $(SHUTTHEFUCKUP)

build: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE_NAME) $(CFLAGS) -MD

debug:
	$(DEBUGFLAGS) $(DONTBEAFUCKINGIDIOT)

clean:
	rm -f build/*

-include $(OBJECTS:.o=.d)