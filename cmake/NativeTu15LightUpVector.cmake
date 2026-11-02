target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/native_tu15_light_up_vector.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_light_up_vector_tests tests/native_tu15_light_up_vector_tests.cpp
        src/rendering/effects/native_tu15_light_up_vector.cpp src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
    target_include_directories(native_tu15_light_up_vector_tests PRIVATE include)
    target_compile_features(native_tu15_light_up_vector_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_light_up_vector_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_tu15_light_up_vector_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_light_up_vector_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_light_up_vector_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_light_up_finite COMMAND native_tu15_light_up_vector_tests --fixture
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/first_person_light_up_tu15.json")
    add_test(NAME original_tu15_native_light_up_vector COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_light_up_vector.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_light_up_vector_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-light-up-vector.json")
    set_tests_properties(original_tu15_native_light_up_vector PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
