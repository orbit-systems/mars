all: build

SRC_DIR = ./src
MARS_LOCATION = ./build/mars
MARS_BUILD_FLAGS = 

MARS_EXEC_FLAGS = 

ifeq ($(OS),Windows_NT)
	MARS_LOCATION = ./build/mars.exe
	MARS_EXEC_FLAGS = # -no-color
endif

# BRUH LMAO

clean:
	@rm -rf ./build

release: clean
	@mkdir build
	@odin build $(SRC_DIR) $(MARS_BUILD_FLAGS) -out:$(MARS_LOCATION) -o:speed

build: clean
	@mkdir build
	@odin build $(SRC_DIR) $(MARS_BUILD_FLAGS) -out:$(MARS_LOCATION)

run: build 
	@$(MARS_LOCATION) ./mars_code $(MARS_EXEC_FLAGS)

test: build
	@$(MARS_LOCATION) ./test $(MARS_EXEC_FLAGS)