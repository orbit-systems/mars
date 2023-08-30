all: build

SRC_DIR = ./src
MARS_LOCATION = ./build/mars
MARS_BUILD_FLAGS = -o:speed -no-bounds-check

ifeq ($(OS),Windows_NT)
	MARS_LOCATION = ./build/mars.exe
endif

clean:
	@rm -rf ./build

build: clean
	@mkdir build
	@odin build $(SRC_DIR) $(MARS_BUILD_FLAGS) -out:$(MARS_LOCATION)

run: build 
	@$(MARS_LOCATION) mars_code/test.mars -no-color

stresstest: build 
	@$(MARS_LOCATION) mars_code/stresstest.mars -no-color