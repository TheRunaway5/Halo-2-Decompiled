cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSLauncherTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(SDL2 REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)
find_path(Vulkan_INCLUDE_DIR vulkan/vulkan.h REQUIRED)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include(CTest)
include("${HALO2_ROOT}/cmake/macos/Application.cmake")
add_executable(vulkan_portability_tests "${HALO2_ROOT}/tests/vulkan_portability_tests.cpp")
target_include_directories(vulkan_portability_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src" "${Vulkan_INCLUDE_DIR}")
add_test(NAME vulkan_portability COMMAND vulkan_portability_tests)
set(launchers halo2_launcher halo2_title_launcher halo2_onboarding)
foreach(target IN LISTS launchers)
    add_executable(${target} "${HALO2_ROOT}/src/app/launcher.cpp")
    set(name Halo2-Launcher)
    if(target STREQUAL "halo2_title_launcher")
        set(name Halo2-Title-Launcher)
        target_compile_definitions(${target} PRIVATE HALO2_TITLE_FIRST=1)
    endif()
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${name}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${target}")
    target_link_libraries(${target} PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
    target_include_directories(${target} PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
    halo2_macos_application(${target})
    target_compile_definitions(${target} PRIVATE HALO2_PROJECT_DIR="."
        HALO2_RUNTIME_DIR="build" HALO2_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}")
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        set(slice "${CMAKE_BINARY_DIR}/slices/${target}/${arch}/${name}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/prepare_slice.py"
                "$<TARGET_FILE:${target}>" "${slice}" "${arch}"
            VERBATIM)
        add_test(NAME macos_launcher_first_run_${target}_${arch}
            COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tests/launcher_first_run_test.py" "${slice}")
        set_tests_properties(macos_launcher_first_run_${target}_${arch} PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE)
    endforeach()
endforeach()
add_executable(macos_executable_tests executable_tests.cpp)
target_include_directories(macos_executable_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(macos_executable_tests PRIVATE nlohmann_json::nlohmann_json)
add_executable(macos_launcher_runtime_recorder runtime_recorder.cpp)
target_link_libraries(macos_launcher_runtime_recorder PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME macos_launcher_executable_paths COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/executable_test.py" "$<TARGET_FILE:macos_executable_tests>"
    "${CMAKE_BINARY_DIR}/launcher-paths.json" ${CMAKE_OSX_ARCHITECTURES})
add_test(NAME macos_launcher_bundle_storage COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/storage_test.py" "$<TARGET_FILE:halo2_launcher>"
    "$<TARGET_FILE:macos_launcher_runtime_recorder>" "${HALO2_ROOT}" "${HALO2_SDL_LICENSE}" "${CMAKE_BINARY_DIR}/launcher-storage.json"
    ${CMAKE_OSX_ARCHITECTURES})
set_tests_properties(macos_launcher_bundle_storage PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
add_test(NAME installation_resource_ownership COMMAND ${Python3_EXECUTABLE} -B
    "${HALO2_ROOT}/tests/test_installation_paths.py")
add_custom_target(macos_launcher_tests DEPENDS ${launchers} macos_executable_tests macos_launcher_runtime_recorder)
