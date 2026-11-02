add_executable(mcc_resident_reflection_template_tests
    tests/mcc_resident_reflection_template_tests.cpp
    src/rendering/mcc_native_world_cooked.cpp src/assets/cache.cpp)
target_include_directories(mcc_resident_reflection_template_tests PRIVATE include src)
target_compile_definitions(mcc_resident_reflection_template_tests PRIVATE HALO2_CACHE_BYTES_ONLY=1)
target_link_libraries(mcc_resident_reflection_template_tests PRIVATE
    nlohmann_json::nlohmann_json OpenSSL::Crypto)
add_test(NAME mcc_resident_reflection_template COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_resident_reflection_template_tests.py
    --repo-root "${CMAKE_CURRENT_SOURCE_DIR}"
    --test-executable $<TARGET_FILE:mcc_resident_reflection_template_tests>)
set_tests_properties(mcc_resident_reflection_template PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
set(mcc_shared_env_asset_root "${HALO2_TEST_ASSET_SOURCE_ROOT}")
if(NOT mcc_shared_env_asset_root)
    set(mcc_shared_env_asset_root "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
add_test(NAME mcc_shared_env_illum COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_shared_env_illum_tests.py
    --repo-root "${CMAKE_CURRENT_SOURCE_DIR}"
    --test-executable $<TARGET_FILE:mcc_native_tex_bump_tests>
    --asset-source-root "${mcc_shared_env_asset_root}")
set_tests_properties(mcc_shared_env_illum PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
