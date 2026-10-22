cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSSDLTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(nlohmann_json REQUIRED CONFIG)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include("${HALO2_ROOT}/cmake/macos/Platform.cmake")
include("${HALO2_ROOT}/cmake/macos/SDL.cmake")
include(CTest)
halo2_macos_sdl_dependency()
add_executable(macos_sdl_dependency_tests MACOSX_BUNDLE dependency_tests.cpp)
target_link_libraries(macos_sdl_dependency_tests PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
add_executable(halo2_macos_graphics_probe MACOSX_BUNDLE "${HALO2_ROOT}/tools/macos/graphics_probe.cpp")
target_include_directories(halo2_macos_graphics_probe PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(halo2_macos_graphics_probe PRIVATE SDL2::SDL2 Vulkan::Vulkan)
set(arch_args)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    list(APPEND arch_args --arch "${arch}")
endforeach()
foreach(target macos_sdl_dependency_tests halo2_macos_graphics_probe)
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "net.halo2forever.${target}")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/macos/bundle.py"
            --app "$<TARGET_BUNDLE_DIR:${target}>" ${arch_args}
            --library "${HALO2_MOLTENVK_LIBRARY}"
            --license "SDL2=${HALO2_SDL_LICENSE}" --license "MoltenVK=${HALO2_MOLTENVK_LICENSE}"
        VERBATIM)
endforeach()
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    add_test(NAME macos_sdl_${arch} COMMAND /usr/bin/arch -${arch}
        $<TARGET_FILE:macos_sdl_dependency_tests> "$<TARGET_BUNDLE_CONTENT_DIR:macos_sdl_dependency_tests>/Frameworks")
    set_tests_properties(macos_sdl_${arch} PROPERTIES RUN_SERIAL TRUE TIMEOUT 30)
endforeach()
add_test(NAME macos_sdl_relocation COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/relocation_test.py"
    "$<TARGET_BUNDLE_DIR:macos_sdl_dependency_tests>" "$<TARGET_BUNDLE_DIR:halo2_macos_graphics_probe>"
    "${CMAKE_BINARY_DIR}/macos-dependencies/sdl" "${CMAKE_BINARY_DIR}/sdl-relocation.json" ${CMAKE_OSX_ARCHITECTURES})
set_tests_properties(macos_sdl_relocation PROPERTIES RUN_SERIAL TRUE TIMEOUT 120)
add_executable(vulkan_portability_tests "${HALO2_ROOT}/tests/vulkan_portability_tests.cpp")
target_include_directories(vulkan_portability_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src" "${Vulkan_INCLUDE_DIR}")
add_test(NAME vulkan_portability COMMAND vulkan_portability_tests)
add_custom_target(macos_sdl_tests DEPENDS macos_sdl_dependency_tests halo2_macos_graphics_probe)
