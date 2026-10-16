# Explicit visible acceptance runner. Missing live features deliberately return
# nonzero; this is not an automatically passing arithmetic/component fixture.
add_executable(physics_multiplayer_acceptance EXCLUDE_FROM_ALL
    tests/physics_multiplayer_acceptance.cpp src/multiplayer_runtime.cpp)
target_link_libraries(physics_multiplayer_acceptance PRIVATE
    halo2_renderer SDL2::SDL2 Vulkan::Vulkan glm::glm nlohmann_json::nlohmann_json)
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


add_executable(campaign_multiplayer_acceptance_tu15 EXCLUDE_FROM_ALL
    tests/campaign_multiplayer_acceptance_tu15.cpp src/multiplayer_runtime.cpp)
target_include_directories(campaign_multiplayer_acceptance_tu15 PRIVATE tests)
target_link_libraries(campaign_multiplayer_acceptance_tu15 PRIVATE
    halo2_renderer SDL2::SDL2 Vulkan::Vulkan glm::glm nlohmann_json::nlohmann_json)
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
