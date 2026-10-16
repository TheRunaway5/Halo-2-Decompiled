# Cold startup executes the exact disabled IEN store against the actual native
# EventPort gate, then retains the next unowned configuration instruction.
# This never completes initializer45 or advertises full D8/global readiness.
if(BUILD_TESTING)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE
            src/source_runtime_sound_interrupt_policy.cpp
            src/source_runtime_sound_event_port.cpp src/native_tu15_sound_event_port_data.cpp
            src/source_runtime_sound_frontend_events.cpp
            src/source_sound_frontend_events_tu15_generated.cpp
            src/source_sound_frontend_dpc_tu15_generated.cpp
            src/source_sound_pending_aggregate_tu15.cpp src/native_tu15_sound_frontend_dpc_data.cpp)
        target_include_directories(${target} PRIVATE src)
        target_link_libraries(${target} PRIVATE halo2_sound_configuration_cpu_tu15)
    endforeach()
    foreach(input none virtual-four)
        if(input STREQUAL none)
            set(suffix "")
        else()
            set(suffix _controller)
        endif()
        add_test(NAME native_tu15_sound_disabled_configuration${suffix}_startup
            COMMAND native_tu15_graphics_startup_tests
                --frontier sound-configuration --input-devices ${input})
        set_tests_properties(native_tu15_sound_disabled_configuration${suffix}_startup
            PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    endforeach()
endif()
