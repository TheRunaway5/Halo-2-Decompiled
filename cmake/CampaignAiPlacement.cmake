# Readable ai_place overloads over the one retained original campaign world.
target_sources(halo2_systems PRIVATE src/source_ai_placement.cpp)
if(BUILD_TESTING)
    add_executable(source_ai_placement_tests tests/source_ai_placement_tests.cpp
        src/source_ai_placement.cpp)
    target_include_directories(source_ai_placement_tests PRIVATE include)
    target_link_libraries(source_ai_placement_tests PRIVATE nlohmann_json::nlohmann_json)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(NAME original_campaign_ai_placement COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py"
            --output-root "${CMAKE_CURRENT_BINARY_DIR}/memory-budget/campaign-ai-placement"
            --timeout 120 -- $<TARGET_FILE:source_ai_placement_tests>
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_placement_original.json")
        set_tests_properties(original_campaign_ai_placement PROPERTIES RUN_SERIAL TRUE TIMEOUT 150)
        add_executable(source_campaign_ai_placement_tu15_tests tests/source_campaign_ai_placement_tu15_tests.cpp)
        target_link_libraries(source_campaign_ai_placement_tu15_tests PRIVATE halo2_systems)
        if(EXISTS "${HALO2_TU15_GAMEPLAY_XBE}" AND EXISTS "${HALO2_TU15_CAMPAIGN_CAIRO_MAP}")
            foreach(mode counted authored starting-location)
                set(name "native_campaign_ai_placement_tu15_${mode}")
                add_test(NAME "${name}" COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                    --output-root "${CMAKE_CURRENT_BINARY_DIR}/memory-budget/campaign-ai-world-${mode}"
                    --timeout 600 -- $<TARGET_FILE:source_campaign_ai_placement_tu15_tests>
                    "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TU15_CAMPAIGN_CAIRO_MAP}" "${mode}")
                set_tests_properties("${name}" PROPERTIES RUN_SERIAL TRUE TIMEOUT 630)
            endforeach()
        endif()
    endif()
endif()
