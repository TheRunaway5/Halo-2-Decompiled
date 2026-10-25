# Native admission is qualified on the existing Linux x86 SSE/x87 backend.
# Other compilers/platforms retain the literal production fallback.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$" AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64|i[3-6]86)$")
    add_executable(source_shadow_native_prefix_tu15_tests
        tests/source_shadow_native_prefix_tu15_tests.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(source_shadow_native_prefix_tu15_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(source_shadow_native_prefix_tu15_tests PRIVATE
        halo2_physics_world_runtime halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(source_shadow_native_prefix_tu15_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shadow_native_prefix_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    foreach(prefix IN ITEMS 2e66b2 2e6810 2e6a50)
        string(TOUPPER "${prefix}" prefix_upper)
        set(suffix "_${prefix}")
        set(prefix_fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_shadow_plane_prefix_${prefix}_tu15.json")
        set(other_prefix 2E6810)
        if(prefix STREQUAL "2e6810")
            set(suffix "")
            set(prefix_fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_shadow_plane_prefix_tu15.json")
            set(other_prefix 2E66B2)
        endif()
        add_test(NAME source_shadow_native_prefix_tu15${suffix}
            COMMAND source_shadow_native_prefix_tu15_tests "${prefix_fixture}" --require-native)
        add_test(NAME source_shadow_native_prefix_reference${suffix}
            COMMAND "${CMAKE_COMMAND}" -E env "HALO2_REFERENCE_SHADOW_${prefix_upper}_PREFIX=1"
                $<TARGET_FILE:source_shadow_native_prefix_tu15_tests> "${prefix_fixture}" --quick --expect-reference)
        add_test(NAME source_shadow_native_prefix_global_reference${suffix}
            COMMAND "${CMAKE_COMMAND}" -E env HALO2_REFERENCE_SHADOW_NATIVE_PREFIXES=1
                $<TARGET_FILE:source_shadow_native_prefix_tu15_tests> "${prefix_fixture}" --quick --expect-reference)
        add_test(NAME source_shadow_native_prefix_reference_red${suffix}
            COMMAND "${Python3_EXECUTABLE}" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_shadow_native_prefix_reference_control.py"
                $<TARGET_FILE:source_shadow_native_prefix_tu15_tests> "${prefix_fixture}")
        add_test(NAME source_shadow_native_prefix_specific_reference_red${suffix}
            COMMAND "${Python3_EXECUTABLE}" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_shadow_native_prefix_reference_control.py"
                $<TARGET_FILE:source_shadow_native_prefix_tu15_tests> "${prefix_fixture}"
                "HALO2_REFERENCE_SHADOW_${prefix_upper}_PREFIX")
        add_test(NAME source_shadow_native_prefix_other_reference${suffix}
            COMMAND "${CMAKE_COMMAND}" -E env "HALO2_REFERENCE_SHADOW_${other_prefix}_PREFIX=1"
                $<TARGET_FILE:source_shadow_native_prefix_tu15_tests> "${prefix_fixture}" --quick --require-native)
        foreach(reference IN ITEMS SMALL_MEMORY_TRANSFERS SCALAR_VECTOR_READS LITERAL_INSTRUCTIONS INSTRUCTION_CALLBACKS)
            string(TOLOWER "${reference}" reference_name)
            add_test(NAME source_shadow_native_prefix_${reference_name}${suffix}
                COMMAND "${CMAKE_COMMAND}" -E env "HALO2_REFERENCE_${reference}=1"
                    $<TARGET_FILE:source_shadow_native_prefix_tu15_tests> "${prefix_fixture}" --quick --expect-reference)
        endforeach()
    endforeach()
    add_executable(source_shadow_native_query_tu15_tests
        tests/source_shadow_native_query_tu15_tests.cpp src/runtime/source_runtime_memory.cpp)
    target_link_libraries(source_shadow_native_query_tu15_tests PRIVATE
        halo2_physics_world_runtime halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_options(source_shadow_native_query_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    set(query_fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_shadow_native_query_tu15_original.json")
    add_test(NAME source_shadow_native_query_differential_tu15
        COMMAND source_shadow_native_query_tu15_tests "${query_fixture}" --require-native)
    add_test(NAME source_shadow_native_query_differential_reference
        COMMAND "${CMAKE_COMMAND}" -E env HALO2_REFERENCE_SHADOW_NATIVE_PREFIXES=1
            $<TARGET_FILE:source_shadow_native_query_tu15_tests> "${query_fixture}" --expect-reference)
endif()
