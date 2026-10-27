include_guard(GLOBAL)
if(NOT WIN32 OR NOT BUILD_TESTING)
    return()
endif()
get_filename_component(_halo2_windows_test_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_executable(windows_platform_resources_tests
    "${_halo2_windows_test_root}/tests/windows_platform_resources_tests.cpp"
    "${_halo2_windows_test_root}/src/network/native_tu15_network_service.cpp")
target_include_directories(windows_platform_resources_tests PRIVATE
    "${_halo2_windows_test_root}/include" "${_halo2_windows_test_root}/src")
target_link_libraries(windows_platform_resources_tests PRIVATE ws2_32)
if(MINGW)
    # The focused test is independent of the packaged SDK's runtime DLLs.
    target_link_options(windows_platform_resources_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
    add_test(NAME windows_platform_resources COMMAND "${Python3_EXECUTABLE}"
        "${_halo2_windows_test_root}/tests/windows_native_test.py"
        "$<TARGET_FILE:windows_platform_resources_tests>")
else()
    add_test(NAME windows_platform_resources COMMAND windows_platform_resources_tests)
endif()
set_tests_properties(windows_platform_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

if(NOT TARGET SDL2::SDL2)
    find_package(SDL2 REQUIRED CONFIG)
endif()
add_executable(windows_thread_local_registry_tests
    "${_halo2_windows_test_root}/tests/windows_thread_local_registry_tests.cpp"
    "${_halo2_windows_test_root}/tests/windows_thread_local_registry_peer.cpp")
target_include_directories(windows_thread_local_registry_tests PRIVATE
    "${_halo2_windows_test_root}/include" "${_halo2_windows_test_root}/src"
    "$<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>")
target_compile_definitions(windows_thread_local_registry_tests PRIVATE SDL_MAIN_HANDLED)
if(MINGW)
    target_link_options(windows_thread_local_registry_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
    add_test(NAME windows_thread_local_registries COMMAND "${Python3_EXECUTABLE}"
        "${_halo2_windows_test_root}/tests/windows_native_test.py"
        "$<TARGET_FILE:windows_thread_local_registry_tests>")
else()
    add_test(NAME windows_thread_local_registries COMMAND windows_thread_local_registry_tests)
endif()
set_tests_properties(windows_thread_local_registries PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
