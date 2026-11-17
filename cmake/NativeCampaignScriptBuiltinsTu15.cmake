# Native semantic TU1.5 campaign HS builtins (no original-code execution).
include("${CMAKE_CURRENT_LIST_DIR}/NativeObjectFactoryTu15.cmake")
# The kernels file keeps strict IEEE ordering: each operation mirrors one
# original MULSS/x87 step under the live world's rounding controls.
set(_halo2_script_builtin_kernels src/campaign/scripting/native_campaign_script_builtins_tu15.cpp
    src/campaign/scripting/native_campaign_script_presentation_tu15.cpp
    src/campaign/scripting/native_campaign_script_ai_tu15.cpp
    ${HALO2_NATIVE_OBJECT_FACTORY_SOURCES})
target_sources(halo2_systems PRIVATE ${_halo2_script_builtin_kernels}
    src/campaign/scripting/native_campaign_script_host_tu15.cpp
    src/campaign/scripting/native_campaign_script_world_tu15.cpp)
target_link_libraries(halo2_systems PRIVATE halo2_native_tu15_data_array_delete)
if(MSVC)
    set_source_files_properties(${_halo2_script_builtin_kernels} PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(${_halo2_script_builtin_kernels} PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    # The unit action support native (1EFA70 history, used by the unitactions
    # track) is already a halo2_systems source (CampaignNativeUnitActionSupport),
    # so it joins only this harness, not _halo2_script_builtin_kernels.
    add_executable(native_campaign_script_builtins_tu15_tests tests/native_campaign_script_builtins_tu15_tests.cpp
        ${_halo2_script_builtin_kernels} src/rendering/effects/native_tu15_lens_query_slots.cpp
        src/rendering/native_tu15_object_scale.cpp src/campaign/ai/native_campaign_unit_action_support_tu15.cpp
        # Track havok's 318360 binds 3174A0 (a halo2_systems source).
        src/physics/native_tu15_body_references.cpp
        ${HALO2_NATIVE_OBJECT_FACTORY_TEST_SOURCES})
    target_include_directories(native_campaign_script_builtins_tu15_tests PRIVATE include src tests)
    target_compile_features(native_campaign_script_builtins_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(native_campaign_script_builtins_tu15_tests PRIVATE nlohmann_json::nlohmann_json
        halo2_native_object_velocity halo2_native_tu15_data_array_delete halo2_native_pair_consumer)
    # The production host references every factory track. Check its actual
    # ordinal/scalar/writer wiring in complete builds; isolated track builds
    # retain their existing leaf-only harness.
    if(NOT HALO2_NATIVE_OBJECT_FACTORY_TRACKS)
        target_sources(native_campaign_script_builtins_tu15_tests PRIVATE
            src/campaign/scripting/native_campaign_script_host_tu15.cpp src/core/kernels.cpp)
        target_compile_definitions(native_campaign_script_builtins_tu15_tests PRIVATE HALO2_NATIVE_SCRIPT_HOST_TESTS=1)
    endif()
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_script_builtins_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_script_builtins_tu15_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_campaign_script_builtins_tu15 COMMAND native_campaign_script_builtins_tu15_tests --self-test)
    set_tests_properties(native_campaign_script_builtins_tu15 PROPERTIES TIMEOUT 60)
    # Physical unchanged-i386 evaluator tails; skips (77) without the owned
    # TU1.5 executable. Requires the oracle Python with capstone.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86" AND HALO2_RENDERER_ORACLE_PYTHON)
        add_test(NAME native_campaign_script_streaming COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_campaign_script_streaming.py")
        set_tests_properties(native_campaign_script_streaming PROPERTIES TIMEOUT 30)
        add_test(NAME original_tu15_native_campaign_script_builtins COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_campaign_script_builtins_tu15.py"
            --native $<TARGET_FILE:native_campaign_script_builtins_tu15_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-campaign-script-builtins-tu15.json")
        # Streams ~141k cases in 2000-case batches (bounded memory); the whole
        # run took ~55 min on a loaded 16-core host, so allow 3 hours.
        set_tests_properties(original_tu15_native_campaign_script_builtins PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 10800)
    endif()
endif()
