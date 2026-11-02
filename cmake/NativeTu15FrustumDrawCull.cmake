if(TARGET halo2_systems)
    target_sources(halo2_systems PRIVATE src/rendering/native_tu15_frustum_draw_cull.cpp)
    if(MSVC)
        set_source_files_properties(src/rendering/native_tu15_frustum_draw_cull.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    else()
        set_source_files_properties(src/rendering/native_tu15_frustum_draw_cull.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    endif()
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_frustum_draw_cull_tests
        tests/native_tu15_frustum_draw_cull_tests.cpp
        src/rendering/native_tu15_frustum_draw_cull.cpp src/rendering/world/source_bsp_ranges.cpp
        src/rendering/world/source_bsp_accelerator.cpp src/rendering/world/source_portal_visibility.cpp
        src/rendering/world/source_visibility.cpp src/core/transform.cpp src/assets/cache.cpp)
    target_include_directories(native_tu15_frustum_draw_cull_tests PRIVATE include)
    target_compile_features(native_tu15_frustum_draw_cull_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_frustum_draw_cull_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_frustum_draw_cull_tests PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off)
    target_link_options(native_tu15_frustum_draw_cull_tests PRIVATE -Wl,--gc-sections)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_frustum_draw_cull_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_frustum_draw_cull_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_frustum_draw_cull COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_frustum_draw_cull.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_frustum_draw_cull_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-frustum-draw-cull.json")
    set_tests_properties(original_tu15_frustum_draw_cull PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
endif()
