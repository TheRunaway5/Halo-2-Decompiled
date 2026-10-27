# Source-owned fragment definitions, compiled independently of runtime assets.
file(GLOB NATIVE_FRAGMENT_REGISTRIES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/native-programs/fragments/*.json)
set(NATIVE_FRAGMENT_CATALOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/native-fragment-catalog)
set(NATIVE_FRAGMENT_HEADER ${NATIVE_FRAGMENT_CATALOG_DIR}/native_fragment_kernels.generated.hpp)
set(native_fragment_registry_args)
foreach(registry IN LISTS NATIVE_FRAGMENT_REGISTRIES)
    list(APPEND native_fragment_registry_args --registry ${registry})
endforeach()
execute_process(COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/compile_native_fragment_kernels.py
    ${native_fragment_registry_args} --shader-root ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    --shader-output-dir ${CMAKE_CURRENT_BINARY_DIR}/shaders --list-outputs
    RESULT_VARIABLE native_fragment_status OUTPUT_VARIABLE NATIVE_FRAGMENT_SPIRV
    ERROR_VARIABLE native_fragment_error OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT native_fragment_status EQUAL 0)
    message(FATAL_ERROR "Native fragment registry is invalid: ${native_fragment_error}")
endif()
add_custom_command(OUTPUT ${NATIVE_FRAGMENT_HEADER}
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/compile_native_fragment_kernels.py
        ${native_fragment_registry_args} --shader-root ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        --shader-output-dir ${CMAKE_CURRENT_BINARY_DIR}/shaders
        --header ${NATIVE_FRAGMENT_HEADER} --header-only
    DEPENDS ${NATIVE_FRAGMENT_REGISTRIES} tools/compile_native_fragment_kernels.py
        tools/native_world_fragment_kernel.py tools/native_combiner_contract.py tools/compile_native_world_kernels.py
        tools/audit_native_renderer_kernels.py VERBATIM)
add_custom_target(native_fragment_catalog DEPENDS ${NATIVE_FRAGMENT_HEADER})
file(GLOB_RECURSE native_fragment_sources CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glsl)
list(FILTER native_fragment_sources EXCLUDE REGEX "/shaders/path_tracing/")
add_custom_command(OUTPUT ${NATIVE_FRAGMENT_CATALOG_DIR}/compiled.json ${NATIVE_FRAGMENT_SPIRV}
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/compile_native_fragment_kernels.py
        ${native_fragment_registry_args} --shader-root ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        --shader-output-dir ${CMAKE_CURRENT_BINARY_DIR}/shaders
        --header ${NATIVE_FRAGMENT_HEADER} --manifest ${NATIVE_FRAGMENT_CATALOG_DIR}/compiled.json
        --compiler ${GLSLANG_VALIDATOR} --optimizer ${SPIRV_OPTIMIZER} --validator ${SPIRV_VALIDATOR}
    DEPENDS ${NATIVE_FRAGMENT_HEADER} ${NATIVE_FRAGMENT_REGISTRIES} ${native_fragment_sources}
        tools/compile_native_fragment_kernels.py tools/native_world_fragment_kernel.py tools/native_combiner_contract.py
        tools/compile_native_world_kernels.py tools/audit_native_renderer_kernels.py
        ${GLSLANG_VALIDATOR} ${SPIRV_OPTIMIZER} ${SPIRV_VALIDATOR} VERBATIM)
add_custom_target(native_fragment_kernels DEPENDS ${NATIVE_FRAGMENT_CATALOG_DIR}/compiled.json ${NATIVE_FRAGMENT_SPIRV})
add_dependencies(native_fragment_kernels native_fragment_catalog)
target_sources(halo2_cache PRIVATE src/rendering/native_fragment_kernel.cpp)
target_include_directories(halo2_cache PRIVATE ${NATIVE_FRAGMENT_CATALOG_DIR})
add_dependencies(halo2_cache native_fragment_catalog)
add_dependencies(halo2_shaders native_fragment_kernels)
