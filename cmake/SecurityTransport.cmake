if(BUILD_TESTING)
    add_executable(confined_file_tests "${CMAKE_CURRENT_LIST_DIR}/../tests/confined_file_tests.cpp")
    target_include_directories(confined_file_tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../include")
    target_compile_features(confined_file_tests PRIVATE cxx_std_20)
    add_test(NAME security_resource_paths COMMAND confined_file_tests)
    add_executable(security_transport_tests "${CMAKE_CURRENT_LIST_DIR}/../tests/security_transport_tests.cpp")
    target_include_directories(security_transport_tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../include")
    target_compile_features(security_transport_tests PRIVATE cxx_std_20)
    target_link_libraries(security_transport_tests PRIVATE halo2_online_protocol OpenSSL::Crypto Threads::Threads)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(security_transport_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(security_transport_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME security_transport COMMAND security_transport_tests)
    add_executable(snapshot_allocation_tests "${CMAKE_CURRENT_LIST_DIR}/../tests/snapshot_allocation_tests.cpp")
    target_link_libraries(snapshot_allocation_tests PRIVATE halo2_systems)
    add_test(NAME native_snapshot_allocation_budget COMMAND snapshot_allocation_tests)
    add_test(NAME security_asset_limits COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s "${CMAKE_CURRENT_LIST_DIR}/../tests" -p test_security_asset_limits.py)
    add_test(NAME security_tool_limits COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s "${CMAKE_CURRENT_LIST_DIR}/../tests" -p test_security_tool_limits.py)
    add_test(NAME security_import_work_limits COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s "${CMAKE_CURRENT_LIST_DIR}/../tests" -p test_security_scan_20261028_imports.py)
    add_test(NAME security_snapshot_expansion_budget COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_LIST_DIR}/../tools/test_snapshot_expansion_budget.py"
        --output "@HALO2_TEST_SCRATCH@/snapshot-allocation.json")
    set_tests_properties(security_snapshot_expansion_budget PROPERTIES TIMEOUT 180)
endif()
