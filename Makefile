all: build

SRC_DIR = ./src
MARS_LOCATION = ./build/mars
MARS_BUILD_FLAGS = -o:speed -no-bounds-check

MARS_EXEC_FLAGS = 

ifeq ($(OS),Windows_NT)
	MARS_LOCATION = ./build/mars.exe
	MARS_EXEC_FLAGS = -no-color
endif



clean:
	@rm -rf ./build

build: clean
	@mkdir build
	@odin build $(SRC_DIR) $(MARS_BUILD_FLAGS) -out:$(MARS_LOCATION)

run: build 
	@$(MARS_LOCATION) mars_code/test.mars $(MARS_EXEC_FLAGS)

stresstest: build 
	@$(MARS_LOCATION) mars_code/stresstest.mars $(MARS_EXEC_FLAGS)