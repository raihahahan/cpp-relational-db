BUILD_DIR := build
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug
.PHONY: all build test test_disk_manager clean
all: build

.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) ..
	@cmake --build $(BUILD_DIR) -j

# ALL
test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

# STORAGE
test_storage: build
	make test_disk_manager
	make test_buffer_manager
	make test_freelist

test_disk_manager:
	@cd $(BUILD_DIR) && ./test_disk_manager

test_buffer_manager:
	@cd $(BUILD_DIR) && ./test_buffer_manager

test_freelist:
	@cd $(BUILD_DIR) && ./test_free_list

test_clock:
	@cd $(BUILD_DIR) && ./test_clock

test_slotted_page:
	@cd $(BUILD_DIR) && ./test_slotted_page

# ACCESS
test_access:
	make test_heap_file
	make test_heap_iterator

test_heap_file:
	@cd $(BUILD_DIR) && ./test_heap_file

test_heap_iterator:
	@cd $(BUILD_DIR) && ./test_heap_iterator

test_catalog:
	@cd $(BUILD_DIR) && ./test_catalog && ./test_catalog_codec && ./test_catalog_tables

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory."
