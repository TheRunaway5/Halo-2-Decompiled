if(BUILD_TESTING)
    add_custom_target(native_object_node_rebuild_tests)
    add_custom_target(native_object_animation_delivery_tests)
    add_custom_target(native_object_prediction_copy_tests)
    set(_halo2_object_node_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_node_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_object_node_rebuild
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_node_rebuild_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --hardware --require-binding
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-node-rebuild.json"
        ${_halo2_object_node_sanitizers})
    add_test(NAME original_tu15_native_object_animation_delivery
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_animation_delivery_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-animation-delivery.json"
        ${_halo2_object_node_sanitizers})
    add_test(NAME original_tu15_native_object_prediction_copy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_prediction_copy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-prediction-copy.json"
        ${_halo2_object_node_sanitizers})
    set_tests_properties(original_tu15_native_object_node_rebuild
        original_tu15_native_object_animation_delivery
        original_tu15_native_object_prediction_copy PROPERTIES TIMEOUT 420)
    set(_halo2_object_node_live_controls)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_node_live_controls --mutations)
    endif()
    add_test(NAME native_object_node_live_admission
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_node_live_admission.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-node-live-admission.json"
        --compiler "${CMAKE_CXX_COMPILER}"
        ${_halo2_object_node_sanitizers} ${_halo2_object_node_live_controls})
    set_tests_properties(native_object_node_live_admission PROPERTIES TIMEOUT 180)
    unset(_halo2_object_node_live_controls)
    unset(_halo2_object_node_sanitizers)
endif()
