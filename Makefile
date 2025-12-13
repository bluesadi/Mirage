BUILD_DIR := build

.PHONY: all build test clean tests-config tests-build tests-ctest

all: build

build:
	mkdir -p $(BUILD_DIR)
	cmake -G Ninja \
		-B $(BUILD_DIR) \
		-DLLVM_TARGETS_TO_BUILD="host" \
		-DLLVM_ENABLE_PROJECTS="clang" \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DLLVM_ENABLE_ASSERTIONS=ON \
		./llvm-project/llvm
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