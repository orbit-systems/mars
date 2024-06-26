TARGETS = aphelion 

SRCPATHS = \
	src/*.c \
	src/mars/*.c \
	src/mars/phobos/*.c \
	src/mars/phobos/parse/*.c \
	src/mars/phobos/analysis/*.c \
	src/atlas/*.c \
	src/atlas/passes/*.c \
	src/atlas/passes/analysis/*.c \
	src/atlas/passes/transform/*.c \
	src/atlas/targets/*.c \
	src/llta/*.c \

SRCPATHS += $(foreach target, $(TARGETS), src/atlas/targets/$(target)/*.c) 
SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = mars
ECHO = echo

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = mars.exe
else
	ECHO = /usr/bin/echo
	# JANK FIX FOR SANDWICH'S DUMB ECHO ON HIS LINUX MACHINE
endif

CC = gcc
LD = gcc

INCLUDEPATHS = -Isrc/ -Isrc/mars/ -Isrc/mars/phobos/ -Isrc/atlas/ -Isrc/atlas/targets
DEBUGFLAGS = -lm -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -MD -Wincompatible-pointer-types -Wno-discarded-qualifiers -lm -Wno-deprecated-declarations -Wreturn-type
OPT = -O2

FILE_NUM = 0

build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	@$(CC) -c -o $@ $< $(INCLUDEPATHS) $(CFLAGS) $(OPT)

build: $(OBJECTS)
	@echo Linking with $(LD)...
	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME) $(CFLAGS)
	@echo Successfully built: $(EXECUTABLE_NAME)

debug: CFLAGS += $(DEBUGFLAGS)
debug: OPT = -O0
debug: build

clean:
	@rm -rf build/
	@mkdir build/
	@mkdir -p $(dir $(OBJECTS))

cleanbuild: clean build

-include $(OBJECTS:.o=.d)