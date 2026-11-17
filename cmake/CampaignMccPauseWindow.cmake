# Exercise the actual campaign main caller, including its MCC-only pause
# construction, Graphics Mode persistence and Restart handoff.
option(HALO2_REQUIRE_MCC_CAMPAIGN_PAUSE_TEST
    "Require actual MCC campaign pause coverage for launcher qualification" OFF)
set(HALO2_MCC_CAMPAIGN_PAUSE_MAP "" CACHE FILEPATH
    "Imported MCC 00a_introduction.map for actual campaign pause qualification")
if(NOT HALO2_MCC_CAMPAIGN_PAUSE_MAP AND HALO2_MCC_MULTIPLAYER_ENGINE_MAP)
    get_filename_component(halo2_campaign_pause_maps "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" DIRECTORY)
    set(HALO2_MCC_CAMPAIGN_PAUSE_MAP "${halo2_campaign_pause_maps}/00a_introduction.map")
endif()
add_executable(mcc_gameplay_pause_assets_tests tests/mcc_gameplay_pause_assets_tests.cpp)
target_link_libraries(mcc_gameplay_pause_assets_tests PRIVATE halo2_cache)
if(EXISTS "${HALO2_MCC_CAMPAIGN_PAUSE_MAP}")
    get_filename_component(halo2_campaign_pause_maps "${HALO2_MCC_CAMPAIGN_PAUSE_MAP}" DIRECTORY)
    add_test(NAME native_mcc_gameplay_pause_assets COMMAND mcc_gameplay_pause_assets_tests
        "${halo2_campaign_pause_maps}")
    set_tests_properties(native_mcc_gameplay_pause_assets PROPERTIES TIMEOUT 180)
endif()
if(HALO2_REQUIRE_MCC_CAMPAIGN_PAUSE_TEST)
    if(NOT UNIX OR APPLE OR NOT HALO2_LAUNCHER_XVFB)
        message(FATAL_ERROR "MCC campaign pause qualification requires Linux xvfb-run")
    endif()
    if(NOT EXISTS "${HALO2_MCC_CAMPAIGN_PAUSE_MAP}")
        message(FATAL_ERROR "MCC campaign pause qualification requires an imported 00a_introduction.map")
    endif()
endif()
if(UNIX AND NOT APPLE AND HALO2_LAUNCHER_XVFB AND EXISTS "${HALO2_MCC_CAMPAIGN_PAUSE_MAP}")
    add_test(NAME native_mcc_campaign_pause_window COMMAND ${HALO2_LAUNCHER_XVFB} -a
        -s "-screen 0 640x480x24" ${Python3_EXECUTABLE} -B
        ${PROJECT_SOURCE_DIR}/tools/test_campaign_mcc_pause_window_flow.py
        --runtime $<TARGET_FILE:halo2_native> --map "${HALO2_MCC_CAMPAIGN_PAUSE_MAP}"
        --validation --timeout-seconds 180
        --report-output "${CMAKE_CURRENT_BINARY_DIR}/campaign-pause-window.json")
    # TestStorage.cmake supplies run_test.py; all probe binaries, preferences,
    # frames and driver caches stay in that invocation's private directory.
    # The helper reports all four cases and never returns a skip result.
    set_tests_properties(native_mcc_campaign_pause_window PROPERTIES TIMEOUT 780 RUN_SERIAL TRUE
        ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;VK_LAYER_VALIDATE_SYNC=1;VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;XDG_CONFIG_HOME=@HALO2_TEST_SCRATCH@/config;XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/pipelines;MESA_SHADER_CACHE_DIR=@HALO2_TEST_SCRATCH@/mesa;__GL_SHADER_DISK_CACHE_PATH=@HALO2_TEST_SCRATCH@/nvidia")
endif()
