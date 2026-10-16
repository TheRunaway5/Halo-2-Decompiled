# Fixed module0 arithmetic and retained history; no install or clock capability.
add_library(halo2_native_tu15_sound_ep_module0_numeric STATIC
    src/native_tu15_sound_ep_module0_numeric.cpp)
target_include_directories(halo2_native_tu15_sound_ep_module0_numeric PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_ep_module0_numeric PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_sound_ep_module0_numeric)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_ep_module0_numeric_probe
        tests/native_tu15_sound_ep_module0_numeric_probe.cpp)
    target_link_libraries(native_tu15_sound_ep_module0_numeric_probe PRIVATE
        halo2_native_tu15_sound_ep_module0_numeric)
    add_executable(native_tu15_sound_ep_module0_numeric_guards
        tests/native_tu15_sound_ep_module0_numeric_guards.cpp)
    target_link_libraries(native_tu15_sound_ep_module0_numeric_guards PRIVATE
        halo2_native_tu15_sound_ep_module0_numeric)
    foreach(kind IN ITEMS final_full_comparison final_tail_comparison reset_comparison_v1)
        add_test(NAME native_tu15_sound_ep_module0_${kind}_reference
            COMMAND "${Python3_EXECUTABLE}"
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_ep_module0.py
            --probe $<TARGET_FILE:native_tu15_sound_ep_module0_numeric_probe>
            --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_ep_module0_${kind}.json.gz
            --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-ep-module0-${kind}-report.json)
        set_tests_properties(native_tu15_sound_ep_module0_${kind}_reference PROPERTIES TIMEOUT 180)
    endforeach()
    add_test(NAME native_tu15_sound_ep_module0_admission
        COMMAND native_tu15_sound_ep_module0_numeric_guards)
endif()
