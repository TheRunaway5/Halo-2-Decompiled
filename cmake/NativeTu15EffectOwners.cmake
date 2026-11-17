# Native TU1.5 effect owners (group E): 20680 lens flare reset, the C35D0 /
# C3300 / C2A80 per-view light fade and the 176A40 / 252100 effect state
# values: seeded comparison against the pinned translated bodies with genuine
# 16EA60 / 4EFA0 stand-ins, census, verify (record/replay) and reference-switch
# modes, sanitizer replay, binding checks and a mutation screen.
if(BUILD_TESTING)
    add_test(NAME native_tu15_effect_owners
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_effect_owners_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-effect-owners.json" --jobs 2)
    set_tests_properties(native_tu15_effect_owners PROPERTIES TIMEOUT 2400)
endif()
