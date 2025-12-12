# Compile and run AES test with specified clang and flags
if(NOT MIRAGE_CLANG)
  message(FATAL_ERROR "MIRAGE_CLANG not set")
endif()
if(NOT AES_SRC)
  message(FATAL_ERROR "AES_SRC not set")
endif()
if(NOT OUT_BIN)
  message(FATAL_ERROR "OUT_BIN not set")
endif()

set(cmd ${MIRAGE_CLANG} ${CLANG_FLAGS} ${AES_SRC} -o ${OUT_BIN})
# Collect multiple -DOBFU_FLAGS occurrences into a single list
foreach(i RANGE 1 8)
  if(DEFINED OBFU${i})
    list(APPEND cmd ${OBFU${i}})
  endif()
endforeach()

execute_process(COMMAND ${cmd}
  RESULT_VARIABLE comp_res
  OUTPUT_VARIABLE comp_out
  ERROR_VARIABLE  comp_err)

if(NOT comp_res EQUAL 0)
  message(FATAL_ERROR "Compilation failed: ${comp_err}\n${comp_out}")
endif()

execute_process(COMMAND ${OUT_BIN}
  RESULT_VARIABLE run_res
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE  run_err)

if(NOT run_res EQUAL 0)
  message(FATAL_ERROR "Run failed: ${run_err}\n${run_out}")
endif()
