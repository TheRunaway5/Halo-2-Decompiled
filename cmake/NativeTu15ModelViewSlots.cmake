target_sources(halo2_physics_world_runtime PRIVATE
    src/rendering/models/native_tu15_model_view_slots.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_model_view_slots_tests
        tests/native_tu15_model_view_slots_tests.cpp
        src/rendering/models/native_tu15_model_view_slots.cpp)
    target_include_directories(native_tu15_model_view_slots_tests PRIVATE include src)
    target_compile_features(native_tu15_model_view_slots_tests PRIVATE cxx_std_20)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_model_view_slots_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_model_view_slots_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_model_view_slots COMMAND native_tu15_model_view_slots_tests)
    add_test(NAME original_tu15_native_model_view_slots
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_view_slots.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_model_view_slots_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-view-slots.json)
    set_tests_properties(original_tu15_native_model_view_slots PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()
