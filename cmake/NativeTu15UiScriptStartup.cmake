target_sources(halo2_systems PRIVATE src/source_ui_script_startup_tu15_generated.cpp
    src/native_tu15_ui_script_startup_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_ui_script_startup_tests
        tests/native_tu15_ui_script_startup_tests.cpp src/source_ui_script_startup_tu15_generated.cpp
        src/native_tu15_ui_script_startup_data.cpp src/native_tu15_storage_formatting.cpp
        src/source_runtime_memory.cpp src/source_runtime_host_storage.cpp src/physics_pool_host_storage.cpp)
    target_include_directories(native_tu15_ui_script_startup_tests PRIVATE src)
    target_link_libraries(native_tu15_ui_script_startup_tests PRIVATE
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_ui_script_startup_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_ui_script_startup_constructors COMMAND native_tu15_ui_script_startup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-ui-effect-startup/original-constructor/report.json)
    set_tests_properties(original_tu15_ui_script_startup_constructors PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/source_ui_script_startup_tu15_generated.cpp
            src/native_tu15_ui_script_startup_data.cpp)
    endforeach()
endif()
