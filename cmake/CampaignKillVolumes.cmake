# One borrowed owner for original campaign kill-volume state and scheduling.
target_sources(halo2_systems PRIVATE src/source_kill_volumes.cpp)
target_sources(halo2_systems PRIVATE src/source_ai_placement.cpp)
if(BUILD_TESTING)
    add_executable(source_ai_placement_tests tests/source_ai_placement_tests.cpp
        src/source_ai_placement.cpp)
    target_include_directories(source_ai_placement_tests PRIVATE include)
    target_link_libraries(source_ai_placement_tests PRIVATE nlohmann_json::nlohmann_json)
    add_executable(source_kill_volumes_tests tests/source_kill_volumes_tests.cpp
        src/source_kill_volumes.cpp src/source_runtime_memory.cpp)
    target_include_directories(source_kill_volumes_tests PRIVATE include)
    target_link_libraries(source_kill_volumes_tests PRIVATE halo2_snapshot_storage
        nlohmann_json::nlohmann_json Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(NAME original_campaign_ai_placement COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py"
            --output-root "${CMAKE_CURRENT_BINARY_DIR}/memory-budget/campaign-ai-placement"
            --timeout 120 -- $<TARGET_FILE:source_ai_placement_tests>
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_ai_placement_original.json")
        set_tests_properties(original_campaign_ai_placement PROPERTIES RUN_SERIAL TRUE TIMEOUT 150)
        add_executable(source_campaign_kill_volumes_tu15_tests tests/source_campaign_kill_volumes_tu15_tests.cpp)
        target_link_libraries(source_campaign_kill_volumes_tu15_tests PRIVATE halo2_systems)
        add_executable(source_campaign_ai_placement_tu15_tests tests/source_campaign_ai_placement_tu15_tests.cpp)
        target_link_libraries(source_campaign_ai_placement_tu15_tests PRIVATE halo2_systems)
        set(HALO2_TU15_CAMPAIGN_CAIRO_MAP
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map"
            CACHE FILEPATH "Owned original Cairo map for campaign owner validation")
        if(EXISTS "${HALO2_TU15_GAMEPLAY_XBE}" AND EXISTS "${HALO2_TU15_CAMPAIGN_CAIRO_MAP}")
            add_test(NAME native_campaign_ai_placement_tu15 COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "${CMAKE_CURRENT_BINARY_DIR}/memory-budget/campaign-ai-world"
                --timeout 600 -- $<TARGET_FILE:source_campaign_ai_placement_tu15_tests>
                "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TU15_CAMPAIGN_CAIRO_MAP}")
            set_tests_properties(native_campaign_ai_placement_tu15 PROPERTIES RUN_SERIAL TRUE TIMEOUT 630)
            add_test(NAME native_campaign_kill_volumes_tu15 COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "${CMAKE_CURRENT_BINARY_DIR}/memory-budget/campaign-kill-world"
                --timeout 600 -- $<TARGET_FILE:source_campaign_kill_volumes_tu15_tests>
                "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TU15_CAMPAIGN_CAIRO_MAP}")
            set_tests_properties(native_campaign_kill_volumes_tu15 PROPERTIES RUN_SERIAL TRUE TIMEOUT 630)
        endif()
        add_test(NAME original_campaign_kill_volumes COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py"
            --output-root "${CMAKE_CURRENT_BINARY_DIR}/memory-budget/campaign-kill-volumes"
            --timeout 120 -- $<TARGET_FILE:source_kill_volumes_tests>
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_kill_volumes_original.json")
        set_tests_properties(original_campaign_kill_volumes PROPERTIES RUN_SERIAL TRUE TIMEOUT 150)
    endif()
endif()
