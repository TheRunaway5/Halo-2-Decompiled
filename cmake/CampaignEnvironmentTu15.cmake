# Complete native source environment producer and ordinary main publication.
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_environment_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_environment_frame_tu15.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/campaign/source_campaign_environment_tu15_generated.cpp
        src/campaign/source_campaign_environment_frame_tu15.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(source_effect_frame_phase_tu15_tests tests/source_effect_frame_phase_tu15_tests.cpp)
    target_link_libraries(source_effect_frame_phase_tu15_tests PRIVATE halo2_systems)
    add_test(NAME source_effect_frame_phase_tu15 COMMAND source_effect_frame_phase_tu15_tests)
    add_executable(campaign_environment_publication_tu15_tests tests/campaign_environment_publication_tu15_tests.cpp)
    target_link_libraries(campaign_environment_publication_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_environment_caller_tu15 COMMAND campaign_environment_publication_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_environment_caller_tu15.json")
    set_tests_properties(original_campaign_environment_caller_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    foreach(_halo2_env_case IN ITEMS opening streets bridge ship)
        if(_halo2_env_case STREQUAL "opening")
            set(_halo2_env_map 00a_introduction)
            set(_halo2_env_bsp 0)
        elseif(_halo2_env_case STREQUAL "streets")
            set(_halo2_env_map 03a_oldmombasa)
            set(_halo2_env_bsp 0)
        elseif(_halo2_env_case STREQUAL "bridge")
            set(_halo2_env_map 03a_oldmombasa)
            set(_halo2_env_bsp 3)
        else()
            set(_halo2_env_map 07b_forerunnership)
            set(_halo2_env_bsp 1)
        endif()
        add_test(NAME native_campaign_environment_${_halo2_env_case}_tu15 COMMAND campaign_environment_publication_tu15_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_environment_caller_tu15.json"
            "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${_halo2_env_map}.map" "${_halo2_env_bsp}")
        set_tests_properties(native_campaign_environment_${_halo2_env_case}_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    endforeach()
    add_executable(campaign_environment_mixer_tu15_tests tests/campaign_environment_mixer_tu15_tests.cpp)
    target_link_libraries(campaign_environment_mixer_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_campaign_environment_mixer_tu15 COMMAND campaign_environment_mixer_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/03a_oldmombasa.map")
    set_tests_properties(native_campaign_environment_mixer_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
endif()

# Completed campaign main-location/environment to WORLD particle consumer.
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_environment_view_tu15.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/campaign/source_campaign_environment_view_tu15.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(campaign_environment_view_tu15_tests tests/campaign_environment_view_tu15_tests.cpp)
    target_link_libraries(campaign_environment_view_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_campaign_environment_view_tu15 COMMAND campaign_environment_view_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_environment_location_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_environment_caller_tu15.json"
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
    set_tests_properties(native_campaign_environment_view_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300 RUN_SERIAL TRUE)
    add_executable(campaign_particle_composition_tu15_tests tests/campaign_particle_composition_tu15_tests.cpp)
    target_link_libraries(campaign_particle_composition_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_campaign_particle_composition_tu15 COMMAND campaign_particle_composition_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
    set_tests_properties(native_campaign_particle_composition_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300 RUN_SERIAL TRUE)
endif()

# Unchanged original fog scalar/RGB consumers, without a live World or GPU.
if(BUILD_TESTING)
    add_executable(campaign_fog_externs_tu15_tests tests/campaign_fog_externs_tu15_tests.cpp)
    target_link_libraries(campaign_fog_externs_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_fog_externs_tu15 COMMAND campaign_fog_externs_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_fog_externs_tu15.json")
    set_tests_properties(original_campaign_fog_externs_tu15 PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Optional unchanged-original bloom tap coordinates; no World or GPU required.
if(BUILD_TESTING)
    add_executable(campaign_bloom_sampling_tu15_tests tests/campaign_bloom_sampling_tu15_tests.cpp)
    target_link_libraries(campaign_bloom_sampling_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    set(HALO2_TU15_BLOOM_COORDINATES_FIXTURE "" CACHE FILEPATH "Optional authenticated original bloom tap coordinate fixture")
    add_test(NAME original_campaign_bloom_sampling_tu15 COMMAND campaign_bloom_sampling_tu15_tests
        "${HALO2_TU15_BLOOM_COORDINATES_FIXTURE}")
    set_tests_properties(original_campaign_bloom_sampling_tu15 PROPERTIES SKIP_RETURN_CODE 77)
endif()
