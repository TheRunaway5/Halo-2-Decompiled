# The strict effect receipt is currently qualified only for the x64 SSE skin.
if(BUILD_TESTING AND NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    add_executable(model_selected_skin_reuse_tests tests/model_selected_skin_reuse_tests.cpp)
    target_include_directories(model_selected_skin_reuse_tests PRIVATE include src)
    target_compile_options(model_selected_skin_reuse_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(model_selected_skin_reuse_tests PRIVATE halo2_systems)
    add_test(NAME native_selected_model_skin_reuse COMMAND model_selected_skin_reuse_tests)
    set_tests_properties(native_selected_model_skin_reuse PROPERTIES TIMEOUT 60)
endif()
