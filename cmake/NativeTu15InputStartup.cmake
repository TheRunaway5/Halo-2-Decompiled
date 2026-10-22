add_library(halo2_input_startup_tu15 STATIC src/input/source_input_startup_tu15_generated.cpp
    src/input/native_tu15_input_startup_data.cpp src/input/native_tu15_input_devices.cpp)
target_include_directories(halo2_input_startup_tu15 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(halo2_input_startup_tu15 PUBLIC cxx_std_20)
target_link_libraries(halo2_input_startup_tu15 PUBLIC SDL2::SDL2)
if(MSVC)
    target_compile_options(halo2_input_startup_tu15 PRIVATE /fp:strict)
else()
    target_compile_options(halo2_input_startup_tu15 PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_input_startup_tu15 PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_input_startup_tu15 PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_input_startup_tu15)
if(BUILD_TESTING)
    add_executable(native_tu15_input_startup_tests tests/native_tu15_input_startup_tests.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_image.cpp src/runtime/source_runtime_host_virtual_memory.cpp)
    target_link_libraries(native_tu15_input_startup_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(native_tu15_input_startup_tests PRIVATE halo2_input_startup_tu15 nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_input_startup_lifecycle COMMAND native_tu15_input_startup_tests)
    add_test(NAME native_tu15_input_startup_host COMMAND native_tu15_input_startup_tests --devices host)
    add_test(NAME original_tu15_input_startup_lifecycle COMMAND native_tu15_input_startup_tests
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/input-startup/original-input.json
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_tu15_input_startup_lifecycle native_tu15_input_startup_host original_tu15_input_startup_lifecycle PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    target_link_libraries(native_tu15_network_link_tests PRIVATE halo2_input_startup_tu15)
    add_test(NAME native_tu15_input_startup_callback COMMAND native_tu15_network_link_tests --frontier input-complete)
    add_test(NAME original_tu15_input_startup_callback COMMAND native_tu15_network_link_tests --frontier input-complete
        --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-input-startup-v3/report.json)
    set_tests_properties(native_tu15_input_startup_callback original_tu15_input_startup_callback PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
endif()
