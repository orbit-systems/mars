TARGETS = aphelion 

SRCPATHS = \
	src/*.c \
	src/phobos/*.c \
	src/deimos/*.c \
	src/deimos/passes/*.c \
	src/deimos/passes/analysis/*.c \
	src/deimos/passes/transform/*.c \
	src/deimos/targets/*.c \

SRCPATHS += $(foreach target, $(TARGETS), src/deimos/targets/$(target)/*.c) 
SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = mars

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = mars.exe
endif

CC = gcc
LD = gcc

INCLUDEPATHS = -Isrc/ -Isrc/phobos/ -Isrc/deimos/ -Isrc/deimos/targets
DEBUGFLAGS = -lm -rdynamic -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -Wincompatible-pointer-types -Wno-discarded-qualifiers -lm
OPT = -O2

FILE_NUM = 0

build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell echo 1>&2 "-e" "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m] Compiling \e[1m$<\e[0m")
	@$(CC) -c -o build/$(notdir $@) $< $(INCLUDEPATHS) $(CFLAGS) $(OPT)

build: $(OBJECTS)
	@echo Linking into $(EXECUTABLE_NAME)
	@$(LD) $(foreach obj, $(notdir $(OBJECTS)), build/$(obj)) -o $(EXECUTABLE_NAME) $(CFLAGS)
	@echo Successfully built: $(EXECUTABLE_NAME)

debug: CFLAGS += $(DEBUGFLAGS)
debug: OPT = -O0
debug: build

clean:
	@rm -rf build/
	@mkdir build/

cleanbuild: clean build