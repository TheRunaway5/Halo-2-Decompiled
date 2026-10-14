target_sources(halo2_systems PRIVATE src/multiplayer_continuation.cpp src/source_match_authority.cpp)
if(BUILD_TESTING)
    add_executable(source_match_local_rebind_tests tests/source_match_local_rebind_tests.cpp)
    target_link_libraries(source_match_local_rebind_tests PRIVATE halo2_systems)
    add_test(NAME source_match_local_rebind_tu15 COMMAND source_match_local_rebind_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_match_local_rebind_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_executable(source_match_network_departure_tests tests/source_match_network_departure_tests.cpp)
    target_link_libraries(source_match_network_departure_tests PRIVATE halo2_systems)
    add_test(NAME source_match_network_departure_tu15 COMMAND source_match_network_departure_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_match_network_departure_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_player_departure_tu15_tests tests/source_player_departure_tu15_tests.cpp)
    target_link_libraries(source_player_departure_tu15_tests PRIVATE halo2_systems)
    add_test(NAME source_player_departure_tu15 COMMAND source_player_departure_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_player_departure_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(multiplayer_continuation_tests tests/multiplayer_continuation_tests.cpp)
    target_link_libraries(multiplayer_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_match_continuation COMMAND multiplayer_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_match_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_match_continuation_tests tests/source_match_continuation_tests.cpp)
    target_link_libraries(source_match_continuation_tests PRIVATE halo2_systems)
    add_test(NAME source_match_continuation_tu15 COMMAND source_match_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_match_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME source_match_active_continuation_tu15 COMMAND source_match_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" --active)
    set_tests_properties(source_match_active_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME source_match_departure_continuation_tu15 COMMAND source_match_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" --departure)
    set_tests_properties(source_match_departure_continuation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
