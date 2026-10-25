if(BUILD_TESTING)
    set(HALO2_MCC_PLAYER_FORCE_MAP "" CACHE FILEPATH
        "Prepared MCC map with a clear floor for live TU1.5 blast/player acceptance")
    set(HALO2_MCC_PLAYER_VEHICLE_MAP "" CACHE FILEPATH
        "Prepared MCC map with authored Warthogs for live TU1.5 vehicle/player acceptance")
    foreach(input IN ITEMS HALO2_MCC_PLAYER_FORCE_MAP HALO2_MCC_PLAYER_VEHICLE_MAP)
        if(NOT "${${input}}" STREQUAL "" AND
           (NOT IS_ABSOLUTE "${${input}}" OR NOT EXISTS "${${input}}" OR IS_DIRECTORY "${${input}}"))
            message(FATAL_ERROR "${input} requires an existing absolute prepared MCC map")
        endif()
    endforeach()
    add_executable(player_external_forces_tu15_tests tests/player_external_forces_tu15_tests.cpp)
    target_link_libraries(player_external_forces_tu15_tests PRIVATE halo2_systems)
    foreach(representation IN ITEMS 2 3)
        foreach(force_case IN ITEMS frag-near frag-middle frag-far frag-blocked plasma-near plasma-far plasma-blocked rocket-near rocket-far rocket-direct vehicle-hit vehicle-splatter vehicle-miss vehicle-neutral)
            if(force_case MATCHES "^vehicle-")
                set(force_map "${HALO2_MCC_PLAYER_VEHICLE_MAP}")
            else()
                set(force_map "${HALO2_MCC_PLAYER_FORCE_MAP}")
            endif()
            add_test(NAME native_mcc_player_force_${force_case}_${representation}
                COMMAND player_external_forces_tu15_tests "${force_map}" "${force_case}" "${representation}"
                    "${CMAKE_CURRENT_BINARY_DIR}/player-force-${force_case}-${representation}.json")
            set_tests_properties(native_mcc_player_force_${force_case}_${representation}
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
        endforeach()
    endforeach()
    foreach(force_case IN ITEMS frag-near frag-blocked plasma-near plasma-blocked rocket-near rocket-direct vehicle-hit vehicle-splatter vehicle-miss)
        if(force_case MATCHES "^vehicle-")
            set(force_map "${HALO2_MCC_PLAYER_VEHICLE_MAP}")
        else()
            set(force_map "${HALO2_MCC_PLAYER_FORCE_MAP}")
        endif()
        add_test(NAME native_mcc_player_force_lan-${force_case}
            COMMAND player_external_forces_tu15_tests "${force_map}" "lan-${force_case}" 3
                "${CMAKE_CURRENT_BINARY_DIR}/player-force-lan-${force_case}.json")
        set_tests_properties(native_mcc_player_force_lan-${force_case}
            PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    endforeach()
    foreach(vehicle_kind IN ITEMS ghost spectre scorpion wraith banshee warthog_gauss)
        foreach(force_case IN ITEMS hit miss neutral)
            foreach(representation IN ITEMS 2 3)
                set(case_name "${vehicle_kind}-${force_case}")
                add_test(NAME native_mcc_player_force_${case_name}_${representation}
                    COMMAND player_external_forces_tu15_tests "${HALO2_MCC_PLAYER_VEHICLE_MAP}"
                        "${case_name}" "${representation}"
                        "${CMAKE_CURRENT_BINARY_DIR}/player-force-${case_name}-${representation}.json")
                set_tests_properties(native_mcc_player_force_${case_name}_${representation}
                    PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
            endforeach()
        endforeach()
    endforeach()
    set(HALO2_PLAYER_FORCE_ORIGINAL_SUBSET_DIR "" CACHE PATH
        "Owned output of test_player_force_original_subset_tu15.py; separate from historical full corpora")
    if(NOT "${HALO2_PLAYER_FORCE_ORIGINAL_SUBSET_DIR}" STREQUAL "" AND
       (NOT IS_ABSOLUTE "${HALO2_PLAYER_FORCE_ORIGINAL_SUBSET_DIR}" OR
        NOT IS_DIRECTORY "${HALO2_PLAYER_FORCE_ORIGINAL_SUBSET_DIR}"))
        message(FATAL_ERROR "HALO2_PLAYER_FORCE_ORIGINAL_SUBSET_DIR requires an existing absolute owned fixture directory")
    endif()
    foreach(subset IN ITEMS vehicle vitality direct delivery feedback)
        set(subset_target source_damage_delivery_runtime_tests)
        if(subset STREQUAL "vehicle")
            set(subset_target physics_vehicle_runtime_tests)
        elseif(subset STREQUAL "vitality")
            set(subset_target source_vitality_runtime_tests)
        elseif(subset STREQUAL "direct")
            set(subset_target source_direct_damage_runtime_tests)
        endif()
        add_test(NAME native_original_player_force_${subset}_subset
            COMMAND ${subset_target} "${HALO2_PLAYER_FORCE_ORIGINAL_SUBSET_DIR}/${subset}/u5.json")
        set_tests_properties(native_original_player_force_${subset}_subset
            PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
endif()
