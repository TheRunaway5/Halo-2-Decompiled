include_guard(GLOBAL)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND HALO2_MCC_PARTY_UI_TEST_CACHE)
    add_test(NAME rust_frontend_clans_http COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_clans_http_test.py" $<TARGET_FILE:frontend_online_tests>
        --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}")
    set_tests_properties(rust_frontend_clans_http PROPERTIES TIMEOUT 100 RUN_SERIAL TRUE SKIP_RETURN_CODE 77)
    foreach(case blocked complete rendered)
        add_test(NAME rust_frontend_lobby_${case} COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_rust_matchmaking_service.py"
            --source "${CMAKE_CURRENT_SOURCE_DIR}" --frontend-buttons-only
            --native-frontend-probe $<TARGET_FILE:frontend_online_lobby_buttons_tests>
            --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}" --frontend-case "${case}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/rust-frontend-${case}")
        set_tests_properties(rust_frontend_lobby_${case} PROPERTIES TIMEOUT 420 RUN_SERIAL TRUE)
    endforeach()
endif()
