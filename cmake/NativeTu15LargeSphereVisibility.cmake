if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_large_sphere_visibility_tests
        tests/native_tu15_large_sphere_visibility_tests.cpp
        src/rendering/world/source_portal_visibility.cpp
        src/rendering/world/source_portal_frustum.cpp
        src/rendering/world/source_portal_clip.cpp
        src/rendering/world/source_frustum_camera.cpp
        src/rendering/world/source_visibility.cpp
        src/rendering/shadows/source_shadow_view.cpp
        src/core/transform.cpp)
    target_include_directories(native_tu15_large_sphere_visibility_tests PRIVATE include)
    target_compile_features(native_tu15_large_sphere_visibility_tests PRIVATE cxx_std_20)
    # The math owner shares a unit with the unused Cache asset loader. This
    # standalone oracle consumes explicit source banks and no Cache services.
    target_compile_options(native_tu15_large_sphere_visibility_tests PRIVATE -ffunction-sections -fdata-sections -ffp-contract=off)
    target_link_options(native_tu15_large_sphere_visibility_tests PRIVATE -Wl,--gc-sections)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_large_sphere_visibility_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_large_sphere_visibility_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_large_sphere_visibility COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/differential_source_portal_visibility.py"
        --binary $<TARGET_FILE:native_tu15_large_sphere_visibility_tests> --light-cubes --count 0
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-large-sphere-visibility.json")
    set_tests_properties(original_tu15_large_sphere_visibility PROPERTIES TIMEOUT 3600)
endif()
