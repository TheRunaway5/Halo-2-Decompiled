# Explicit opt-in terminal acceptance. No matchmaking default is enabled.
include_guard(GLOBAL)
if(TARGET online_couch_launch_tests)
    target_include_directories(online_couch_launch_tests PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
    target_link_libraries(online_couch_launch_tests PRIVATE halo2_online_protocol)
    add_dependencies(online_couch_launch_tests halo2_online_protocol_rust_build)
    # Use the same adjacent process-main worker/official SDK packaging as the
    # native application; no environment-selected worker or fake SDK seam.
    halo2_bundle_eos(online_couch_launch_tests)
    if(BUILD_TESTING AND HALO2_MCC_PARTY_UI_TEST_CACHE)
        foreach(eos_mode IN ITEMS terminal-eos terminal-eos-abrupt)
            string(REPLACE "-" "_" eos_test "${eos_mode}")
            add_test(NAME online_couch_${eos_test} COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/online_couch_launch_http_test.py"
                $<TARGET_FILE:online_couch_launch_tests> ${eos_mode} "${HALO2_MCC_PARTY_UI_TEST_CACHE}")
            set_tests_properties(online_couch_${eos_test} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 620 RUN_SERIAL TRUE)
        endforeach()
    endif()
endif()
