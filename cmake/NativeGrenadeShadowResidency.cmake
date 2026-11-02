include_guard(GLOBAL)

if(BUILD_TESTING AND TARGET halo2_native AND Python3_EXECUTABLE)
    set(grenade_shadow_maps "")
    if(HALO2_ORIGINAL_LOADING_CACHE)
        set(grenade_shadow_maps "${HALO2_ORIGINAL_LOADING_CACHE}/halo2/h2_maps_win64_dx11")
    elseif(HALO2_MCC_MULTIPLAYER_ENGINE_MAP AND EXISTS "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}")
        get_filename_component(grenade_shadow_maps "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" DIRECTORY)
    endif()
    set(grenade_shadow_root "${HALO2_TEST_ASSET_SOURCE_ROOT}")
    if(NOT grenade_shadow_root)
        set(grenade_shadow_root "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    find_program(HALO2_GRENADE_SHADOW_XVFB_RUN xvfb-run)
    if(HALO2_GRENADE_SHADOW_XVFB_RUN)
        foreach(kind neutral frag plasma receiver)
            set(grenade_shadow_map "${grenade_shadow_maps}/coagulation.map")
            if(kind STREQUAL "plasma")
                set(grenade_shadow_map "${grenade_shadow_maps}/midship.map")
            elseif(kind STREQUAL "receiver")
                set(grenade_shadow_map "${grenade_shadow_maps}/zanzibar.map")
            endif()
            if(EXISTS "${grenade_shadow_map}")
                set(test_name "native_mcc_grenade_shadow_${kind}")
                add_test(NAME ${test_name}
                    COMMAND "${HALO2_GRENADE_SHADOW_XVFB_RUN}" -a -s "-screen 0 1280x720x24"
                        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_grenade_shadow_residency.py"
                        --root "${grenade_shadow_root}" --runtime "$<TARGET_FILE:halo2_native>"
                        --map "${grenade_shadow_map}" --shared-maps "${grenade_shadow_maps}"
                        --case "${kind}"
                        --report-output "${CMAKE_CURRENT_BINARY_DIR}/grenade-shadow-${kind}.json")
                set_tests_properties(${test_name} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
                    LABELS "mcc;grenade;shadow;native-runtime"
                    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy")
            endif()
        endforeach()
    endif()
endif()
