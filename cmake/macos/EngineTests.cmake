include_guard(GLOBAL)

function(halo2_macos_engine_tests target)
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME macos_engine_cli_${arch} COMMAND ${Python3_EXECUTABLE} -B
            "${root}/tests/macos/engine_cli_test.py" "$<TARGET_BUNDLE_DIR:${target}>" "${arch}"
            "${CMAKE_BINARY_DIR}/macos-engine-cli-${arch}.json")
        set_tests_properties(macos_engine_cli_${arch} PROPERTIES TIMEOUT 90 RUN_SERIAL TRUE)
    endforeach()
endfunction()
