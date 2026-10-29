include_guard(GLOBAL)

if(BUILD_TESTING AND TARGET halo2_native AND Python3_EXECUTABLE)
    set(split_screen_startup_maps "")
    if(HALO2_MCC_MULTIPLAYER_ENGINE_MAP AND EXISTS "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}")
        get_filename_component(split_screen_startup_maps "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" DIRECTORY)
    elseif(HALO2_ORIGINAL_LOADING_CACHE)
        set(split_screen_startup_maps "${HALO2_ORIGINAL_LOADING_CACHE}/halo2/h2_maps_win64_dx11")
    endif()

    find_program(HALO2_SPLIT_SCREEN_STARTUP_XVFB_RUN xvfb-run)
    if(HALO2_SPLIT_SCREEN_STARTUP_XVFB_RUN AND EXISTS "${split_screen_startup_maps}/mainmenu.map")
        add_test(NAME native_mcc_split_screen_menu_startup
            COMMAND "${HALO2_SPLIT_SCREEN_STARTUP_XVFB_RUN}" -a -s "-screen 0 1280x720x24"
                "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/native_split_screen_menu_startup.py"
                --root "${HALO2_TEST_ASSET_SOURCE_ROOT}" --runtime "$<TARGET_FILE:halo2_native>"
                --maps "${split_screen_startup_maps}"
                --report "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-menu-startup.json")
        set_tests_properties(native_mcc_split_screen_menu_startup PROPERTIES
            TIMEOUT 240 RUN_SERIAL TRUE LABELS "mcc;frontend;split-screen;native-runtime"
            ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;XDG_CONFIG_HOME=@HALO2_TEST_SCRATCH@/config;XDG_DATA_HOME=@HALO2_TEST_SCRATCH@/data;HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/vulkan-cache;MESA_SHADER_CACHE_DIR=@HALO2_TEST_SCRATCH@/mesa-cache;__GL_SHADER_DISK_CACHE_PATH=@HALO2_TEST_SCRATCH@/nvidia-cache")
    endif()
endif()
