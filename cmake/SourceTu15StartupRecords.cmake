add_library(halo2_source_tu15_startup_records STATIC src/source_tu15_startup_records.cpp)
target_include_directories(halo2_source_tu15_startup_records PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_source_tu15_startup_records PUBLIC cxx_std_20)
target_link_libraries(halo2_source_tu15_startup_records PUBLIC halo2_source_process_heap_platform)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_source_tu15_startup_records PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_source_tu15_startup_records PUBLIC -fsanitize=address,undefined)
endif()
if(TARGET halo2_systems)
    target_link_libraries(halo2_systems PUBLIC halo2_source_tu15_startup_records)
endif()
if(BUILD_TESTING)
    add_executable(source_tu15_startup_records_tests tests/source_tu15_startup_records_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_tu15_startup_records_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(source_tu15_startup_records_tests PRIVATE halo2_source_tu15_startup_records nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_fresh_startup_records COMMAND source_tu15_startup_records_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-startup-records-20261011/original-verified/fixture.json
        /home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo\ 2\ DLC/content/u5English/default.xbe)
    set_tests_properties(original_tu15_fresh_startup_records PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_fresh_startup_kernel_data COMMAND source_tu15_startup_records_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012/original-v1/fixture.json
        /home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo\ 2\ DLC/content/u5English/default.xbe)
    set_tests_properties(original_tu15_fresh_startup_kernel_data PROPERTIES SKIP_RETURN_CODE 77)
endif()
