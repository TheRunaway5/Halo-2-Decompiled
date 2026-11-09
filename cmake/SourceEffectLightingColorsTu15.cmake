add_library(halo2_effect_lighting_colors_tu15 STATIC
    src/rendering/effects/source_effect_lighting_colors_tu15.cpp)
target_include_directories(halo2_effect_lighting_colors_tu15 PUBLIC include)
target_compile_features(halo2_effect_lighting_colors_tu15 PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_effect_lighting_colors_tu15 PRIVATE /fp:strict)
else()
    target_compile_options(halo2_effect_lighting_colors_tu15 PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(halo2_effect_lighting_colors_tu15 PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_effect_lighting_colors_tu15 PUBLIC -fsanitize=address,undefined)
    endif()
endif()
target_link_libraries(halo2_systems PUBLIC halo2_effect_lighting_colors_tu15)
if(BUILD_TESTING)
    add_executable(source_effect_lighting_colors_tu15_tests
        tests/source_effect_lighting_colors_tu15_tests.cpp)
    target_link_libraries(source_effect_lighting_colors_tu15_tests PRIVATE
        halo2_effect_lighting_colors_tu15 nlohmann_json::nlohmann_json)
    add_test(NAME source_effect_lighting_colors_tu15
        COMMAND source_effect_lighting_colors_tu15_tests)
endif()
