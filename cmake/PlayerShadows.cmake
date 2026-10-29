option(HALO2_PLAYER_SHADOW_NATIVE_TESTS
    "Run ordinary native player shadow GPU submission checks in all graphics modes" OFF)
set(HALO2_PLAYER_SHADOW_MAP "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" CACHE FILEPATH
    "Prepared MCC map for ordinary player shadow validation")
if(BUILD_TESTING AND HALO2_PLAYER_SHADOW_NATIVE_TESTS AND HALO2_PLAYER_SHADOW_MAP)
    add_library(player_shadows_test_runtime OBJECT EXCLUDE_FROM_ALL
        src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(player_shadows_test_runtime PRIVATE
        halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
    target_compile_definitions(player_shadows_test_runtime PRIVATE
        HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
    add_executable(player_shadows_joined_runtime_tests EXCLUDE_FROM_ALL
        tests/player_shadows_joined_runtime_tests.cpp
        $<TARGET_OBJECTS:player_shadows_test_runtime>)
    target_link_libraries(player_shadows_joined_runtime_tests PRIVATE
        halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
    target_compile_definitions(player_shadows_joined_runtime_tests PRIVATE
        HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
    add_dependencies(player_shadows_joined_runtime_tests halo2_shaders)
    add_executable(player_shadows_online_runtime_tests EXCLUDE_FROM_ALL
        tests/source_shadow_transport_online_tests.cpp
        $<TARGET_OBJECTS:player_shadows_test_runtime>)
    target_link_libraries(player_shadows_online_runtime_tests PRIVATE
        halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json
        LibDataChannel::LibDataChannelStatic)
    target_compile_definitions(player_shadows_online_runtime_tests PRIVATE
        HALO2_SHADOW_ONLINE_GPU=1 HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
    add_dependencies(player_shadows_online_runtime_tests halo2_shaders path_tracing_shaders)
    find_program(HALO2_PLAYER_SHADOW_XVFB_RUN xvfb-run)
    foreach(mode Original pStencil PathTracing)
        set(name player_shadows_native_${mode})
        add_test(NAME ${name} COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_player_shadows_native.py"
            --runtime "$<TARGET_FILE:halo2_native>" --map "${HALO2_PLAYER_SHADOW_MAP}"
            --mode "${mode}" --verify-visible --report-output "${CMAKE_CURRENT_BINARY_DIR}/${name}.json")
        set_tests_properties(${name} PROPERTIES RUN_SERIAL TRUE TIMEOUT 420
            LABELS "mcc;player-shadows;native-runtime;gpu")
    endforeach()
    if(HALO2_PLAYER_SHADOW_XVFB_RUN)
        get_filename_component(shadow_shared_maps "${HALO2_PLAYER_SHADOW_MAP}" DIRECTORY)
        foreach(mode Original pStencil PathTracing)
            set(online_name player_shadows_online_${mode})
            add_test(NAME ${online_name} COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_player_shadows_online.py"
                --fixture "$<TARGET_FILE:player_shadows_online_runtime_tests>"
                --map "${HALO2_PLAYER_SHADOW_MAP}" --mode "${mode}"
                --report-output "${CMAKE_CURRENT_BINARY_DIR}/${online_name}.json")
            set_tests_properties(${online_name} PROPERTIES RUN_SERIAL TRUE TIMEOUT 360
                LABELS "mcc;player-shadows;ONLINE;direct-peer;native-runtime;gpu"
                ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy")
            set(name player_shadows_joined_${mode})
            add_test(NAME ${name} COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_player_shadows_joined.py"
                --fixture "$<TARGET_FILE:player_shadows_joined_runtime_tests>"
                --map "${HALO2_PLAYER_SHADOW_MAP}" --mode "${mode}"
                --report-output "${CMAKE_CURRENT_BINARY_DIR}/${name}.json")
            set_tests_properties(${name} PROPERTIES RUN_SERIAL TRUE TIMEOUT 300
                LABELS "mcc;player-shadows;joined;native-runtime;gpu"
                ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/vulkan-cache")
        endforeach()
    endif()
endif()
