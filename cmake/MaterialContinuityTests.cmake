# Keep the complete production-owner fixture fast on the qualified ELF host.
# Mixed cache/material translation units retain their original implementations;
# section GC discards only functions the full fixture does not call.
add_executable(material_presentation_continuity_tests tests/material_presentation_continuity_tests.cpp)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8
    AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$"
    AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    target_sources(material_presentation_continuity_tests PRIVATE
        src/rendering/renderer/material_presentation_continuity.cpp
        src/assets/cache.cpp
        src/rendering/world/world_material.cpp
        src/rendering/renderer/render_program.cpp
        src/rendering/source_vertex_extern.cpp
        src/rendering/source_shader_state.cpp
        src/rendering/source_shader_rgb.cpp
        src/rendering/shader_animation.cpp
        src/assets/appearance.cpp
        src/gameplay/objects/object_lighting.cpp
        src/ui/hud/motion_sensor.cpp
        src/runtime/native_parallel.cpp)
    target_include_directories(material_presentation_continuity_tests PRIVATE include)
    target_compile_features(material_presentation_continuity_tests PRIVATE cxx_std_20)
    target_compile_options(material_presentation_continuity_tests PRIVATE
        -ffunction-sections -fdata-sections -ffp-contract=off)
    target_link_options(material_presentation_continuity_tests PRIVATE "LINKER:--gc-sections")
    target_link_libraries(material_presentation_continuity_tests PRIVATE
        Threads::Threads nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(material_presentation_continuity_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(material_presentation_continuity_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
else()
    target_link_libraries(material_presentation_continuity_tests PRIVATE
        halo2_renderer nlohmann_json::nlohmann_json)
endif()
add_test(NAME material_presentation_continuity COMMAND material_presentation_continuity_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_material_animation_original.json")
