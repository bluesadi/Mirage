BUILD_DIR := build

.PHONY: all build

all: build

build:
	mkdir -p $(BUILD_DIR)
	cmake -G Ninja -B $(BUILD_DIR) \
		-DLLVM_ENABLE_PROJECTS="clang" \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		./llvm
	ninja -C $(BUILD_DIR)
