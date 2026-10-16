if(BUILD_TESTING)
    add_executable(native_tu15_sound_shared_word24_owners_tests tests/native_tu15_sound_shared_word24_owners_tests.cpp)
    target_link_libraries(native_tu15_sound_shared_word24_owners_tests PRIVATE
        halo2_native_tu15_sound_gain_fanout
        halo2_native_tu15_sound_mono_distortion
        halo2_native_tu15_sound_two_product_accumulator
        halo2_native_tu15_sound_chorus
        halo2_native_tu15_sound_crosstalk
        halo2_native_tu15_sound_controller_arithmetic)
    add_test(NAME native_tu15_sound_shared_word24_owners_tests COMMAND native_tu15_sound_shared_word24_owners_tests)
    set_tests_properties(native_tu15_sound_shared_word24_owners_tests PROPERTIES TIMEOUT 60)
endif()
