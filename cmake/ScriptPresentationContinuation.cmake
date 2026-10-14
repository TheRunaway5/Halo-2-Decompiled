target_sources(halo2_systems PRIVATE src/script_continuation.cpp src/presentation_continuation.cpp)
if(BUILD_TESTING)
    add_executable(script_presentation_continuation_tests tests/script_presentation_continuation_tests.cpp)
    target_link_libraries(script_presentation_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_script_presentation_continuation COMMAND script_presentation_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map")
    set_tests_properties(native_script_presentation_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
