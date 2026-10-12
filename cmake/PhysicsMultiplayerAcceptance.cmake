# Explicit visible acceptance runner. Missing live features deliberately return
# nonzero; this is not an automatically passing arithmetic/component fixture.
add_executable(physics_multiplayer_acceptance EXCLUDE_FROM_ALL
    tests/physics_multiplayer_acceptance.cpp src/multiplayer_runtime.cpp)
target_link_libraries(physics_multiplayer_acceptance PRIVATE
    halo2_renderer SDL2::SDL2 Vulkan::Vulkan glm::glm nlohmann_json::nlohmann_json)
target_compile_definitions(physics_multiplayer_acceptance PRIVATE
    HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
add_dependencies(physics_multiplayer_acceptance halo2_shaders)
