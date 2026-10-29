# Same retained bank and native resource owners for the TU1.5 live world.
# Keep new strict-FP owners in a small library: adding a per-source flag to
# halo2_systems changes its shared flags file and rebuilds unrelated systems.
add_library(halo2_native_constraint_solver STATIC
    src/physics/native_tu15_constraint_solver.cpp
    src/physics/native_tu15_constraint_solver_rows.cpp
    src/physics/native_tu15_constraint_solver_initializer.cpp
    src/physics/native_tu15_constraint_custom_schema.cpp
    src/physics/native_tu15_constraint_clock.cpp)
target_include_directories(halo2_native_constraint_solver PUBLIC include)
target_compile_features(halo2_native_constraint_solver PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_native_constraint_solver PRIVATE /fp:strict)
else()
    target_compile_options(halo2_native_constraint_solver PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(halo2_native_constraint_solver PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_native_constraint_solver PUBLIC -fsanitize=address,undefined)
    endif()
endif()
target_link_libraries(halo2_systems PUBLIC halo2_native_constraint_solver)
add_library(halo2_native_array_reserve STATIC src/physics/native_tu15_array_reserve.cpp)
target_include_directories(halo2_native_array_reserve PUBLIC include)
target_compile_features(halo2_native_array_reserve PUBLIC cxx_std_20)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_native_array_reserve PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_native_array_reserve PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_native_array_reserve)
add_library(halo2_native_island_agents STATIC src/physics/native_tu15_island_agent_phase.cpp
    src/physics/native_tu15_island_contact_advance.cpp)
target_include_directories(halo2_native_island_agents PUBLIC include)
target_compile_features(halo2_native_island_agents PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_native_island_agents PRIVATE /fp:strict)
else()
    target_compile_options(halo2_native_island_agents PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(halo2_native_island_agents PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_native_island_agents PUBLIC -fsanitize=address,undefined)
    endif()
endif()
target_link_libraries(halo2_systems PUBLIC halo2_native_island_agents)
add_library(halo2_native_shape_bounds STATIC src/physics/native_tu15_geometry.cpp
    src/physics/native_tu15_shape_bounds.cpp src/physics/native_tu15_centered_shape_bounds.cpp
    src/physics/native_tu15_compound_shape_bounds.cpp)
target_include_directories(halo2_native_shape_bounds PUBLIC include)
target_compile_features(halo2_native_shape_bounds PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_native_shape_bounds PRIVATE /fp:strict)
else()
    target_compile_options(halo2_native_shape_bounds PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(halo2_native_shape_bounds PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_native_shape_bounds PUBLIC -fsanitize=address,undefined)
    endif()
endif()
target_link_libraries(halo2_systems PUBLIC halo2_native_shape_bounds)
target_sources(halo2_systems PRIVATE src/physics/physics_live_world_tu15.cpp src/runtime/source_runtime_cache_tags_tu15.cpp)
target_sources(halo2_systems PRIVATE src/physics/native_tu15_physics_frame.cpp
    src/physics/native_tu15_physics_frame_residuals.cpp src/physics/native_tu15_motion_forces.cpp
    src/physics/native_tu15_motion_pose.cpp src/physics/native_tu15_island_deactivation.cpp
    src/physics/native_tu15_body_deactivation_policy.cpp src/physics/native_tu15_island_transition.cpp
    src/physics/native_tu15_kinematic_motion.cpp
    src/physics/native_tu15_array_growth.cpp src/physics/native_tu15_array_allocator.cpp
    src/physics/native_tu15_world_collision.cpp
    src/physics/native_tu15_world_collision_listeners.cpp
    src/physics/native_tu15_constraint_step.cpp src/physics/native_tu15_constraint_step_residuals.cpp
    src/physics/native_tu15_dynamic_accumulator.cpp
    src/physics/native_tu15_island_collision.cpp
    src/physics/native_tu15_motion_commit.cpp
    src/physics/native_tu15_broad_phase.cpp
    src/physics/native_tu15_island_integration.cpp src/physics/native_tu15_world_integration.cpp
    src/physics/native_tu15_world_integration_residuals.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_constraint_residuals_tests EXCLUDE_FROM_ALL
        tests/native_tu15_constraint_residuals_tests.cpp)
    target_link_libraries(native_tu15_constraint_residuals_tests PRIVATE halo2_systems)
    add_test(NAME native_constraint_binding_model_tu15 COMMAND native_tu15_constraint_residuals_tests)
    set_tests_properties(native_constraint_binding_model_tu15 PROPERTIES TIMEOUT 30)
endif()
if(MSVC)
    set_source_files_properties(src/physics/native_tu15_physics_frame.cpp
        src/physics/native_tu15_physics_frame_residuals.cpp src/physics/native_tu15_motion_forces.cpp
    src/physics/native_tu15_motion_pose.cpp src/physics/native_tu15_island_deactivation.cpp
    src/physics/native_tu15_body_deactivation_policy.cpp src/physics/native_tu15_island_transition.cpp
    src/physics/native_tu15_kinematic_motion.cpp
    src/physics/native_tu15_array_growth.cpp src/physics/native_tu15_array_allocator.cpp
    src/physics/native_tu15_world_collision.cpp src/physics/native_tu15_world_collision_listeners.cpp
    src/physics/native_tu15_constraint_step.cpp src/physics/native_tu15_constraint_step_residuals.cpp
    src/physics/native_tu15_dynamic_accumulator.cpp src/physics/native_tu15_island_collision.cpp
    src/physics/native_tu15_motion_commit.cpp src/physics/native_tu15_broad_phase.cpp PROPERTIES
        COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(src/physics/native_tu15_physics_frame.cpp
        src/physics/native_tu15_physics_frame_residuals.cpp src/physics/native_tu15_motion_forces.cpp
    src/physics/native_tu15_motion_pose.cpp src/physics/native_tu15_island_deactivation.cpp
    src/physics/native_tu15_body_deactivation_policy.cpp src/physics/native_tu15_island_transition.cpp
    src/physics/native_tu15_kinematic_motion.cpp
    src/physics/native_tu15_array_growth.cpp src/physics/native_tu15_array_allocator.cpp
    src/physics/native_tu15_world_collision.cpp src/physics/native_tu15_world_collision_listeners.cpp
    src/physics/native_tu15_constraint_step.cpp src/physics/native_tu15_constraint_step_residuals.cpp
    src/physics/native_tu15_dynamic_accumulator.cpp src/physics/native_tu15_island_collision.cpp
    src/physics/native_tu15_motion_commit.cpp src/physics/native_tu15_broad_phase.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/physics/native_tu15_island_integration.cpp
        src/physics/native_tu15_world_integration.cpp src/physics/native_tu15_world_integration_residuals.cpp
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(src/physics/native_tu15_island_integration.cpp
        src/physics/native_tu15_world_integration.cpp src/physics/native_tu15_world_integration_residuals.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/DamageModelChildrenTu15.cmake")
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/world/source_teleporter_tu15_generated.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/world/source_teleporter_tu15_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/world/source_teleporter_tu15_generated.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/campaign/scripting/mcc_script_sound_world_access.cpp src/campaign/scripting/mcc_script_sound_query_service_tu15.cpp)
# Native startup residency hints use the Linux program-header iterator.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(halo2_systems PRIVATE ${CMAKE_DL_LIBS})
endif()
target_sources(halo2_systems PRIVATE src/rendering/effects/source_contrail_snapshot.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_selection_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_multiplayer_render_history_tu15.cpp
    src/rendering/shadows/source_presented_model_pose_tu15.cpp
    src/rendering/shadows/source_shadow_presentation_owner.cpp
    src/rendering/shadows/source_native_render_timeline.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/models/source_presented_model_input_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_roots_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_main_draws_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_main_inputs_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_phase_inputs_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_storage_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/source_live_effect_light_commands_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_live_shadow_ordered_draws_tu15.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/rendering/shadows/source_live_shadow_roots_tu15.cpp
        src/rendering/shadows/source_live_shadow_phase_inputs_tu15.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/runtime/source_execution_inputs.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_runtime_resources.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_cache_io_tu15.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_cache_query_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_scalar_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_listener_selection_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_gain_tu15_generated.cpp
    src/audio/source_sound_random_scalar_tu15_generated.cpp src/audio/source_sound_properties_tu15_generated.cpp
    src/audio/source_sound_random_short_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_range_selection_tu15_generated.cpp
    src/audio/source_sound_permutation_tu15_generated.cpp src/audio/source_sound_binding_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_sound_chunk_readiness_tu15_generated.cpp)
# Native sound manager kernels; same strict floating-point model as the
# routines they replace.
set(HALO2_NATIVE_SOUND_KERNELS src/audio/native_tu15_sound_manager.cpp
    src/audio/native_tu15_sound_kernels_gain.cpp src/audio/native_tu15_sound_kernels_properties.cpp
    src/audio/native_tu15_sound_kernels_listener.cpp src/audio/native_tu15_sound_kernels_resources.cpp
    src/audio/native_tu15_sound_kernels_startup.cpp src/audio/native_tu15_sound_kernels_object_loop.cpp
    src/audio/native_tu15_sound_kernels_object_loop_frame.cpp src/audio/native_tu15_sound_kernels_object_loop_update.cpp
    src/audio/native_tu15_sound_kernels_object_loop_sources.cpp
    src/audio/native_tu15_sound_kernels_campaign_audio.cpp src/audio/native_tu15_sound_kernels_campaign_playback.cpp
    src/audio/native_tu15_sound_kernels_campaign_mix.cpp src/audio/native_tu15_sound_kernels_campaign_channels.cpp
    src/audio/native_tu15_sound_kernels_unit_speech.cpp)
target_sources(halo2_systems PRIVATE ${HALO2_NATIVE_SOUND_KERNELS})
if(MSVC)
    set_source_files_properties(${HALO2_NATIVE_SOUND_KERNELS} PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(${HALO2_NATIVE_SOUND_KERNELS} PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/audio/source_sound_listener_selection_tu15_generated.cpp
        src/audio/source_sound_gain_tu15_generated.cpp src/audio/source_sound_random_scalar_tu15_generated.cpp
        src/audio/source_sound_properties_tu15_generated.cpp
        src/audio/source_sound_random_short_tu15_generated.cpp
        src/audio/source_sound_range_selection_tu15_generated.cpp src/audio/source_sound_permutation_tu15_generated.cpp
        src/audio/source_sound_binding_tu15_generated.cpp
        src/audio/source_sound_chunk_readiness_tu15_generated.cpp
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(src/audio/source_sound_listener_selection_tu15_generated.cpp
        src/audio/source_sound_gain_tu15_generated.cpp src/audio/source_sound_random_scalar_tu15_generated.cpp
        src/audio/source_sound_properties_tu15_generated.cpp
        src/audio/source_sound_random_short_tu15_generated.cpp
        src/audio/source_sound_range_selection_tu15_generated.cpp src/audio/source_sound_permutation_tu15_generated.cpp
        src/audio/source_sound_binding_tu15_generated.cpp
        src/audio/source_sound_chunk_readiness_tu15_generated.cpp
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(native_tu15_physics_frame_tests tests/native_tu15_physics_frame_tests.cpp)
    target_link_libraries(native_tu15_physics_frame_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_frame_model_tu15 COMMAND native_tu15_physics_frame_tests)
    add_executable(native_tu15_world_residuals_tests tests/native_tu15_world_residuals_tests.cpp)
    target_link_libraries(native_tu15_world_residuals_tests PRIVATE halo2_systems)
    add_test(NAME native_world_residuals_model_tu15 COMMAND native_tu15_world_residuals_tests)
    foreach(owner IN ITEMS island world)
        add_executable(native_tu15_${owner}_integration_tests tests/native_tu15_${owner}_integration_tests.cpp)
        target_link_libraries(native_tu15_${owner}_integration_tests PRIVATE halo2_systems)
        string(TOUPPER "${owner}" owner_upper)
        set(HALO2_NATIVE_${owner_upper}_INTEGRATION_TU15_FIXTURE "" CACHE FILEPATH
            "Independently executed English TU1.5 ${owner} integration comparison fixture")
        if(HALO2_NATIVE_${owner_upper}_INTEGRATION_TU15_FIXTURE)
            add_test(NAME native_${owner}_integration_original_tu15 COMMAND native_tu15_${owner}_integration_tests
                "${HALO2_NATIVE_${owner_upper}_INTEGRATION_TU15_FIXTURE}")
        endif()
    endforeach()
    foreach(owner IN ITEMS body_deactivation_policy island_transition kinematic_motion)
        add_executable(native_tu15_${owner}_tests tests/native_tu15_${owner}_tests.cpp)
        target_link_libraries(native_tu15_${owner}_tests PRIVATE halo2_systems)
        string(TOUPPER "${owner}" owner_upper)
        set(HALO2_NATIVE_${owner_upper}_TU15_FIXTURE "" CACHE FILEPATH
            "Independently executed English TU1.5 ${owner} comparison fixture")
        if(HALO2_NATIVE_${owner_upper}_TU15_FIXTURE)
            add_test(NAME native_${owner}_original_tu15 COMMAND native_tu15_${owner}_tests
                "${HALO2_NATIVE_${owner_upper}_TU15_FIXTURE}")
        endif()
    endforeach()
    add_test(NAME native_island_transition_model_tu15 COMMAND native_tu15_island_transition_tests)
    add_executable(native_tu15_island_deactivation_tests tests/native_tu15_island_deactivation_tests.cpp)
    target_link_libraries(native_tu15_island_deactivation_tests PRIVATE halo2_systems)
    set(HALO2_NATIVE_ISLAND_DEACTIVATION_TU15_FIXTURE "" CACHE FILEPATH
        "Independently executed English TU1.5 island deactivation fixture")
    if(HALO2_NATIVE_ISLAND_DEACTIVATION_TU15_FIXTURE)
        add_test(NAME native_island_deactivation_original_tu15 COMMAND native_tu15_island_deactivation_tests
            "${HALO2_NATIVE_ISLAND_DEACTIVATION_TU15_FIXTURE}")
    endif()
    add_executable(native_tu15_motion_pose_tests tests/native_tu15_motion_pose_tests.cpp)
    target_link_libraries(native_tu15_motion_pose_tests PRIVATE halo2_systems)
    set(HALO2_NATIVE_MOTION_POSE_TU15_FIXTURE "" CACHE FILEPATH
        "Independently executed English TU1.5 pose comparison fixture")
    if(HALO2_NATIVE_MOTION_POSE_TU15_FIXTURE)
        add_test(NAME native_motion_pose_original_tu15 COMMAND native_tu15_motion_pose_tests
            "${HALO2_NATIVE_MOTION_POSE_TU15_FIXTURE}")
    endif()
    add_executable(native_tu15_motion_forces_tests tests/native_tu15_motion_forces_tests.cpp)
    target_link_libraries(native_tu15_motion_forces_tests PRIVATE halo2_systems)
    set(HALO2_NATIVE_MOTION_FORCES_TU15_FIXTURE "" CACHE FILEPATH
        "Independently executed English TU1.5 motion-force comparison fixture")
    if(HALO2_NATIVE_MOTION_FORCES_TU15_FIXTURE)
        add_test(NAME native_motion_forces_original_tu15 COMMAND native_tu15_motion_forces_tests
            "${HALO2_NATIVE_MOTION_FORCES_TU15_FIXTURE}")
    endif()
    set(HALO2_NATIVE_PHYSICS_FRAME_TU15_FIXTURE "" CACHE FILEPATH
        "Independently executed English TU1.5 physics-frame comparison fixture")
    if(HALO2_NATIVE_PHYSICS_FRAME_TU15_FIXTURE)
        add_test(NAME native_physics_frame_original_tu15 COMMAND native_tu15_physics_frame_tests
            --oracle-fixture "${HALO2_NATIVE_PHYSICS_FRAME_TU15_FIXTURE}")
    endif()
    if(UNIX AND NOT APPLE)
        add_test(NAME native_source_multiplayer_event_boundary_tu15 COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_source_multiplayer_event_boundary_tu15.py
            --report-output "@HALO2_TEST_SCRATCH@/native-announcer-caller.json")
        set_tests_properties(native_source_multiplayer_event_boundary_tu15 PROPERTIES TIMEOUT 60)
    endif()
    foreach(sound_owner IN ITEMS gain random_scalar properties random_short range_selection permutation binding chunk_readiness)
        if(sound_owner STREQUAL "random_scalar")
            set(sound_owner_argument "random-scalar")
        elseif(sound_owner STREQUAL "random_short")
            set(sound_owner_argument "random-short")
        elseif(sound_owner STREQUAL "range_selection")
            set(sound_owner_argument "range-selection")
        elseif(sound_owner STREQUAL "chunk_readiness")
            set(sound_owner_argument "chunk-readiness")
        else()
            set(sound_owner_argument "${sound_owner}")
        endif()
        add_executable(source_sound_${sound_owner}_tu15_tests EXCLUDE_FROM_ALL
            tests/source_sound_${sound_owner}_tu15_tests.cpp
            src/audio/source_sound_${sound_owner}_tu15_generated.cpp)
        target_include_directories(source_sound_${sound_owner}_tu15_tests PRIVATE include src)
        target_link_libraries(source_sound_${sound_owner}_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        if(HALO2_SANITIZERS AND NOT MSVC)
            target_compile_options(source_sound_${sound_owner}_tu15_tests PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(source_sound_${sound_owner}_tu15_tests PRIVATE -fsanitize=address,undefined)
        endif()
        add_test(NAME original_tu15_sound_${sound_owner}
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_sound_parameter_cpu_tu15.py
                --owner ${sound_owner_argument}
                --binary $<TARGET_FILE:source_sound_${sound_owner}_tu15_tests>
                --generated-source ${PROJECT_SOURCE_DIR}/src/audio/source_sound_${sound_owner}_tu15_generated.cpp
                --output ${CMAKE_BINARY_DIR}/sound-${sound_owner_argument}-acceptance)
        set_tests_properties(original_tu15_sound_${sound_owner} PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    endforeach()
    add_executable(source_sound_listener_selection_tu15_tests EXCLUDE_FROM_ALL
        tests/source_sound_listener_selection_tu15_tests.cpp src/audio/source_sound_listener_selection_tu15_generated.cpp)
    target_include_directories(source_sound_listener_selection_tu15_tests PRIVATE include src)
    target_link_libraries(source_sound_listener_selection_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(source_sound_listener_selection_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(source_sound_listener_selection_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_sound_listener_selection
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_sound_listener_selection_tu15.py
            --binary $<TARGET_FILE:source_sound_listener_selection_tu15_tests>
            --output ${CMAKE_BINARY_DIR}/sound-listener-selection-acceptance
            --generated-source ${PROJECT_SOURCE_DIR}/src/audio/source_sound_listener_selection_tu15_generated.cpp)
    set_tests_properties(original_tu15_sound_listener_selection PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    add_executable(source_sound_scalar_tu15_tests EXCLUDE_FROM_ALL
        tests/source_sound_scalar_tu15_tests.cpp src/audio/source_sound_scalar_tu15_generated.cpp)
    target_include_directories(source_sound_scalar_tu15_tests PRIVATE include src)
    target_link_libraries(source_sound_scalar_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(source_sound_scalar_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(source_sound_scalar_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_sound_scalar
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_sound_scalar_tu15.py
            --binary $<TARGET_FILE:source_sound_scalar_tu15_tests>
            --generated-source ${PROJECT_SOURCE_DIR}/src/audio/source_sound_scalar_tu15_generated.cpp)
    set_tests_properties(original_tu15_sound_scalar PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    # Native sound manager kernels against the lifted routines they replace.
    add_executable(native_tu15_sound_manager_tests EXCLUDE_FROM_ALL tests/native_tu15_sound_manager_tests.cpp
        ${HALO2_NATIVE_SOUND_KERNELS} src/audio/source_sound_random_short_tu15_generated.cpp
        src/audio/source_sound_random_scalar_tu15_generated.cpp src/audio/source_sound_scalar_tu15_generated.cpp
        src/audio/source_sound_range_selection_tu15_generated.cpp src/audio/source_sound_permutation_tu15_generated.cpp
        src/audio/source_sound_sdk_tu15_generated.cpp src/audio/source_object_loop_audio_tu15_generated.cpp
        src/campaign/source_campaign_audio_tu15_generated.cpp)
    target_include_directories(native_tu15_sound_manager_tests PRIVATE include src tests)
    if(NOT MSVC)
        target_compile_options(native_tu15_sound_manager_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME native_tu15_sound_manager COMMAND native_tu15_sound_manager_tests)
    set_tests_properties(native_tu15_sound_manager PROPERTIES TIMEOUT 120)
    # Each further kernel group: its differential test against its lifted owners.
    function(halo2_native_sound_group_test group)
        set(lifted)
        foreach(owner IN LISTS ARGN)
            list(APPEND lifted src/audio/source_sound_${owner}_tu15_generated.cpp)
        endforeach()
        add_executable(native_tu15_sound_${group}_tests EXCLUDE_FROM_ALL tests/native_tu15_sound_${group}_tests.cpp
            ${HALO2_NATIVE_SOUND_KERNELS} ${lifted} src/audio/source_object_loop_audio_tu15_generated.cpp
            src/campaign/source_campaign_audio_tu15_generated.cpp)
        target_include_directories(native_tu15_sound_${group}_tests PRIVATE include src tests)
        if(NOT MSVC)
            target_compile_options(native_tu15_sound_${group}_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        endif()
        # Coverage checks read the lifted sources relative to the repository.
        add_test(NAME native_tu15_sound_${group} COMMAND native_tu15_sound_${group}_tests
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        set_tests_properties(native_tu15_sound_${group} PROPERTIES TIMEOUT 300)
    endfunction()
    halo2_native_sound_group_test(resources binding cache_query chunk_readiness)
    halo2_native_sound_group_test(gain gain)
    halo2_native_sound_group_test(properties properties)
    halo2_native_sound_group_test(listener listener_selection)
    halo2_native_sound_group_test(object_loop_update listener_selection gain)
    halo2_native_sound_group_test(object_loop_sources listener_selection)
    halo2_native_sound_group_test(object_loop_frame listener_selection gain)
    add_executable(source_sound_cache_query_tu15_tests EXCLUDE_FROM_ALL
        tests/source_sound_cache_query_tu15_tests.cpp src/audio/source_sound_cache_query_tu15_generated.cpp)
    target_include_directories(source_sound_cache_query_tu15_tests PRIVATE include src)
    target_link_libraries(source_sound_cache_query_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(source_sound_cache_query_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(source_sound_cache_query_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_sound_cache_query
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_sound_cache_query_tu15.py
            --binary $<TARGET_FILE:source_sound_cache_query_tu15_tests>
            --generated-source ${PROJECT_SOURCE_DIR}/src/audio/source_sound_cache_query_tu15_generated.cpp)
    set_tests_properties(original_tu15_sound_cache_query PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    add_executable(source_sound_cache_io_tu15_tests EXCLUDE_FROM_ALL
        tests/source_sound_cache_io_tu15_tests.cpp src/audio/source_sound_cache_io_tu15.cpp)
    target_include_directories(source_sound_cache_io_tu15_tests PRIVATE include)
    target_link_libraries(source_sound_cache_io_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(source_sound_cache_io_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(source_sound_cache_io_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_sound_cache_io
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/validate_original_sound_cache_io_tu15.py
            --binary $<TARGET_FILE:source_sound_cache_io_tu15_tests>)
    set_tests_properties(original_tu15_sound_cache_io PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/network/multiplayer_source_devices_tu15.cpp)
target_sources(halo2_systems PRIVATE src/platform/source_boot_smc_video_mode.cpp)
target_sources(halo2_systems PRIVATE src/platform/source_boot_smc_video_mode_input.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/source_object_velocity_read_only_tu15.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_runtime_dsp_section.cpp src/audio/runtime/source_runtime_sound_output.cpp src/audio/runtime/source_runtime_sound_sdk_resources.cpp src/audio/source_sound_sdk_pool.cpp)
target_sources(halo2_systems PRIVATE src/platform/native_tu15_world_memory.cpp)
target_sources(halo2_systems PRIVATE src/rendering/source_geometry_cache_frame_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics/native_tu15_physics_branch_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/native_tu15_ctf_static_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/native_tu15_player_result_data.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/camera/native_tu15_camera_packet_data.cpp)
target_sources(halo2_systems PRIVATE src/assets/native_tu15_prt_cache_data.cpp)
target_sources(halo2_systems PRIVATE src/rendering/renderer/native_tu15_main_view_data.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/scripting/native_mcc_script_builtin.cpp)
add_executable(verify_mcc_native_world tools/verify_mcc_native_world.cpp)
target_link_libraries(verify_mcc_native_world PRIVATE halo2_systems)
add_executable(verify_campaign_activation_tu15 tools/verify_campaign_activation_tu15.cpp)
target_link_libraries(verify_campaign_activation_tu15 PRIVATE halo2_renderer)
if(BUILD_TESTING)
  add_executable(mcc_script_layout_tests tests/mcc_script_layout_tests.cpp)
  target_link_libraries(mcc_script_layout_tests PRIVATE halo2_cache)
  add_test(NAME mcc_script_signature_layout COMMAND mcc_script_layout_tests)
  add_executable(mcc_cairo_vanilla_script_tests tests/mcc_cairo_vanilla_script_tests.cpp)
  target_link_libraries(mcc_cairo_vanilla_script_tests PRIVATE halo2_systems)
  set(HALO2_MCC_CAIRO_SCRIPT_REFERENCE "" CACHE FILEPATH "Optional original Cairo map for the offline script differential")
  set(HALO2_MCC_CAIRO_SCRIPT_DONOR "" CACHE FILEPATH "Owned MCC Cairo map for the script projection differential")
  add_test(NAME mcc_cairo_vanilla_script_projection COMMAND mcc_cairo_vanilla_script_tests
      "${HALO2_MCC_CAIRO_SCRIPT_DONOR}" "${HALO2_MCC_CAIRO_SCRIPT_REFERENCE}")
  set_tests_properties(mcc_cairo_vanilla_script_projection PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME mcc_cairo_vanilla_script_profile COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_cairo_vanilla_scripts.py --check)
  add_executable(mcc_cairo_vanilla_plan_tests tests/mcc_cairo_vanilla_plan_tests.cpp)
  target_link_libraries(mcc_cairo_vanilla_plan_tests PRIVATE halo2_systems)
  set(HALO2_MCC_CAIRO_SCRIPT_RAW_DONOR "" CACHE PATH "Optional authenticated raw Cairo script banks for offline conversion validation")
  add_test(NAME mcc_cairo_vanilla_internal_data COMMAND mcc_cairo_vanilla_plan_tests
      "${HALO2_MCC_CAIRO_SCRIPT_DONOR}" "${HALO2_MCC_CAIRO_SCRIPT_REFERENCE}"
      "${HALO2_MCC_CAIRO_SCRIPT_RAW_DONOR}")
  set_tests_properties(mcc_cairo_vanilla_internal_data PROPERTIES
      SKIP_RETURN_CODE 77 TIMEOUT 120 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME mcc_cairo_vanilla_authoring_profile COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_cairo_vanilla_authoring.py --check)
  add_executable(mcc_cairo_vanilla_global_count_tests tests/mcc_cairo_vanilla_global_count_tests.cpp)
  target_link_libraries(mcc_cairo_vanilla_global_count_tests PRIVATE halo2_systems)
  add_test(NAME mcc_cairo_vanilla_global_namespace COMMAND mcc_cairo_vanilla_global_count_tests
      "${HALO2_MCC_CAIRO_SCRIPT_DONOR}" "${HALO2_MCC_CAIRO_SCRIPT_REFERENCE}")
  set_tests_properties(mcc_cairo_vanilla_global_namespace PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(source_map_world_crt_tu15_tests tests/source_map_world_crt_tu15_tests.cpp)
  target_link_libraries(source_map_world_crt_tu15_tests PRIVATE halo2_systems)
  set(HALO2_MAP_WORLD_CRT_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original CPU fixture for reached TU1.5 CRT callback")
  add_test(NAME source_map_world_crt_tu15 COMMAND source_map_world_crt_tu15_tests
      "${HALO2_MAP_WORLD_CRT_REFERENCE_FIXTURE}")
  set_tests_properties(source_map_world_crt_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
  add_executable(source_object_permutation_tu15_tests tests/source_object_permutation_tu15_tests.cpp)
  target_link_libraries(source_object_permutation_tu15_tests PRIVATE halo2_systems)
  set(HALO2_OBJECT_PERMUTATION_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original CPU fixture for TU1.5 model permutation owners")
  add_test(NAME source_object_permutation_tu15 COMMAND source_object_permutation_tu15_tests
      "${HALO2_OBJECT_PERMUTATION_REFERENCE_FIXTURE}")
  set_tests_properties(source_object_permutation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
  add_executable(mcc_campaign_vanilla_tests tests/mcc_campaign_vanilla_tests.cpp)
  target_link_libraries(mcc_campaign_vanilla_tests PRIVATE halo2_cache)
  set(HALO2_MCC_CAMPAIGN_RAW_FIXTURES "" CACHE PATH "Optional owned MCC campaign metadata fixtures for authoring conversion validation")
  add_test(NAME mcc_campaign_vanilla_authoring COMMAND mcc_campaign_vanilla_tests
      "${HALO2_MCC_CAMPAIGN_RAW_FIXTURES}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-campaign-vanilla-authoring.json")
  set_tests_properties(mcc_campaign_vanilla_authoring PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_test(NAME mcc_campaign_vanilla_authoring_profile COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_campaign_vanilla.py --check)
  add_executable(native_mcc_script_builtin_tests tests/native_mcc_script_builtin_tests.cpp)
  target_link_libraries(native_mcc_script_builtin_tests PRIVATE halo2_systems)
  add_test(NAME native_mcc_script_clock_getters COMMAND native_mcc_script_builtin_tests)
  add_executable(script_clock_builtin_tests tests/script_clock_builtin_tests.cpp)
  target_link_libraries(script_clock_builtin_tests PRIVATE halo2_systems)
  add_test(NAME mcc_script_clock_initializer_runtime COMMAND script_clock_builtin_tests)
  add_executable(mcc_rumble_alias_tests tests/mcc_rumble_alias_tests.cpp)
  target_link_libraries(mcc_rumble_alias_tests PRIVATE halo2_systems)
  add_test(NAME mcc_script_rumble_original_dispatch COMMAND mcc_rumble_alias_tests)
  # This integration target consumes the caller's imported MCC map. It has
  # no dependency on an executable/reference asset or checkout-local map.
  add_executable(mcc_native_gameplay_startup_tests tests/mcc_native_gameplay_startup_tests.cpp)
  target_link_libraries(mcc_native_gameplay_startup_tests PRIVATE halo2_systems)
  add_executable(source_runtime_native_cache_tests tests/source_runtime_native_cache_tests.cpp)
  target_link_libraries(source_runtime_native_cache_tests PRIVATE halo2_systems)
  add_test(NAME source_runtime_native_cache_ownership COMMAND source_runtime_native_cache_tests)
  add_executable(native_cache_authority_tu15_tests tests/native_cache_authority_tu15_tests.cpp)
  target_link_libraries(native_cache_authority_tu15_tests PRIVATE halo2_systems)
  set(HALO2_MCC_AUTHORITY_CACHE "" CACHE FILEPATH "Owned MCC multiplayer cache for native authority admission")
  set(HALO2_MCC_AUTHORITY_REFERENCE_XBE "" CACHE FILEPATH "Optional offline TU1.5 startup reference for native authority admission")
  add_test(NAME native_mcc_cache_authority_tu15 COMMAND native_cache_authority_tu15_tests
      "${HALO2_MCC_AUTHORITY_CACHE}" "${CMAKE_CURRENT_BINARY_DIR}/native-mcc-cache-authority"
      "${HALO2_MCC_AUTHORITY_REFERENCE_XBE}")
  set_tests_properties(native_mcc_cache_authority_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(mcc_sound_duration_world_tests tests/mcc_sound_duration_world_tests.cpp)
  target_link_libraries(mcc_sound_duration_world_tests PRIVATE halo2_systems)
  add_test(NAME mcc_sound_duration_world_native COMMAND mcc_sound_duration_world_tests
      "${HALO2_MCC_AUTHORITY_CACHE}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-sound-duration-world")
  set_tests_properties(mcc_sound_duration_world_native PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(mcc_resident_cache_address_tests tests/mcc_resident_cache_address_tests.cpp)
  target_link_libraries(mcc_resident_cache_address_tests PRIVATE halo2_systems)
  add_test(NAME mcc_resident_cache_address_ownership COMMAND mcc_resident_cache_address_tests
      "${HALO2_MCC_AUTHORITY_CACHE}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-resident-cache-address")
  set_tests_properties(mcc_resident_cache_address_ownership PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
endif()
target_link_libraries(halo2_systems PRIVATE OpenSSL::Crypto)
include(cmake/NativeTranslationUnits.cmake)
halo2_partition_native_translation(halo2_physics_world_runtime src/physics/physics_live_support_tu15_generated.cpp 32)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_unit_view_reset_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_unit_animation_cancel_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/vehicles/source_vehicle_evasion_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/audio/source_sound_duration_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/audio/mcc_sound_duration_tu15.cpp)
add_library(halo2_script_sound_clock_tu15 STATIC src/campaign/scripting/source_script_sound_clock_tu15.cpp)
target_sources(halo2_script_sound_clock_tu15 PRIVATE src/campaign/scripting/source_script_sound_clock_native64_tu15.cpp)
target_include_directories(halo2_script_sound_clock_tu15 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_script_sound_clock_tu15 PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(halo2_script_sound_clock_tu15 PRIVATE /fp:strict)
else()
  target_compile_options(halo2_script_sound_clock_tu15 PRIVATE
      -ffp-contract=off -fno-fast-math -frounding-math)
endif()
target_link_libraries(halo2_systems PRIVATE halo2_script_sound_clock_tu15)
target_sources(halo2_systems PRIVATE src/audio/mcc_sound_variation_provider.cpp)
if(BUILD_TESTING)
  add_executable(source_script_sound_clock_tu15_tests tests/source_script_sound_clock_tu15_tests.cpp)
  target_link_libraries(source_script_sound_clock_tu15_tests PRIVATE halo2_script_sound_clock_tu15)
  add_test(NAME source_script_sound_clock_tu15_contract COMMAND source_script_sound_clock_tu15_tests)
  add_executable(source_script_sound_clock_hs_tail_tu15_tests tests/source_script_sound_clock_hs_tail_tu15_tests.cpp)
  target_link_libraries(source_script_sound_clock_hs_tail_tu15_tests PRIVATE halo2_script_sound_clock_tu15)
  add_test(NAME source_script_sound_clock_hs_tail_tu15_contract COMMAND source_script_sound_clock_hs_tail_tu15_tests)
  add_executable(source_script_sound_clock_native64_tu15_tests tests/source_script_sound_clock_native64_tu15_tests.cpp)
  target_link_libraries(source_script_sound_clock_native64_tu15_tests PRIVATE halo2_script_sound_clock_tu15)
  add_test(NAME source_script_sound_clock_native64_tu15_contract COMMAND source_script_sound_clock_native64_tu15_tests)
  add_executable(multiplayer_prepared_script_sound_tu15_tests tests/multiplayer_prepared_script_sound_tu15_tests.cpp)
  target_link_libraries(multiplayer_prepared_script_sound_tu15_tests PRIVATE halo2_systems)
  set(HALO2_MCC_PREPARED_SOUND_MAP "" CACHE FILEPATH "Optional owned MCC Lockout map for native prepared Audio validation")
  add_test(NAME multiplayer_prepared_script_sound_tu15_mcc_only
      COMMAND multiplayer_prepared_script_sound_tu15_tests "${HALO2_MCC_PREPARED_SOUND_MAP}")
  set_tests_properties(multiplayer_prepared_script_sound_tu15_mcc_only PROPERTIES
      SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(mcc_sound_variation_provider_tests tests/mcc_sound_variation_provider_tests.cpp)
  target_link_libraries(mcc_sound_variation_provider_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  set(HALO2_MCC_NATIVE_SOUND_MAP "" CACHE FILEPATH "Optional owned MCC map for converted native sound metadata validation")
  add_executable(mcc_sound_metadata_publication_tests tests/mcc_sound_metadata_publication_tests.cpp)
  target_link_libraries(mcc_sound_metadata_publication_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
  add_test(NAME mcc_sound_metadata_publication_mcc_only
      COMMAND mcc_sound_metadata_publication_tests "${HALO2_MCC_NATIVE_SOUND_MAP}")
  set_tests_properties(mcc_sound_metadata_publication_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_test(NAME mcc_sound_variation_provider_mcc_only
      COMMAND mcc_sound_variation_provider_tests "${HALO2_MCC_NATIVE_SOUND_MAP}")
  set_tests_properties(mcc_sound_variation_provider_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_executable(mcc_native_audio_metadata_tests tests/mcc_native_audio_metadata_tests.cpp)
  target_link_libraries(mcc_native_audio_metadata_tests PRIVATE halo2_systems)
  set(HALO2_MCC_NATIVE_AUDIO_MAP "" CACHE FILEPATH "Optional owned MCC map for converted metadata to Audio validation")
  add_test(NAME mcc_native_audio_metadata_mcc_only
      COMMAND mcc_native_audio_metadata_tests "${HALO2_MCC_NATIVE_AUDIO_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-native-audio-metadata-fixture")
  set_tests_properties(mcc_native_audio_metadata_mcc_only PROPERTIES
      SKIP_RETURN_CODE 77 TIMEOUT 300 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(mcc_native_audio_first_permutation_tests tests/mcc_native_audio_first_permutation_tests.cpp)
  target_link_libraries(mcc_native_audio_first_permutation_tests PRIVATE halo2_systems)
  set(HALO2_MCC_NATIVE_AUDIO_FIRST_PERMUTATION_MAP "" CACHE FILEPATH "Optional owned MCC map for shared initial selection to native PCM validation")
  add_test(NAME mcc_native_audio_first_permutation_mcc_only
      COMMAND mcc_native_audio_first_permutation_tests "${HALO2_MCC_NATIVE_AUDIO_FIRST_PERMUTATION_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-native-audio-first-permutation-fixture")
  set_tests_properties(mcc_native_audio_first_permutation_mcc_only PROPERTIES
      SKIP_RETURN_CODE 77 TIMEOUT 300 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(mcc_native_audio_owner_pairing_tests tests/mcc_native_audio_owner_pairing_tests.cpp)
  target_link_libraries(mcc_native_audio_owner_pairing_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME mcc_native_audio_owner_pairing_mcc_only
      COMMAND mcc_native_audio_owner_pairing_tests "${HALO2_MCC_NATIVE_AUDIO_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-native-audio-owner-pairing-fixture")
  set_tests_properties(mcc_native_audio_owner_pairing_mcc_only PROPERTIES
      SKIP_RETURN_CODE 77 TIMEOUT 300 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(mcc_script_sound_world_access_tests tests/mcc_script_sound_world_access_tests.cpp)
  target_link_libraries(mcc_script_sound_world_access_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  set(HALO2_MCC_SCRIPT_SOUND_WORLD_MAP "" CACHE FILEPATH "Optional owned MCC map for actual World script sound policy validation")
  add_test(NAME mcc_script_sound_world_access_mcc_only
      COMMAND mcc_script_sound_world_access_tests "${HALO2_MCC_SCRIPT_SOUND_WORLD_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-script-sound-world-fixture")
  set_tests_properties(mcc_script_sound_world_access_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(mcc_script_sound_selected_seconds_tests tests/mcc_script_sound_selected_seconds_tests.cpp)
  target_link_libraries(mcc_script_sound_selected_seconds_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME mcc_script_sound_selected_seconds_mcc_only
      COMMAND mcc_script_sound_selected_seconds_tests "${HALO2_MCC_SCRIPT_SOUND_WORLD_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-script-sound-selected-seconds-fixture")
  set_tests_properties(mcc_script_sound_selected_seconds_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 360)
  add_executable(mcc_script_sound_first_permutation_tests tests/mcc_script_sound_first_permutation_tests.cpp)
  target_link_libraries(mcc_script_sound_first_permutation_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  set(HALO2_MCC_SCRIPT_SOUND_FIRST_PERMUTATION_MAP "" CACHE FILEPATH "Optional owned MCC map for native shared first-permutation validation")
  add_test(NAME mcc_script_sound_first_permutation_mcc_only
      COMMAND mcc_script_sound_first_permutation_tests "${HALO2_MCC_SCRIPT_SOUND_FIRST_PERMUTATION_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-script-sound-first-permutation-fixture")
  set_tests_properties(mcc_script_sound_first_permutation_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
  add_executable(script_sound_query_tu15_tests tests/script_sound_query_tu15_tests.cpp)
  target_link_libraries(script_sound_query_tu15_tests PRIVATE halo2_systems)
  add_test(NAME script_sound_query_tu15_contract COMMAND script_sound_query_tu15_tests)
  add_executable(script_sound_query_world_tu15_tests tests/script_sound_query_world_tu15_tests.cpp)
  target_link_libraries(script_sound_query_world_tu15_tests PRIVATE halo2_systems)
  set(HALO2_MCC_SCRIPT_SOUND_QUERY_MAP "" CACHE FILEPATH "Optional owned MCC map for World-backed HS sound query validation")
  add_test(NAME script_sound_query_world_tu15_mcc_only
      COMMAND script_sound_query_world_tu15_tests "${HALO2_MCC_SCRIPT_SOUND_QUERY_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/script-sound-query-world-fixture")
  set_tests_properties(script_sound_query_world_tu15_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
  add_executable(mcc_script_sound_query_service_tu15_tests tests/mcc_script_sound_query_service_tu15_tests.cpp)
  target_link_libraries(mcc_script_sound_query_service_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME mcc_script_sound_query_service_tu15_mcc_only
      COMMAND mcc_script_sound_query_service_tu15_tests "${HALO2_MCC_SCRIPT_SOUND_QUERY_MAP}"
      "${CMAKE_CURRENT_BINARY_DIR}/mcc-script-sound-query-service-fixture")
  set_tests_properties(mcc_script_sound_query_service_tu15_mcc_only PROPERTIES
      SKIP_RETURN_CODE 77 TIMEOUT 360 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(native_tu15_submitted_lens_inputs_tests tests/native_tu15_submitted_lens_inputs_tests.cpp)
  target_link_libraries(native_tu15_submitted_lens_inputs_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME native_tu15_submitted_lens_inputs_contract COMMAND native_tu15_submitted_lens_inputs_tests)
  set(HALO2_TU15_SUBMITTED_LENS_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original CPU lens input fixture")
  add_test(NAME native_tu15_submitted_lens_inputs_reference
      COMMAND native_tu15_submitted_lens_inputs_tests "${HALO2_TU15_SUBMITTED_LENS_REFERENCE_FIXTURE}")
  set_tests_properties(native_tu15_submitted_lens_inputs_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
  add_executable(native_tu15_renderer_resource_factory_tests tests/native_tu15_renderer_resource_factory_tests.cpp)
  target_link_libraries(native_tu15_renderer_resource_factory_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME native_tu15_renderer_resource_factory_contract COMMAND native_tu15_renderer_resource_factory_tests)
  set(HALO2_TU15_RESOURCE_RESET_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original CPU resource reset fixture")
  add_test(NAME native_tu15_renderer_resource_factory_reference
      COMMAND native_tu15_renderer_resource_factory_tests "${HALO2_TU15_RESOURCE_RESET_REFERENCE_FIXTURE}")
  set_tests_properties(native_tu15_renderer_resource_factory_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
  add_executable(source_sound_duration_tu15_tests tests/source_sound_duration_tu15_tests.cpp)
  target_link_libraries(source_sound_duration_tu15_tests PRIVATE halo2_physics_world_runtime halo2_mcc_sound_tag_layout)
  add_test(NAME source_sound_duration_tu15_admission COMMAND source_sound_duration_tu15_tests)
  add_test(NAME source_sound_duration_tu15_manifest COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_source_sound_duration_tu15.py --check)
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/objects/source_object_material_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_damage_feedback_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/scripting/source_recorded_animation_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_level_handles_new_map_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/source_havok_static_lifetime_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/shadows/source_shadow_frame_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_receiver_request_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/source_transparent_group_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/source_transparent_collection_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/rendering/renderer/source_live_transparent_groups_tu15.cpp)
if(BUILD_TESTING)
    add_executable(source_transparent_group_tu15_tests EXCLUDE_FROM_ALL
        tools/compare_transparent_group_tu15.cpp src/rendering/source_transparent_group_tu15_generated.cpp)
    target_include_directories(source_transparent_group_tu15_tests PRIVATE include src)
    add_test(NAME original_tu15_transparent_group_cpu
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/compare_transparent_group_tu15.py
            --native $<TARGET_FILE:source_transparent_group_tu15_tests>
            --manifest ${PROJECT_SOURCE_DIR}/tests/data/source_transparent_group_tu15.json
            --output ${CMAKE_BINARY_DIR}/transparent-group-cpu)
    set_tests_properties(original_tu15_transparent_group_cpu PROPERTIES TIMEOUT 210 SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_cinematic_shadow_control_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_cinematic_shadow_control_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_effect_light_view_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/objects/source_object_sound_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/audio/source_object_loop_audio_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(object_loop_audio_runtime_tu15_tests EXCLUDE_FROM_ALL tests/object_loop_audio_runtime_tu15_tests.cpp)
    target_link_libraries(object_loop_audio_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(banshee_evasion_tu15_tests tests/banshee_evasion_tu15_tests.cpp)
    target_link_libraries(banshee_evasion_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json SDL2::SDL2)
    set(HALO2_MCC_BANSHEE_EVASION_MAP "" CACHE FILEPATH "Prepared MCC Coagulation map for Banshee evasion controls")
    get_filename_component(HALO2_MCC_BANSHEE_EVASION_MAPS "${HALO2_MCC_BANSHEE_EVASION_MAP}" DIRECTORY)
    add_test(NAME banshee_evasion_tu15 COMMAND banshee_evasion_tu15_tests
        "${HALO2_MCC_BANSHEE_EVASION_MAP}" "${HALO2_MCC_BANSHEE_EVASION_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/banshee-evasion.json")
    set_tests_properties(banshee_evasion_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/weapons/source_weapon_hud_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/platform/source_crt_float_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_player_departure_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/source_physics_deactivation_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/vehicles/source_vehicle_auxiliary_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/source_cloth_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/source_constraint_instance_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/source_body_impulse_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/collision/source_collision_instance_storage_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/physics/collision/source_collision_query_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/world/source_conveyor_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/vehicles/source_vehicle_occupancy_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/vehicles/source_vehicle_look_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/vehicles/source_vehicle_director_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_dialogue_admission_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_combat_action_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/units/source_targeting_input_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/input/source_input_collection_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/input/source_profile_binding_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/input/native_tu15_input_collection_data.cpp)
target_link_libraries(halo2_physics_world_runtime PRIVATE halo2_input_startup_tu15)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_ai_actor_tu15_generated.cpp)
halo2_partition_native_translation(halo2_physics_world_runtime src/campaign/ai/source_ai_frame_tu15_generated.cpp 12)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_ai_actor_contact_selector_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(source_ai_actor_contact_selector_tu15_tests
        tests/source_ai_actor_contact_selector_tu15_tests.cpp
        src/campaign/ai/source_ai_actor_contact_selector_tu15_generated.cpp
        src/campaign/ai/source_ai_frame_tu15_generated.cpp
        src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp
        src/campaign/scripting/native_mcc_script_builtin.cpp
        src/runtime/source_runtime_memory.cpp)
    target_link_libraries(source_ai_actor_contact_selector_tu15_tests PRIVATE
        halo2_snapshot_storage Threads::Threads)
    if(MSVC)
        target_compile_options(source_ai_actor_contact_selector_tu15_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_ai_actor_contact_selector_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME source_ai_actor_contact_selector_tu15
        COMMAND source_ai_actor_contact_selector_tu15_tests)
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_runtime_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/multiplayer/source_ctf_configuration_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/multiplayer/source_ctf_player_iterator_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/multiplayer/source_ctf_activation_spawn_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/multiplayer/source_ctf_flag_position_tu15_generated.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_configuration_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_player_iterator_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_activation_spawn_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_flag_position_tu15_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_configuration_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_player_iterator_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_activation_spawn_tu15_generated.cpp
        src/gameplay/multiplayer/source_ctf_flag_position_tu15_generated.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_contrail_geometry_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_lens_flare_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/scripting/source_script_effect_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_cinematic_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_completion_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_activation_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_audio_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/campaign/source_campaign_completion_tu15_generated.cpp
        src/campaign/source_level_handles_new_map_tu15_generated.cpp
        src/campaign/source_campaign_activation_tu15_generated.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    set_source_files_properties(src/campaign/source_campaign_audio_tu15_generated.cpp
        src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_camera_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/world/source_texture_camera_producer_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/world/source_texture_camera_frame_capture_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_texture_camera_original_view_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_texture_camera_frame_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_screen_native_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_campaign_dof_recipe_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/animation/source_animation_composition_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/objects/source_object_functions_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/source_unit_cinematic_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_unit_speech_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/assets/source_resource_cache_pressure_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/objects/source_sandbox_special_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/audio/source_sound_startup_tu15_generated.cpp src/audio/source_sound_sdk_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/assets/source_cache_bitmap_view_tu15.cpp)
if(BUILD_TESTING)
    # Explicit actual bank/lease probe. Captures are supplied on the command
    # line; no declared camera fixture substitutes for an absent actual bank.
    add_executable(source_live_shadow_phase_inputs_tu15_tests
        tests/source_live_shadow_phase_inputs_tu15_tests.cpp)
    target_link_libraries(source_live_shadow_phase_inputs_tu15_tests PRIVATE
        halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME source_live_shadow_phase_inputs_tu15 COMMAND source_live_shadow_phase_inputs_tu15_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/analysis/shader-parity-20261013/shadow-native-live-phase-640-off-v7/shadow-admission/0/265856/before.bin"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/analysis/shader-parity-20261013/shadow-native-cold-camera-v1/shadow-admission/0/265856/before.bin"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shadow_raw_tu15_native_v21.bin"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        "@HALO2_TEST_SCRATCH@/phase-owner")
    set_tests_properties(source_live_shadow_phase_inputs_tu15 PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    add_executable(source_world_depth_frames_tests tests/source_world_depth_frames_tests.cpp)
    target_link_libraries(source_world_depth_frames_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME native_source_world_depth_animated_pages COMMAND source_world_depth_frames_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/analysis/shader-parity-20261013/shadow-native-live-phase-640-off-v7/shadow-admission/0/265856/before.bin"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/source-world-depth-pages.json")
    set_tests_properties(native_source_world_depth_animated_pages PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(source_model_depth_input_tests tests/source_model_depth_input_tests.cpp)
    target_link_libraries(source_model_depth_input_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME native_source_model_depth_owners COMMAND source_model_depth_input_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/analysis/shader-parity-20261013/shadow-native-live-phase-640-off-v7/shadow-admission/0/265856/before.bin"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shadow_raw_tu15_native_v21.bin"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/source-model-depth-owners.json")
    set_tests_properties(native_source_model_depth_owners PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    add_executable(native_vehicle_camera_tu15_tests tests/native_vehicle_camera_tu15_tests.cpp)
    target_link_libraries(native_vehicle_camera_tu15_tests PRIVATE halo2_systems halo2_authority_codec nlohmann_json::nlohmann_json)
    add_test(NAME native_vehicle_camera_tu15 COMMAND native_vehicle_camera_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_vehicle_camera_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    foreach(camera_case ghost-driver warthog-passenger)
      add_test(NAME native_vehicle_camera_continuation_${camera_case}_tu15 COMMAND native_vehicle_camera_tu15_tests
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
          "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
      set_tests_properties(native_vehicle_camera_continuation_${camera_case}_tu15 PROPERTIES
          TIMEOUT 240 SKIP_RETURN_CODE 77
          ENVIRONMENT "HALO2_VEHICLE_CAMERA_CASE=${camera_case};HALO2_VEHICLE_CAMERA_CONTINUATION=1;HALO2_CAMERA_CONTINUATION_OUTPUT=@HALO2_TEST_SCRATCH@/camera-continuation-${camera_case}")
    endforeach()
    add_test(NAME native_vehicle_camera_continuation_death_tu15 COMMAND native_vehicle_camera_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_vehicle_camera_continuation_death_tu15 PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77
        ENVIRONMENT "HALO2_VEHICLE_CAMERA_CASE=ghost-driver;HALO2_TU15_TEST_SEATED_DEATH=1;HALO2_VEHICLE_CAMERA_CONTINUATION=1;HALO2_CAMERA_CONTINUATION_OUTPUT=@HALO2_TEST_SCRATCH@/camera-continuation-death")
    foreach(input_context local remote)
      add_test(NAME native_vehicle_camera_${input_context}_driver_reset_tu15 COMMAND native_vehicle_camera_tu15_tests
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
          "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
      set_tests_properties(native_vehicle_camera_${input_context}_driver_reset_tu15 PROPERTIES
          TIMEOUT 180 SKIP_RETURN_CODE 77
          ENVIRONMENT "HALO2_VEHICLE_CAMERA_CASE=ghost-driver;HALO2_TU15_TEST_LOOK_RESET=1")
      if(input_context STREQUAL "remote")
        set_property(TEST native_vehicle_camera_${input_context}_driver_reset_tu15 APPEND PROPERTY
            ENVIRONMENT "HALO2_TU15_TEST_REMOTE_LOOK_RESET=1")
      endif()
    endforeach()
    foreach(camera_case ghost-driver warthog-passenger warthog-turret scorpion-driver)
      add_test(NAME native_vehicle_camera_death_${camera_case}_tu15 COMMAND native_vehicle_camera_tu15_tests
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/coagulation.map"
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
          "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
      set_tests_properties(native_vehicle_camera_death_${camera_case}_tu15 PROPERTIES
          TIMEOUT 180 SKIP_RETURN_CODE 77
          ENVIRONMENT "HALO2_VEHICLE_CAMERA_CASE=${camera_case};HALO2_TU15_TEST_SEATED_DEATH=1")
    endforeach()
    add_executable(compare_xbox_tu15_vehicle_camera tools/compare_xbox_tu15_vehicle_camera.cpp)
    target_link_libraries(compare_xbox_tu15_vehicle_camera PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_campaign_vehicle_camera_tu15_tests tests/native_campaign_vehicle_camera_tu15_tests.cpp)
    target_link_libraries(native_campaign_vehicle_camera_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    target_include_directories(native_campaign_vehicle_camera_tu15_tests PRIVATE tests)
    add_test(NAME native_campaign_ghost_camera_tu15 COMMAND native_campaign_vehicle_camera_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/05a_deltaapproach.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "@HALO2_TEST_SCRATCH@/campaign-ghost-camera-tu15")
    set_tests_properties(native_campaign_ghost_camera_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(shadow_commands_replay_tu15 tools/replay_live_shadow_commands_tu15.cpp
        src/rendering/renderer/source_renderer_transparent_storage_tu15_generated.cpp)
    target_include_directories(shadow_commands_replay_tu15 PRIVATE src)
    target_link_libraries(shadow_commands_replay_tu15 PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    target_compile_options(shadow_commands_replay_tu15 PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_executable(shadow_entry_tu15_tests tests/shadow_entry_tu15_tests.cpp)
    target_link_libraries(shadow_entry_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_shadow_entry COMMAND shadow_entry_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shadow_entry_tu15_original.json")
    set_tests_properties(original_tu15_shadow_entry PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(source_execution_inputs_tests tests/source_execution_inputs_tests.cpp)
    target_link_libraries(source_execution_inputs_tests PRIVATE halo2_systems)
    add_test(NAME source_execution_inputs COMMAND source_execution_inputs_tests)
    add_executable(source_execution_inputs_native_tests tests/source_execution_inputs_native_tests.cpp)
    target_link_libraries(source_execution_inputs_native_tests PRIVATE halo2_systems)
    add_test(NAME source_execution_inputs_native_tu15 COMMAND source_execution_inputs_native_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(source_execution_inputs_native_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(lens_request_tu15_tests tests/lens_request_tu15_tests.cpp)
    target_link_libraries(lens_request_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_lens_requests COMMAND lens_request_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_request_tu15_original.json")
    add_test(NAME original_tu15_lens_frame COMMAND lens_request_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/lens_frame_tu15_original.json")
    set_tests_properties(original_tu15_lens_requests original_tu15_lens_frame PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(source_map_quaternion_tu15_tests tests/source_map_quaternion_tu15_tests.cpp)
    target_link_libraries(source_map_quaternion_tu15_tests PRIVATE halo2_physics_phantom_world_tu15 nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_map_quaternion COMMAND source_map_quaternion_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/map_quaternion_tu15_original.json")
    set_tests_properties(original_tu15_map_quaternion PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    set(HALO2_TU15_AI_FRAME_CAPTURE_DIRECTORY "" CACHE PATH "Owned original TU1.5 AI frame and hardware replay")
    add_executable(ai_frame_tu15_tests tools/replay_native_ai_frame_tu15.cpp)
    target_link_libraries(ai_frame_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_ai_frame COMMAND ai_frame_tu15_tests
        "${HALO2_TU15_AI_FRAME_CAPTURE_DIRECTORY}" 0x1d0670
        "${CMAKE_CURRENT_BINARY_DIR}/original-ai-frame-native")
    set_tests_properties(original_tu15_ai_frame PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_TU15_DAMAGE_FEEDBACK_CAPTURE_DIRECTORY "" CACHE PATH "Owned same-input original damage-feedback replay and host i386 evidence")
    add_executable(damage_feedback_tu15_tests tools/replay_native_ai_frame_tu15.cpp)
    target_compile_definitions(damage_feedback_tu15_tests PRIVATE
        HALO2_REPLAY_DAMAGE_FEEDBACK_TU15=1 HALO2_REPLAY_DAMAGE_FEEDBACK_SUPPORT_TU15=1)
    target_include_directories(damage_feedback_tu15_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(damage_feedback_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_damage_feedback COMMAND damage_feedback_tu15_tests
        "${HALO2_TU15_DAMAGE_FEEDBACK_CAPTURE_DIRECTORY}" 0x1f5620
        "${CMAKE_CURRENT_BINARY_DIR}/original-damage-feedback-native")
    set_tests_properties(original_tu15_damage_feedback PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_TU15_UNIT_REACTION_CAPTURE_DIRECTORY "" CACHE PATH "Owned same-input original unit-reaction replay and host i386 evidence")
    add_test(NAME original_tu15_unit_reaction COMMAND damage_feedback_tu15_tests
        "${HALO2_TU15_UNIT_REACTION_CAPTURE_DIRECTORY}" 0xe3e20
        "${CMAKE_CURRENT_BINARY_DIR}/original-unit-reaction-native")
    set_tests_properties(original_tu15_unit_reaction PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_TU15_PROJECTILE_CONTACT_EFFECT_CAPTURE_DIRECTORY "" CACHE PATH "Owned same-input original projectile contact-effect replay and host i386 evidence")
    add_executable(projectile_contact_effect_tu15_tests tools/replay_native_ai_frame_tu15.cpp)
    target_compile_definitions(projectile_contact_effect_tu15_tests PRIVATE
        HALO2_REPLAY_EFFECT_PARTICLE_FRAME_TU15=1 HALO2_REPLAY_EFFECT_PARTICLE_SUPPORT_TU15=1)
    target_link_libraries(projectile_contact_effect_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_projectile_contact_effect COMMAND projectile_contact_effect_tu15_tests
        "${HALO2_TU15_PROJECTILE_CONTACT_EFFECT_CAPTURE_DIRECTORY}" 0x178e30
        "${CMAKE_CURRENT_BINARY_DIR}/original-projectile-contact-effect-native")
    set_tests_properties(original_tu15_projectile_contact_effect PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_TU15_BOOT_SMC_CAPTURE_DIRECTORY "" CACHE PATH "Observed owned original boot SMC transaction")
    add_executable(source_sound_audio_mode_tu15_tests tests/source_sound_audio_mode_tu15_tests.cpp)
    target_link_libraries(source_sound_audio_mode_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_audio_mode COMMAND source_sound_audio_mode_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${HALO2_TU15_BOOT_SMC_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_USER_EEPROM}"
        "@HALO2_TEST_SCRATCH@/native-sound-audio-mode")
    set_tests_properties(native_tu15_sound_audio_mode PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    set(HALO2_TU15_BOOT_SMC_INPUT "" CACHE FILEPATH "Owned H2SMCR1 boot SMC transaction input resource")
    set(HALO2_TU15_BOOT_SMC_INPUT_REPORT "" CACHE FILEPATH "Independent observed boot SMC transaction report")
    add_executable(source_boot_smc_video_mode_input_tests tests/source_boot_smc_video_mode_input_tests.cpp)
    target_link_libraries(source_boot_smc_video_mode_input_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_boot_smc_input COMMAND source_boot_smc_video_mode_input_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${HALO2_TU15_BOOT_SMC_INPUT}" "${HALO2_TU15_BOOT_SMC_INPUT_REPORT}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-boot-smc-input")
    set_tests_properties(native_tu15_boot_smc_input PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(contrail_geometry_tu15_tests tests/contrail_geometry_tu15_tests.cpp)
    target_link_libraries(contrail_geometry_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_contrail_geometry COMMAND contrail_geometry_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_geometry_tu15_original.json")
    add_test(NAME original_tu15_contrail_frame COMMAND contrail_geometry_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_frame_tu15_original.json")
    set(HALO2_TU15_PARTICLE_FRAME_CAPTURE_DIRECTORY "" CACHE PATH "Owned original charge-particle frame captures")
    add_executable(effect_particle_frame_tu15_tests tools/replay_native_effect_particle_tu15.cpp)
    target_link_libraries(effect_particle_frame_tu15_tests PRIVATE halo2_systems)
    foreach(frame_case charge-particle-original-after-stop-v2 charge-particle-original-after-stop-frame2-v2)
        add_test(NAME original_tu15_${frame_case} COMMAND effect_particle_frame_tu15_tests
            "${HALO2_TU15_PARTICLE_FRAME_CAPTURE_DIRECTORY}/${frame_case}" 0x179df0
            "${CMAKE_CURRENT_BINARY_DIR}/${frame_case}-native")
        set_tests_properties(original_tu15_${frame_case} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    foreach(init_case "effect-frame-audio-ctor-original-v1|0x22c440" "effect-frame-audio-reset-original-v1|0x22c480"
            "particle-visibility-original-v1|0x3f3e0" "particle-visible-frame-original-v1|0x179df0")
        string(REPLACE "|" ";" init_fields "${init_case}")
        list(GET init_fields 0 init_name)
        list(GET init_fields 1 init_entry)
        add_test(NAME original_tu15_${init_name} COMMAND effect_particle_frame_tu15_tests
            "${HALO2_TU15_PARTICLE_FRAME_CAPTURE_DIRECTORY}/${init_name}" "${init_entry}"
            "${CMAKE_CURRENT_BINARY_DIR}/${init_name}-native")
        set_tests_properties(original_tu15_${init_name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    set(HALO2_TU15_SOUND_STARTUP_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 sound startup capture")
    set(HALO2_TU15_SOUND_STARTUP_ORACLE_REPORT "" CACHE FILEPATH "Accepted independent original sound CPU replay")
    set(HALO2_TU15_DSP_SECTION_REPORT "" CACHE FILEPATH "Owned original TU1.5 DSP section reference capture")
    set(HALO2_TU15_SOUND_DEVICE_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 sound driver capture")
    set(HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 SDK factory capture")
    set(HALO2_TU15_SOUND_POOL_RULES_CAPTURE_DIRECTORY "" CACHE PATH "Owned kernel pool rules observed during unmodified TU1.5 construction")
    set(HALO2_TU15_SOUND_AUDIO_CAPTURE_DIRECTORY "" CACHE PATH "Owned unmodified TU1.5 audio query capture")
    set(HALO2_TU15_SOUND_USER_EEPROM "" CACHE FILEPATH "Actual owned EEPROM resource for audio query acceptance")
    add_executable(source_runtime_sound_audio_settings_tests tests/source_runtime_sound_audio_settings_tests.cpp)
    target_link_libraries(source_runtime_sound_audio_settings_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_audio_settings COMMAND source_runtime_sound_audio_settings_tests
        "${HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_AUDIO_CAPTURE_DIRECTORY}"
        "${HALO2_TU15_SOUND_USER_EEPROM}" "@HALO2_TEST_SCRATCH@/native-sound-audio-settings")
    set_tests_properties(native_tu15_sound_audio_settings PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_sound_sdk_pool_tests tests/source_sound_sdk_pool_tests.cpp)
    target_link_libraries(source_sound_sdk_pool_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_sdk_pool_capacity COMMAND source_sound_sdk_pool_tests
        "${HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_POOL_RULES_CAPTURE_DIRECTORY}"
        "@HALO2_TEST_SCRATCH@/native-sound-sdk-pool")
    set_tests_properties(native_tu15_sound_sdk_pool_capacity PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_runtime_sound_sdk_tests tests/source_runtime_sound_sdk_tests.cpp)
    target_link_libraries(source_runtime_sound_sdk_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_sdk_resources COMMAND source_runtime_sound_sdk_tests
        "${HALO2_TU15_SOUND_SDK_CAPTURE_DIRECTORY}" "@HALO2_TEST_SCRATCH@/native-sound-sdk-prefix")
    set_tests_properties(native_tu15_sound_sdk_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_runtime_sound_output_tests tests/source_runtime_sound_output_tests.cpp)
    target_link_libraries(source_runtime_sound_output_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_output_resources COMMAND source_runtime_sound_output_tests
        "${HALO2_TU15_SOUND_DEVICE_CAPTURE_DIRECTORY}" "@HALO2_TEST_SCRATCH@/native-sound-output-prefix")
    set_tests_properties(native_tu15_sound_output_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_runtime_dsp_demand_tests tests/source_runtime_dsp_demand_tests.cpp)
    add_executable(source_runtime_mcc_resources_tests tests/source_runtime_mcc_resources_tests.cpp)
    target_link_libraries(source_runtime_mcc_resources_tests PRIVATE halo2_systems)
    add_test(NAME native_mcc_resource_ownership COMMAND source_runtime_mcc_resources_tests)
    add_executable(mcc_opening_geometry_cache_frame_tests tests/mcc_opening_geometry_cache_frame_tests.cpp)
    target_link_libraries(mcc_opening_geometry_cache_frame_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME mcc_opening_geometry_cache_frame COMMAND mcc_opening_geometry_cache_frame_tests
        "${HALO2_MCC_OPENING_MODEL_DONOR}" "${HALO2_TU15_GAMEPLAY_XBE}"
        "${PROJECT_SOURCE_DIR}/tests/fixtures/geometry_cache_frame_tu15.original.json")
    set_tests_properties(mcc_opening_geometry_cache_frame PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RUN_SERIAL TRUE)
    add_executable(mcc_opening_sound_geometry_tests tests/mcc_opening_sound_geometry_tests.cpp)
    target_link_libraries(mcc_opening_sound_geometry_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME mcc_opening_sound_geometry COMMAND mcc_opening_sound_geometry_tests
        "${HALO2_MCC_OPENING_MODEL_DONOR}"
        "${PROJECT_SOURCE_DIR}/tests/fixtures/mcc_opening_facial_resource.original.json")
    set_tests_properties(mcc_opening_sound_geometry PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RUN_SERIAL TRUE)
    add_executable(native_tu15_world_memory_tests tests/native_tu15_world_memory_tests.cpp)
    target_link_libraries(native_tu15_world_memory_tests PRIVATE halo2_systems)
    add_test(NAME native_tu15_world_memory_ownership COMMAND native_tu15_world_memory_tests)
    add_executable(native_tu15_weapon_runtime_tables_tests tests/native_tu15_weapon_runtime_tables_tests.cpp)
    target_link_libraries(native_tu15_weapon_runtime_tables_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_weapon_runtime_tables COMMAND native_tu15_weapon_runtime_tables_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_runtime_tables_tu15_original.json")
    add_executable(native_tu15_renderer_context_data_tests tests/native_tu15_renderer_context_data_tests.cpp)
    target_link_libraries(native_tu15_renderer_context_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_renderer_context_data_contract COMMAND native_tu15_renderer_context_data_tests)
    add_executable(native_tu15_model_animation_gate_data_tests tests/native_tu15_model_animation_gate_data_tests.cpp)
    target_link_libraries(native_tu15_model_animation_gate_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_model_animation_gate_data_contract COMMAND native_tu15_model_animation_gate_data_tests)
    set(HALO2_TU15_MODEL_ANIMATION_GATE_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original model animation predicate comparison fixture")
    add_test(NAME native_tu15_model_animation_gate_data_reference COMMAND native_tu15_model_animation_gate_data_tests
        "${HALO2_TU15_MODEL_ANIMATION_GATE_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_model_animation_gate_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_MODEL_ANIMATION_GATE_MAP "" CACHE FILEPATH "Optional owned MCC map for model animation predicate data validation")
    add_test(NAME native_tu15_model_animation_gate_data_mcc_only COMMAND native_tu15_model_animation_gate_data_tests
        --mcc "${HALO2_MCC_MODEL_ANIMATION_GATE_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-model-animation-gate-fixture")
    set_tests_properties(native_tu15_model_animation_gate_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_model_probe_vectors_data_tests tests/native_tu15_model_probe_vectors_data_tests.cpp)
    target_link_libraries(native_tu15_model_probe_vectors_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_model_probe_vectors_data_contract COMMAND native_tu15_model_probe_vectors_data_tests)
    set(HALO2_TU15_MODEL_PROBE_VECTORS_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original model vectors comparison fixture")
    add_test(NAME native_tu15_model_probe_vectors_data_reference COMMAND native_tu15_model_probe_vectors_data_tests "${HALO2_TU15_MODEL_PROBE_VECTORS_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_model_probe_vectors_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_MODEL_PROBE_VECTORS_MAP "" CACHE FILEPATH "Optional owned MCC map for model vectors data validation")
    add_test(NAME native_tu15_model_probe_vectors_data_mcc_only COMMAND native_tu15_model_probe_vectors_data_tests
        --mcc "${HALO2_MCC_MODEL_PROBE_VECTORS_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-model-vectors-fixture")
    set_tests_properties(native_tu15_model_probe_vectors_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_model_probe_selector_data_tests tests/native_tu15_model_probe_selector_data_tests.cpp)
    target_link_libraries(native_tu15_model_probe_selector_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_model_probe_selector_data_contract COMMAND native_tu15_model_probe_selector_data_tests)
    set(HALO2_TU15_MODEL_PROBE_SELECTOR_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original model selector comparison fixture")
    add_test(NAME native_tu15_model_probe_selector_data_reference COMMAND native_tu15_model_probe_selector_data_tests "${HALO2_TU15_MODEL_PROBE_SELECTOR_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_model_probe_selector_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_MODEL_PROBE_SELECTOR_MAP "" CACHE FILEPATH "Optional owned MCC map for model selector data validation")
    add_test(NAME native_tu15_model_probe_selector_data_mcc_only COMMAND native_tu15_model_probe_selector_data_tests
        --mcc "${HALO2_MCC_MODEL_PROBE_SELECTOR_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-model-selector-fixture")
    set_tests_properties(native_tu15_model_probe_selector_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_model_probe_colors_data_tests tests/native_tu15_model_probe_colors_data_tests.cpp)
    target_link_libraries(native_tu15_model_probe_colors_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_model_probe_colors_data_contract COMMAND native_tu15_model_probe_colors_data_tests)
    set(HALO2_TU15_MODEL_PROBE_COLORS_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original model colors comparison fixture")
    add_test(NAME native_tu15_model_probe_colors_data_reference COMMAND native_tu15_model_probe_colors_data_tests "${HALO2_TU15_MODEL_PROBE_COLORS_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_model_probe_colors_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_MODEL_PROBE_COLORS_MAP "" CACHE FILEPATH "Optional owned MCC map for model colors data validation")
    add_test(NAME native_tu15_model_probe_colors_data_mcc_only COMMAND native_tu15_model_probe_colors_data_tests
        --mcc "${HALO2_MCC_MODEL_PROBE_COLORS_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-model-colors-fixture")
    set_tests_properties(native_tu15_model_probe_colors_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_model_probe_clamp_data_tests tests/native_tu15_model_probe_clamp_data_tests.cpp)
    target_link_libraries(native_tu15_model_probe_clamp_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_model_probe_clamp_data_contract COMMAND native_tu15_model_probe_clamp_data_tests)
    set(HALO2_TU15_MODEL_PROBE_CLAMP_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original model clamp comparison fixture")
    add_test(NAME native_tu15_model_probe_clamp_data_reference COMMAND native_tu15_model_probe_clamp_data_tests
        "${HALO2_TU15_MODEL_PROBE_CLAMP_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_model_probe_clamp_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_MODEL_PROBE_CLAMP_MAP "" CACHE FILEPATH "Optional owned MCC map for model clamp data validation")
    add_test(NAME native_tu15_model_probe_clamp_data_mcc_only COMMAND native_tu15_model_probe_clamp_data_tests
        --mcc "${HALO2_MCC_MODEL_PROBE_CLAMP_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-model-clamp-fixture")
    set_tests_properties(native_tu15_model_probe_clamp_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_first_person_model_count_data_tests tests/native_tu15_first_person_model_count_data_tests.cpp)
    target_link_libraries(native_tu15_first_person_model_count_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_first_person_model_count_data_contract COMMAND native_tu15_first_person_model_count_data_tests)
    set(HALO2_TU15_FP_MODEL_COUNT_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original first-person count comparison fixture")
    add_test(NAME native_tu15_first_person_model_count_data_reference COMMAND native_tu15_first_person_model_count_data_tests
        "${HALO2_TU15_FP_MODEL_COUNT_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_first_person_model_count_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_FP_MODEL_COUNT_MAP "" CACHE FILEPATH "Optional owned MCC map for first-person count storage validation")
    add_test(NAME native_tu15_first_person_model_count_data_mcc_only COMMAND native_tu15_first_person_model_count_data_tests
        --mcc "${HALO2_MCC_FP_MODEL_COUNT_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-fp-model-count-fixture")
    set_tests_properties(native_tu15_first_person_model_count_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_rasterizer_globals_pointer_data_tests tests/native_tu15_rasterizer_globals_pointer_data_tests.cpp)
    target_link_libraries(native_tu15_rasterizer_globals_pointer_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_rasterizer_globals_pointer_data_contract COMMAND native_tu15_rasterizer_globals_pointer_data_tests)
    set(HALO2_TU15_RASTERIZER_POINTER_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original rasterizer globals pointer comparison fixture")
    add_test(NAME native_tu15_rasterizer_globals_pointer_data_reference COMMAND native_tu15_rasterizer_globals_pointer_data_tests
        "${HALO2_TU15_RASTERIZER_POINTER_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_rasterizer_globals_pointer_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_RASTERIZER_POINTER_MAP "" CACHE FILEPATH "Optional owned MCC map for rasterizer globals pointer storage validation")
    add_test(NAME native_tu15_rasterizer_globals_pointer_data_mcc_only COMMAND native_tu15_rasterizer_globals_pointer_data_tests
        --mcc "${HALO2_MCC_RASTERIZER_POINTER_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-rasterizer-pointer-fixture")
    set_tests_properties(native_tu15_rasterizer_globals_pointer_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_viewport_coordinate_data_tests tests/native_tu15_viewport_coordinate_data_tests.cpp)
    target_link_libraries(native_tu15_viewport_coordinate_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_viewport_coordinate_data_contract COMMAND native_tu15_viewport_coordinate_data_tests)
    set(HALO2_TU15_VIEWPORT_COORDINATE_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original viewport CPU comparison fixture")
    add_test(NAME native_tu15_viewport_coordinate_data_reference COMMAND native_tu15_viewport_coordinate_data_tests
        "${HALO2_TU15_VIEWPORT_COORDINATE_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_viewport_coordinate_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_VIEWPORT_COORDINATE_MAP "" CACHE FILEPATH "Optional owned MCC map for viewport output storage validation")
    add_test(NAME native_tu15_viewport_coordinate_data_mcc_only COMMAND native_tu15_viewport_coordinate_data_tests
        --mcc "${HALO2_MCC_VIEWPORT_COORDINATE_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-viewport-coordinate-fixture")
    set_tests_properties(native_tu15_viewport_coordinate_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_TU15_RENDERER_CONTEXT_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original renderer-context CPU comparison fixture")
    add_test(NAME native_tu15_renderer_context_data_reference COMMAND native_tu15_renderer_context_data_tests
        "${HALO2_TU15_RENDERER_CONTEXT_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_renderer_context_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    set(HALO2_MCC_RENDERER_CONTEXT_MAP "" CACHE FILEPATH "Optional owned MCC map for current renderer-context storage validation")
    add_test(NAME native_tu15_renderer_context_data_mcc_only COMMAND native_tu15_renderer_context_data_tests
        --mcc "${HALO2_MCC_RENDERER_CONTEXT_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-renderer-context-fixture")
    set_tests_properties(native_tu15_renderer_context_data_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_camera_packet_data_tests tests/native_tu15_camera_packet_data_tests.cpp)
    target_link_libraries(native_tu15_camera_packet_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_camera_packet_data_contract COMMAND native_tu15_camera_packet_data_tests)
    set(HALO2_TU15_CAMERA_PACKET_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original camera CRT CPU comparison fixture")
    add_test(NAME native_tu15_camera_packet_data_reference COMMAND native_tu15_camera_packet_data_tests
        "${HALO2_TU15_CAMERA_PACKET_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_camera_packet_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(native_tu15_camera_dispatch_table_tests tests/native_tu15_camera_dispatch_table_tests.cpp)
    target_link_libraries(native_tu15_camera_dispatch_table_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_camera_dispatch_table_contract COMMAND native_tu15_camera_dispatch_table_tests)
    set(HALO2_TU15_CAMERA_DISPATCH_REFERENCE_XBE "" CACHE FILEPATH "Optional offline original camera mode table comparison image")
    add_test(NAME native_tu15_camera_dispatch_table_reference COMMAND native_tu15_camera_dispatch_table_tests
        "${HALO2_TU15_CAMERA_DISPATCH_REFERENCE_XBE}")
    set_tests_properties(native_tu15_camera_dispatch_table_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(native_tu15_prt_cache_data_tests tests/native_tu15_prt_cache_data_tests.cpp)
    target_link_libraries(native_tu15_prt_cache_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_prt_cache_data_contract COMMAND native_tu15_prt_cache_data_tests)
    set(HALO2_TU15_PRT_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original PRT CPU comparison fixture")
    add_test(NAME native_tu15_prt_cache_data_reference COMMAND native_tu15_prt_cache_data_tests
        "${HALO2_TU15_PRT_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_prt_cache_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(native_tu15_main_view_data_tests tests/native_tu15_main_view_data_tests.cpp)
    target_link_libraries(native_tu15_main_view_data_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_main_view_data_contract COMMAND native_tu15_main_view_data_tests)
    set(HALO2_TU15_MAIN_VIEW_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original main-view CPU comparison fixture")
    add_test(NAME native_tu15_main_view_data_reference COMMAND native_tu15_main_view_data_tests
        "${HALO2_TU15_MAIN_VIEW_REFERENCE_FIXTURE}")
    set_tests_properties(native_tu15_main_view_data_reference PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(native_tu15_renderer_transparent_factory_tests tests/native_tu15_renderer_transparent_factory_tests.cpp)
    target_link_libraries(native_tu15_renderer_transparent_factory_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_renderer_transparent_factory_contract COMMAND native_tu15_renderer_transparent_factory_tests)
    set(HALO2_MCC_TRANSPARENT_MAP "" CACHE FILEPATH "Optional owned MCC map for native transparent storage constructor validation")
    add_test(NAME native_tu15_renderer_transparent_factory_mcc_only
        COMMAND native_tu15_renderer_transparent_factory_tests "${HALO2_MCC_TRANSPARENT_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-transparent-factory-fixture")
    set_tests_properties(native_tu15_renderer_transparent_factory_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    target_link_libraries(source_runtime_dsp_demand_tests PRIVATE halo2_systems)
    add_test(NAME native_pcm_dsp_demand COMMAND source_runtime_dsp_demand_tests)
    add_executable(source_runtime_dsp_section_tests tests/source_runtime_dsp_section_tests.cpp)
    target_link_libraries(source_runtime_dsp_section_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_dsp_section_resources COMMAND source_runtime_dsp_section_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${HALO2_TU15_DSP_SECTION_REPORT}" "${HALO2_TU15_SOUND_STARTUP_CAPTURE_DIRECTORY}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-dsp-section-prefix")
    set_tests_properties(native_tu15_dsp_section_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(source_sound_startup_tu15_tests tests/source_sound_startup_tu15_tests.cpp)
    target_link_libraries(source_sound_startup_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_sound_startup_cpu COMMAND source_sound_startup_tu15_tests
        "${HALO2_TU15_SOUND_STARTUP_CAPTURE_DIRECTORY}" "${HALO2_TU15_SOUND_STARTUP_ORACLE_REPORT}")
    set_tests_properties(native_tu15_sound_startup_cpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(light_fade_curves_tu15_tests tests/light_fade_curves_tu15_tests.cpp)
    target_link_libraries(light_fade_curves_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_light_fade_curves COMMAND light_fade_curves_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/light_fade_curves_tu15_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_executable(physics_live_world_continuation_tests tests/physics_live_world_continuation_tests.cpp)
    target_link_libraries(physics_live_world_continuation_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_live_world_continuation COMMAND physics_live_world_continuation_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_physics_live_world_continuation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(decal_pass_tu15_tests tests/decal_pass_tu15_tests.cpp)
    target_link_libraries(decal_pass_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_decal_pass COMMAND decal_pass_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/decal_pass_tu15_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_executable(cache_vertex_view_tu15_tests tests/cache_vertex_view_tu15_tests.cpp)
    target_link_libraries(cache_vertex_view_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_cache_vertex_view COMMAND cache_vertex_view_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cache_vertex_view_tu15_original.json")
    add_executable(native_grenade_input_tests tests/native_grenade_input_tests.cpp)
    target_link_libraries(native_grenade_input_tests PRIVATE halo2_systems)
    add_test(NAME native_grenade_selector_input COMMAND native_grenade_input_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_executable(grenade_selection_tu15_tests tests/grenade_selection_tu15_tests.cpp src/gameplay/objects/inventory.cpp)
    target_include_directories(grenade_selection_tu15_tests PRIVATE include)
    target_link_libraries(grenade_selection_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_grenade_selection COMMAND grenade_selection_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/grenade_selection_tu15_original.json")
    add_executable(sandbox_special_tu15_tests tests/sandbox_special_tu15_tests.cpp)
    target_link_libraries(sandbox_special_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_sandbox_special COMMAND sandbox_special_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sandbox_special_tu15_original.json")
    set_tests_properties(original_tu15_sandbox_special PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_sandbox_primary COMMAND sandbox_special_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sandbox_primary_tu15_original.json")
    set_tests_properties(original_tu15_sandbox_primary PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(particle_collision_admission_tu15_tests tests/particle_collision_admission_tu15_tests.cpp)
    target_include_directories(particle_collision_admission_tu15_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(particle_collision_admission_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_particle_collision_admission COMMAND particle_collision_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/particle_collision_admission_tu15_original.jsonl")
    add_test(NAME original_tu15_effect_event_timeline COMMAND source_effect_event_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_event_tu15_original.jsonl")
    add_executable(particle_effect_color_tu15_tests tests/particle_effect_color_tu15_tests.cpp)
    target_include_directories(particle_effect_color_tu15_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(particle_effect_color_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_particle_effect_color COMMAND particle_effect_color_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/particle_effect_color_tu15_original.json")
    set_tests_properties(original_tu15_particle_effect_color PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(capture_source_melee_contact tools/capture_source_melee_contact.cpp)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_sources(capture_source_melee_contact PRIVATE
            tools/capture_sword_movement_tu15.cpp tools/sword_online_socket_fixture.cpp)
        target_compile_definitions(capture_source_melee_contact PRIVATE HALO2_SWORD_SOCKET_FIXTURE)
        target_link_libraries(capture_source_melee_contact PRIVATE halo2_online_matchmaking)
        target_link_options(capture_source_melee_contact PRIVATE
            "-Wl,--wrap=_ZN5halo231source_run_sandbox_special_tu15ERNS_26SourcePhysicsContactMemoryEjRNS_30SourcePhysicsContactRuntimeAbiENS_33SourcePhysicsFactoryFloatingPointERKNS_35SourcePhysicsContactRuntimeServicesERKSt8functionIFvRKNS_31SourcePhysicsContactRecordStoreEEE")
    endif()
    add_executable(capture_source_locomotion_tu15 tools/capture_source_locomotion_tu15.cpp)
    add_executable(capture_source_dual_animation_tu15 tools/capture_source_dual_animation_tu15.cpp)
    target_link_libraries(capture_source_dual_animation_tu15 PRIVATE halo2_systems)
    add_executable(capture_source_lift_animation_tu15 tools/capture_source_lift_animation_tu15.cpp)
    target_link_libraries(capture_source_lift_animation_tu15 PRIVATE halo2_systems)
    add_executable(capture_source_render_pose_tu15 tools/capture_source_render_pose_tu15.cpp)
    target_link_libraries(capture_source_render_pose_tu15 PRIVATE halo2_systems)
    add_executable(capture_source_weapon_effect_events_tu15 tools/capture_source_weapon_effect_events_tu15.cpp)
    target_link_libraries(capture_source_weapon_effect_events_tu15 PRIVATE halo2_systems)
    add_test(NAME native_tu15_unmapped_source_weapon_effects COMMAND ${Python3_EXECUTABLE}
        "${PROJECT_SOURCE_DIR}/tools/test_source_weapon_effects_mcc_tu15.py"
        --root "${PROJECT_SOURCE_DIR}" --runtime "$<TARGET_FILE:capture_source_weapon_effect_events_tu15>"
        --mode unmapped --report-output "${CMAKE_CURRENT_BINARY_DIR}/source-unmapped-weapon-effects.json")
    set_tests_properties(native_tu15_unmapped_source_weapon_effects PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_source_weapon_effect_locations_lan COMMAND ${Python3_EXECUTABLE}
        "${PROJECT_SOURCE_DIR}/tools/test_source_weapon_effects_mcc_tu15.py"
        --root "${PROJECT_SOURCE_DIR}" --runtime "$<TARGET_FILE:capture_source_weapon_effect_events_tu15>"
        --mode network --report-output "${CMAKE_CURRENT_BINARY_DIR}/source-network-weapon-effects.json")
    set_tests_properties(native_tu15_source_weapon_effect_locations_lan PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_source_weapon_effect_events COMMAND ${Python3_EXECUTABLE}
        "${PROJECT_SOURCE_DIR}/tools/test_source_weapon_effects_mcc_tu15.py"
        --root "${PROJECT_SOURCE_DIR}" --runtime "$<TARGET_FILE:capture_source_weapon_effect_events_tu15>"
        --mode direct --report-output "${CMAKE_CURRENT_BINARY_DIR}/source-weapon-effect-events.json")
    set_tests_properties(native_tu15_source_weapon_effect_events PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_mcc_source_weapon_instant_charge COMMAND ${Python3_EXECUTABLE}
        "${PROJECT_SOURCE_DIR}/tools/test_source_weapon_effects_mcc_tu15.py"
        --root "${PROJECT_SOURCE_DIR}" --runtime "$<TARGET_FILE:capture_source_weapon_effect_events_tu15>"
        --mode charge-instant --report-output "${CMAKE_CURRENT_BINARY_DIR}/source-weapon-instant-charge.json")
    set_tests_properties(native_tu15_mcc_source_weapon_instant_charge PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_mcc_source_weapon_input_admission COMMAND ${Python3_EXECUTABLE}
        "${PROJECT_SOURCE_DIR}/tests/test_source_weapon_effects_mcc_runner.py")
    target_link_libraries(capture_source_locomotion_tu15 PRIVATE halo2_systems)
    add_test(NAME native_tu15_elite_magnum_render_pose COMMAND capture_source_render_pose_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-elite-magnum-render-pose" elite 4)
    set_tests_properties(native_tu15_elite_magnum_render_pose PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    foreach(species human elite)
        add_test(NAME native_tu15_${species}_dual_animation_effects COMMAND capture_source_dual_animation_tu15
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_BINARY_DIR}/source-${species}-dual-animation-effects" ${species} 5 5)
        set_tests_properties(native_tu15_${species}_dual_animation_effects PROPERTIES
            SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT "HALO2_SOURCE_FP_PARTICLE_PROBE=1")
        add_test(NAME native_tu15_${species}_lift_animation COMMAND capture_source_lift_animation_tu15
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_BINARY_DIR}/source-${species}-lift-animation" ${species})
        set_tests_properties(native_tu15_${species}_lift_animation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME native_tu15_${species}_render_pose COMMAND capture_source_render_pose_tu15
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_BINARY_DIR}/source-${species}-render-pose" ${species} 5)
        set_tests_properties(native_tu15_${species}_render_pose PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME native_tu15_${species}_locomotion COMMAND capture_source_locomotion_tu15
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_BINARY_DIR}/source-${species}-locomotion" ${species})
        set_tests_properties(native_tu15_${species}_locomotion PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    target_link_libraries(capture_source_melee_contact PRIVATE halo2_systems)
    set(HALO2_MCC_SWORD_LUNGE_MAP "" CACHE FILEPATH
        "Prepared MCC map for native TU1.5 sword lunge contact and lethal damage acceptance")
    if(NOT "${HALO2_MCC_SWORD_LUNGE_MAP}" STREQUAL "" AND
       (NOT IS_ABSOLUTE "${HALO2_MCC_SWORD_LUNGE_MAP}" OR
        NOT EXISTS "${HALO2_MCC_SWORD_LUNGE_MAP}" OR
        IS_DIRECTORY "${HALO2_MCC_SWORD_LUNGE_MAP}"))
        message(FATAL_ERROR "HALO2_MCC_SWORD_LUNGE_MAP requires an existing absolute prepared MCC map")
    endif()
    get_filename_component(HALO2_MCC_SWORD_LUNGE_MAPS "${HALO2_MCC_SWORD_LUNGE_MAP}" DIRECTORY)
    if(NOT "${HALO2_MCC_SWORD_LUNGE_MAP}" STREQUAL "")
        add_test(NAME native_mcc_sword_lunge_timing_metadata COMMAND mcc_first_person_binding_tests
            "${HALO2_MCC_SWORD_LUNGE_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-sword-lunge-timing.json")
        set_tests_properties(native_mcc_sword_lunge_timing_metadata PROPERTIES TIMEOUT 120)
    endif()
    add_test(NAME native_mcc_sword_lunge_damage COMMAND capture_source_melee_contact
        "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
        "@HALO2_TEST_SCRATCH@/source-mcc-sword-lunge" lunge)
    set_tests_properties(native_mcc_sword_lunge_damage PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(native_input_collection_tu15_tests tests/native_input_collection_tu15_tests.cpp)
    target_link_libraries(native_input_collection_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    target_include_directories(native_input_collection_tu15_tests PRIVATE tests)
    add_test(NAME native_input_collection_tu15 COMMAND native_input_collection_tu15_tests
        "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}"
        native-mcc
        "@HALO2_TEST_SCRATCH@/native-input-collection")
    set_tests_properties(native_input_collection_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    foreach(sword_network_case IN ITEMS lunge-host lunge-client sword-close-host sword-close-client)
        add_test(NAME native_mcc_sword_${sword_network_case}_damage COMMAND capture_source_melee_contact
            "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
            "@HALO2_TEST_SCRATCH@/source-mcc-sword-${sword_network_case}" "${sword_network_case}")
        set_tests_properties(native_mcc_sword_${sword_network_case}_damage PROPERTIES
            TIMEOUT 180 SKIP_RETURN_CODE 77 ENVIRONMENT "H2_NET_PROFILE=complete")
    endforeach()
    foreach(sword_attacker IN ITEMS host client)
        add_test(NAME native_mcc_sword_primary-intent-${sword_attacker}_input COMMAND capture_source_melee_contact
            "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
            "@HALO2_TEST_SCRATCH@/source-mcc-sword-primary-intent-${sword_attacker}" "sword-flight-swap-${sword_attacker}")
        set_tests_properties(native_mcc_sword_primary-intent-${sword_attacker}_input PROPERTIES
            TIMEOUT 180 SKIP_RETURN_CODE 77
            ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_PRIMARY_INTENT_PROBE=1")
        if(NOT "${HALO2_MCC_SWORD_LUNGE_MAP}" STREQUAL "" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
           CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            add_test(NAME native_mcc_sword_online-socket-${sword_attacker}_damage
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_sword_online_socket_tu15.py"
                --binary "$<TARGET_FILE:capture_source_melee_contact>" --map "${HALO2_MCC_SWORD_LUNGE_MAP}"
                --mode "lunge-${sword_attacker}" --report-output "@HALO2_TEST_SCRATCH@/online-sword.json")
            set_tests_properties(native_mcc_sword_online-socket-${sword_attacker}_damage PROPERTIES
                TIMEOUT 300 ENVIRONMENT "H2_NET_PROFILE=complete")
            foreach(sword_socket_sequence IN ITEMS cancel-neutral cancel-jump cancel-switch flight-swap)
                add_test(NAME native_mcc_sword_online-socket-${sword_socket_sequence}-${sword_attacker}_input
                    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_sword_online_socket_tu15.py"
                    --binary "$<TARGET_FILE:capture_source_melee_contact>" --map "${HALO2_MCC_SWORD_LUNGE_MAP}"
                    --mode "sword-${sword_socket_sequence}-${sword_attacker}" --report-output "@HALO2_TEST_SCRATCH@/online-sword.json")
                set_tests_properties(native_mcc_sword_online-socket-${sword_socket_sequence}-${sword_attacker}_input PROPERTIES
                    TIMEOUT 300 ENVIRONMENT "H2_NET_PROFILE=complete")
            endforeach()
        endif()
        add_test(NAME native_mcc_sword_lunge-${sword_attacker}_held_damage COMMAND capture_source_melee_contact
            "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
            "@HALO2_TEST_SCRATCH@/source-mcc-sword-lunge-${sword_attacker}-held" "lunge-${sword_attacker}")
        set_tests_properties(native_mcc_sword_lunge-${sword_attacker}_held_damage PROPERTIES
            TIMEOUT 180 SKIP_RETURN_CODE 77
            ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_TRIGGER_TICKS=6")
    endforeach()
    foreach(sword_sequence IN ITEMS cancel-neutral cancel-jump cancel-reattack cancel-switch flight-swap)
        foreach(sword_attacker IN ITEMS host client)
            add_test(NAME native_mcc_sword_${sword_sequence}-${sword_attacker}_input COMMAND capture_source_melee_contact
                "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
                "@HALO2_TEST_SCRATCH@/source-mcc-sword-${sword_sequence}-${sword_attacker}" "sword-${sword_sequence}-${sword_attacker}")
            set_tests_properties(native_mcc_sword_${sword_sequence}-${sword_attacker}_input PROPERTIES
                TIMEOUT 180 SKIP_RETURN_CODE 77 ENVIRONMENT "H2_NET_PROFILE=complete")
        endforeach()
    endforeach()
    foreach(sword_distance IN ITEMS near far)
        set(sword_rapid_firearm smg)
        if(sword_distance STREQUAL "far")
            set(sword_rapid_firearm battle-rifle)
        endif()
        foreach(sword_sequence IN ITEMS y-rt y-then-rt held-rt-y x-y-b-rt x-y-b-rt-same y-b y-release-rt x-y-b-then-rt)
            foreach(sword_attacker IN ITEMS host client)
                add_test(NAME native_mcc_sword_rapid-${sword_distance}-${sword_sequence}-${sword_attacker}_input
                    COMMAND capture_source_melee_contact "${HALO2_MCC_SWORD_LUNGE_MAP}"
                    "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
                    "@HALO2_TEST_SCRATCH@/source-mcc-sword-rapid-${sword_distance}-${sword_sequence}-${sword_attacker}"
                    "sword-rapid-${sword_distance}-${sword_attacker}")
                set_tests_properties(native_mcc_sword_rapid-${sword_distance}-${sword_sequence}-${sword_attacker}_input PROPERTIES
                    TIMEOUT 180 SKIP_RETURN_CODE 77
                    ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_RAPID_SEQUENCE=${sword_sequence};HALO2_SWORD_RAPID_FIREARM=${sword_rapid_firearm}")
                if(NOT "${HALO2_MCC_SWORD_LUNGE_MAP}" STREQUAL "" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
                   CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND
                   (sword_sequence STREQUAL "x-y-b-rt" OR sword_sequence STREQUAL "y-release-rt"))
                    add_test(NAME native_mcc_sword_online-socket-rapid-${sword_distance}-${sword_sequence}-${sword_attacker}_input
                        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_sword_online_socket_tu15.py"
                        --binary "$<TARGET_FILE:capture_source_melee_contact>" --map "${HALO2_MCC_SWORD_LUNGE_MAP}"
                        --mode "sword-rapid-${sword_distance}-${sword_attacker}" --report-output "@HALO2_TEST_SCRATCH@/online-sword.json")
                    set_tests_properties(native_mcc_sword_online-socket-rapid-${sword_distance}-${sword_sequence}-${sword_attacker}_input PROPERTIES
                        TIMEOUT 300
                        ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_RAPID_SEQUENCE=${sword_sequence};HALO2_SWORD_RAPID_FIREARM=${sword_rapid_firearm}")
                endif()
            endforeach()
        endforeach()
    endforeach()
    add_test(NAME native_mcc_sword_cancel-jump-client_elite_input COMMAND capture_source_melee_contact
        "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
        "@HALO2_TEST_SCRATCH@/source-mcc-sword-cancel-jump-client-elite" sword-cancel-jump-client)
    set_tests_properties(native_mcc_sword_cancel-jump-client_elite_input PROPERTIES
        TIMEOUT 180 SKIP_RETURN_CODE 77 ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_REPRESENTATION=3")
    foreach(sword_attacker IN ITEMS host client)
        add_test(NAME native_mcc_sword_lunge-${sword_attacker}_airborne_damage COMMAND capture_source_melee_contact
            "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
            "@HALO2_TEST_SCRATCH@/source-mcc-sword-lunge-${sword_attacker}-airborne" "lunge-${sword_attacker}")
        set_tests_properties(native_mcc_sword_lunge-${sword_attacker}_airborne_damage PROPERTIES
            TIMEOUT 180 SKIP_RETURN_CODE 77 ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_JUMP_BEFORE_ATTACK=1")
        add_test(NAME native_mcc_sword_cancel-neutral-${sword_attacker}_airborne_input COMMAND capture_source_melee_contact
            "${HALO2_MCC_SWORD_LUNGE_MAP}" "${HALO2_MCC_SWORD_LUNGE_MAPS}" native-mcc
            "@HALO2_TEST_SCRATCH@/source-mcc-sword-cancel-neutral-${sword_attacker}-airborne" "sword-cancel-neutral-${sword_attacker}")
        set_tests_properties(native_mcc_sword_cancel-neutral-${sword_attacker}_airborne_input PROPERTIES
            TIMEOUT 180 SKIP_RETURN_CODE 77 ENVIRONMENT "H2_NET_PROFILE=complete;HALO2_SWORD_JUMP_BEFORE_ATTACK=1")
    endforeach()
    add_test(NAME native_tu15_plasma_decal_lifecycle COMMAND capture_source_melee_contact
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/source-plasma-decal-lifecycle" plasma-cache)
    set_tests_properties(native_tu15_plasma_decal_lifecycle PROPERTIES TIMEOUT 120)
    add_executable(capture_source_weapon_cycle_tu15 tools/capture_source_weapon_cycle_tu15.cpp)
    target_link_libraries(capture_source_weapon_cycle_tu15 PRIVATE halo2_systems)
    add_executable(capture_source_fp_runtime_tu15 tools/capture_source_weapon_cycle_tu15.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_compile_definitions(capture_source_fp_runtime_tu15 PRIVATE HALO2_CAPTURE_SOURCE_FP_RUNTIME=1)
    target_link_libraries(capture_source_fp_runtime_tu15 PRIVATE halo2_renderer halo2_systems SDL2::SDL2)
    add_test(NAME native_tu15_scheduled_weapon_effects COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-scheduled-effects" first-person-scheduled-effects)
    set_tests_properties(native_tu15_scheduled_weapon_effects PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_sound_submit_events COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-sound-submit-acceptance" first-person-events)
    set_tests_properties(native_tu15_sound_submit_events PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_first_person_render_models COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-first-person-render-acceptance" first-person-render)
    set_tests_properties(native_tu15_first_person_render_models PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_primary_weapon_cycle COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-primary-acceptance")
    add_test(NAME native_tu15_primary_weapon_reload COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-reload-acceptance" reload)
    set_tests_properties(native_tu15_primary_weapon_cycle native_tu15_primary_weapon_reload
        PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_first_person_weapon_cycle COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-first-person-acceptance" first-person)
    set_tests_properties(native_tu15_first_person_weapon_cycle PROPERTIES TIMEOUT 180)
    add_test(NAME native_tu15_first_person_player_birth COMMAND capture_source_weapon_cycle_tu15
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_BINARY_DIR}/sandbox-first-person-birth-acceptance" first-person-start)
    set_tests_properties(native_tu15_first_person_player_birth PROPERTIES TIMEOUT 180)
    if(UNIX AND NOT APPLE)
        target_link_options(capture_source_melee_contact PRIVATE -Wl,--strip-debug)
    endif()
    add_executable(engine_team_admission_tu15_tests tests/engine_team_admission_tu15_tests.cpp)
    target_link_libraries(engine_team_admission_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_engine_team_admission COMMAND engine_team_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_team_admission_tu15_original.json")
    set_tests_properties(original_tu15_engine_team_admission PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_engine_team_reset COMMAND engine_team_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_team_reset_tu15_original.json")
    set_tests_properties(original_tu15_engine_team_reset PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(combat_action_tu15_tests tests/combat_action_tu15_tests.cpp)
    target_link_libraries(combat_action_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_combat_action COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combat_action_tu15_original.json")
    add_test(NAME original_tu15_decal_lifetime COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/decal_lifetime_tu15_original.json")
    add_test(NAME original_tu15_texture_cache_frame COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/texture_cache_frame_tu15_original.json")
    add_test(NAME original_tu15_effect_location_resolver COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_location_resolver_tu15_original.json")
    set_tests_properties(original_tu15_combat_action PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_cache_bitmap_view COMMAND combat_action_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/combat_cache_bitmap_view_tu15_original.json")
    set_tests_properties(original_tu15_cache_bitmap_view PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tu15_grenade_weapon_gate_tests tests/tu15_grenade_weapon_gate_tests.cpp)
    target_link_libraries(tu15_grenade_weapon_gate_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_grenade_weapon_gate COMMAND tu15_grenade_weapon_gate_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_grenade_weapon_gate_original.json")
    set_tests_properties(original_tu15_grenade_weapon_gate PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_device_script_runtime_tu15_tests tests/source_device_script_runtime_tu15_tests.cpp)
    target_link_libraries(source_device_script_runtime_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_source_device_script_tu15 COMMAND source_device_script_runtime_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
    set_tests_properties(native_source_device_script_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME native_source_wheel_power_tu15 COMMAND source_device_script_runtime_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map" --wheel-power)
    set_tests_properties(native_source_wheel_power_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME native_source_script_prop_physics_tu15 COMMAND source_device_script_runtime_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/colossus.map" --script-props)
    set_tests_properties(native_source_script_prop_physics_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME native_source_elongation_prop_physics_tu15 COMMAND source_device_script_runtime_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/elongation.map" --script-props)
    set_tests_properties(native_source_elongation_prop_physics_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
        ENVIRONMENT "HALO2_TU15_SCRIPT_PROP_NAME=crate_tech_giant;HALO2_TU15_REQUIRE_CONVEYOR_TRANSPORT=1")
    add_executable(physics_live_world_tu15_tests tests/physics_live_world_tu15_tests.cpp)
    target_link_libraries(physics_live_world_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_physics_live_world_tu15 COMMAND physics_live_world_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(native_physics_live_world_tu15 PROPERTIES TIMEOUT 120)
endif()

target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/weapons/source_projectile_effect_observer.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_effect_compiled_symbols_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/vehicles/source_vehicle_compiled_symbols_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp)
if(BUILD_TESTING)
    add_executable(source_weapon_animation_query_symbols_tu15_tests
        tests/source_weapon_animation_query_symbols_tu15_tests.cpp
        src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp)
    target_include_directories(source_weapon_animation_query_symbols_tu15_tests PRIVATE include)
    target_link_libraries(source_weapon_animation_query_symbols_tu15_tests PRIVATE Threads::Threads)
    add_test(NAME source_weapon_animation_query_symbols_tu15 COMMAND source_weapon_animation_query_symbols_tu15_tests)
    add_executable(source_vehicle_compiled_symbols_tu15_tests
        tests/source_vehicle_compiled_symbols_tu15_tests.cpp
        src/gameplay/vehicles/source_vehicle_compiled_symbols_tu15.cpp)
    target_include_directories(source_vehicle_compiled_symbols_tu15_tests PRIVATE include src)
    target_link_libraries(source_vehicle_compiled_symbols_tu15_tests PRIVATE Threads::Threads)
    add_test(NAME source_vehicle_compiled_symbols_tu15 COMMAND source_vehicle_compiled_symbols_tu15_tests)
    add_executable(source_effect_compiled_symbols_tu15_tests
        tests/source_effect_compiled_symbols_tu15_tests.cpp
        src/rendering/effects/source_effect_compiled_symbols_tu15.cpp)
    target_include_directories(source_effect_compiled_symbols_tu15_tests PRIVATE include)
    target_link_libraries(source_effect_compiled_symbols_tu15_tests PRIVATE Threads::Threads)
    add_test(NAME source_effect_compiled_symbols_tu15 COMMAND source_effect_compiled_symbols_tu15_tests)
    add_executable(object_function_symbol_tu15_tests EXCLUDE_FROM_ALL tests/object_function_symbol_tu15_tests.cpp)
    target_link_libraries(object_function_symbol_tu15_tests PRIVATE halo2_systems)
    set(HALO2_OBJECT_FUNCTION_MCC_MAP "" CACHE FILEPATH "Prepared MCC map for original object-function namespace regression")
    add_test(NAME object_function_symbol_namespace COMMAND object_function_symbol_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_firing_material_tu15.json"
        "${HALO2_OBJECT_FUNCTION_MCC_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/object-function-symbol.json"
        "${PROJECT_SOURCE_DIR}/tests/fixtures/object_function_folded_symbols_tu15.json")
    set_tests_properties(object_function_symbol_namespace PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_effect_world_locations.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/objects/source_effect_replica_readback_tu15.cpp)
# Effect readback calls the systems-owned SourceRuntimeMemory implementation.
# Declare its owner so CMake rescans the complete static dependency cycle.
target_link_libraries(halo2_physics_world_runtime PRIVATE halo2_systems)
target_link_libraries(halo2_physics_world_runtime PUBLIC halo2_raw_datum_pool_identity halo2_mcc_sound_tag_layout halo2_cache)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/source_projectile_presentation.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/source_combat_attachments.cpp)

if(BUILD_TESTING)
    add_executable(replay_contrail_snapshot_tu15 tools/replay_contrail_snapshot_tu15.cpp)
    target_link_libraries(replay_contrail_snapshot_tu15 PRIVATE halo2_systems nlohmann_json::nlohmann_json)
endif()

if(BUILD_TESTING)
    add_executable(source_contrail_snapshot_tests tests/source_contrail_snapshot_tests.cpp)
    target_link_libraries(source_contrail_snapshot_tests PRIVATE halo2_systems)
    set(HALO2_TU15_CONTRAIL_XBE "" CACHE FILEPATH "Owned English TU1.5 XBE for contrail sparse-input tests")
    add_test(NAME original_tu15_contrail_snapshot_inputs COMMAND source_contrail_snapshot_tests "${HALO2_TU15_CONTRAIL_XBE}")
    set_tests_properties(original_tu15_contrail_snapshot_inputs PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

if(BUILD_TESTING)
    add_executable(campaign_vehicle_physics_tu15_tests tests/campaign_vehicle_physics_tu15_tests.cpp)
    target_link_libraries(campaign_vehicle_physics_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
endif()

target_sources(halo2_systems PRIVATE src/rendering/effects/source_effect_light_birth_tu15.cpp)

if(BUILD_TESTING)
    add_executable(source_machine_world_tu15_tests EXCLUDE_FROM_ALL tests/source_machine_world_tu15_tests.cpp)
    target_link_libraries(source_machine_world_tu15_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    add_executable(fp_inherited_binding_tu15_tests EXCLUDE_FROM_ALL tests/fp_inherited_binding_tu15_tests.cpp)
    target_link_libraries(fp_inherited_binding_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_multiplayer_world_tu15_tests EXCLUDE_FROM_ALL tests/campaign_multiplayer_world_tu15_tests.cpp)
    target_link_libraries(campaign_multiplayer_world_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
endif()

if(BUILD_TESTING)
  # These tests retain only their small JSON verdicts; original ABI/RAM captures
  # and executable scratch are removed by run_test on every exit path.
  foreach(context IN ITEMS sentinelwalls floodlab)
    if(context STREQUAL "sentinelwalls")
      set(child_map "06a_sentinelwalls.map")
      set(child_placement 69)
      set(child_bsp 0)
    else()
      set(child_map "04b_floodlab.map")
      set(child_placement 8)
      set(child_bsp 5)
    endif()
    add_test(NAME campaign_model_child_${context}_tu15
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py
        --scratch-root ${CMAKE_CURRENT_BINARY_DIR}/test-scratch --
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_model_child_runner.py
        $<TARGET_FILE:campaign_multiplayer_world_tu15_tests>
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${child_map}
        ${CMAKE_CURRENT_BINARY_DIR}/campaign-child-${context}.json)
    set_tests_properties(campaign_model_child_${context}_tu15 PROPERTIES TIMEOUT 240
      ENVIRONMENT "HALO2_TU15_CAMPAIGN_PLACEMENT=${child_placement};HALO2_TU15_CAMPAIGN_BSP=${child_bsp};HALO2_TU15_CHILD_AUTHORITY_CHECK=1")
  endforeach()
endif()
