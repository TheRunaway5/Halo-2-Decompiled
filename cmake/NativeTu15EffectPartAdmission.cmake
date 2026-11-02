add_library(halo2_native_tu15_effect_part_admission STATIC
    src/rendering/effects/source_effect_part_admission_tu15.cpp)
target_include_directories(halo2_native_tu15_effect_part_admission PUBLIC include)
target_compile_features(halo2_native_tu15_effect_part_admission PUBLIC cxx_std_20)
target_link_libraries(halo2_systems PUBLIC halo2_native_tu15_effect_part_admission)
if(BUILD_TESTING)
    # Compile the actual consumers without pulling the full game into this
    # bounded CPU check. This is compilation evidence, not live playback.
    add_library(native_tu15_effect_sound_consumer_compile OBJECT EXCLUDE_FROM_ALL
        src/rendering/effects/world_effects.cpp src/rendering/effects/effect_sound.cpp)
    target_include_directories(native_tu15_effect_sound_consumer_compile PRIVATE include src)
    target_compile_features(native_tu15_effect_sound_consumer_compile PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_effect_sound_consumer_compile PRIVATE halo2_glm SDL2::SDL2)
    add_executable(native_tu15_effect_part_admission_tests tests/native_tu15_effect_part_admission_tests.cpp)
    target_link_libraries(native_tu15_effect_part_admission_tests PRIVATE
        halo2_native_tu15_effect_part_admission nlohmann_json::nlohmann_json)
    add_dependencies(native_tu15_effect_part_admission_tests native_tu15_effect_sound_consumer_compile)
    add_test(NAME native_tu15_effect_part_admission_reference COMMAND native_tu15_effect_part_admission_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_part_admission_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_sound_parts_destruction_tu15.json")
    set_tests_properties(native_tu15_effect_part_admission_reference PROPERTIES TIMEOUT 60)
endif()
