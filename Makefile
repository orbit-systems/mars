SRC_DIR = ./src
MARS_LOCATION = ./build/mars
MARS_BUILD_FLAGS = -o:speed

ifeq ($(OS),Windows_NT)
	MARS_LOCATION = ./build/mars.exe
endif

clean:
	rm -rf ./build

build: clean
	mkdir build
	odin build $(SRC_DIR) $(MARS_BUILD_FLAGS) -out:$(MARS_LOCATION)