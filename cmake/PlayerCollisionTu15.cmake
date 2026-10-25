if(BUILD_TESTING)
    set(HALO2_MCC_PLAYER_COLLISION_MAP "" CACHE FILEPATH
        "Prepared MCC map for live TU1.5 player versus player collision acceptance")
    if(NOT "${HALO2_MCC_PLAYER_COLLISION_MAP}" STREQUAL "" AND
       (NOT IS_ABSOLUTE "${HALO2_MCC_PLAYER_COLLISION_MAP}" OR
        NOT EXISTS "${HALO2_MCC_PLAYER_COLLISION_MAP}" OR
        IS_DIRECTORY "${HALO2_MCC_PLAYER_COLLISION_MAP}"))
        message(FATAL_ERROR "HALO2_MCC_PLAYER_COLLISION_MAP requires an existing absolute prepared MCC map")
    endif()
    add_executable(player_collision_tu15_tests tests/player_collision_tu15_tests.cpp)
    target_link_libraries(player_collision_tu15_tests PRIVATE halo2_systems)
    foreach(collision_case IN ITEMS stationary head-on crouched beside stacked stacked-beside lan-head-on teammate enemy)
        add_test(NAME native_mcc_player_collision_${collision_case}
            COMMAND player_collision_tu15_tests "${HALO2_MCC_PLAYER_COLLISION_MAP}" "${collision_case}" 22)
        set_tests_properties(native_mcc_player_collision_${collision_case}
            PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    foreach(representations IN ITEMS 22 23 32 33)
        foreach(boost_case IN ITEMS boost-standing boost-crouched)
            add_test(NAME native_mcc_player_collision_${boost_case}_${representations}
                COMMAND player_collision_tu15_tests "${HALO2_MCC_PLAYER_COLLISION_MAP}" "${boost_case}" "${representations}")
            set_tests_properties(native_mcc_player_collision_${boost_case}_${representations}
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
        endforeach()
    endforeach()
    add_test(NAME native_mcc_player_collision_boost-lan
        COMMAND player_collision_tu15_tests "${HALO2_MCC_PLAYER_COLLISION_MAP}" boost-lan 23)
    set_tests_properties(native_mcc_player_collision_boost-lan PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_test(NAME native_mcc_player_collision_boost-lan_32
        COMMAND player_collision_tu15_tests "${HALO2_MCC_PLAYER_COLLISION_MAP}" boost-lan 32)
    set_tests_properties(native_mcc_player_collision_boost-lan_32 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    foreach(representations IN ITEMS 23 32)
        add_test(NAME native_mcc_player_collision_boost-lan-crouched_${representations}
            COMMAND player_collision_tu15_tests "${HALO2_MCC_PLAYER_COLLISION_MAP}" boost-lan-crouched "${representations}")
        set_tests_properties(native_mcc_player_collision_boost-lan-crouched_${representations}
            PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    endforeach()
    foreach(representations IN ITEMS 23 32 33)
        add_test(NAME native_mcc_player_collision_head-on_${representations}
            COMMAND player_collision_tu15_tests "${HALO2_MCC_PLAYER_COLLISION_MAP}" head-on "${representations}")
        set_tests_properties(native_mcc_player_collision_head-on_${representations}
            PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
endif()
