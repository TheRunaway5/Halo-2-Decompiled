if(TARGET halo2_systems)
    target_sources(halo2_systems PRIVATE src/rendering/world/native_tu15_draw_units.cpp
        src/rendering/world/native_tu15_draw_collect.cpp src/rendering/world/native_tu15_sphere_rectangle.cpp
        src/rendering/models/native_tu15_light_parent_biped.cpp)
    if(BUILD_TESTING)
        add_executable(source_native_draw_units_tests tests/source_native_draw_units_tests.cpp)
        target_link_libraries(source_native_draw_units_tests PRIVATE halo2_systems)
        add_test(NAME native_bank_draw_units_world_tu15 COMMAND source_native_draw_units_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
        set_tests_properties(native_bank_draw_units_world_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
        add_executable(native_tu15_model_draw_bridge_tests tests/native_tu15_model_draw_bridge_tests.cpp)
        target_link_libraries(native_tu15_model_draw_bridge_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
            add_executable(native_tu15_model_draw_child_world_tests tests/native_tu15_model_draw_child_world_tests.cpp)
            target_link_libraries(native_tu15_model_draw_child_world_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
            add_test(NAME native_tu15_model_draw_real_child COMMAND native_tu15_model_draw_child_world_tests
                "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
                "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-draw-child-world.json")
            set_tests_properties(native_tu15_model_draw_real_child PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
            add_test(NAME original_tu15_model_draw_bridge COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_draw_bridge.py"
                --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
                --map "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map"
                --native $<TARGET_FILE:native_tu15_model_draw_bridge_tests>
                --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-draw-bridge.json")
            set_tests_properties(original_tu15_model_draw_bridge PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
        endif()
    endif()
endif()
if(BUILD_TESTING)
    add_executable(native_tu15_draw_units_tests tests/native_tu15_draw_units_tests.cpp
        src/rendering/world/native_tu15_draw_units.cpp
        src/rendering/world/native_tu15_draw_collect.cpp
        src/rendering/world/native_tu15_sphere_rectangle.cpp
        src/rendering/models/native_tu15_light_parent_biped.cpp
        src/rendering/world/native_tu15_render_lighting.cpp
        src/rendering/world/native_tu15_render_prepare.cpp
        src/rendering/source_geometry_resource_request.cpp src/rendering/source_geometry_resource.cpp)
    target_include_directories(native_tu15_draw_units_tests PRIVATE include)
    target_compile_features(native_tu15_draw_units_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_draw_units_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_draw_units_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_draw_units_tests PRIVATE -fsanitize=address,undefined)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        if(NOT HALO2_DRAW_UNITS_ORACLE_SCRIPT)
            set(HALO2_DRAW_UNITS_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_draw_units.py")
        endif()
        add_test(NAME original_tu15_native_draw_units COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${HALO2_DRAW_UNITS_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --native $<TARGET_FILE:native_tu15_draw_units_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-draw-units.json")
        set_tests_properties(original_tu15_native_draw_units PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    endif()
endif()
