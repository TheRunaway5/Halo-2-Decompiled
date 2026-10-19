target_sources(halo2_systems PRIVATE src/source_world_particle_draws_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_particle_constant_storage_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_particle_graphics_binding_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_particle_declaration_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_particle_view_pass_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_particle_world_execution_tu15.cpp)
target_sources(halo2_systems PRIVATE src/source_particle_target_tu15_generated.cpp)
if(BUILD_TESTING)
    add_executable(source_particle_target_tu15_tests tests/source_particle_target_tu15_tests.cpp)
    target_link_libraries(source_particle_target_tu15_tests PRIVATE halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_target_selection COMMAND source_particle_target_tu15_tests)
    add_executable(source_particle_view_context_tu15_tests tests/source_particle_view_context_tu15_tests.cpp)
    target_link_libraries(source_particle_view_context_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_view_context COMMAND source_particle_view_context_tu15_tests)
    add_executable(source_particle_view_pass_tu15_tests tests/source_particle_view_pass_tu15_tests.cpp)
    target_link_libraries(source_particle_view_pass_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_view_pass_packet COMMAND source_particle_view_pass_tu15_tests)
    add_executable(source_particle_view_gates_tu15_tests tests/source_particle_view_gates_tu15_tests.cpp)
    target_link_libraries(source_particle_view_gates_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_view_pass_gates COMMAND source_particle_view_gates_tu15_tests)
    add_executable(source_particle_world_execution_tu15_tests tests/source_particle_world_execution_tu15_tests.cpp)
    target_link_libraries(source_particle_world_execution_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_world_execution
        COMMAND source_particle_world_execution_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/particle-draw-parent-20261015/view1-snapshot-240")
    set_tests_properties(native_tu15_particle_world_execution PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_particle_view_state_tu15_tests
        tests/source_particle_view_state_tu15_tests.cpp)
    target_link_libraries(source_particle_view_state_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_view_state
        COMMAND source_particle_view_state_tu15_tests)
    add_executable(source_particle_view_initializer_tu15_tests
        tests/source_particle_view_initializer_tu15_tests.cpp)
    target_link_libraries(source_particle_view_initializer_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_view_initializer
        COMMAND source_particle_view_initializer_tu15_tests)
    add_executable(source_particle_graphics_layout_tu15_tests
        tests/source_particle_graphics_layout_tu15_tests.cpp)
    target_link_libraries(source_particle_graphics_layout_tu15_tests PRIVATE
        halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_graphics_layout
        COMMAND source_particle_graphics_layout_tu15_tests)
    add_executable(source_particle_graphics_binding_tu15_tests
        tests/source_particle_graphics_binding_tu15_tests.cpp)
    target_link_libraries(source_particle_graphics_binding_tu15_tests PRIVATE
        halo2_systems halo2_renderer)
    find_program(HALO2_PARTICLE_BINDING_XVFB NAMES xvfb-run)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_graphics_binding
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a
            $<TARGET_FILE:source_particle_graphics_binding_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    else()
        add_test(NAME native_tu15_particle_graphics_binding
            COMMAND source_particle_graphics_binding_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    endif()
    set_tests_properties(native_tu15_particle_graphics_binding PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_program_binding_tu15_tests
        tests/source_particle_program_binding_tu15_tests.cpp)
    target_link_libraries(source_particle_program_binding_tu15_tests PRIVATE
        halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_program_binding
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a
            $<TARGET_FILE:source_particle_program_binding_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_program_binding
            COMMAND source_particle_program_binding_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_program_binding PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_program_load_tu15_tests
        tests/source_particle_program_load_tu15_tests.cpp)
    target_link_libraries(source_particle_program_load_tu15_tests PRIVATE
        halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_program_load
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a
            $<TARGET_FILE:source_particle_program_load_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_program_load
            COMMAND source_particle_program_load_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_program_load PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_declaration_tu15_tests
        tests/source_particle_declaration_tu15_tests.cpp)
    target_link_libraries(source_particle_declaration_tu15_tests PRIVATE
        halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_declaration
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a
            $<TARGET_FILE:source_particle_declaration_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_declaration
            COMMAND source_particle_declaration_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_declaration PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(renderer_particle_view_tests tests/renderer_particle_view_tests.cpp)
    target_link_libraries(renderer_particle_view_tests PRIVATE halo2_systems halo2_renderer)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_view_inputs
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a $<TARGET_FILE:renderer_particle_view_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_view_inputs COMMAND renderer_particle_view_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_view_inputs PROPERTIES
        SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_camera_binding_tu15_tests tests/source_particle_camera_binding_tu15_tests.cpp)
    target_link_libraries(source_particle_camera_binding_tu15_tests PRIVATE halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_camera_publisher
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a $<TARGET_FILE:source_particle_camera_binding_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_camera_publisher COMMAND source_particle_camera_binding_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_camera_publisher PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_prepared_view_tu15_tests tests/source_particle_prepared_view_tu15_tests.cpp)
    target_link_libraries(source_particle_prepared_view_tu15_tests PRIVATE halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_prepared_view
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a $<TARGET_FILE:source_particle_prepared_view_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_prepared_view COMMAND source_particle_prepared_view_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_prepared_view PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_view_pass_binding_tu15_tests tests/source_particle_view_pass_binding_tu15_tests.cpp)
    target_link_libraries(source_particle_view_pass_binding_tu15_tests PRIVATE halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_view_pass_binding
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a $<TARGET_FILE:source_particle_view_pass_binding_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_view_pass_binding COMMAND source_particle_view_pass_binding_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_view_pass_binding PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    add_executable(source_particle_camera_order_tu15_tests tests/source_particle_camera_order_tu15_tests.cpp)
    target_link_libraries(source_particle_camera_order_tu15_tests PRIVATE halo2_systems halo2_renderer nlohmann_json::nlohmann_json)
    if(HALO2_PARTICLE_BINDING_XVFB)
        add_test(NAME native_tu15_particle_camera_order
            COMMAND "${HALO2_PARTICLE_BINDING_XVFB}" -a $<TARGET_FILE:source_particle_camera_order_tu15_tests>
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    else()
        add_test(NAME native_tu15_particle_camera_order COMMAND source_particle_camera_order_tu15_tests
            "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
    set_tests_properties(native_tu15_particle_camera_order PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RESOURCE_LOCK halo2_gpu)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
        set_tests_properties(native_tu15_particle_graphics_binding native_tu15_particle_program_binding native_tu15_particle_program_load native_tu15_particle_declaration native_tu15_particle_view_inputs native_tu15_particle_camera_publisher native_tu15_particle_prepared_view native_tu15_particle_view_pass_binding native_tu15_particle_camera_order PROPERTIES
            ENVIRONMENT_MODIFICATION
            "VK_LAYER_PATH=set:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib;SDL_VIDEODRIVER=set:x11;WAYLAND_DISPLAY=unset:")
    endif()
    add_executable(source_particle_constant_storage_tu15_tests
        tests/source_particle_constant_storage_tu15_tests.cpp)
    target_link_libraries(source_particle_constant_storage_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_particle_constant_storage
        COMMAND source_particle_constant_storage_tu15_tests)
    add_executable(source_world_particle_draws_tu15_tests
        tests/source_world_particle_draws_tu15_tests.cpp)
    target_link_libraries(source_world_particle_draws_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_tu15_source_swarm_frame_draws
        COMMAND source_world_particle_draws_tu15_tests "${HALO2_TU15_GAMEPLAY_XBE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(native_tu15_source_swarm_frame_draws PROPERTIES SKIP_RETURN_CODE 77)
endif()
