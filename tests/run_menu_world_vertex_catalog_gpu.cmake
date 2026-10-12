# Keep each measured bank so repeated CTest runs never replace prior evidence.
string(TIMESTAMP stamp "%Y%m%dT%H%M%S" UTC)
string(RANDOM LENGTH 8 ALPHABET 0123456789abcdef suffix)
set(output "${OUTPUT_ROOT}/${stamp}-${suffix}")
execute_process(COMMAND "${TEST_BINARY}" "${MAP}" "${FIXTURES}" "${ATTACHMENTS}"
    "${CAPTURE_SHADER}" "${output}" RESULT_VARIABLE result)
if(result EQUAL 77)
    message("SKIP: catalog fixtures or required Vulkan features unavailable")
elseif(NOT result EQUAL 0)
    message(FATAL_ERROR "Compact catalog comparison failed (${result}); results: ${output}")
else()
    message("Compact catalog results: ${output}")
endif()
