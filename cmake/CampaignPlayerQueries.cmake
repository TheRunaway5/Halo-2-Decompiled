# Campaign script queries borrow the retained player/options bank.
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_player_queries_tu15.cpp)
if(BUILD_TESTING)
    add_executable(campaign_player_queries_tu15_tests
        tests/campaign_player_queries_tu15_tests.cpp
        src/campaign/source_campaign_player_queries_tu15.cpp src/campaign/scripting/source_script_lists.cpp
        src/core/datum_pool.cpp src/gameplay/objects/raw_datum_pool.cpp src/gameplay/objects/raw_datum_pool_identity.cpp
        src/gameplay/objects/raw_datum_pool_startup.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(campaign_player_queries_tu15_tests PRIVATE include)
    target_link_libraries(campaign_player_queries_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_player_queries_tu15
        COMMAND campaign_player_queries_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_player_queries_tu15.json")
endif()
