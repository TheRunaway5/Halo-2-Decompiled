target_sources(halo2_systems PRIVATE src/native_tu15_renderer_depth.cpp src/native_tu15_renderer_depth_state_data.cpp src/native_tu15_renderer_output.cpp)
if(BUILD_TESTING)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/native_tu15_renderer_depth.cpp src/native_tu15_renderer_depth_state_data.cpp src/native_tu15_renderer_output.cpp)
    endforeach()
    add_test(NAME native_tu15_graphics_depth_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth --input-devices none)
    add_test(NAME original_tu15_graphics_depth_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-depth/report.json)
    add_test(NAME native_tu15_graphics_depth_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth --input-devices virtual-four)
    add_test(NAME original_tu15_graphics_depth_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-depth-controller/report.json)
    set_tests_properties(native_tu15_graphics_depth_startup original_tu15_graphics_depth_startup
        native_tu15_graphics_depth_controller_startup original_tu15_graphics_depth_controller_startup
        PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    add_test(NAME native_tu15_graphics_depth_state_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth-state --input-devices none)
    add_test(NAME original_tu15_graphics_depth_state_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth-state --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-depth-state/report.json)
    add_test(NAME native_tu15_graphics_depth_state_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth-state --input-devices virtual-four)
    add_test(NAME original_tu15_graphics_depth_state_controller_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-depth-state --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-graphics-depth-state-controller/report.json)
    set_tests_properties(native_tu15_graphics_depth_state_startup original_tu15_graphics_depth_state_startup
        native_tu15_graphics_depth_state_controller_startup original_tu15_graphics_depth_state_controller_startup
        PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
    foreach(input none virtual-four)
        if(input STREQUAL none)
            set(suffix "")
            set(original original-graphics-output)
        else()
            set(suffix _controller)
            set(original original-graphics-output-controller)
        endif()
        add_test(NAME native_tu15_graphics_output${suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-output --input-devices ${input})
        add_test(NAME original_tu15_graphics_output${suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-output --input-devices ${input}
            --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/${original}/report.json)
        set_tests_properties(native_tu15_graphics_output${suffix}_startup original_tu15_graphics_output${suffix}_startup PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
endif()
