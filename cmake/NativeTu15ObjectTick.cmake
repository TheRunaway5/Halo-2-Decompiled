# Native TU1.5 per-tick object update owners (B8F40/B9090/B9180/BE4A0/10DD10),
# their children (BE850/BE610/C0F60/10DD80/C0200/1D5790/1E15D0) and the
# campaign environment cluster distance flood (131470): seeded
# comparison against the pinned translated bodies with genuine child
# stand-ins, census and verify (record/replay) modes, sanitizer replay and
# mutation screens.
if(BUILD_TESTING)
    add_custom_target(native_object_tick_tests)
    add_test(NAME native_tu15_object_tick
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_tick_closures_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-tick.json" --jobs 2)
    set_tests_properties(native_tu15_object_tick PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_cluster_distance_flood
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_cluster_distance_flood_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cluster-distance-flood.json" --jobs 2)
    set_tests_properties(native_tu15_cluster_distance_flood PROPERTIES TIMEOUT 2400)
    add_test(NAME native_tu15_object_subtree
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_subtree_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-subtree.json" --jobs 2)
    set_tests_properties(native_tu15_object_subtree PROPERTIES TIMEOUT 2400)
    # The device update callbacks 1179C0/10B840 (scene device unit).
    add_test(NAME native_tu15_device_update
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_device_update_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-device-update.json" --jobs 2)
    set_tests_properties(native_tu15_device_update PROPERTIES TIMEOUT 3600)

    if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i.86)$")
        add_executable(native_tu15_liquid_widget_tests tests/native_tu15_liquid_widget_tests.cpp)
        target_include_directories(native_tu15_liquid_widget_tests PRIVATE include src)
        target_compile_features(native_tu15_liquid_widget_tests PRIVATE cxx_std_20)
        target_compile_options(native_tu15_liquid_widget_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        add_test(NAME native_tu15_liquid_widget_retirement COMMAND native_tu15_liquid_widget_tests)
    endif()
endif()
