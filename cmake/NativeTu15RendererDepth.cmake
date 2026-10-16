target_sources(halo2_systems PRIVATE src/native_tu15_renderer_depth.cpp src/native_tu15_renderer_depth_state_data.cpp src/native_tu15_renderer_output.cpp src/native_tu15_renderer_coordinates.cpp src/native_tu15_renderer_resource_storage_data.cpp src/source_renderer_resource_storage_tu15_generated.cpp src/native_tu15_framebuffer_aliases.cpp src/native_tu15_framebuffer_startup.cpp)
if(BUILD_TESTING)
    add_executable(renderer_framebuffer_alias_tests tests/renderer_framebuffer_alias_tests.cpp src/native_tu15_framebuffer_aliases.cpp)
    target_link_libraries(renderer_framebuffer_alias_tests PRIVATE halo2_renderer)
    add_test(NAME native_renderer_framebuffer_aliases COMMAND renderer_framebuffer_alias_tests
        ${CMAKE_CURRENT_BINARY_DIR}/renderer-framebuffer-alias-validation)
    set_tests_properties(native_renderer_framebuffer_aliases PROPERTIES TIMEOUT 240)
    add_executable(renderer_targets_tests tests/renderer_targets_tests.cpp)
    target_link_libraries(renderer_targets_tests PRIVATE halo2_renderer)
    add_test(NAME native_renderer_target_lifetimes COMMAND renderer_targets_tests
        ${CMAKE_CURRENT_BINARY_DIR}/renderer-target-validation)
    set_tests_properties(native_renderer_target_lifetimes PROPERTIES TIMEOUT 240)
    add_executable(renderer_source_buffers_tests tests/renderer_source_buffers_tests.cpp)
    target_link_libraries(renderer_source_buffers_tests PRIVATE halo2_renderer)
    add_test(NAME native_renderer_source_color_buffers COMMAND renderer_source_buffers_tests
        ${CMAKE_CURRENT_BINARY_DIR}/renderer-source-buffer-validation)
    set_tests_properties(native_renderer_source_color_buffers PROPERTIES TIMEOUT 240)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_link_libraries(${target} PRIVATE halo2_weather_depth_alias)
        target_sources(${target} PRIVATE src/native_tu15_renderer_depth.cpp src/native_tu15_renderer_depth_state_data.cpp src/native_tu15_renderer_output.cpp src/native_tu15_renderer_coordinates.cpp src/native_tu15_renderer_resource_storage_data.cpp src/source_renderer_resource_storage_tu15_generated.cpp src/native_tu15_framebuffer_aliases.cpp src/native_tu15_framebuffer_startup.cpp)
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

if(BUILD_TESTING)
    foreach(coordinate_input none virtual-four)
        if(coordinate_input STREQUAL none)
            set(coordinate_suffix "")
            set(coordinate_original original-graphics-coordinates)
        else()
            set(coordinate_suffix _controller)
            set(coordinate_original original-graphics-coordinates-controller)
        endif()
        add_test(NAME native_tu15_graphics_coordinates${coordinate_suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-coordinates --input-devices ${coordinate_input})
        add_test(NAME original_tu15_graphics_coordinates${coordinate_suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-coordinates --input-devices ${coordinate_input}
            --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/${coordinate_original}/report.json)
        set_tests_properties(native_tu15_graphics_coordinates${coordinate_suffix}_startup original_tu15_graphics_coordinates${coordinate_suffix}_startup PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
        add_test(NAME native_tu15_graphics_surfaces${coordinate_suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-surfaces --input-devices ${coordinate_input})
        set_tests_properties(native_tu15_graphics_surfaces${coordinate_suffix}_startup PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
endif()
