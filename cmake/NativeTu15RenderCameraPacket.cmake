target_sources(halo2_physics_world_runtime PRIVATE
    src/rendering/world/native_tu15_render_camera_packet.cpp
    src/rendering/world/native_tu15_render_camera_packet_binding.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/rendering/world/native_tu15_render_camera_packet.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_render_camera_packet_tests
        tests/native_tu15_render_camera_packet_tests.cpp
        src/rendering/world/native_tu15_render_camera_packet.cpp
        src/rendering/world/native_tu15_render_camera_packet_binding.cpp)
    target_include_directories(native_tu15_render_camera_packet_tests PRIVATE include)
    target_compile_features(native_tu15_render_camera_packet_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_render_camera_packet_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_render_camera_packet
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_tu15_render_camera_packet_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_render_camera_packet.json.gz")
    set_tests_properties(native_tu15_render_camera_packet PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90)
endif()
