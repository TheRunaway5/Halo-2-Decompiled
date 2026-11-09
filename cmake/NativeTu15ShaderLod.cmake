# Qualified as a dependency before the whole41030 World emitter is admitted.
if(BUILD_TESTING)
    add_executable(native_tu15_shader_lod_tests tests/native_tu15_shader_lod_tests.cpp
        src/rendering/world/native_tu15_shader_lod.cpp)
    target_include_directories(native_tu15_shader_lod_tests PRIVATE include)
    target_compile_features(native_tu15_shader_lod_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_shader_lod_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_shader_lod_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_shader_lod_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_shader_lod_tests PRIVATE -fsanitize=address,undefined)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        add_test(NAME original_tu15_shader_lod COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_shader_lod.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_shader_lod_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-shader-lod.json")
        set_tests_properties(original_tu15_shader_lod PROPERTIES TIMEOUT 900)
    endif()
endif()
