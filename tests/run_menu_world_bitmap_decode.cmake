# Preserve each actual-device readback, including mismatches and capability skips.
string(TIMESTAMP stamp "%Y%m%dT%H%M%S" UTC)
string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef suffix)
set(output "${OUTPUT_ROOT}/${stamp}-${suffix}")
execute_process(COMMAND "${TEST_BINARY}" "${MAP}" "${FIXTURES}" "${SHADER}" "${output}"
    RESULT_VARIABLE result)
if(result EQUAL 77)
    message("SKIP: bitmap decode fixtures or required validated native image capabilities unavailable")
elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "Native bitmap GPU decode comparison failed (${result}); results: ${output}")
else()
    message("Native bitmap GPU decode results: ${output}")
endif()
