include_guard(GLOBAL)
# Route owners join the runtime and composed publication tests through
# CampaignNativeAiPublication. This target adds their direct physical replay.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    get_target_property(_halo2_route_sources native_campaign_ai_publication_tu15_tests SOURCES)
    list(FILTER _halo2_route_sources EXCLUDE REGEX "^tests/")
    list(REMOVE_DUPLICATES _halo2_route_sources)
    add_executable(native_campaign_ai_route_tu15_tests
        tests/native_campaign_ai_route_tu15_tests.cpp ${_halo2_route_sources})
    target_include_directories(native_campaign_ai_route_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_ai_route_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_ai_route_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_ai_route_tu15_tests PRIVATE nlohmann_json::nlohmann_json
        halo2_native_tu15_data_array_delete halo2_native_tu15_data_array_lookup halo2_native_pair_consumer
        halo2_systems halo2_native_weapon_effect_dispatch_tu15)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_ai_route_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_ai_route_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_ai_route_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_route_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_route_tu15.json.gz")
    set_tests_properties(native_campaign_ai_route_tu15 PROPERTIES TIMEOUT 300)
    foreach(_owner biped_refresh navigation_lookup navigation_parents route_segments route_traversal route_obstacle_geometry route_obstacles route_smoothing route_corridor route_special_link)
        if(_owner STREQUAL "route_traversal" OR _owner STREQUAL "route_special_link")
            add_executable(native_campaign_ai_${_owner}_tu15_tests
                tests/native_campaign_ai_${_owner}_tu15_tests.cpp)
        elseif(_owner STREQUAL "navigation_parents")
            add_executable(native_campaign_ai_${_owner}_tu15_tests
                tests/native_campaign_ai_navigation_lookup_tu15_tests.cpp ${_halo2_route_sources})
            target_compile_definitions(native_campaign_ai_${_owner}_tu15_tests PRIVATE HALO2_NAVIGATION_PARENT_REPLAY=1)
        else()
            add_executable(native_campaign_ai_${_owner}_tu15_tests
                tests/native_campaign_ai_${_owner}_tu15_tests.cpp ${_halo2_route_sources})
        endif()
        if(_owner STREQUAL "route_traversal")
            target_sources(native_campaign_ai_${_owner}_tu15_tests PRIVATE
                src/campaign/ai/native_campaign_ai_route_traversal_tu15.cpp)
        elseif(_owner STREQUAL "route_special_link")
            target_link_libraries(native_campaign_ai_${_owner}_tu15_tests PRIVATE halo2_physics_world_runtime)
        endif()
        target_include_directories(native_campaign_ai_${_owner}_tu15_tests PRIVATE include src)
        target_compile_features(native_campaign_ai_${_owner}_tu15_tests PRIVATE cxx_std_20)
        target_compile_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        target_link_libraries(native_campaign_ai_${_owner}_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        if(NOT _owner STREQUAL "route_traversal")
            target_link_libraries(native_campaign_ai_${_owner}_tu15_tests PRIVATE
                halo2_native_tu15_data_array_delete halo2_native_tu15_data_array_lookup halo2_native_pair_consumer
                halo2_systems halo2_native_weapon_effect_dispatch_tu15)
        endif()
        if(HALO2_SANITIZERS AND NOT MSVC)
            target_compile_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fsanitize=address,undefined)
        endif()
        add_test(NAME native_campaign_ai_${_owner}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:native_campaign_ai_${_owner}_tu15_tests>"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_${_owner}_tu15.json.gz")
        set_tests_properties(native_campaign_ai_${_owner}_tu15 PROPERTIES TIMEOUT 300)
    endforeach()
    add_test(NAME native_campaign_ai_route_traversal_links_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_route_traversal_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_route_traversal_links_tu15.json.gz")
    set_tests_properties(native_campaign_ai_route_traversal_links_tu15 PROPERTIES TIMEOUT 300)
    add_test(NAME native_campaign_ai_route_segments_child_prefix_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_route_segments_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_route_segments_child_prefix_tu15.json.gz")
    set_tests_properties(native_campaign_ai_route_segments_child_prefix_tu15 PROPERTIES TIMEOUT 300)
    foreach(_cohort extended prefix)
        add_test(NAME native_campaign_ai_route_corridor_${_cohort}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:native_campaign_ai_route_corridor_tu15_tests>"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_route_corridor_${_cohort}_tu15.json.gz")
        set_tests_properties(native_campaign_ai_route_corridor_${_cohort}_tu15 PROPERTIES TIMEOUT 300)
    endforeach()
endif()
include(cmake/CampaignNativeAiCommands.cmake)
