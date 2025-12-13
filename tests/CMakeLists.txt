cmake_minimum_required(VERSION 3.16)
project(MirageTests C)

enable_testing()

add_executable(aes_test aes/aes.c)

# Default NIST vector
add_test(NAME aes_nist COMMAND aes_test)

# Additional vectors (AES-128 ECB from SP 800-38A F.1.1 blocks)
# Block 2
add_test(NAME aes_vec2 COMMAND aes_test
  2b7e151628aed2a6abf7158809cf4f3c
  ae2d8a571e03ac9c9eb76fac45af8e51
  f5d3d58503b9699de785895a96fdbaaf)
# Block 3
add_test(NAME aes_vec3 COMMAND aes_test
  2b7e151628aed2a6abf7158809cf4f3c
  30c81c46a35ce411e5fbc1191a0a52ef
  43b1cd7f598ece23881b00e3ed030688)
# Block 4
add_test(NAME aes_vec4 COMMAND aes_test
  2b7e151628aed2a6abf7158809cf4f3c
  f69f2445df4f9b17ad2b417be66c3710
  7b0c785e27e8ad3f8223207104725dd4)

### Obfuscation-enabled tests using project clang
set(MIRAGE_CLANG "${CMAKE_SOURCE_DIR}/../build/bin/clang")
if(EXISTS "${MIRAGE_CLANG}")
  # Helper CMake script to compile and run AES
  add_test(
    NAME aes_sub
    COMMAND ${CMAKE_COMMAND}
            -DMIRAGE_CLANG=${MIRAGE_CLANG}
            -DAES_SRC=${CMAKE_SOURCE_DIR}/aes/aes.c
            -DOUT_BIN=${CMAKE_BINARY_DIR}/aes_sub
            -DCLANG_FLAGS=-O0
            -DOBFU1=-mllvm
            -DOBFU2=-enable-sub-obfu
            -P ${CMAKE_SOURCE_DIR}/cmake/CompileRunAES.cmake
  )
  add_test(
    NAME aes_fla
    COMMAND ${CMAKE_COMMAND}
            -DMIRAGE_CLANG=${MIRAGE_CLANG}
            -DAES_SRC=${CMAKE_SOURCE_DIR}/aes/aes.c
            -DOUT_BIN=${CMAKE_BINARY_DIR}/aes_flat
            -DCLANG_FLAGS=-O0
            -DOBFU1=-mllvm
            -DOBFU2=-enable-fla-obfu
            -P ${CMAKE_SOURCE_DIR}/cmake/CompileRunAES.cmake
  )
  add_test(
    NAME aes_bcf
    COMMAND ${CMAKE_COMMAND}
            -DMIRAGE_CLANG=${MIRAGE_CLANG}
            -DAES_SRC=${CMAKE_SOURCE_DIR}/aes/aes.c
            -DOUT_BIN=${CMAKE_BINARY_DIR}/aes_bcf
            -DCLANG_FLAGS=-O0
            -DOBFU1=-mllvm
            -DOBFU2=-enable-bcf-obfu
            -P ${CMAKE_SOURCE_DIR}/cmake/CompileRunAES.cmake
  )
  add_test(
    NAME aes_all
    COMMAND ${CMAKE_COMMAND}
            -DMIRAGE_CLANG=${MIRAGE_CLANG}
            -DAES_SRC=${CMAKE_SOURCE_DIR}/aes/aes.c
            -DOUT_BIN=${CMAKE_BINARY_DIR}/aes_all
            -DCLANG_FLAGS=-O0
            -DOBFU1=-mllvm
            -DOBFU2=-enable-bcf-obfu
            -DOBFU3=-mllvm
            -DOBFU4=-enable-sub-obfu
            -DOBFU5=-mllvm
            -DOBFU6=-enable-fla-obfu
            -P ${CMAKE_SOURCE_DIR}/cmake/CompileRunAES.cmake
  )
endif()
