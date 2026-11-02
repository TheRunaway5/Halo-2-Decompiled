include_guard(GLOBAL)
add_library(halo2_native_camera_projection STATIC
    src/rendering/world/native_tu15_camera_projection.cpp)
target_include_directories(halo2_native_camera_projection PUBLIC include)
target_compile_features(halo2_native_camera_projection PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_native_camera_projection PRIVATE /fp:strict)
elseif(HALO2_SANITIZERS)
    target_compile_options(halo2_native_camera_projection PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_native_camera_projection PUBLIC -fsanitize=address,undefined)
endif()
target_sources(halo2_physics_world_runtime PRIVATE
    src/rendering/world/native_tu15_camera_projection_binding.cpp)
target_link_libraries(halo2_physics_world_runtime PUBLIC halo2_native_camera_projection)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/rendering/world/native_tu15_camera_projection.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_camera_projection_tests
        tests/native_tu15_camera_projection_tests.cpp
        src/rendering/world/native_tu15_camera_projection.cpp
        src/rendering/world/native_tu15_camera_projection_binding.cpp)
    target_include_directories(native_tu15_camera_projection_tests PRIVATE include)
    target_compile_features(native_tu15_camera_projection_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_camera_projection_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_camera_projection
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_tu15_camera_projection_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_camera_projection_v2.json.gz")
    set_tests_properties(native_tu15_camera_projection PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
