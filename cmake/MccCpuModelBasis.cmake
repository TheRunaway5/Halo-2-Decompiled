if(BUILD_TESTING)
    add_executable(mcc_cpu_model_basis_handoff_tests tests/mcc_cpu_model_basis_handoff_tests.cpp)
    target_link_libraries(mcc_cpu_model_basis_handoff_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    if(NOT MSVC)
        target_compile_options(mcc_cpu_model_basis_handoff_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    set(HALO2_MCC_CPU_BASIS_CACHE "" CACHE FILEPATH "Explicit genuine MCC opening cache for CPU basis regression")
    set(HALO2_ORIGINAL_CPU_BASIS_CACHE "" CACHE FILEPATH "Explicit Original opening cache for CPU basis regression")
    if(HALO2_MCC_CPU_BASIS_CACHE AND HALO2_ORIGINAL_CPU_BASIS_CACHE)
        add_test(NAME native_mcc_cpu_model_basis_handoff COMMAND mcc_cpu_model_basis_handoff_tests
            "${HALO2_MCC_CPU_BASIS_CACHE}" "${HALO2_ORIGINAL_CPU_BASIS_CACHE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/captured_mcc_body_basis_v12.json")
        set_tests_properties(native_mcc_cpu_model_basis_handoff PROPERTIES TIMEOUT 180
            ENVIRONMENT "HALO2_MCC_CPU_BASIS_REPORT=${CMAKE_CURRENT_BINARY_DIR}/mcc-cpu-model-basis.json")
    endif()
endif()
