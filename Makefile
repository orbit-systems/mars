TARGETS = aphelion 

SRCPATHS = \
	src/common/*.c \
	src/mars/*.c \
	src/mars/phobos/*.c \
	src/iron/*.c \
	src/iron/passes/*.c \
	src/iron/passes/analysis/*.c \
	src/iron/passes/transform/*.c \
	src/iron/targets/*.c \
	src/llta/*.c \

SRCPATHS += $(foreach target, $(TARGETS), src/iron/targets/$(target)/*.c) 
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

INCLUDEPATHS = -Isrc
DEBUGFLAGS = -lm -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -MD -Wall -Wno-format -Wno-unused-value -Werror=incompatible-pointer-types -Wno-discarded-qualifiers -lm
OPT = -O3

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