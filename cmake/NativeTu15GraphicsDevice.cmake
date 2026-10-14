target_sources(halo2_systems PRIVATE src/native_tu15_graphics_device.cpp src/native_tu15_graphics_device_data.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_renderer_device_lease)
if(BUILD_TESTING)
    target_sources(native_tu15_network_link_tests PRIVATE src/native_tu15_graphics_device.cpp src/native_tu15_graphics_device_data.cpp)
    target_link_libraries(native_tu15_network_link_tests PRIVATE halo2_renderer_device_lease)
    get_target_property(HALO2_NATIVE_STARTUP_SOURCES native_tu15_network_link_tests SOURCES)
    get_target_property(HALO2_NATIVE_STARTUP_LIBRARIES native_tu15_network_link_tests LINK_LIBRARIES)
    add_executable(native_tu15_graphics_startup_tests ${HALO2_NATIVE_STARTUP_SOURCES} tests/native_tu15_graphics_startup_main.cpp)
    target_include_directories(native_tu15_graphics_startup_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(native_tu15_graphics_startup_tests PRIVATE HALO2_NATIVE_GRAPHICS_STARTUP_MAIN=1)
    target_compile_options(native_tu15_graphics_startup_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    target_link_libraries(native_tu15_graphics_startup_tests PRIVATE ${HALO2_NATIVE_STARTUP_LIBRARIES} halo2_renderer)
    add_test(NAME native_tu15_graphics_device_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-device --input-devices none)
    add_test(NAME original_tu15_graphics_device_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-device --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-device/report.json)
    add_test(NAME native_tu15_graphics_device_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-device --input-devices virtual-four)
    add_test(NAME original_tu15_graphics_device_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-device --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-device-controller/report.json)
    set_tests_properties(native_tu15_graphics_device_startup original_tu15_graphics_device_startup
        native_tu15_graphics_device_controller_startup original_tu15_graphics_device_controller_startup
        PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
endif()
