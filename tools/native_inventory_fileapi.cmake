# Private configure-only evidence hook, supplied through
# CMAKE_PROJECT_TOP_LEVEL_INCLUDES by tools/run_tests.py. Normal project CMake
# and game targets remain unchanged; this target prepares sources, not objects.
if(NOT COMMAND cmake_file_api)
    message(FATAL_ERROR "Native production inventory requires CMake3.27+ File API queries")
endif()
cmake_file_api(QUERY API_VERSION 1 CODEMODEL 2 CACHE 2 TOOLCHAINS 1 CMAKEFILES 1)
if(NOT HALO2_NATIVE_INVENTORY_PYTHON OR NOT HALO2_NATIVE_INVENTORY_TOOL)
    message(FATAL_ERROR "Supply the owned inventory Python and exporter paths")
endif()
add_custom_target(halo2_native_inventory_closure
    COMMAND "${HALO2_NATIVE_INVENTORY_PYTHON}" "${HALO2_NATIVE_INVENTORY_TOOL}"
        --source-root "${CMAKE_SOURCE_DIR}" --build "${CMAKE_BINARY_DIR}"
        --output "${CMAKE_BINARY_DIR}/native-production-closure.json"
        --configuration "$<CONFIG>" --prepare-aliases
        --target-system "${CMAKE_SYSTEM_NAME}"
        --target-processor "${CMAKE_SYSTEM_PROCESSOR}"
        --collection-context "${HALO2_NATIVE_INVENTORY_COLLECTION_CONTEXT}"
    VERBATIM)
