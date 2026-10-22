# The proven standalone cold-data closure is Linux/x86_64 ELF only. Other
# platforms retain their existing configuration; no unproved GC-link variant.
if(BUILD_TESTING)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
        AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        # Compile real data owners directly: linking halo2_systems would pull
        # the full game/physics/effect ABI build into this cold-only check.
        add_executable(native_tu15_cold_data_tests tests/native_tu15_cold_data_tests.cpp
            src/assets/cache.cpp
            src/assets/cache_file_snapshot.cpp
            src/physics/native_tu15_arena_data.cpp
            src/gameplay/objects/native_tu15_aux_object_state_data.cpp
            src/assets/native_tu15_cache_startup_data.cpp
            src/gameplay/camera/native_tu15_camera_packet_data.cpp
            src/platform/native_tu15_clock_startup_data.cpp
            src/rendering/effects/native_tu15_contrail_draw_data.cpp
            src/gameplay/multiplayer/native_tu15_ctf_static_data.cpp
            src/platform/native_tu15_final_startup_storage_data.cpp
            src/gameplay/multiplayer/native_tu15_game_startup_data.cpp
            src/platform/native_tu15_heap_data.cpp
            src/input/native_tu15_input_startup_data.cpp
            src/campaign/native_tu15_level_handles_startup_data.cpp
            src/rendering/renderer/native_tu15_main_view_data.cpp
            src/gameplay/multiplayer/native_tu15_multiplayer_branch_data.cpp
            src/gameplay/multiplayer/native_tu15_multiplayer_data.cpp
            src/network/native_tu15_network_session_data.cpp
            src/network/native_tu15_network_session_presence_data.cpp
            src/gameplay/objects/native_tu15_object_storage_data.cpp
            src/gameplay/camera/native_tu15_observer_data.cpp
            src/platform/native_tu15_operands.generated.cpp
            src/physics/native_tu15_physics_branch_data.cpp
            src/gameplay/units/native_tu15_player_result_data.cpp
            src/gameplay/units/native_tu15_player_startup_data.cpp
            src/assets/native_tu15_prt_cache_data.cpp
            src/rendering/renderer/native_tu15_render_object_startup_data.cpp
            src/rendering/renderer/native_tu15_renderer_cache_pools_data.cpp
            src/rendering/renderer/native_tu15_renderer_request_data.cpp
            src/rendering/renderer/native_tu15_renderer_resource_storage_data.cpp
            src/rendering/renderer/native_tu15_renderer_startup_data.cpp
            src/rendering/renderer/native_tu15_renderer_tail_storage_data.cpp
            src/rendering/renderer/native_tu15_renderer_target_storage_data.cpp
            src/rendering/renderer/native_tu15_renderer_transparent_storage_data.cpp
            src/platform/native_tu15_rng_startup.cpp
            src/core/native_tu15_scalar_state_data.cpp
            src/audio/native_tu15_sound_sdk_prefix_data.cpp
            src/audio/native_tu15_startup_sound_storage_data.cpp
            src/platform/native_tu15_state_context_startup_data.cpp
            src/core/native_tu15_static_owners.cpp
            src/platform/native_tu15_static_registration_data.cpp
            src/ui/native_tu15_ui_script_startup_data.cpp
            src/rendering/renderer/native_tu15_view_storage_data.cpp
            src/platform/native_tu15_world_memory.cpp
            src/runtime/source_runtime_memory.cpp
            src/runtime/source_runtime_snapshot_bytes.cpp
        )
        target_include_directories(native_tu15_cold_data_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_compile_features(native_tu15_cold_data_tests PRIVATE cxx_std_20)
        target_compile_options(native_tu15_cold_data_tests PRIVATE
            -ffunction-sections -fdata-sections -ffp-contract=off)
        target_link_options(native_tu15_cold_data_tests PRIVATE "LINKER:--gc-sections")
        target_link_libraries(native_tu15_cold_data_tests PRIVATE Threads::Threads)
        if(HALO2_SANITIZERS)
            target_compile_options(native_tu15_cold_data_tests PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(native_tu15_cold_data_tests PRIVATE -fsanitize=address,undefined)
        endif()
        add_test(NAME native_tu15_cold_data COMMAND native_tu15_cold_data_tests)
        set_tests_properties(native_tu15_cold_data PROPERTIES
            TIMEOUT 30 LABELS "cold-data;source-ownership;component")
    else()
        message(STATUS "Standalone TU1.5 cold-data regression requires Linux x86_64 GNU/Clang ELF section GC")
    endif()
endif()
