if(BUILD_TESTING)
    add_executable(native_tu15_command_emit_tests tests/native_tu15_command_emit_tests.cpp
        src/rendering/world/native_tu15_command_emit.cpp src/rendering/world/native_tu15_shader_lod.cpp
        src/rendering/world/native_tu15_command_walk.cpp src/rendering/world/native_tu15_command_part_prepare.cpp
        src/rendering/world/native_tu15_material_selection.cpp
        src/rendering/source_opaque_sort.cpp)
    target_include_directories(native_tu15_command_emit_tests PRIVATE include)
    target_compile_features(native_tu15_command_emit_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_command_emit_tests PRIVATE nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_command_emit_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_command_emit_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_command_emit_tests PRIVATE -fsanitize=address,undefined)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        add_test(NAME original_tu15_command_emit COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_command_emit.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_command_emit_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-command-emit.json")
        set_tests_properties(original_tu15_command_emit PROPERTIES TIMEOUT 900)
        add_test(NAME original_tu15_command_group COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_command_group.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_command_emit_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-command-group.json")
        set_tests_properties(original_tu15_command_group PROPERTIES TIMEOUT 900)
        add_test(NAME original_tu15_section_queue COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_command_group.py" --section-queue
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_command_emit_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-section-queue.json")
        set_tests_properties(original_tu15_section_queue PROPERTIES TIMEOUT 900)
        add_test(NAME original_tu15_camouflage_queue COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_command_group.py" --camouflage-queue
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_command_emit_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camouflage-queue.json")
        set_tests_properties(original_tu15_camouflage_queue PROPERTIES TIMEOUT 900)
        add_test(NAME original_tu15_material_selection COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_material_selection.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_command_emit_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-material-selection.json")
        set_tests_properties(original_tu15_material_selection PROPERTIES TIMEOUT 900)
        add_test(NAME original_tu15_descriptor_sort COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_descriptor_sort.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_command_emit_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-descriptor-sort.json")
        set_tests_properties(original_tu15_descriptor_sort PROPERTIES TIMEOUT 900)
    endif()
endif()
