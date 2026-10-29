# Ordinary Linux default builds also produce the native Windows Vulkan runtime.
# Focused tests and the Python coordinator opt out to keep one owner per build.
set(_halo2_windows_default OFF)
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
        AND NOT DEFINED ENV{HALO2_TEST_SCRATCH})
    set(_halo2_windows_default ON)
endif()
option(HALO2_BUILD_WINDOWS "Build the native Windows Vulkan runtime with each default Linux build" ${_halo2_windows_default})
unset(_halo2_windows_default)
if(HALO2_BUILD_WINDOWS AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND NOT DEFINED ENV{HALO2_TEST_SCRATCH})
    add_custom_target(halo2_windows ALL
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_runtime.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --build-dir "${CMAKE_BINARY_DIR}-windows"
            --platform windows --no-gui --companion
        USES_TERMINAL
        COMMENT "Building and packaging native Windows x86_64 Vulkan runtime"
        VERBATIM)
endif()

if(BUILD_TESTING AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    add_custom_target(halo2_build_profile_checks)
    add_test(NAME native_windows_package COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_windows_package.py")
    set_tests_properties(native_windows_package PROPERTIES TIMEOUT 120 LABELS "windows;packaging")
endif()

# The Windows preset also yields a runnable directory rather than only a PE
# image whose imported SDK DLLs still live in the cross-build sysroot.
if(WIN32 AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND HALO2_STAGE_BUILD_OUTPUT)
    add_custom_target(halo2_windows_package ALL
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/package_runtime.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --build-dir "${CMAKE_BINARY_DIR}"
            --sdk "${HALO2_WINDOWS_SDK}"
        DEPENDS halo2_native halo2_launcher halo2_title_launcher halo2_onboarding
        USES_TERMINAL
        COMMENT "Packaging native Windows Vulkan executable and runtime dependencies"
        VERBATIM)
    if(TARGET halo2_mcc_loading_audio)
        add_dependencies(halo2_windows_package halo2_mcc_loading_audio)
    endif()
endif()
