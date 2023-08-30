all: build

SRC_DIR = ./src
MARS_LOCATION = ./build/mars
MARS_BUILD_FLAGS = 

ifeq ($(OS),Windows_NT)
	MARS_LOCATION = ./build/mars.exe
endif

clean:
	rm -rf ./build

build: clean
	mkdir build
	odin build $(SRC_DIR) $(MARS_BUILD_FLAGS) -out:$(MARS_LOCATION)

run: build 
	$(MARS_LOCATION) spec/test.mars

stresstest: build 
	$(MARS_LOCATION) spec/stresstest.mars