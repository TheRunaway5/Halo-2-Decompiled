target_link_libraries(halo2_cache PRIVATE halo2_snapshot_storage)
# The shared tag module is linked into every cache consumer and the engine.
target_sources(halo2_cache PRIVATE
    src/tags/types.cpp
    src/tags/binary.cpp
    src/tags/directory.cpp
    src/tags/system.cpp)
add_executable(inspect_tags tools/inspect_tags.cpp)
target_link_libraries(inspect_tags PRIVATE halo2_cache)
if(BUILD_TESTING)
    add_executable(generate_native_tu15_operands tools/generate_native_tu15_operands.cpp)
    target_link_libraries(generate_native_tu15_operands PRIVATE halo2_cache)
    add_executable(native_tu15_operand_tests tests/native_tu15_operand_tests.cpp)
    target_link_libraries(native_tu15_operand_tests PRIVATE halo2_cache)
    add_test(NAME native_tu15_operands_without_executable COMMAND native_tu15_operand_tests)
    add_test(NAME native_tu15_operands_reference COMMAND native_tu15_operand_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_tu15_operands_reference PROPERTIES SKIP_RETURN_CODE 77)
    find_program(HALO2_OPERAND_SCHEMA_CLANG NAMES clang++)
    if(HALO2_OPERAND_SCHEMA_CLANG)
        add_test(NAME native_tu15_operand_schema COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_native_tu15_operand_schema.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --compiler "${HALO2_OPERAND_SCHEMA_CLANG}" --check)
    endif()
    add_executable(multiplayer_snapshot_check_tests tests/multiplayer_snapshot_check_tests.cpp)
    target_link_libraries(multiplayer_snapshot_check_tests PRIVATE halo2_systems)
    add_test(NAME native_multiplayer_snapshot_checks COMMAND multiplayer_snapshot_check_tests)
    add_executable(cache_file_snapshot_tests tests/cache_file_snapshot_tests.cpp)
    target_link_libraries(cache_file_snapshot_tests PRIVATE halo2_snapshot_storage)
    add_test(NAME cache_file_snapshot COMMAND cache_file_snapshot_tests)
    set_tests_properties(cache_file_snapshot PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(native_footstep_position_tests tests/native_footstep_position_tests.cpp)
    target_link_libraries(native_footstep_position_tests PRIVATE halo2_systems)
    add_test(NAME native_footstep_position COMMAND native_footstep_position_tests)
    add_executable(scene_capsule_contact_tests tests/scene_capsule_contact_tests.cpp)
    target_link_libraries(scene_capsule_contact_tests PRIVATE halo2_systems)
    add_test(NAME native_scene_capsule_contacts COMMAND scene_capsule_contact_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/headlong.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    set_tests_properties(native_scene_capsule_contacts PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tag_vehicle_template_tests tests/tag_vehicle_template_tests.cpp)
    target_link_libraries(tag_vehicle_template_tests PRIVATE halo2_systems)
    add_test(NAME tag_vehicle_template_original_assets COMMAND tag_vehicle_template_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(tag_vehicle_template_original_assets PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tag_object_prediction_tests tests/tag_object_prediction_tests.cpp)
    target_link_libraries(tag_object_prediction_tests PRIVATE halo2_systems)
    add_test(NAME tag_object_prediction_original_assets COMMAND tag_object_prediction_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/object_prediction_resources.tsv")
    set_tests_properties(tag_object_prediction_original_assets PROPERTIES SKIP_RETURN_CODE 77)
    # Explicit live verification target; run manually with original files and
    # a display. It drives SDL inputs through complete rendered matches.
    add_executable(tag_gameplay_match_tests tests/tag_gameplay_match_tests.cpp src/multiplayer_runtime.cpp)
    target_link_libraries(tag_gameplay_match_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan glm::glm)
    target_compile_definitions(tag_gameplay_match_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(tag_gameplay_match_tests halo2_shaders)
    # Separate host/client processes exercise each rendered tag world.
    add_executable(tag_network_match_tests tests/tag_network_match_tests.cpp src/multiplayer_runtime.cpp)
    target_link_libraries(tag_network_match_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan glm::glm)
    target_compile_definitions(tag_network_match_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(tag_network_match_tests halo2_shaders)
    # Manual guarded simulation: one real tag world and lightweight UDP peers.
    # This does not start SDL devices or prove presentation/live-match parity.
    add_executable(tag_multiplayer_simulation_tests tests/tag_multiplayer_simulation_tests.cpp)
    target_link_libraries(tag_multiplayer_simulation_tests PRIVATE halo2_systems)
    add_executable(tag_campaign_simulation_tests tests/tag_campaign_simulation_tests.cpp)
    target_link_libraries(tag_campaign_simulation_tests PRIVATE halo2_renderer)
    add_executable(tag_campaign_residency_tests tests/tag_campaign_residency_tests.cpp)
    target_link_libraries(tag_campaign_residency_tests PRIVATE halo2_systems)
    add_executable(tag_campaign_definition_tests tests/tag_campaign_definition_tests.cpp)
    target_link_libraries(tag_campaign_definition_tests PRIVATE halo2_systems)
    add_executable(tag_campaign_encounter_tests tests/tag_campaign_encounter_tests.cpp)
    target_link_libraries(tag_campaign_encounter_tests PRIVATE halo2_systems)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_library(tag_campaign_runtime_entry OBJECT src/main.cpp)
        target_link_libraries(tag_campaign_runtime_entry PRIVATE halo2_renderer)
        target_compile_definitions(tag_campaign_runtime_entry PRIVATE main=tag_campaign_runtime_entry
            HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        add_executable(tag_campaign_gameplay_tests tests/tag_campaign_gameplay_tests.cpp
            src/frontend.cpp src/multiplayer_runtime.cpp $<TARGET_OBJECTS:tag_campaign_runtime_entry>)
        target_link_libraries(tag_campaign_gameplay_tests PRIVATE halo2_renderer)
        target_compile_definitions(tag_campaign_gameplay_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        target_link_options(tag_campaign_gameplay_tests PRIVATE
            "-Wl,--wrap=_ZN5halo210Simulation4stepERKNS_8ControlsE" "-Wl,--wrap=SDL_CreateWindow")
        add_dependencies(tag_campaign_gameplay_tests halo2_shaders)
    endif()
    add_test(NAME tag_system_engine_access COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_engine_tag_access.py" "${CMAKE_CURRENT_SOURCE_DIR}")
    add_executable(tag_system_tests tests/tag_system_tests.cpp)
    target_link_libraries(tag_system_tests PRIVATE halo2_cache)
    add_test(NAME tag_system_contract COMMAND tag_system_tests)
    add_executable(tag_system_asset_tests tests/tag_system_asset_tests.cpp)
    target_link_libraries(tag_system_asset_tests PRIVATE halo2_cache)
    add_test(NAME tag_system_original_assets COMMAND tag_system_asset_tests
        "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(tag_system_original_assets PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tag_system_oracle_tests tests/tag_system_oracle_tests.cpp)
    target_link_libraries(tag_system_oracle_tests PRIVATE halo2_cache)
    add_test(NAME tag_system_retail_oracle COMMAND tag_system_oracle_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tag_system_retail.tsv"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(tag_system_retail_oracle PROPERTIES SKIP_RETURN_CODE 77)
endif()
