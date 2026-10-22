if(BUILD_TESTING)
    add_executable(weapon_scope_parity_tests tests/weapon_scope_parity_tests.cpp
        src/input/pc_settings.cpp src/gameplay/camera/source_camera.cpp
        src/gameplay/units/unit_zoom.cpp src/gameplay/units/unit_scope_camera.cpp)
    target_include_directories(weapon_scope_parity_tests PRIVATE include)
    target_link_libraries(weapon_scope_parity_tests PRIVATE SDL2::SDL2 halo2_glm nlohmann_json::nlohmann_json)
    if(NOT MSVC)
        target_compile_options(weapon_scope_parity_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME original_weapon_scope_parity COMMAND weapon_scope_parity_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_weapon_optics.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/scope-camera/audit-frozen/constants.json
        ${CMAKE_CURRENT_BINARY_DIR}/weapon_scope_parity.json)
endif()
