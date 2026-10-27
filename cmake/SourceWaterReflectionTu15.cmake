target_sources(halo2_cache PRIVATE src/rendering/world/source_water_reflection_tu15.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/world/source_water_reflection_tu15.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/source_water_reflection_tu15.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(source_water_reflection_tu15_tests EXCLUDE_FROM_ALL tests/source_water_reflection_tu15_tests.cpp)
    target_link_libraries(source_water_reflection_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_water_reflection COMMAND source_water_reflection_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_water_reflection_tu15_original.json")
    set_tests_properties(original_tu15_water_reflection PROPERTIES TIMEOUT 30)
endif()
