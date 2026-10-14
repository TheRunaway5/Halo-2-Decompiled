target_sources(halo2_systems PRIVATE src/native_tu15_renderer_pool_data.cpp src/source_renderer_pool_tu15_generated.cpp)
if(BUILD_TESTING)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/native_tu15_renderer_pool_data.cpp src/source_renderer_pool_tu15_generated.cpp)
    endforeach()
    add_test(NAME native_tu15_graphics_pool_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-pool --input-devices none)
    add_test(NAME original_tu15_graphics_pool_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-pool --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-pool-v2/report.json)
    add_test(NAME native_tu15_graphics_pool_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-pool --input-devices virtual-four)
    add_test(NAME original_tu15_graphics_pool_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-pool --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-pool-controller-v2/report.json)
    set_tests_properties(native_tu15_graphics_pool_startup original_tu15_graphics_pool_startup
        native_tu15_graphics_pool_controller_startup original_tu15_graphics_pool_controller_startup
        PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
endif()
