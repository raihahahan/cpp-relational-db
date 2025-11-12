BUILD_DIR := build
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug
.PHONY: all build test test_disk_manager clean
all: build

.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) ..
	@cmake --build $(BUILD_DIR) -j

test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

test_disk_manager: build
	@cd $(BUILD_DIR) && ./test_disk_manager

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory."
