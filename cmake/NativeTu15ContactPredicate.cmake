if(BUILD_TESTING)
    add_custom_target(native_contact_predicate_tests)
    set(_halo2_contact_predicate_checks)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_contact_predicate_checks --sanitizers --mutations)
    endif()
    function(halo2_add_contact_predicate_test name driver report)
        add_test(NAME "${name}"
            COMMAND "${CMAKE_COMMAND}" -E env --unset=HALO2_REFERENCE_CONTACT_PREDICATE
            "${HALO2_RENDERER_ORACLE_PYTHON}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py"
            --scratch-root "${CMAKE_CURRENT_BINARY_DIR}/native-contact-predicate-scratch" --
            "${HALO2_RENDERER_ORACLE_PYTHON}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/${driver}"
            --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/${report}" ${ARGN})
        set_tests_properties("${name}" PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endfunction()
    halo2_add_contact_predicate_test(original_tu15_native_contact_predicate_raw
        validate_native_contact_predicate_tu15.py native-contact-predicate-raw.json
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --prefixes ${_halo2_contact_predicate_checks})
    halo2_add_contact_predicate_test(original_tu15_native_contact_predicate_source
        validate_native_contact_predicate_source_tu15.py native-contact-predicate-source.json
        --ordered-reference --bound-candidate --require-installed --structure ${_halo2_contact_predicate_checks})
    halo2_add_contact_predicate_test(original_tu15_native_contact_predicate_generation
        validate_native_contact_predicate_source_tu15.py native-contact-predicate-generation.json
        --ordered-reference --bound-candidate --require-installed --structure --fresh ${_halo2_contact_predicate_checks})
    unset(_halo2_contact_predicate_checks)
endif()
