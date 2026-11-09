add_library(halo2_native_weapon_effect_dispatch_tu15 STATIC src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/source_private_particle_phase_tu15.cpp)
if(BUILD_TESTING)
    add_executable(native_private_particle_phase_tu15_tests tests/native_private_particle_phase_tu15_tests.cpp)
    target_link_libraries(native_private_particle_phase_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_weapon_effect_frame_tu15_tests tests/native_weapon_effect_frame_tu15_tests.cpp)
    target_include_directories(native_weapon_effect_frame_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_frame_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(route direct frame light ai combat)
        set(frame_test_name native_weapon_effect_frame_${route}_tu15)
        if(route STREQUAL "light" OR route STREQUAL "ai" OR route STREQUAL "combat")
            set(frame_test_name native_weapon_effect_pool_scan_${route}_tu15)
        endif()
        add_test(NAME ${frame_test_name} COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_frame_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_frame_tu15_original.json.gz" "${route}")
        set_tests_properties(${frame_test_name} PROPERTIES TIMEOUT 180)
    endforeach()
    foreach(route direct frame)
        add_test(NAME native_weapon_particle_frame_${route}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_frame_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_particle_frame_tu15_original.json.gz" "${route}")
        set_tests_properties(native_weapon_particle_frame_${route}_tu15 PROPERTIES TIMEOUT 180)
        add_test(NAME native_weapon_effect_systems_${route}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_frame_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_systems_tu15_original.json.gz" "${route}")
        set_tests_properties(native_weapon_effect_systems_${route}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()

endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_marker_emission_graph_tu15_tests tests/native_weapon_effect_marker_emission_graph_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_emission_graph_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_marker_emission_graph_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_marker_emission_graph_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_marker_emission_graph_tu15 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:native_weapon_effect_marker_emission_graph_tu15_tests>" --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_emission_graph_tu15_original.json.gz")
    add_executable(native_weapon_effect_marker_emission_graph_callers_tu15_tests tests/native_weapon_effect_marker_emission_graph_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_emission_graph_callers_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_marker_emission_graph_callers_tu15_tests PRIVATE HALO2_NATIVE_EMISSION_GRAPH_CALLERS=1)
    target_include_directories(native_weapon_effect_marker_emission_graph_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_marker_emission_graph_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support ai device sandbox vehicle)
        add_test(NAME native_weapon_effect_marker_emission_graph_${module}_tu15 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:native_weapon_effect_marker_emission_graph_callers_tu15_tests>" --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_emission_graph_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_marker_emission_graph_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_marker_lookup_tu15_tests tests/native_weapon_effect_marker_lookup_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_lookup_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_marker_lookup_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_marker_lookup_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_marker_lookup_tu15 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:native_weapon_effect_marker_lookup_tu15_tests>" --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_lookup_tu15_original.json.gz")
    add_executable(native_weapon_effect_marker_lookup_callers_tu15_tests tests/native_weapon_effect_marker_lookup_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_lookup_callers_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_marker_lookup_callers_tu15_tests PRIVATE HALO2_NATIVE_MARKER_LOOKUP_COMPILED_CALLERS=1)
    target_include_directories(native_weapon_effect_marker_lookup_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_marker_lookup_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support ai device sandbox vehicle)
        add_test(NAME native_weapon_effect_marker_lookup_${module}_tu15 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:native_weapon_effect_marker_lookup_callers_tu15_tests>" --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_lookup_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_marker_lookup_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_marker_mode_tu15_tests tests/native_weapon_effect_marker_mode_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_mode_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_marker_mode_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_marker_mode_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_marker_mode_tu15 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:native_weapon_effect_marker_mode_tu15_tests>" --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_mode_tu15_original.json.gz")
    add_executable(native_weapon_effect_marker_mode_support_tu15_tests tests/native_weapon_effect_marker_mode_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_mode_support_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_marker_mode_support_tu15_tests PRIVATE HALO2_NATIVE_MARKER_MODE_COMPILED_CALLER=1)
    target_include_directories(native_weapon_effect_marker_mode_support_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_marker_mode_support_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_effect_marker_mode_support_tu15 COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py" --probe "$<TARGET_FILE:native_weapon_effect_marker_mode_support_tu15_tests>" --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_mode_tu15_original.json.gz")
endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_marker_parent_composed_tu15_tests
        tests/native_weapon_effect_marker_parent_composed_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_parent_composed_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_marker_parent_composed_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_marker_parent_composed_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_marker_parent_composed_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_marker_parent_composed_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_parent_composed_tu15_original.json.gz")
    add_executable(native_weapon_effect_marker_parent_composed_callers_tu15_tests tests/native_weapon_effect_marker_parent_composed_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_parent_composed_callers_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_marker_parent_composed_callers_tu15_tests PRIVATE HALO2_NATIVE_PARENT_COMPOSED_CALLERS=1)
    target_include_directories(native_weapon_effect_marker_parent_composed_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_marker_parent_composed_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support ai device vehicle)
        add_test(NAME native_weapon_effect_marker_parent_composed_${module}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_marker_parent_composed_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_parent_composed_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_marker_parent_composed_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_marker_parent_tu15_tests
        tests/native_weapon_effect_marker_parent_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_parent_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_marker_parent_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_marker_parent_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_marker_parent_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_marker_parent_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_parent_tu15_original.json.gz")
    add_executable(native_weapon_effect_marker_parent_callers_tu15_tests tests/native_weapon_effect_marker_parent_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_parent_callers_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_marker_parent_callers_tu15_tests PRIVATE HALO2_NATIVE_MARKER_PARENT_COMPILED_CALLERS=1)
    target_include_directories(native_weapon_effect_marker_parent_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_marker_parent_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support ai device vehicle)
        add_test(NAME native_weapon_effect_marker_parent_${module}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_marker_parent_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_parent_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_marker_parent_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_marker_matrix_tu15_tests
        tests/native_weapon_effect_marker_matrix_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_matrix_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_marker_matrix_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_marker_matrix_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_marker_matrix_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_marker_matrix_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_matrix_tu15_original.json.gz")
    add_executable(native_weapon_effect_marker_matrix_callers_tu15_tests tests/native_weapon_effect_marker_matrix_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_marker_matrix_callers_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_marker_matrix_callers_tu15_tests PRIVATE HALO2_NATIVE_MARKER_MATRIX_COMPILED_CALLERS=1)
    target_include_directories(native_weapon_effect_marker_matrix_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_marker_matrix_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support ai sandbox device damage damage-selected vehicle)
        add_test(NAME native_weapon_effect_marker_matrix_${module}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_marker_matrix_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_marker_matrix_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_marker_matrix_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
endif()
if(BUILD_TESTING)
    add_executable(native_weapon_effect_location_copy_tu15_tests
        tests/native_weapon_effect_location_copy_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_location_copy_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(native_weapon_effect_location_copy_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_location_copy_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_location_copy_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_location_copy_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_location_copy_tu15_original.json.gz")
    add_executable(native_weapon_effect_location_copy_callers_tu15_tests tests/native_weapon_effect_location_copy_tu15_tests.cpp)
    target_compile_features(native_weapon_effect_location_copy_callers_tu15_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_weapon_effect_location_copy_callers_tu15_tests PRIVATE HALO2_NATIVE_LOCATION_COPY_COMPILED_CALLERS=1)
    target_include_directories(native_weapon_effect_location_copy_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_location_copy_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support frame death combat)
        add_test(NAME native_weapon_effect_location_copy_${module}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_location_copy_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_location_copy_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_location_copy_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
endif()
target_compile_features(halo2_native_weapon_effect_dispatch_tu15 PUBLIC cxx_std_20)
target_include_directories(halo2_native_weapon_effect_dispatch_tu15 PUBLIC include)
target_link_libraries(halo2_native_weapon_effect_dispatch_tu15 PUBLIC halo2_glm)
target_link_libraries(halo2_physics_world_runtime PUBLIC halo2_native_weapon_effect_dispatch_tu15)
if(TARGET halo2_physics_vehicle_motion)
    target_link_libraries(halo2_physics_vehicle_motion PUBLIC halo2_native_weapon_effect_dispatch_tu15)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_native_weapon_effect_dispatch_tu15)
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_presentation_step.cpp)
if(TARGET halo2_audio_continuation_owners)
    target_link_libraries(halo2_audio_continuation_owners PUBLIC halo2_native_weapon_effect_dispatch_tu15)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_native_weapon_effect_dispatch_tu15 PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_native_weapon_effect_dispatch_tu15 INTERFACE -fsanitize=address,undefined)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/weapons/native_weapon_zoom_audio_tu15.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_weapon_zoom_sound.cpp)
if(BUILD_TESTING)
    add_executable(native_weapon_effect_dispatch_tu15_tests tests/native_weapon_effect_dispatch_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_dispatch_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_dispatch_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_dispatch_tu15 COMMAND native_weapon_effect_dispatch_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_dispatch_tu15_original.json")
    add_executable(native_weapon_effect_request_tu15_tests tests/native_weapon_effect_request_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_request_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_request_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_request_tu15 COMMAND native_weapon_effect_request_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_request_tu15_original.json")
    add_test(NAME native_weapon_effect_restart_tu15 COMMAND native_weapon_effect_request_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_restart_tu15_original.json")
    add_executable(native_weapon_effect_location_tu15_tests tests/native_weapon_effect_location_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_location_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_location_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_location_tu15 COMMAND native_weapon_effect_location_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_location_tu15_original.json")
    add_executable(native_weapon_effect_iterator_tu15_tests tests/native_weapon_effect_iterator_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_iterator_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_iterator_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_iterator_tu15 COMMAND native_weapon_effect_iterator_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_iterator_tu15_original.json")
    add_executable(native_weapon_effect_iterator_callers_tu15_tests tests/native_weapon_effect_iterator_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_iterator_callers_tu15_tests PRIVATE HALO2_NATIVE_ITERATOR_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_iterator_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module frame support death)
        add_test(NAME native_weapon_effect_iterator_${module}_tu15 COMMAND native_weapon_effect_iterator_callers_tu15_tests
            "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_iterator_tu15_original.json" "${module}")
        set_tests_properties(native_weapon_effect_iterator_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_random_range_tu15_tests tests/native_weapon_effect_random_range_tu15_tests.cpp)
    target_include_directories(native_weapon_effect_random_range_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_random_range_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_random_range_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_random_range_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_random_range_tu15_original.json.gz")
    add_executable(native_weapon_effect_random_range_callers_tu15_tests tests/native_weapon_effect_random_range_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_random_range_callers_tu15_tests PRIVATE HALO2_NATIVE_RANDOM_RANGE_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_random_range_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support ai)
        add_test(NAME native_weapon_effect_random_range_${module}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_random_range_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_random_range_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_random_range_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_next_event_tu15_tests tests/native_weapon_effect_next_event_tu15_tests.cpp)
    target_include_directories(native_weapon_effect_next_event_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_next_event_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_next_event_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_next_event_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_next_event_tu15_original.json.gz")
    add_executable(native_weapon_effect_next_event_callers_tu15_tests tests/native_weapon_effect_next_event_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_next_event_callers_tu15_tests PRIVATE HALO2_NATIVE_NEXT_EVENT_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_next_event_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_effect_next_event_death_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_next_event_callers_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_next_event_tu15_original.json.gz" death)
    set_tests_properties(native_weapon_effect_next_event_tu15 native_weapon_effect_next_event_death_tu15 PROPERTIES TIMEOUT 180)
    add_executable(native_weapon_effect_state_tu15_tests tests/native_weapon_effect_state_tu15_tests.cpp
       )
    add_executable(native_weapon_effect_advance_tu15_tests tests/native_weapon_effect_advance_tu15_tests.cpp)
    target_include_directories(native_weapon_effect_advance_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_advance_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_advance_tu15 COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_advance_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_advance_tu15_original.json.gz")
    add_executable(native_weapon_effect_advance_callers_tu15_tests tests/native_weapon_effect_advance_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_advance_callers_tu15_tests PRIVATE HALO2_NATIVE_ADVANCE_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_advance_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support death)
        add_test(NAME native_weapon_effect_advance_${module}_tu15 COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_advance_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_advance_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_advance_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    target_include_directories(native_weapon_effect_state_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_state_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_state_tu15 COMMAND native_weapon_effect_state_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_state_tu15_original.json")
    add_executable(native_weapon_effect_state_callers_tu15_tests tests/native_weapon_effect_state_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_state_callers_tu15_tests PRIVATE HALO2_NATIVE_STATE_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_state_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support death)
        add_test(NAME native_weapon_effect_state_${module}_tu15 COMMAND native_weapon_effect_state_callers_tu15_tests
            "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_state_tu15_original.json" "${module}")
        set_tests_properties(native_weapon_effect_state_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_creation_tu15_tests tests/native_weapon_effect_creation_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_creation_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_creation_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_creation_tu15 COMMAND native_weapon_effect_creation_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_creation_tu15_original.json")
    add_executable(native_weapon_effect_creation_callers_tu15_tests tests/native_weapon_effect_creation_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_creation_callers_tu15_tests PRIVATE HALO2_NATIVE_CREATION_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_creation_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module frame death)
        add_test(NAME native_weapon_effect_creation_${module}_tu15 COMMAND native_weapon_effect_creation_callers_tu15_tests
            "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_creation_tu15_original.json" "${module}")
        set_tests_properties(native_weapon_effect_creation_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_initialization_tu15_tests tests/native_weapon_effect_initialization_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_initialization_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_initialization_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_initialization_tu15 COMMAND native_weapon_effect_initialization_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_initialization_tu15_original.json")
    add_executable(native_weapon_effect_initialization_callers_tu15_tests tests/native_weapon_effect_initialization_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_initialization_callers_tu15_tests PRIVATE HALO2_NATIVE_INITIALIZATION_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_initialization_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support death)
        add_test(NAME native_weapon_effect_initialization_${module}_tu15 COMMAND native_weapon_effect_initialization_callers_tu15_tests
            "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_initialization_tu15_original.json" "${module}")
        set_tests_properties(native_weapon_effect_initialization_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_allocation_tu15_tests tests/native_weapon_effect_allocation_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_allocation_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_allocation_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_allocation_tu15 COMMAND native_weapon_effect_allocation_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_allocation_tu15_original.json")
    add_executable(native_weapon_effect_allocation_callers_tu15_tests tests/native_weapon_effect_allocation_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_allocation_callers_tu15_tests PRIVATE HALO2_NATIVE_ALLOCATION_COMPILED_CALLERS=1)
    target_link_libraries(native_weapon_effect_allocation_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support death)
        add_test(NAME native_weapon_effect_allocation_${module}_tu15 COMMAND native_weapon_effect_allocation_callers_tu15_tests
            "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_allocation_tu15_original.json" "${module}")
        set_tests_properties(native_weapon_effect_allocation_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_selector_tu15_tests tests/native_weapon_effect_selector_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_selector_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_selector_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_selector_tu15 COMMAND native_weapon_effect_selector_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_selector_tu15_original.json")
    add_executable(native_weapon_effect_dispatch_strict_tu15_tests tests/native_weapon_effect_dispatch_strict_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_dispatch_strict_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_dispatch_strict_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_dispatch_strict_tu15 COMMAND native_weapon_effect_dispatch_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_dispatch_strict_tu15_original.json")
    add_executable(native_weapon_zoom_reset_tu15_tests tests/native_weapon_zoom_reset_tu15_tests.cpp
       )
    target_include_directories(native_weapon_zoom_reset_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_zoom_reset_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_zoom_reset_tu15 COMMAND native_weapon_zoom_reset_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_zoom_reset_tu15_original.json")
    add_executable(native_weapon_zoom_reset_strict_tu15_tests tests/native_weapon_zoom_reset_strict_tu15_tests.cpp
       )
    target_include_directories(native_weapon_zoom_reset_strict_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_zoom_reset_strict_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_zoom_reset_strict_tu15 COMMAND native_weapon_zoom_reset_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_zoom_reset_strict_tu15_original.json")
    add_executable(native_weapon_audio_request_strict_tu15_tests tests/native_weapon_audio_request_strict_tu15_tests.cpp
       )
    target_include_directories(native_weapon_audio_request_strict_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_audio_request_strict_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_audio_request_strict_tu15 COMMAND native_weapon_audio_request_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_audio_request_strict_tu15_original.json")
    add_executable(native_weapon_effect_lifecycle_strict_tu15_tests tests/native_weapon_effect_lifecycle_strict_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_lifecycle_strict_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_lifecycle_strict_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_lifecycle_strict_tu15 COMMAND native_weapon_effect_lifecycle_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_lifecycle_strict_tu15_original.json")
    add_test(NAME native_weapon_effect_attached_strict_tu15 COMMAND native_weapon_effect_lifecycle_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_attached_strict_tu15_original.json")
    add_executable(native_weapon_effect_location_strict_tu15_tests tests/native_weapon_effect_location_strict_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_location_strict_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_location_strict_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_location_strict_tu15 COMMAND native_weapon_effect_location_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_location_strict_tu15_original.json")
    add_test(NAME native_weapon_effect_direction_strict_tu15 COMMAND native_weapon_effect_location_strict_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_direction_strict_tu15_original.json")
    add_executable(native_weapon_effect_datum_allocate_tu15_tests tests/native_weapon_effect_datum_allocate_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_datum_allocate_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_datum_allocate_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_datum_allocate_tu15
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_datum_allocate_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_datum_allocate_tu15_original.json.gz")
    add_executable(native_weapon_effect_datum_allocate_callers_tu15_tests tests/native_weapon_effect_datum_allocate_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_datum_allocate_callers_tu15_tests PRIVATE HALO2_NATIVE_DATUM_COMPILED_CALLERS=1)
    target_include_directories(native_weapon_effect_datum_allocate_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_datum_allocate_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support pool level ai scene shadow frame frame-canonical)
        add_test(NAME native_weapon_effect_datum_allocate_${module}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_datum_allocate_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_effect_datum_allocate_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_datum_allocate_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    add_executable(native_weapon_effect_sound_local_tu15_tests tests/native_weapon_effect_sound_local_tu15_tests.cpp
       )
    target_include_directories(native_weapon_effect_sound_local_tu15_tests PRIVATE include tests)
    target_link_libraries(native_weapon_effect_sound_local_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm halo2_native_weapon_effect_dispatch_tu15)
    add_test(NAME native_weapon_effect_sound_local_tu15
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
        --probe "$<TARGET_FILE:native_weapon_effect_sound_local_tu15_tests>"
        --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_sound_local_tu15_original.json.gz")
    add_executable(native_weapon_effect_sound_local_callers_tu15_tests tests/native_weapon_effect_sound_local_tu15_tests.cpp)
    target_compile_definitions(native_weapon_effect_sound_local_callers_tu15_tests PRIVATE HALO2_NATIVE_SOUND_LOCAL_COMPILED_CALLERS=1)
    target_include_directories(native_weapon_effect_sound_local_callers_tu15_tests PRIVATE tests)
    target_link_libraries(native_weapon_effect_sound_local_callers_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    foreach(module support map)
        add_test(NAME native_weapon_effect_sound_local_${module}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_native_weapon_compressed_reference.py"
            --probe "$<TARGET_FILE:native_weapon_effect_sound_local_callers_tu15_tests>"
            --fixture "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_sound_local_tu15_original.json.gz" "${module}")
        set_tests_properties(native_weapon_effect_sound_local_${module}_tu15 PROPERTIES TIMEOUT 180)
    endforeach()
    if(HALO2_SANITIZERS AND NOT MSVC)
        foreach(target native_weapon_effect_sound_local_tu15_tests native_weapon_effect_datum_allocate_tu15_tests native_weapon_effect_dispatch_tu15_tests native_weapon_effect_request_tu15_tests native_weapon_effect_location_tu15_tests native_weapon_effect_iterator_tu15_tests native_weapon_effect_state_tu15_tests native_weapon_effect_creation_tu15_tests native_weapon_effect_initialization_tu15_tests native_weapon_effect_allocation_tu15_tests native_weapon_effect_selector_tu15_tests native_weapon_effect_dispatch_strict_tu15_tests native_weapon_zoom_reset_tu15_tests native_weapon_zoom_reset_strict_tu15_tests native_weapon_audio_request_strict_tu15_tests native_weapon_effect_lifecycle_strict_tu15_tests native_weapon_effect_location_strict_tu15_tests)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endforeach()
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/NativeWeaponEffectRoutesTu15.cmake")
    add_executable(native_weapon_zoom_audio_tu15_tests tests/native_weapon_zoom_audio_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_audio_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    set(HALO2_MCC_WEAPON_EFFECTS_MAP "" CACHE FILEPATH "Prepared MCC map for actual weapon zoom/effect validation")
    get_filename_component(HALO2_MCC_WEAPON_EFFECTS_MAPS "${HALO2_MCC_WEAPON_EFFECTS_MAP}" DIRECTORY)
    add_test(NAME native_private_particle_phase_tu15 COMMAND native_private_particle_phase_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-private-particle-phase.json")
    set_tests_properties(native_private_particle_phase_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    # Distinct Lockout firearm definitions from the authenticated zoom/effect
    # catalog. Sword primary/melee has a separate lifecycle and is not claimed.
    foreach(representation 2 3)
        foreach(weapon 3 4 5 6 7 8 9 10 11 12 13 14 18 19)
            set(name "native_private_particle_phase_tu15_r${representation}_w${weapon}")
            add_test(NAME "${name}" COMMAND native_private_particle_phase_tu15_tests
                "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
                "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" "${weapon}" "${representation}" 1)
            set_tests_properties("${name}" PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
        endforeach()
        set(name "native_private_particle_phase_tu15_r${representation}_w6_charged")
        add_test(NAME "${name}" COMMAND native_private_particle_phase_tu15_tests
            "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
            "${CMAKE_CURRENT_BINARY_DIR}/${name}.json" 6 "${representation}" 120)
        set_tests_properties("${name}" PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    endforeach()
    add_executable(native_weapon_zoom_fire_caller_tu15_tests tests/native_weapon_zoom_fire_caller_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_fire_caller_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_zoom_fire_caller_tu15 COMMAND native_weapon_zoom_fire_caller_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-zoom-fire-caller.json")
    set_tests_properties(native_weapon_zoom_fire_caller_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(native_weapon_zoom_damage_caller_tu15_tests tests/native_weapon_zoom_damage_caller_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_damage_caller_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_zoom_damage_caller_tu15 COMMAND native_weapon_zoom_damage_caller_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-zoom-damage-caller.json")
    set_tests_properties(native_weapon_zoom_damage_caller_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(native_weapon_presentation_contracts_tu15_tests tests/native_weapon_presentation_contracts_tu15_tests.cpp)
    target_link_libraries(native_weapon_presentation_contracts_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_presentation_contracts_tu15 COMMAND native_weapon_presentation_contracts_tu15_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/weapon_zoom_shooting_contracts_tu15_original.json"
        "${HALO2_MCC_WEAPON_EFFECTS_MAPS}" "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-presentation-contracts.json")
    set_tests_properties(native_weapon_presentation_contracts_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    add_test(NAME native_weapon_zoom_audio_tu15 COMMAND native_weapon_zoom_audio_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-zoom-audio.json")
    set_tests_properties(native_weapon_zoom_audio_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(native_weapon_zoom_recipient_tu15_tests tests/native_weapon_zoom_recipient_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_recipient_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_zoom_recipient_boundary_tu15 COMMAND native_weapon_zoom_recipient_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-zoom-recipient.json")
    set_tests_properties(native_weapon_zoom_recipient_boundary_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(native_weapon_zoom_network_tu15_tests tests/native_weapon_zoom_network_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_network_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_zoom_network_tu15 COMMAND native_weapon_zoom_network_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-zoom-network.json")
    set_tests_properties(native_weapon_zoom_network_tu15 PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(native_weapon_interaction_timeout_tu15_tests tests/native_weapon_interaction_timeout_tu15_tests.cpp)
    add_executable(native_particle_history_tu15_tests tests/native_particle_history_tu15_tests.cpp)
    add_executable(native_weapon_plasma_particle_delivery_tu15_tests tests/native_weapon_plasma_particle_delivery_tu15_tests.cpp)
    target_link_libraries(native_weapon_plasma_particle_delivery_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_plasma_particle_delivery_tu15 COMMAND native_weapon_plasma_particle_delivery_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-plasma-particle-delivery.json")
    set_tests_properties(native_weapon_plasma_particle_delivery_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(native_weapon_source_journal_tu15_tests tests/native_weapon_source_journal_tu15_tests.cpp)
    target_link_libraries(native_weapon_source_journal_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_source_journal_tu15 COMMAND native_weapon_source_journal_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-source-journal.json")
    set_tests_properties(native_weapon_source_journal_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(native_weapon_world_journal_tu15_tests tests/native_weapon_world_journal_tu15_tests.cpp)
    target_link_libraries(native_weapon_world_journal_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_world_journal_tu15 COMMAND native_weapon_world_journal_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-world-journal.json")
    set_tests_properties(native_weapon_world_journal_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(native_weapon_shared_timeline_tu15_tests tests/native_weapon_shared_timeline_tu15_tests.cpp)
    target_link_libraries(native_weapon_shared_timeline_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_shared_timeline_tu15 COMMAND native_weapon_shared_timeline_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-shared-timeline.json")
    set_tests_properties(native_weapon_shared_timeline_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    target_link_libraries(native_particle_history_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_particle_history_continuation_tu15 COMMAND native_particle_history_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-particle-history-continuation.json")
    add_test(NAME native_particle_history_stale_control_tu15 COMMAND native_particle_history_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-particle-history-stale-control.json" without-retention)
    set_tests_properties(native_particle_history_continuation_tu15 native_particle_history_stale_control_tu15
        PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    target_link_libraries(native_weapon_interaction_timeout_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_interaction_timeout_tu15 COMMAND native_weapon_interaction_timeout_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-interaction-timeout.json")
    set_tests_properties(native_weapon_interaction_timeout_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(native_weapon_zoom_audio_route_tu15_tests tests/native_weapon_zoom_audio_route_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_audio_route_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_weapon_zoom_audio_route_tu15 COMMAND native_weapon_zoom_audio_route_tu15_tests)
    add_executable(native_weapon_zoom_continuation_tu15_tests tests/native_weapon_zoom_continuation_tu15_tests.cpp)
    target_link_libraries(native_weapon_zoom_continuation_tu15_tests PRIVATE halo2_authority_codec nlohmann_json::nlohmann_json)
    add_test(NAME native_weapon_zoom_continuation_tu15 COMMAND native_weapon_zoom_continuation_tu15_tests
        "${HALO2_MCC_WEAPON_EFFECTS_MAP}" "${HALO2_MCC_WEAPON_EFFECTS_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/native-weapon-zoom-continuation.json")
    set_tests_properties(native_weapon_zoom_continuation_tu15 PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    add_executable(mcc_weapon_effect_scalar_tests tests/mcc_weapon_effect_scalar_tests.cpp)
    target_link_libraries(mcc_weapon_effect_scalar_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_weapon_effect_scalar_profiles COMMAND mcc_weapon_effect_scalar_tests
        "${PROJECT_SOURCE_DIR}/tests/fixtures/mcc_weapon_effect_scalar_profiles.json")
    if(EXISTS "${HALO2_MCC_WEAPON_EFFECTS_MAP}")
        add_test(NAME mcc_weapon_effect_scalar_actual COMMAND mcc_weapon_effect_scalar_tests
            "${PROJECT_SOURCE_DIR}/tests/fixtures/mcc_weapon_effect_scalar_profiles.json"
            "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map" "${HALO2_MCC_WEAPON_EFFECTS_MAP}")
    endif()
endif()

if(BUILD_TESTING)
    # One Make invocation traverses the common test dependencies once.
    add_custom_target(native_weapon_effect_qualification_tu15 DEPENDS
        native_weapon_effect_dispatch_tu15_tests
        native_weapon_effect_request_tu15_tests
        native_weapon_effect_location_tu15_tests
        native_weapon_effect_iterator_tu15_tests
        native_weapon_effect_iterator_callers_tu15_tests
        native_weapon_zoom_reset_tu15_tests
        native_weapon_zoom_audio_tu15_tests
        native_weapon_zoom_recipient_tu15_tests
        native_weapon_zoom_network_tu15_tests
        native_weapon_interaction_timeout_tu15_tests
        native_particle_history_tu15_tests
        native_weapon_plasma_particle_delivery_tu15_tests
        native_weapon_source_journal_tu15_tests
        native_weapon_world_journal_tu15_tests
        native_weapon_shared_timeline_tu15_tests
        native_weapon_zoom_audio_route_tu15_tests
        native_weapon_zoom_continuation_tu15_tests
        native_weapon_presentation_contracts_tu15_tests
        source_weapon_effect_retirement_tu15_tests
        capture_source_weapon_effect_events_tu15
        mcc_weapon_effect_scalar_tests
        mcc_gameplay_reference_tests
        native_weapon_runtime_tests
        multiplayer_tests
        loop_audio_locations_tu15_tests
        native_weapon_effect_state_tu15_tests
        native_weapon_effect_state_callers_tu15_tests
        native_weapon_effect_advance_tu15_tests
        native_weapon_effect_advance_callers_tu15_tests
        native_weapon_effect_next_event_tu15_tests
        native_weapon_effect_next_event_callers_tu15_tests
        native_weapon_effect_frame_tu15_tests
        native_private_particle_phase_tu15_tests
        native_weapon_effect_random_range_tu15_tests
        native_weapon_effect_random_range_callers_tu15_tests
        native_weapon_effect_creation_tu15_tests
        native_weapon_effect_creation_callers_tu15_tests
        native_weapon_effect_initialization_tu15_tests
        native_weapon_effect_initialization_callers_tu15_tests
        native_weapon_effect_allocation_tu15_tests
        native_weapon_effect_allocation_callers_tu15_tests
        native_weapon_effect_selector_tu15_tests
        native_weapon_effect_dispatch_strict_tu15_tests
        native_weapon_zoom_reset_strict_tu15_tests
        native_weapon_audio_request_strict_tu15_tests
        native_weapon_effect_lifecycle_strict_tu15_tests
        native_weapon_effect_location_strict_tu15_tests
        native_weapon_effect_datum_allocate_tu15_tests
        native_weapon_effect_datum_allocate_callers_tu15_tests
        native_weapon_effect_sound_local_tu15_tests
        native_weapon_effect_sound_local_callers_tu15_tests
        native_weapon_effect_selector_compiled_routes_tu15_tests
        native_weapon_effect_dispatch_strict_compiled_routes_tu15_tests
        native_weapon_effect_location_strict_compiled_routes_tu15_tests
        native_weapon_effect_lifecycle_strict_compiled_routes_tu15_tests
        native_weapon_effect_initialization_compiled_routes_tu15_tests
        native_weapon_effect_audio_request_strict_compiled_routes_tu15_tests
        native_weapon_effect_zoom_reset_strict_compiled_routes_tu15_tests
        native_weapon_zoom_fire_caller_tu15_tests
        native_weapon_zoom_damage_caller_tu15_tests
        native_weapon_effect_location_copy_tu15_tests
        native_weapon_effect_location_copy_callers_tu15_tests
        native_weapon_effect_marker_matrix_tu15_tests
        native_weapon_effect_marker_matrix_callers_tu15_tests
        native_weapon_effect_marker_parent_tu15_tests
        native_weapon_effect_marker_parent_callers_tu15_tests
        native_weapon_effect_marker_mode_tu15_tests
        native_weapon_effect_marker_mode_support_tu15_tests
        native_weapon_effect_marker_parent_composed_tu15_tests
        native_weapon_effect_marker_parent_composed_callers_tu15_tests
        native_weapon_effect_marker_lookup_tu15_tests
        native_weapon_effect_marker_lookup_callers_tu15_tests
        native_weapon_effect_marker_emission_graph_tu15_tests
        native_weapon_effect_marker_emission_graph_callers_tu15_tests
    )
endif()
