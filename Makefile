BUILD_DIR := build

# Detect OS: Windows_NT is set automatically on Windows
ifdef OS
	HOST_OS := Windows
else
	HOST_OS := $(shell uname -s)
endif

CMAKE_ARGS := -G Ninja \
	-B $(BUILD_DIR) \
	-DLLVM_ENABLE_PROJECTS="clang" \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo

# macOS specific config
ifeq ($(HOST_OS),Darwin)
	CMAKE_ARGS += -DDEFAULT_SYSROOT="$(shell xcrun --show-sdk-path)"
endif

.PHONY: all build test clean

all: build

build:
	mkdir -p $(BUILD_DIR)
	cmake $(CMAKE_ARGS) ./llvm-project/llvm
	ninja -C $(BUILD_DIR)

test:
	$(BUILD_DIR)/bin/clang++ -O3 -mllvm --enable-fla-obfu -o test_flattening tests/TestFlattening.cpp
	./test_flattening

clean:
	rm -rf $(BUILD_DIR) test_flattening