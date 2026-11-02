if(BUILD_TESTING)
    add_executable(native_tu15_pending_response_tests tests/native_tu15_pending_response_tests.cpp)
    target_include_directories(native_tu15_pending_response_tests PRIVATE include src)
    target_compile_features(native_tu15_pending_response_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_pending_response_tests PRIVATE halo2_glm nlohmann_json::nlohmann_json)
    # Build the real linked consumers once, rather than duplicate object-only
    # copies of their large translation units for the component check.
    add_dependencies(native_tu15_pending_response_tests
        source_vitality_runtime_tests source_damage_delivery_runtime_tests)
    add_test(NAME native_tu15_pending_response_reference COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_pending_response.py
        --probe $<TARGET_FILE:native_tu15_pending_response_tests>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_pending_response_words.bin.gz
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/pending_response_step_tu15.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-pending-response-report.json)
    set_tests_properties(native_tu15_pending_response_reference PROPERTIES TIMEOUT 90)
    set(HALO2_PENDING_REFERENCE_PYTHON "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/bin/python" CACHE FILEPATH
        "Python with Unicorn and Capstone for original pending-response parents")
    add_test(NAME native_tu15_pending_response_parents COMMAND "${HALO2_PENDING_REFERENCE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_pending_response_parents.py
        --vitality $<TARGET_FILE:source_vitality_runtime_tests>
        --delivery $<TARGET_FILE:source_damage_delivery_runtime_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-pending-response-parents-report.json)
    set_tests_properties(native_tu15_pending_response_parents PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
endif()
