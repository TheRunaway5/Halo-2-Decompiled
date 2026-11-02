# Explicit visible acceptance runner. Missing live features deliberately return
# nonzero; this is not an automatically passing arithmetic/component fixture.
add_executable(physics_multiplayer_acceptance EXCLUDE_FROM_ALL
    tests/physics_multiplayer_acceptance.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(physics_multiplayer_acceptance PRIVATE
    halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
target_compile_definitions(physics_multiplayer_acceptance PRIVATE
    HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
add_dependencies(physics_multiplayer_acceptance halo2_shaders)
if(UNIX AND NOT APPLE)
    find_package(X11 QUIET)
    if(X11_FOUND)
        target_link_libraries(physics_multiplayer_acceptance PRIVATE X11::X11)
        target_compile_definitions(physics_multiplayer_acceptance PRIVATE HALO2_ACCEPTANCE_X11_FOCUS=1)
    endif()
endif()


add_executable(submitted_vehicle_projection_tu15_tests tests/submitted_vehicle_projection_tu15_tests.cpp)
target_link_libraries(submitted_vehicle_projection_tu15_tests PRIVATE halo2_cache)
add_test(NAME submitted_vehicle_projection_tu15 COMMAND submitted_vehicle_projection_tu15_tests)
add_executable(mcc_vehicle_hud_residency_tests tests/mcc_vehicle_hud_residency_tests.cpp)
target_link_libraries(mcc_vehicle_hud_residency_tests PRIVATE halo2_cache)
add_test(NAME mcc_vehicle_hud_residency COMMAND mcc_vehicle_hud_residency_tests
    "${HALO2_MCC_INTRO_LUMINOUS_MAP}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_vehicle_hud_residency.json")
set_tests_properties(mcc_vehicle_hud_residency PROPERTIES SKIP_RETURN_CODE 77)
add_executable(campaign_multiplayer_acceptance_tu15 EXCLUDE_FROM_ALL
    tests/campaign_multiplayer_acceptance_tu15.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_include_directories(campaign_multiplayer_acceptance_tu15 PRIVATE tests)
target_link_libraries(campaign_multiplayer_acceptance_tu15 PRIVATE
    halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
target_compile_definitions(campaign_multiplayer_acceptance_tu15 PRIVATE
    HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
add_dependencies(campaign_multiplayer_acceptance_tu15 halo2_shaders)

add_executable(campaign_cinematic_material_tu15_tests EXCLUDE_FROM_ALL
    tests/campaign_cinematic_material_tu15_tests.cpp)
target_link_libraries(campaign_cinematic_material_tu15_tests PRIVATE halo2_cache)

add_executable(campaign_water_material_tu15_tests EXCLUDE_FROM_ALL
    tests/campaign_water_material_tu15_tests.cpp)
target_link_libraries(campaign_water_material_tu15_tests PRIVATE halo2_cache)

add_executable(campaign_scenery_regions_tu15_tests EXCLUDE_FROM_ALL
    tests/campaign_scenery_regions_tu15_tests.cpp)
target_link_libraries(campaign_scenery_regions_tu15_tests PRIVATE halo2_renderer)

# Late native effects must have real material resources before GPU upload.
# Reuse the actual acceptance runtime configuration, including late shader flags.
add_executable(native_late_effect_light_tu15_tests EXCLUDE_FROM_ALL
    tests/native_late_effect_light_tu15_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(native_late_effect_light_tu15_tests PRIVATE
    $<TARGET_PROPERTY:physics_multiplayer_acceptance,LINK_LIBRARIES>)
target_compile_definitions(native_late_effect_light_tu15_tests PRIVATE
    $<TARGET_PROPERTY:physics_multiplayer_acceptance,COMPILE_DEFINITIONS>)
add_dependencies(native_late_effect_light_tu15_tests halo2_shaders)
if(UNIX AND NOT APPLE AND EXISTS "${HALO2_MCC_WATER_MAP_DIRECTORY}/zanzibar.map")
    find_program(HALO2_LATE_LIGHT_XVFB xvfb-run)
    if(HALO2_LATE_LIGHT_XVFB)
        add_test(NAME native_late_effect_light_creation
            COMMAND "${HALO2_LATE_LIGHT_XVFB}" -a -s "-screen 0 1280x720x24 -nolisten tcp"
                $<TARGET_FILE:native_late_effect_light_tu15_tests>
                "${HALO2_MCC_WATER_MAP_DIRECTORY}/zanzibar.map" "${HALO2_MCC_WATER_MAP_DIRECTORY}"
                "@HALO2_TEST_SCRATCH@/late-light"
                "${CMAKE_CURRENT_BINARY_DIR}/native-late-effect-light-creation.json")
        set_tests_properties(native_late_effect_light_creation PROPERTIES TIMEOUT 240
            ENVIRONMENT "SDL_VIDEODRIVER=x11;HALO2_PROBE_FRAMES=120;HALO2_PROBE_LIGHT_VEHICLE_CHOICE=2;XDG_DATA_HOME=@HALO2_TEST_SCRATCH@/data;XDG_CONFIG_HOME=@HALO2_TEST_SCRATCH@/config;XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/pipelines;MESA_SHADER_CACHE_DIR=@HALO2_TEST_SCRATCH@/mesa;__GL_SHADER_DISK_CACHE_PATH=@HALO2_TEST_SCRATCH@/nvidia")
    endif()
endif()
