set(HALO2_FIRST_PERSON_BODY_MAP "${HALO2_MCC_MULTIPLAYER_ENGINE_MAP}" CACHE FILEPATH
    "Prepared MCC map for grounded TU1.5 first-person camera/body checks")
if(BUILD_TESTING)
    add_executable(first_person_body_geometry_tests EXCLUDE_FROM_ALL
        tests/first_person_body_geometry_tests.cpp)
    target_link_libraries(first_person_body_geometry_tests PRIVATE halo2_cache)
    add_test(NAME first_person_body_geometry_original COMMAND first_person_body_geometry_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map")
    set_tests_properties(first_person_body_geometry_original PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
        LABELS "first-person;asset-component")
    if(HALO2_FIRST_PERSON_BODY_MAP)
        add_test(NAME first_person_body_geometry_mcc COMMAND first_person_body_geometry_tests
            "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map" "${HALO2_FIRST_PERSON_BODY_MAP}")
        set_tests_properties(first_person_body_geometry_mcc PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
            LABELS "mcc;first-person;asset-component")
    endif()
    add_executable(first_person_body_tu15_tests EXCLUDE_FROM_ALL
        tests/first_person_body_tu15_tests.cpp)
    target_link_libraries(first_person_body_tu15_tests PRIVATE
        halo2_systems nlohmann_json::nlohmann_json)
    if(TARGET halo2_renderer)
        add_executable(first_person_body_runtime_tu15_tests EXCLUDE_FROM_ALL
            tests/first_person_body_runtime_tu15_tests.cpp
            src/gameplay/multiplayer/multiplayer_runtime.cpp)
        target_link_libraries(first_person_body_runtime_tu15_tests PRIVATE
            halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm nlohmann_json::nlohmann_json)
        target_compile_definitions(first_person_body_runtime_tu15_tests PRIVATE
            HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
        add_dependencies(first_person_body_runtime_tu15_tests halo2_shaders)
        find_program(HALO2_FIRST_PERSON_BODY_XVFB_RUN xvfb-run)
    endif()
    if(HALO2_FIRST_PERSON_BODY_MAP)
        get_filename_component(first_person_body_maps "${HALO2_FIRST_PERSON_BODY_MAP}" DIRECTORY)
        foreach(species human elite)
            if("${species}" STREQUAL "elite")
                set(representation 3)
            else()
                set(representation 2)
            endif()
            foreach(stance standing crouching)
                set(name native_tu15_first_person_body_${species}_${stance})
                add_test(NAME ${name} COMMAND first_person_body_tu15_tests
                    "${HALO2_FIRST_PERSON_BODY_MAP}" "${first_person_body_maps}"
                    "${representation}" "${stance}" "${CMAKE_CURRENT_BINARY_DIR}/${name}.json")
                set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180 RUN_SERIAL TRUE
                    LABELS "mcc;first-person;native-component"
                    ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_SOUND_ENTRY_CENSUS=1")
                if(TARGET first_person_body_runtime_tu15_tests)
                    foreach(render_mode original pstencil pathtracing)
                        if("${species}" STREQUAL "elite" AND NOT "${render_mode}" STREQUAL "original")
                            continue()
                        endif()
                        set(name native_tu15_first_person_body_render_${species}_${stance})
                        set(mode_arguments)
                        if(NOT "${render_mode}" STREQUAL "original")
                            string(APPEND name _${render_mode})
                            list(APPEND mode_arguments "${render_mode}")
                        endif()
                        if(HALO2_FIRST_PERSON_BODY_XVFB_RUN)
                            add_test(NAME ${name} COMMAND "${HALO2_FIRST_PERSON_BODY_XVFB_RUN}" -a
                                -s "-screen 0 800x600x24" "$<TARGET_FILE:first_person_body_runtime_tu15_tests>"
                                "${HALO2_FIRST_PERSON_BODY_MAP}" "${first_person_body_maps}"
                                "${representation}" "${stance}" "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" ${mode_arguments})
                        else()
                            add_test(NAME ${name} COMMAND first_person_body_runtime_tu15_tests
                                "${HALO2_FIRST_PERSON_BODY_MAP}" "${first_person_body_maps}"
                                "${representation}" "${stance}" "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" ${mode_arguments})
                        endif()
                        set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300 RUN_SERIAL TRUE
                            LABELS "mcc;first-person;native-component;gpu;${render_mode}"
                            ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_SOUND_ENTRY_CENSUS=1;XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache;HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/vulkan-cache;MESA_SHADER_CACHE_DIR=@HALO2_TEST_SCRATCH@/mesa-cache;__GL_SHADER_DISK_CACHE_PATH=@HALO2_TEST_SCRATCH@/nvidia-cache")
                        if(HALO2_FIRST_PERSON_BODY_XVFB_RUN)
                            set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
                                "SDL_VIDEODRIVER=x11;HALO2_FIRST_PERSON_BODY_EXPECT_VIDEO_DRIVER=x11")
                        endif()
                    endforeach()
                endif()
            endforeach()
        endforeach()
    endif()
endif()
