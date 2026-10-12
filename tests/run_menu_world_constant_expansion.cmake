# Every execution retains its own GPU readbacks, including failed comparisons.
string(TIMESTAMP stamp "%Y%m%dT%H%M%S" UTC)
string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef suffix)
set(output "${OUTPUT_ROOT}/${stamp}-${suffix}")
execute_process(COMMAND "${TEST_BINARY}" "${FIXTURES}" "${SHADER}" "${output}"
    RESULT_VARIABLE result)
if(result EQUAL 77)
    message("SKIP: constant fixtures or validated Vulkan device unavailable")
elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "Production constant comparison failed (${result}); results: ${output}")
else()
    message("Production constant results: ${output}")
endif()
