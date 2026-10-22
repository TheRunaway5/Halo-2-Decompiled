target_sources(halo2_systems PRIVATE src/gameplay/units/death_cycle_runtime_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_coop.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/death_cycle_runtime_tu15_generated.cpp src/campaign/source_campaign_coop.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_cycle_runtime_tu15_generated.cpp src/campaign/source_campaign_coop.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(campaign_coop_respawn_tests tests/campaign_coop_respawn_tests.cpp)
    target_link_libraries(campaign_coop_respawn_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_coop_respawn COMMAND campaign_coop_respawn_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_coop_respawn_original.json)
    add_test(NAME original_campaign_death_camera COMMAND death_camera_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_death_camera_original.tsv)
    add_test(NAME original_campaign_death_lifecycle COMMAND death_camera_lifecycle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_death_lifecycle_original.tsv)
    set_tests_properties(original_campaign_coop_respawn original_campaign_death_camera
        original_campaign_death_lifecycle PROPERTIES SKIP_RETURN_CODE 77)
    if(UNIX AND NOT APPLE)
        add_executable(native_death_cycle_tu15_tests tests/native_death_cycle_tu15_tests.cpp)
        target_link_libraries(native_death_cycle_tu15_tests PRIVATE halo2_systems)
        add_executable(native_death_match_tu15_tests tests/native_death_match_tu15_tests.cpp)
        target_link_libraries(native_death_match_tu15_tests PRIVATE halo2_systems)
        add_executable(native_death_render_tu15_tests tests/native_death_render_tu15_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
        target_link_libraries(native_death_render_tu15_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
        target_compile_definitions(native_death_render_tu15_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        add_dependencies(native_death_render_tu15_tests halo2_shaders)
        add_test(NAME native_death_render_tu15 COMMAND native_death_render_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
            ${CMAKE_CURRENT_BINARY_DIR}/native-death-render-tu15.ppm)
        set_tests_properties(native_death_render_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
        add_executable(native_death_remote_render_tu15_tests tests/native_death_remote_render_tu15_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
        target_link_libraries(native_death_remote_render_tu15_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
        target_compile_definitions(native_death_remote_render_tu15_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        add_dependencies(native_death_remote_render_tu15_tests halo2_shaders)
        add_test(NAME native_death_remote_render_tu15 COMMAND native_death_remote_render_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_death_remote_render_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
        add_test(NAME native_vehicle_camera_remote_lifetime_tu15 COMMAND native_death_remote_render_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map)
        set_tests_properties(native_vehicle_camera_remote_lifetime_tu15 PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 180 ENVIRONMENT "HALO2_DEATH_SEATED_COMPONENT=1")
        add_test(NAME native_death_match_stationary_tu15 COMMAND native_death_match_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_death_match_stationary_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME native_death_match_remote_tu15 COMMAND native_death_match_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_death_match_remote_tu15 PROPERTIES
            ENVIRONMENT HALO2_TU15_TEST_NETWORK=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        foreach(transport IN ITEMS local remote)
            add_test(NAME native_death_spectate_${transport}_tu15 COMMAND native_death_match_tu15_tests
                "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
            set_tests_properties(native_death_spectate_${transport}_tu15 PROPERTIES
                ENVIRONMENT HALO2_TU15_TEST_SPECTATE=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        endforeach()
        set_property(TEST native_death_spectate_remote_tu15 APPEND PROPERTY
            ENVIRONMENT HALO2_TU15_TEST_NETWORK=1)
        foreach(transport IN ITEMS local remote)
            add_test(NAME native_death_spectate_auto_${transport}_tu15 COMMAND native_death_match_tu15_tests
                "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
            set_tests_properties(native_death_spectate_auto_${transport}_tu15 PROPERTIES
                ENVIRONMENT HALO2_TU15_TEST_AUTO_SPECTATE=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        endforeach()
        set_property(TEST native_death_spectate_auto_remote_tu15 APPEND PROPERTY
            ENVIRONMENT HALO2_TU15_TEST_NETWORK=1)
        add_test(NAME native_death_match_moving_tu15 COMMAND native_death_match_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_death_match_moving_tu15 PROPERTIES
            ENVIRONMENT HALO2_TU15_TEST_MOVING_VICTIM=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME native_death_match_lifetime_tu15 COMMAND native_death_match_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_death_match_lifetime_tu15 PROPERTIES
            ENVIRONMENT HALO2_TU15_TEST_LONG_CORPSE=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        foreach(kind IN ITEMS melee radius fall)
            add_test(NAME native_death_match_${kind}_tu15 COMMAND native_death_match_tu15_tests
                "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
            set_tests_properties(native_death_match_${kind}_tu15 PROPERTIES
                ENVIRONMENT "HALO2_TU15_TEST_DAMAGE=${kind}" SKIP_RETURN_CODE 77 TIMEOUT 120)
        endforeach()
        add_test(NAME native_death_camera_tu15 COMMAND native_death_cycle_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_death_camera_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME native_campaign_camera_policy_tu15 COMMAND native_death_cycle_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_campaign_camera_policy_tu15 PROPERTIES
            ENVIRONMENT HALO2_TU15_TEST_CAMPAIGN_CAMERA=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        foreach(branch IN ITEMS ordinary predefined continuous)
            add_test(NAME native_death_cleanup_${branch}_tu15 COMMAND physics_live_world_tu15_tests
                "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
            set_tests_properties(native_death_cleanup_${branch}_tu15 PROPERTIES
                ENVIRONMENT HALO2_TU15_TEST_DEATH=1 SKIP_RETURN_CODE 77 TIMEOUT 120)
        endforeach()
        set_property(TEST native_death_cleanup_predefined_tu15 APPEND PROPERTY
            ENVIRONMENT HALO2_TU15_TEST_LANDING_DEATH=1)
        set_property(TEST native_death_cleanup_continuous_tu15 APPEND PROPERTY
            ENVIRONMENT HALO2_TU15_TEST_DEATH_TICKS=1)
    endif()
    add_executable(death_cycle_helpers_tu15_tests tests/death_cycle_helpers_tu15_tests.cpp)
    target_link_libraries(death_cycle_helpers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_death_cycle_helpers_tu15 COMMAND death_cycle_helpers_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_cycle_helpers_tu15.json)
    set_tests_properties(original_death_cycle_helpers_tu15 PROPERTIES SKIP_RETURN_CODE 77)
endif()
