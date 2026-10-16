# Optional pure CPU component. Native38E8C9/38E75B policy/output admission is
# deliberately separate and must be supplied before a cold caller can return.
add_library(halo2_sound_configuration_cpu_tu15 STATIC
    src/source_sound_configuration_tu15_generated.cpp
    src/native_tu15_sound_configuration_data.cpp)
target_include_directories(halo2_sound_configuration_cpu_tu15 PUBLIC include PRIVATE src)
target_link_libraries(halo2_sound_configuration_cpu_tu15 PUBLIC halo2_physics_world_runtime)
target_compile_options(halo2_sound_configuration_cpu_tu15 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_configuration_cpu_tests
        tests/native_tu15_sound_configuration_cpu_tests.cpp
        src/source_sound_reset_tu15_generated.cpp src/source_runtime_memory.cpp)
    target_include_directories(native_tu15_sound_configuration_cpu_tests PRIVATE src)
    target_link_libraries(native_tu15_sound_configuration_cpu_tests PRIVATE
        halo2_sound_configuration_cpu_tu15 halo2_snapshot_storage nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_sound_configuration_cpu_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_sound_configuration_cpu_transcripts
        COMMAND native_tu15_sound_configuration_cpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-sound-configuration/original-v3/report.json
        ${CMAKE_CURRENT_BINARY_DIR}/native-sound-configuration-cpu-report.json)
    set_tests_properties(original_tu15_sound_configuration_cpu_transcripts PROPERTIES TIMEOUT 60)
endif()
