# Preserve every completed diagnostic report while allowing repeated CTest runs.
string(TIMESTAMP stamp "%Y%m%dT%H%M%S" UTC)
string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef suffix)
set(output "${OUTPUT_ROOT}/${stamp}-${suffix}")
set(extra)
if(DEFINED FULL_FOG)
    list(APPEND extra --full-fog "${FULL_FOG}")
endif()
execute_process(COMMAND "${TEST_BINARY}" "${FIXTURES}" "${output}"
    ${extra}
    RESULT_VARIABLE result)
if(result EQUAL 77)
    message("SKIP: attachment fixtures or required Vulkan features unavailable")
elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "World depth attachment comparison failed (${result}); results: ${output}")
else()
    message("World depth attachment results: ${output}")
endif()
