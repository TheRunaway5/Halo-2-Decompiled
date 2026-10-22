# Separate direct-MCC cases; existing legacy reference invocations stay intact.
# Never follow the global selector implicitly or turn a missing import into a
# mixed Vista/disc input. The runner validates the explicit complete owner.
option(HALO2_MCC_FRONTEND_RENDER_TESTS "Run private hardware-GPU rendered MCC ONLINE evidence tests" OFF)
if(BUILD_TESTING AND TARGET frontend_online_tests AND
   DEFINED HALO2_MCC_FRONTEND_TEST_CACHE AND
   NOT "${HALO2_MCC_FRONTEND_TEST_CACHE}" STREQUAL "" AND
   NOT "${HALO2_MCC_FRONTEND_TEST_CACHE}" STREQUAL "auto")
    get_filename_component(_halo2_mcc_online_cache "${HALO2_MCC_FRONTEND_TEST_CACHE}"
        ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(IS_DIRECTORY "${_halo2_mcc_online_cache}")
        # Actual copied native-font owners and supervisor input selection only;
        # this CPU regression does not start a display or claim rendered proof.
        add_test(NAME frontend_mcc_rendered_input_owners COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_rendered_input_pins.py"
            --mcc-root "${_halo2_mcc_online_cache}")
        set_tests_properties(frontend_mcc_rendered_input_owners PROPERTIES TIMEOUT 30)
        foreach(_halo2_mcc_case IN ITEMS clans parties matchmaking)
            add_test(NAME frontend_mcc_${_halo2_mcc_case}_http COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_${_halo2_mcc_case}_http_test.py"
                $<TARGET_FILE:frontend_online_tests> --mcc-root "${_halo2_mcc_online_cache}")
            set_tests_properties(frontend_mcc_${_halo2_mcc_case}_http PROPERTIES
                SKIP_RETURN_CODE 77 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=dummy")
        endforeach()
        set_tests_properties(frontend_mcc_clans_http PROPERTIES TIMEOUT 70)
        set_tests_properties(frontend_mcc_parties_http PROPERTIES TIMEOUT 100)
        set_tests_properties(frontend_mcc_matchmaking_http PROPERTIES TIMEOUT 55)
        if(HALO2_MCC_FRONTEND_RENDER_TESTS)
            foreach(_halo2_mcc_case IN ITEMS clans parties social)
                add_test(NAME frontend_mcc_${_halo2_mcc_case}_rendered COMMAND ${Python3_EXECUTABLE}
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_mcc_rendered_test.py"
                    --case "${_halo2_mcc_case}" --native $<TARGET_FILE:frontend_online_tests>
                    --root "${CMAKE_CURRENT_SOURCE_DIR}" --mcc-root "${_halo2_mcc_online_cache}"
                    --shader-root "${HALO2_RUNTIME_SHADER_DIR}"
                    --output "${CMAKE_CURRENT_BINARY_DIR}/mcc-online-ui-${_halo2_mcc_case}")
                set_tests_properties(frontend_mcc_${_halo2_mcc_case}_rendered PROPERTIES
                    TIMEOUT 350 RUN_SERIAL TRUE RESOURCE_LOCK halo2_gpu
                    ENVIRONMENT "SDL_AUDIODRIVER=dummy;SDL_VIDEODRIVER=x11")
            endforeach()
        endif()
    endif()
    unset(_halo2_mcc_online_cache)
    unset(_halo2_mcc_case)
endif()
