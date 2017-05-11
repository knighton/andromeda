CXX := clang++
SRC_ROOT := src
BIN_DIR := bin
BUILD_DIR := build

MY_CXXFLAGS = \
	-Wno-c++98-compat-pedantic \
	-Wno-unused-parameter \
	-Wno-float-equal \
	-Wno-old-style-cast \
	-Wno-format-nonliteral \
	-Wno-padded \
	-Wno-weak-vtables \
	-Wno-unused-function \
	-Wno-double-promotion \
	-Wno-unused-exception-parameter \
	-Wno-exit-time-destructors \
	-Wno-cast-align \
	-Wno-packed \
	-Wno-unused-member-function

CXXFLAGS = \
	-std=gnu++14 \
	-O3 \
	-Wpedantic \
	-Wall \
	-Wextra \
	-Weverything \
	-I$(SRC_ROOT)/ \
	$(MY_CXXFLAGS)

LDFLAGS =

LDLIBS = \
	-lpthread \
	-larmadillo

# Convention:
# * .cpp for sources that will compile to a corresponding binary
# * .cc for all other sources
CC_SOURCES := $(shell find $(SRC_ROOT) -type f -name "*.cc" | \
                find src -path "*/env" -prune -o -type f -name "*.cc" -print)
CPP_SOURCES := $(shell find $(SRC_ROOT) -type f -name "*.cpp" | \
                 find src -path "*/env" -prune -o -type f -name "*.cpp" -print)
OBJECTS := $(patsubst $(SRC_ROOT)/%.cc, $(BUILD_DIR)/%.o, $(CC_SOURCES))
BINARIES := $(patsubst $(SRC_ROOT)/%.cpp, $(BIN_DIR)/%, $(CPP_SOURCES))

$(BUILD_DIR)/%.o: $(SRC_ROOT)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN_DIR)/%: $(SRC_ROOT)/%.cpp $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $< -o $@ $(LDFLAGS) $(LDLIBS)

all: $(BINARIES)

test: $(BINARIES)
	$(shell find $(BIN_DIR)/ -type f -name "*_test" -print)

setup:
	sudo apt-get install libarmadillo-dev

clean:
	@rm -rf build/
	@rm -rf bin/

print-%:
	@echo $* = $($*)
