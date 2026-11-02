if(TARGET halo2_systems)
    target_sources(halo2_systems PRIVATE src/rendering/effects/native_tu15_light_receiver_flags.cpp
        src/rendering/effects/effect_light_receiver_flags.cpp)
    if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        add_executable(native_tu15_light_receiver_flags_world_tests tests/native_tu15_light_receiver_flags_world_tests.cpp)
        target_link_libraries(native_tu15_light_receiver_flags_world_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
        add_test(NAME original_tu15_light_receiver_flags_world COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_light_receiver_flags_world.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --map "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map"
            --native $<TARGET_FILE:native_tu15_light_receiver_flags_world_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-light-receiver-flags-world.json")
        set_tests_properties(original_tu15_light_receiver_flags_world PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
        add_executable(native_tu15_light_material_externs_tests tests/native_tu15_light_material_externs_tests.cpp)
        target_link_libraries(native_tu15_light_material_externs_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
        set(HALO2_LIGHT_FLAGS_MCC_MAPS "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-zanzibar-tu15-20261101-v9/halo2/h2_maps_win64_dx11" CACHE PATH "Prepared native shader content qualification")
        add_test(NAME native_tu15_light_material_externs COMMAND native_tu15_light_material_externs_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_LIGHT_FLAGS_MCC_MAPS}"
            "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-light-material-externs.json")
        set_tests_properties(native_tu15_light_material_externs PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800)
    endif()
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_light_receiver_flags_tests tests/native_tu15_light_receiver_flags_tests.cpp
        src/rendering/effects/native_tu15_light_receiver_flags.cpp src/rendering/effects/effect_light_receiver_flags.cpp)
    target_include_directories(native_tu15_light_receiver_flags_tests PRIVATE include)
    target_compile_features(native_tu15_light_receiver_flags_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_light_receiver_flags_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_light_receiver_flags_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_light_receiver_flags_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_light_receiver_flags COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_light_receiver_flags.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_light_receiver_flags_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-light-receiver-flags.json")
    set_tests_properties(original_tu15_light_receiver_flags PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
endif()

# Whole BSP receiver prefix uses its original-input generator in test scratch.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    if(TARGET effect_light_receiver_tests)
        add_test(NAME original_large_sphere_receiver_bsp_prefix COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle_effect_light_large_receiver.py"
            --native $<TARGET_FILE:effect_light_receiver_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/large-sphere-receiver-bsp-prefix.json")
        set_tests_properties(original_large_sphere_receiver_bsp_prefix PROPERTIES TIMEOUT 600)
    endif()
endif()
