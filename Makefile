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

.PHONY: all build test clean tests-config tests-build tests-ctest

all: build

build:
	mkdir -p $(BUILD_DIR)
	cmake $(CMAKE_ARGS) ./llvm-project/llvm
	ninja -C $(BUILD_DIR)

## Standalone tests (CTest) under tests/
tests-config:
	cmake -S tests -B tests/build

tests-build: tests-config
	cmake --build tests/build -j8

tests-ctest: tests-build
	ctest --test-dir tests/build -j4 --output-on-failure

test: tests-ctest

clean:
	rm -rf $(BUILD_DIR) tests/build test_flattening