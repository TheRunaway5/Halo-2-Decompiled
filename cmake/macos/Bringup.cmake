add_executable(halo2_macos_graphics_probe tools/macos/graphics_probe.cpp)
target_include_directories(halo2_macos_graphics_probe PRIVATE include)
target_link_libraries(halo2_macos_graphics_probe PRIVATE SDL2::SDL2 Vulkan::Vulkan)
halo2_macos_bundle(halo2_macos_graphics_probe "Halo 2 Graphics Check")
include(CTest)
if(BUILD_TESTING)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME macos_moltenvk_${arch} COMMAND ${Python3_EXECUTABLE} -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/macos/graphics_test.py"
            $<TARGET_FILE:halo2_macos_graphics_probe> ${arch}
            "${CMAKE_CURRENT_BINARY_DIR}/macos-${arch}.json")
    endforeach()
    add_executable(vulkan_portability_tests tests/vulkan_portability_tests.cpp)
    target_include_directories(vulkan_portability_tests PRIVATE include "${Vulkan_INCLUDE_DIR}")
    add_test(NAME vulkan_portability COMMAND vulkan_portability_tests)
    add_test(NAME macos_bundle COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/macos/bundle_tests.py")
endif()
