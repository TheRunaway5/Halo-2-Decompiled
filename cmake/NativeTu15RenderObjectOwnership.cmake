add_library(halo2_native_tu15_render_ownership STATIC
    src/rendering/models/native_tu15_render_object_ownership.cpp
    src/rendering/models/native_tu15_render_object_ownership_binding.cpp)
target_include_directories(halo2_native_tu15_render_ownership PUBLIC include PRIVATE src)
target_compile_features(halo2_native_tu15_render_ownership PUBLIC cxx_std_20)
target_compile_options(halo2_native_tu15_render_ownership PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_native_tu15_render_ownership PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_native_tu15_render_ownership PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_physics_world_runtime PUBLIC halo2_native_tu15_render_ownership)
target_link_libraries(halo2_physics_vehicle_update PUBLIC halo2_native_tu15_render_ownership)
# Standalone oracle executables compile these generated families directly.
# Bind their new kernel dependency without pulling in the whole live world.
set(_ownership_generated_families
    source_ai_frame_tu15_generated.cpp source_scene_device_constructor_tu15_generated.cpp
    source_lens_flare_tu15_generated.cpp source_model_render_tu15_generated.cpp
    source_shadow_frame_tu15_generated.cpp source_combat_action_tu15_generated.cpp
    source_sandbox_special_tu15_generated.cpp source_vehicle_director_tu15_generated.cpp
    physics_live_support_tu15_generated.cpp
    physics_world_runtime_generated.cpp physics_vehicle_update_generated.cpp)
get_property(_ownership_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_target IN LISTS _ownership_targets)
    if(_target STREQUAL halo2_physics_world_runtime)
        continue()
    endif()
    get_target_property(_sources ${_target} SOURCES)
    foreach(_source IN LISTS _sources)
        get_filename_component(_name "${_source}" NAME)
        if(_name IN_LIST _ownership_generated_families)
            target_link_libraries(${_target} PRIVATE halo2_native_tu15_render_ownership)
            break()
        endif()
    endforeach()
endforeach()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_render_ownership_tests
        tests/native_tu15_render_ownership_tests.cpp)
    target_include_directories(native_tu15_render_ownership_tests PRIVATE include src)
    target_compile_features(native_tu15_render_ownership_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_render_ownership_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    target_link_libraries(native_tu15_render_ownership_tests PRIVATE
        halo2_native_tu15_render_ownership nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_render_ownership_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_render_ownership_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_render_ownership
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_render_ownership.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_render_ownership_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-render-ownership.json")
    set_tests_properties(original_tu15_native_render_ownership PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
endif()
