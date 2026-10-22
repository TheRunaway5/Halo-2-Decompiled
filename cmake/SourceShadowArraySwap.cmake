add_library(halo2_shadow_array_swap_tu15 STATIC src/rendering/shadows/source_shadow_array_swap_tu15_generated.cpp)
target_include_directories(halo2_shadow_array_swap_tu15 PUBLIC include PRIVATE src)
target_compile_options(halo2_shadow_array_swap_tu15 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
target_link_libraries(halo2_physics_world_runtime PUBLIC halo2_shadow_array_swap_tu15)
if(BUILD_TESTING)
    add_executable(shadow_array_swap_tu15_tests tests/shadow_array_swap_tu15_tests.cpp)
    target_link_libraries(shadow_array_swap_tu15_tests PRIVATE halo2_shadow_array_swap_tu15 nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_shadow_array_swap COMMAND shadow_array_swap_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shadow_array_swap_tu15_original.json")
endif()
