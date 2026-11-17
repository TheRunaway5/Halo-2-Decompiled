# Native TU1.5 frame/gameplay owners of group F
# (docs/native_replacement/FRAME_MISC_OWNERS.md): seeded comparison against
# the pinned translated bodies with genuine child stand-ins, census and
# verify (record/replay) modes, sanitizer replay and mutation screens.
if(BUILD_TESTING)
    foreach(owner IN ITEMS 1306b0 d7230 1eee80 41970 133a70)
        add_test(NAME native_tu15_frame_misc_${owner}
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
            "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_frame_misc_tu15.py"
            --owner ${owner} --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-frame-misc-${owner}.json" --jobs 2)
        set_tests_properties(native_tu15_frame_misc_${owner} PROPERTIES TIMEOUT 2400)
    endforeach()
endif()
