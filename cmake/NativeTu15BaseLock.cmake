if(BUILD_TESTING)
    add_executable(native_tu15_base_lock_tests tests/native_tu15_base_lock_tests.cpp)
    target_include_directories(native_tu15_base_lock_tests PRIVATE include)
    target_compile_features(native_tu15_base_lock_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_base_lock_tests PRIVATE halo2_glm nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_base_lock_reference COMMAND native_tu15_base_lock_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_base_lock_release.json)
    set_tests_properties(native_tu15_base_lock_reference PROPERTIES TIMEOUT 30)
endif()
