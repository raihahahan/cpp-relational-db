BUILD_DIR := build
TARGET := parser_driver

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -g -O0

INCLUDES := -I../../include

SRCS := lexer.cpp parser.cpp analyzer.cpp main.cpp ../error/dberror.cpp

.PHONY: all clean

all:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
