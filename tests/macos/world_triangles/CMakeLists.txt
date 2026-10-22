cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2WorldTriangleTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(nlohmann_json REQUIRED CONFIG)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
include("${HALO2_ROOT}/cmake/Glm.cmake")
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include(CTest)
if(APPLE)
    include("${HALO2_ROOT}/cmake/macos/Platform.cmake")
    find_package(SDL2 REQUIRED CONFIG)
else()
    find_package(Vulkan REQUIRED)
endif()
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
find_program(SPIRV_VALIDATOR spirv-val REQUIRED)
file(GLOB menu_sources CONFIGURE_DEPENDS "${HALO2_ROOT}/shaders/native-programs/menu/*.vert" "${HALO2_ROOT}/shaders/pstencil/native-programs/*.vert")
set(catalog "${CMAKE_BINARY_DIR}/catalog")
add_custom_command(OUTPUT "${catalog}/compiled.json" "${catalog}/catalog.hpp"
    COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/compile_menu_world_vertex_catalog.py"
        --sources "${HALO2_ROOT}/shaders/native-programs/menu/sources.json"
        --additional-sources "${HALO2_ROOT}/shaders/pstencil/native-programs/sources.json"
        --triangle-vertices --compiler "${GLSLANG_VALIDATOR}" --validator "${SPIRV_VALIDATOR}"
        --header "${catalog}/catalog.hpp" --manifest "${catalog}/compiled.json"
    DEPENDS ${menu_sources} "${HALO2_ROOT}/tools/compile_menu_world_vertex_catalog.py"
        "${HALO2_ROOT}/tools/world_triangle_vertex.py" "${HALO2_ROOT}/shaders/source_world_vertex_fetch.glsl"
        "${HALO2_ROOT}/shaders/world_depth_geometry.glsl" VERBATIM)
add_custom_target(world_triangle_catalog DEPENDS "${catalog}/compiled.json")
set(shader_dir "${CMAKE_BINARY_DIR}/shaders")
add_custom_command(OUTPUT "${shader_dir}/manifest.json"
    COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/build_shaders.py"
        --compiler "${GLSLANG_VALIDATOR}" --validator "${SPIRV_VALIDATOR}" --output "${shader_dir}"
    DEPENDS ${menu_sources} build_shaders.py assemble.comp "${HALO2_ROOT}/tools/world_triangle_vertex.py"
        "${HALO2_ROOT}/shaders/source_world_vertex_fetch.glsl" "${HALO2_ROOT}/shaders/world_depth_geometry.glsl"
        "${HALO2_ROOT}/tests/menu_world_vertex_catalog_capture.geom" VERBATIM)
add_custom_target(world_triangle_capture_shaders DEPENDS "${shader_dir}/manifest.json")
# Compile actual production owners universally without dragging in deferred
# source CPU execution. GPU tests below run the generated shader arithmetic.
add_library(world_triangle_owners OBJECT
    "${HALO2_ROOT}/src/ui/menu_world_native_draw.cpp" "${HALO2_ROOT}/src/ui/menu_world_source_storage.cpp"
    "${HALO2_ROOT}/src/ui/menu_world_triangle_storage.cpp" "${HALO2_ROOT}/src/ui/menu_world_vertex_catalog.cpp")
target_include_directories(world_triangle_owners PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(world_triangle_owners PRIVATE Vulkan::Vulkan halo2_glm OpenSSL::Crypto)
if(APPLE)
    target_link_libraries(world_triangle_owners PRIVATE SDL2::SDL2)
endif()
target_compile_definitions(world_triangle_owners PRIVATE HALO2_SHADER_DIR="${shader_dir}")
add_executable(world_triangle_tests world_triangle_tests.cpp)
target_link_libraries(world_triangle_tests PRIVATE Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
target_include_directories(world_triangle_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_compile_definitions(world_triangle_tests PRIVATE DEPTH_SHADER_DIR="${shader_dir}")
add_dependencies(world_triangle_tests world_triangle_catalog world_triangle_capture_shaders world_triangle_owners)
add_executable(vulkan_portability_tests "${HALO2_ROOT}/tests/vulkan_portability_tests.cpp")
target_include_directories(vulkan_portability_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(vulkan_portability_tests PRIVATE Vulkan::Vulkan)
add_test(NAME vulkan_portability COMMAND vulkan_portability_tests)
set(HALO2_WORLD_TRIANGLE_ORACLE "" CACHE PATH "Owned original VS+GS outputs for Mac differential validation")
if(APPLE)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME world_triangle_${arch} COMMAND /usr/bin/arch -${arch}
            $<TARGET_FILE:world_triangle_tests> "${CMAKE_BINARY_DIR}/world-triangle-${arch}.json" "${HALO2_WORLD_TRIANGLE_ORACLE}")
        set_tests_properties(world_triangle_${arch} PROPERTIES RUN_SERIAL TRUE TIMEOUT 240)
    endforeach()
else()
    if(HALO2_WORLD_TRIANGLE_ORACLE)
        set(oracle_args "${HALO2_WORLD_TRIANGLE_ORACLE}")
    endif()
    add_test(NAME world_triangle_host COMMAND world_triangle_tests "${CMAKE_BINARY_DIR}/world-triangle-host.json" ${oracle_args})
    set_tests_properties(world_triangle_host PROPERTIES RUN_SERIAL TRUE TIMEOUT 240)
endif()
