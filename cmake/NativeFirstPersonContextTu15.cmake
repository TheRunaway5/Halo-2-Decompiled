include_guard(GLOBAL)

# Keep strict native bridge options local to this object target. Adding another
# bridge must not rewrite halo2_systems' options for every existing source.
add_library(halo2_first_person_context_tu15 OBJECT
    src/gameplay/animation/source_first_person_context_tu15.cpp)
target_include_directories(halo2_first_person_context_tu15 PRIVATE include src)
target_link_libraries(halo2_first_person_context_tu15 PRIVATE SDL2::SDL2)
if(MSVC)
    target_compile_options(halo2_first_person_context_tu15 PRIVATE /fp:strict)
else()
    target_compile_options(halo2_first_person_context_tu15 PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
endif()
target_sources(halo2_systems PRIVATE
    $<TARGET_OBJECTS:halo2_first_person_context_tu15>
    src/gameplay/animation/source_first_person_action_tu15.cpp)

if(BUILD_TESTING)
    foreach(name native_first_person_context_tests native_first_person_context_role_tests
                 native_first_person_context_warm_tests)
        add_executable(${name} tests/${name}.cpp)
        target_link_libraries(${name} PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    endforeach()
    if(HALO2_MCC_ONLINE_PRESENTATION_MAP)
        get_filename_component(context_maps "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" DIRECTORY)
        foreach(mode ordinary declared-actions)
            set(arguments)
            if(mode STREQUAL "declared-actions")
                list(APPEND arguments declared-actions)
            endif()
            add_test(NAME native_mcc_fp_context_${mode} COMMAND native_first_person_context_tests
                "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${context_maps}"
                "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_fp_context_${mode}.json" ${arguments})
            set_tests_properties(native_mcc_fp_context_${mode} PROPERTIES
                TIMEOUT 300 RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component"
                ENVIRONMENT "SDL_AUDIODRIVER=dummy")
        endforeach()
        add_test(NAME native_mcc_fp_context_role COMMAND native_first_person_context_role_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${context_maps}"
            "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_fp_context_role.json")
        set_tests_properties(native_mcc_fp_context_role PROPERTIES
            TIMEOUT 300 RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy")
        add_test(NAME native_mcc_fp_context_warm COMMAND native_first_person_context_warm_tests
            "${HALO2_MCC_ONLINE_PRESENTATION_MAP}" "${context_maps}"
            "${CMAKE_CURRENT_BINARY_DIR}/native_mcc_fp_context_warm.json")
        set_tests_properties(native_mcc_fp_context_warm PROPERTIES
            TIMEOUT 300 RUN_SERIAL TRUE LABELS "online-presentation;mcc;native-component"
            ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    endif()
endif()
