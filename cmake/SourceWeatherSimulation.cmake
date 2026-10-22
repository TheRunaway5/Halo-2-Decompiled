# Source arithmetic over caller-owned, independently admitted weather inputs.
# No live scheduler or implicitly selected reciprocal-square-root profile.
function(halo2_add_source_weather_simulation weather_source_root)
    add_library(halo2_weather_simulation
        "${weather_source_root}/src/rendering/weather/source_weather_particle_simulation.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_rsqrt.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_execution.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_plate_program.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_plate_caller.cpp"
        "${weather_source_root}/src/rendering/weather/source_weather_plate_submission.cpp")
    target_include_directories(halo2_weather_simulation PUBLIC "${weather_source_root}/include")
    target_link_libraries(halo2_weather_simulation PUBLIC halo2_cache)
    if(MSVC)
        target_compile_options(halo2_weather_simulation PRIVATE /fp:strict)
    else()
        target_compile_options(halo2_weather_simulation PRIVATE -fno-fast-math -ffp-contract=off)
    endif()

    if(NOT BUILD_TESTING)
        return()
    endif()
    # The current arithmetic admission and ISA-fixture test inspect x86 SSE
    # controls. Other hosts can build the API, but have no admitted test path.
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64|i[3-6]86)$")
        return()
    endif()
    foreach(weather_test source_weather_particle_simulation source_weather_plate_program
                         source_weather_plate_caller source_weather_plate_submission)
        add_executable(${weather_test}_tests "${weather_source_root}/tests/${weather_test}_tests.cpp")
        target_link_libraries(${weather_test}_tests PRIVATE halo2_weather_simulation)
        if(MSVC)
            target_compile_options(${weather_test}_tests PRIVATE /fp:strict)
        else()
            target_compile_options(${weather_test}_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
    endforeach()
    add_test(NAME retail_source_weather_particle_simulation
        COMMAND source_weather_particle_simulation_tests
            "${weather_source_root}/tests/fixtures/retail_source_weather_particle_simulation.tsv")
    add_test(NAME retail_source_weather_particle_simulation_controls
        COMMAND source_weather_particle_simulation_tests
            "${weather_source_root}/tests/fixtures/retail_source_weather_particle_simulation_controls.tsv")
    add_test(NAME retail_source_weather_plate_program
        COMMAND source_weather_plate_program_tests
            "${weather_source_root}/tests/fixtures/retail_source_weather_plate_program.tsv")
    add_test(NAME source_weather_plate_program_admission
        COMMAND source_weather_plate_program_tests --admission)
    add_test(NAME retail_source_weather_plate_caller
        COMMAND source_weather_plate_caller_tests
            "${weather_source_root}/tests/fixtures/retail_source_weather_plate_caller.tsv")
    add_test(NAME source_weather_plate_caller_controls
        COMMAND source_weather_plate_caller_tests --controls)
    add_test(NAME retail_source_weather_plate_submission
        COMMAND source_weather_plate_submission_tests
            "${weather_source_root}/tests/fixtures/retail_source_weather_plate_submission.tsv")
    add_test(NAME source_weather_plate_submission_controls
        COMMAND source_weather_plate_submission_tests --controls)
    set_tests_properties(retail_source_weather_particle_simulation
        retail_source_weather_particle_simulation_controls
        retail_source_weather_plate_program source_weather_plate_program_admission
        retail_source_weather_plate_caller source_weather_plate_caller_controls
        retail_source_weather_plate_submission source_weather_plate_submission_controls
        PROPERTIES LABELS "weather;source-fixture")
endfunction()
