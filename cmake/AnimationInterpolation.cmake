if(BUILD_TESTING)
    add_test(NAME original_tu15_collision_attachment_filter COMMAND sandbox_special_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_collision_attachment_filter_tu15_original.json")
    set_tests_properties(original_tu15_collision_attachment_filter PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME native_tu15_authored_idle_coverage COMMAND multiplayer_animation_runtime_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_TU15_GAMEPLAY_XBE}" --idle-audit)
    set_tests_properties(native_tu15_authored_idle_coverage PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    foreach(species human elite)
        set(species_argument "")
        if(species STREQUAL elite)
            set(species_argument --elite)
        endif()
        foreach(sample half endpoint)
            add_test(NAME native_tu15_${species}_completed_pose_${sample} COMMAND multiplayer_animation_runtime_tests
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_TU15_GAMEPLAY_XBE}"
                --split2 --completed-poses ${species_argument})
            set_tests_properties(native_tu15_${species}_completed_pose_${sample} PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 300 RUN_SERIAL TRUE
                ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_VERIFY_FRAME_GEOMETRY=1;HALO2_VERIFY_BORROWED_FRAME_VERTICES=1")
            if(sample STREQUAL endpoint)
                set_property(TEST native_tu15_${species}_completed_pose_${sample} APPEND PROPERTY
                    ENVIRONMENT "HALO2_COMPLETED_POSE_ENDPOINT=1")
            endif()
        endforeach()
    endforeach()
endif()
