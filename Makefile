IRON_SRC_PATHS = \
	src/iron/*.c \
	src/iron/opt/*.c \
	src/iron/xr17032/*.c \
	src/iron/mir/*.c \
	src/common/vec.c

COYOTE_SRC_PATHS = \
	src/coyote/*.c \
	src/common/*.c \

COBALT_SRC_PATHS = \
	src/cobalt/*.c \
	src/common/*.c \
	
MARS_SRC_PATHS = \
	src/mars/*.c \
	src/common/*.c \

IRON_SRC = $(wildcard $(IRON_SRC_PATHS))
IRON_OBJECTS = $(IRON_SRC:src/%.c=build/%.o)

COYOTE_SRC = $(wildcard $(COYOTE_SRC_PATHS))
COYOTE_OBJECTS = $(COYOTE_SRC:src/%.c=build/%.o)

COBALT_SRC = $(wildcard $(COBALT_SRC_PATHS))
COBALT_OBJECTS = $(COBALT_SRC:src/%.c=build/%.o)

MARS_SRC = $(wildcard $(MARS_SRC_PATHS))
MARS_OBJECTS = $(MARS_SRC:src/%.c=build/%.o)

CC = gcc
LD = gcc

INCLUDEPATHS = -Iinclude/
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -std=gnu23 -fwrapv -fno-strict-aliasing
WARNINGS = -Wall -Wimplicit-fallthrough -Wno-override-init -Wno-enum-compare -Wno-unused -Wno-enum-conversion -Wno-discarded-qualifiers -Wno-strict-aliasing
ALLFLAGS = $(CFLAGS) $(WARNINGS)
OPT = -g3 -O0

LDFLAGS =

ifneq ($(OS),Windows_NT)
	CFLAGS += -rdynamic
endif

ifdef ASAN_ENABLE
	CFLAGS += $(ASANFLAGS)
	LDFLAGS += $(ASANFLAGS)
endif

FILE_NUM = 0
build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell echo 1>&2 -e "Compiling \e[1m$<\e[0m")
	
	@$(CC) -c -o $@ $< -MD $(INCLUDEPATHS) $(ALLFLAGS) $(OPT)

.PHONY: coyote
coyote: bin/coyote
bin/coyote: bin/libiron.a $(COYOTE_OBJECTS)
	@$(LD) $(LDFLAGS) $(COYOTE_OBJECTS) -o bin/coyote -Lbin -liron

.PHONY: cobalt
cobalt: bin/cobalt
bin/cobalt: bin/libiron.a $(COBALT_OBJECTS)
	@$(LD) $(LDFLAGS) $(COBALT_OBJECTS) -o bin/cobalt -Lbin -liron

.PHONY: mars
mars: bin/mars
bin/mars: bin/libiron.a $(MARS_OBJECTS)
	@$(LD) $(LDFLAGS) $(MARS_OBJECTS) -o bin/mars -Lbin -liron -lm

.PHONY: iron-test
iron-test: bin/iron-test
bin/iron-test: bin/libiron.a src/iron/driver/driver.c
	@$(CC) src/iron/driver/driver.c -o bin/iron-test $(INCLUDEPATHS) $(CFLAGS) $(OPT) -Lbin -liron

bin/libiron.o: $(IRON_OBJECTS)
	@$(LD) $(LDFLAGS) $(IRON_OBJECTS) -r -o bin/libiron.o

.PHONY: libiron
libiron: bin/libiron.a
bin/libiron.a: bin/libiron.o
	@ar rcs bin/libiron.a bin/libiron.o

.PHONY: clean
clean:
	@rm -rf build/
	@rm -rf bin/
	@mkdir build/
	@mkdir bin/
	@mkdir -p $(dir $(IRON_OBJECTS))
	@mkdir -p $(dir $(COYOTE_OBJECTS))
	@mkdir -p $(dir $(COBALT_OBJECTS))
	@mkdir -p $(dir $(MARS_OBJECTS))

-include $(IRON_OBJECTS:.o=.d)
-include $(COYOTE_OBJECTS:.o=.d)
-include $(COBALT_OBJECTS:.o=.d)
-include $(MARS_OBJECTS:.o=.d)

# generate compile commands with bear if u got it!!! 
# very good highly recommended ʕ·ᴥ·ʔ
.PHONY: bear-gen-cc
bear-gen-cc: clean
	bear -- $(MAKE) coyote iron-test
