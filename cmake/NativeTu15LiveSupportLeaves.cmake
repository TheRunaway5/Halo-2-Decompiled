# Native TU1.5 live-support leaves (group L: C9550, 1EC970, 1F7F60, C1E90,
# 110350, 243F20, 32CC0C, 16E600, 171010, 1211C0, 1F7E90, 32A4E0, 16E4B0,
# 16E450, the pool startup unit's store-site copies, and the leaves with
# child boundaries 22A790, 1F8200, 138D90 and 179CD0): seeded comparison
# against the pinned translated bodies,
# leaf verify and census modes, branch-direction coverage, sanitizer replay,
# production binding checks and mutation screen.
if(BUILD_TESTING)
    add_custom_target(native_live_support_leaves_tests)
    add_test(NAME native_tu15_live_support_leaves
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_live_support_leaves_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-live-support-leaves.json" --jobs 2)
    set_tests_properties(native_tu15_live_support_leaves PROPERTIES TIMEOUT 5400)
endif()
