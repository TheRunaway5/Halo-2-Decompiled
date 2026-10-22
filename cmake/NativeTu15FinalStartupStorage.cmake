target_sources(halo2_systems PRIVATE
    src/platform/source_final_startup_storage_tu15_generated.cpp
    src/platform/native_tu15_final_startup_storage_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_final_startup_storage_tests
        tests/native_tu15_final_startup_storage_tests.cpp
        src/platform/source_final_startup_storage_tu15_generated.cpp
        src/platform/native_tu15_final_startup_storage_data.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_storage.cpp
        src/physics/physics_pool_host_storage.cpp)
    target_include_directories(native_tu15_final_startup_storage_tests PRIVATE src)
    target_link_libraries(native_tu15_final_startup_storage_tests PRIVATE
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_options(native_tu15_final_startup_storage_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_final_startup_storage_constructors
        COMMAND native_tu15_final_startup_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-final-startup-storage/original-discovery/report.json)
    set_tests_properties(original_tu15_final_startup_storage_constructors PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/platform/source_final_startup_storage_tu15_generated.cpp
            src/platform/native_tu15_final_startup_storage_data.cpp)
    endforeach()
endif()
