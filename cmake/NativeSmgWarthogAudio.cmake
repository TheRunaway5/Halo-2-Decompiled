include_guard(GLOBAL)

if(BUILD_TESTING AND TARGET halo2_native AND Python3_EXECUTABLE)
    set(smg_warthog_maps "")
    if(HALO2_ORIGINAL_LOADING_CACHE)
        set(smg_warthog_maps "${HALO2_ORIGINAL_LOADING_CACHE}/halo2/h2_maps_win64_dx11")
    elseif(HALO2_MCC_MULTIPLAYER_ENGINE_MAP AND EXISTS "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}")
        get_filename_component(smg_warthog_maps "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" DIRECTORY)
    endif()
    set(smg_warthog_root "${HALO2_TEST_ASSET_SOURCE_ROOT}")
    if(NOT smg_warthog_root)
        set(smg_warthog_root "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    set(smg_warthog_input "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/smg_warthog_loop_audio_inputs.tsv")
    set(smg_warthog_metadata "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/smg_warthog_loop_audio_inputs.json")
    set(smg_warthog_capture_tick 0)
    if(EXISTS "${smg_warthog_metadata}")
        file(SIZE "${smg_warthog_metadata}" smg_warthog_metadata_bytes)
        if(smg_warthog_metadata_bytes GREATER 0 AND smg_warthog_metadata_bytes LESS_EQUAL 65536)
            file(READ "${smg_warthog_metadata}" smg_warthog_metadata_json)
            string(JSON smg_warthog_capture_tick ERROR_VARIABLE smg_warthog_metadata_error
                GET "${smg_warthog_metadata_json}" capture_tick)
            if(smg_warthog_metadata_error OR NOT smg_warthog_capture_tick MATCHES "^[0-9]+$")
                set(smg_warthog_capture_tick 0)
            endif()
        endif()
    endif()
    find_program(HALO2_SMG_WARTHOG_XVFB_RUN xvfb-run)
    # The authored schedule is admitted only after its real adaptive run has
    # qualified. A missing fixture never substitutes a synthetic input tape;
    # the launcher builder separately requires this test's positive receipt.
    if(HALO2_SMG_WARTHOG_XVFB_RUN AND EXISTS "${smg_warthog_maps}/coagulation.map"
            AND EXISTS "${smg_warthog_input}" AND EXISTS "${smg_warthog_metadata}"
            AND smg_warthog_capture_tick GREATER_EQUAL 600 AND smg_warthog_capture_tick LESS_EQUAL 1800)
        add_test(NAME native_mcc_smg_warthog_audio
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_smg_warthog_audio_replay.py"
                --root "${smg_warthog_root}" --runtime "$<TARGET_FILE:halo2_native>"
                --map "${smg_warthog_maps}/coagulation.map" --shared-maps "${smg_warthog_maps}"
                --input-script "${smg_warthog_input}" --capture-tick "${smg_warthog_capture_tick}"
                --report-output "${CMAKE_CURRENT_BINARY_DIR}/smg-warthog-audio.json")
        set_tests_properties(native_mcc_smg_warthog_audio PROPERTIES
            TIMEOUT 600 RUN_SERIAL TRUE LABELS "mcc;smg;warthog;audio;native-runtime"
            ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy")
    endif()
endif()
