if(TARGET halo2_systems)
    target_sources(halo2_systems PRIVATE src/rendering/world/native_tu15_command_walk.cpp
        src/rendering/world/native_tu15_command_part_prepare.cpp
        src/rendering/world/native_tu15_command_emit.cpp
        src/rendering/world/native_tu15_material_selection.cpp
        src/rendering/world/native_tu15_shader_lod.cpp
        src/rendering/world/source_native_command_walk_tu15.cpp)
    set_source_files_properties(src/rendering/world/native_tu15_command_part_prepare.cpp
        src/rendering/world/native_tu15_command_emit.cpp src/rendering/world/native_tu15_shader_lod.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    if(BUILD_TESTING)
        add_executable(source_native_command_walk_tests tests/source_native_command_walk_tests.cpp)
        target_link_libraries(source_native_command_walk_tests PRIVATE halo2_systems)
        add_test(NAME native_command_walk_world_tu15 COMMAND source_native_command_walk_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
        set_tests_properties(native_command_walk_world_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endif()
endif()
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15CommandPartPrepare.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15CommandEmit.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15ShaderLod.cmake")
if(BUILD_TESTING)
    add_executable(native_tu15_command_walk_tests tests/native_tu15_command_walk_tests.cpp
        src/rendering/world/native_tu15_command_walk.cpp)
    target_include_directories(native_tu15_command_walk_tests PRIVATE include)
    target_compile_features(native_tu15_command_walk_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_command_walk_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_command_walk_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_command_walk_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_command_walk_controls COMMAND native_tu15_command_walk_tests)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        if(NOT HALO2_COMMAND_WALK_ORACLE_SCRIPT)
            set(HALO2_COMMAND_WALK_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_command_walk.py")
        endif()
        add_test(NAME original_tu15_native_command_walk COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${HALO2_COMMAND_WALK_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --native $<TARGET_FILE:native_tu15_command_walk_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-command-walk.json")
        set_tests_properties(original_tu15_native_command_walk PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    endif()
endif()
