# Keep every validated-device readback; never replace prior evidence.
string(TIMESTAMP stamp "%Y%m%dT%H%M%S" UTC)
string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef suffix)
set(output "${OUTPUT_ROOT}/${stamp}-${suffix}")
execute_process(COMMAND "${TEST_BINARY}" "${FIXTURES}" "${SHADER}" "${output}"
    RESULT_VARIABLE result)
if(result EQUAL 77)
    message("SKIP: BC2 fixtures or required validated native image capabilities unavailable")
elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "Native BC2 GPU storage comparison failed (${result}); results: ${output}")
else()
    message("Native BC2 GPU storage results: ${output}")
endif()
