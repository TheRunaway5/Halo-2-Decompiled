include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/CampaignNativeAiTargetMerge.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CampaignNativeAiBehavior129.cmake")
target_sources(halo2_physics_world_runtime PRIVATE
    src/campaign/ai/native_campaign_ai_command_movement_tu15.cpp
    src/campaign/ai/native_campaign_ai_command_lifecycle_tu15.cpp
    src/campaign/ai/native_campaign_ai_command_facing_tu15.cpp
    src/campaign/ai/native_campaign_ai_movement_link_parameters_tu15.cpp
    src/campaign/ai/native_campaign_ai_movement_jump_tu15.cpp
    src/campaign/ai/native_campaign_ai_movement_link_transition_tu15.cpp
    src/campaign/scripting/native_campaign_hs_command_builtins_tu15.cpp)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    get_target_property(_halo2_command_sources native_campaign_ai_publication_tu15_tests SOURCES)
    list(FILTER _halo2_command_sources EXCLUDE REGEX "^tests/")
    list(APPEND _halo2_command_sources
        src/campaign/ai/native_campaign_ai_command_movement_tu15.cpp
        src/campaign/ai/native_campaign_ai_command_lifecycle_tu15.cpp)
    list(REMOVE_DUPLICATES _halo2_command_sources)
    foreach(_owner command_movement command_lifecycle movement_resume)
        add_executable(native_campaign_ai_${_owner}_tu15_tests
            tests/native_campaign_ai_${_owner}_tu15_tests.cpp ${_halo2_command_sources})
        target_include_directories(native_campaign_ai_${_owner}_tu15_tests PRIVATE include src)
        target_compile_features(native_campaign_ai_${_owner}_tu15_tests PRIVATE cxx_std_20)
        target_compile_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        target_link_libraries(native_campaign_ai_${_owner}_tu15_tests PRIVATE
            nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete halo2_native_tu15_data_array_lookup
            halo2_native_pair_consumer halo2_systems halo2_native_weapon_effect_dispatch_tu15)
        if(HALO2_SANITIZERS AND NOT MSVC)
            target_compile_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fsanitize=address,undefined)
        endif()
        add_test(NAME native_campaign_ai_${_owner}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:native_campaign_ai_${_owner}_tu15_tests>"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_${_owner}_tu15.json.gz")
        set_tests_properties(native_campaign_ai_${_owner}_tu15 PROPERTIES TIMEOUT 300)
    endforeach()
    add_test(NAME native_campaign_ai_command_movement_kind3_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_command_movement_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_command_movement_kind3_tu15.json.gz")
    set_tests_properties(native_campaign_ai_command_movement_kind3_tu15 PROPERTIES TIMEOUT 300)
    foreach(_owner movement_link_parameters movement_jump movement_link_transition)
        add_executable(native_campaign_ai_${_owner}_tu15_tests
            tests/native_campaign_ai_${_owner}_tu15_tests.cpp)
        target_include_directories(native_campaign_ai_${_owner}_tu15_tests PRIVATE include src)
        target_compile_features(native_campaign_ai_${_owner}_tu15_tests PRIVATE cxx_std_20)
        target_compile_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE
            -fno-fast-math -ffp-contract=off -frounding-math)
        target_link_libraries(native_campaign_ai_${_owner}_tu15_tests PRIVATE
            halo2_physics_world_runtime nlohmann_json::nlohmann_json)
        if(HALO2_SANITIZERS AND NOT MSVC)
            target_compile_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(native_campaign_ai_${_owner}_tu15_tests PRIVATE -fsanitize=address,undefined)
        endif()
        add_test(NAME native_campaign_ai_${_owner}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:native_campaign_ai_${_owner}_tu15_tests>"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_${_owner}_tu15.json.gz")
        set_tests_properties(native_campaign_ai_${_owner}_tu15 PROPERTIES TIMEOUT 300)
    endforeach()
    add_executable(native_campaign_hs_command_builtins_tu15_tests
        tests/native_campaign_hs_command_builtins_tu15_tests.cpp)
    target_include_directories(native_campaign_hs_command_builtins_tu15_tests PRIVATE include src)
    target_compile_features(native_campaign_hs_command_builtins_tu15_tests PRIVATE cxx_std_20)
    target_compile_options(native_campaign_hs_command_builtins_tu15_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_campaign_hs_command_builtins_tu15_tests PRIVATE
        halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_campaign_hs_command_builtins_tu15_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_campaign_hs_command_builtins_tu15_tests PRIVATE
            -fsanitize=address,undefined)
    endif()
    foreach(_fixture native_campaign_hs_command_builtins native_campaign_hs_command_face
            native_campaign_hs_command_look native_campaign_hs_command_go_face
            native_campaign_hs_combat_status_abort
            native_campaign_ai_command_facing native_campaign_ai_command_hs_parents)
        add_test(NAME ${_fixture}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:native_campaign_hs_command_builtins_tu15_tests>"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${_fixture}_tu15.json.gz")
        set_tests_properties(${_fixture}_tu15 PROPERTIES TIMEOUT 300)
    endforeach()
    foreach(_cohort extra identity composed kind3)
        add_test(NAME native_campaign_ai_command_lifecycle_${_cohort}_tu15
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:native_campaign_ai_command_lifecycle_tu15_tests>"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_command_lifecycle_${_cohort}_tu15.json.gz")
        set_tests_properties(native_campaign_ai_command_lifecycle_${_cohort}_tu15 PROPERTIES TIMEOUT 300)
    endforeach()
    add_test(NAME native_campaign_ai_command_point_tu15
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_campaign_ai_publication_tu15_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_campaign_ai_command_point_tu15.json.gz")
    set_tests_properties(native_campaign_ai_command_point_tu15 PROPERTIES TIMEOUT 300)
endif()
