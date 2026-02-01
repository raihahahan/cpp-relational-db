BUILD_DIR := build
TARGET := lexer

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
            -fsanitize=address,undefined -fno-omit-frame-pointer

INCLUDES := -I../../include

SRCS := lexer.cpp main.cpp ../error/dberror.cpp

.PHONY: all clean

all:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
