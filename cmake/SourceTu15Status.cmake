add_library(halo2_source_tu15_status STATIC src/source_tu15_status.cpp src/source_tu15_status_runtime_generated.cpp)
target_include_directories(halo2_source_tu15_status PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_source_tu15_status PUBLIC cxx_std_20)
target_link_libraries(halo2_source_tu15_status PUBLIC halo2_source_tu15_settings)
if(MSVC)
    target_compile_options(halo2_source_tu15_status PRIVATE /fp:strict)
else()
    target_compile_options(halo2_source_tu15_status PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_source_tu15_status PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_source_tu15_status PUBLIC -fsanitize=address,undefined)
endif()
if(TARGET halo2_systems)
    target_link_libraries(halo2_systems PUBLIC halo2_source_tu15_status)
endif()
if(BUILD_TESTING)
    add_executable(source_tu15_status_tests tests/source_tu15_status_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_tu15_status_tests PRIVATE halo2_source_tu15_status nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_fresh_startup_status COMMAND source_tu15_status_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-status-startup-20261012/original-v1/fixture.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012/boot-v3/eeprom.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-status-startup-20261012/boot-v3/kernel)
    set_tests_properties(original_tu15_fresh_startup_status PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_tu15_status_tables_tests tests/source_tu15_status_tables_tests.cpp
        src/source_runtime_memory.cpp src/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(source_tu15_status_tables_tests PRIVATE halo2_source_tu15_status nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_status_tables COMMAND source_tu15_status_tables_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-status-startup-20261012/table-original-v1/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-status-startup-20261012/boot-v3/kernel)
    set_tests_properties(original_tu15_status_tables PROPERTIES SKIP_RETURN_CODE 77)
endif()
