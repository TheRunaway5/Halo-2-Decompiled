target_sources(halo2_systems PRIVATE src/native_tu15_renderer_request_data.cpp src/source_renderer_request_tu15_generated.cpp)
if(BUILD_TESTING)
    target_sources(native_tu15_network_link_tests PRIVATE src/native_tu15_renderer_request_data.cpp src/source_renderer_request_tu15_generated.cpp)
    add_test(NAME native_tu15_renderer_request_frontier COMMAND native_tu15_network_link_tests --frontier renderer-request)
    add_test(NAME original_tu15_renderer_request_frontier COMMAND native_tu15_network_link_tests --frontier renderer-request
        --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-renderer-request-v2/report.json)
    add_test(NAME native_tu15_renderer_request_controller_frontier COMMAND native_tu15_network_link_tests --frontier renderer-request --input-devices virtual-four)
    add_test(NAME original_tu15_renderer_request_controller_frontier COMMAND native_tu15_network_link_tests --frontier renderer-request --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-renderer-controller-request-v2/report.json)
    set_tests_properties(native_tu15_renderer_request_frontier original_tu15_renderer_request_frontier
        native_tu15_renderer_request_controller_frontier original_tu15_renderer_request_controller_frontier
        PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
endif()
