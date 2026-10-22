target_sources(halo2_systems PRIVATE src/platform/native_tu15_static_registration_data.cpp src/platform/source_static_registration_tu15_generated.cpp)
if(BUILD_TESTING)
    target_sources(native_tu15_network_link_tests PRIVATE src/platform/native_tu15_static_registration_data.cpp src/platform/source_static_registration_tu15_generated.cpp)
    add_test(NAME native_tu15_static_registration_startup_callback COMMAND native_tu15_network_link_tests --frontier static-registration)
    add_test(NAME original_tu15_static_registration_startup_callback COMMAND native_tu15_network_link_tests --frontier static-registration
        --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-static-registration-startup/report.json)
    add_test(NAME native_tu15_static_registration_controller_startup_callback COMMAND native_tu15_network_link_tests --frontier static-registration --input-devices virtual-four)
    add_test(NAME original_tu15_static_registration_controller_startup_callback COMMAND native_tu15_network_link_tests --frontier static-registration --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-static-registration-controller-startup/report.json)
    set_tests_properties(native_tu15_static_registration_startup_callback original_tu15_static_registration_startup_callback
        native_tu15_static_registration_controller_startup_callback original_tu15_static_registration_controller_startup_callback
        PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
endif()
