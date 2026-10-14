target_sources(halo2_systems PRIVATE src/native_tu15_player_startup_data.cpp)
if(BUILD_TESTING)
    target_sources(native_tu15_network_link_tests PRIVATE src/native_tu15_player_startup_data.cpp)
    add_test(NAME native_tu15_player_startup_callback COMMAND native_tu15_network_link_tests --frontier players)
    add_test(NAME original_tu15_player_startup_callback COMMAND native_tu15_network_link_tests --frontier players
        --input-devices none
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-player-startup/report.json)
    set_tests_properties(native_tu15_player_startup_callback original_tu15_player_startup_callback PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    add_test(NAME native_tu15_player_controller_startup_callback COMMAND native_tu15_network_link_tests --frontier players --input-devices virtual-four)
    add_test(NAME original_tu15_player_controller_startup_callback COMMAND native_tu15_network_link_tests --frontier players --input-devices virtual-four
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-player-controller-startup/report.json)
    set_tests_properties(native_tu15_player_controller_startup_callback original_tu15_player_controller_startup_callback PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
endif()
