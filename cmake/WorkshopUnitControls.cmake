# Original evaluator component. World/input scheduling and MCC admission
# require their own integration acceptance before this enters gameplay.
add_library(halo2_workshop_unit_controls_tu15 STATIC EXCLUDE_FROM_ALL
    src/campaign/scripting/source_workshop_unit_controls_tu15_generated.cpp)
target_include_directories(halo2_workshop_unit_controls_tu15 PRIVATE include src)
target_compile_features(halo2_workshop_unit_controls_tu15 PRIVATE cxx_std_20)
target_compile_options(halo2_workshop_unit_controls_tu15 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
add_executable(workshop_unit_controls_tests tests/workshop_unit_controls_tests.cpp)
target_include_directories(workshop_unit_controls_tests PRIVATE include)
target_link_libraries(workshop_unit_controls_tests PRIVATE halo2_workshop_unit_controls_tu15 nlohmann_json::nlohmann_json)
target_compile_features(workshop_unit_controls_tests PRIVATE cxx_std_20)
add_test(NAME workshop_unit_controls COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_unit_controls_tu15.json.zlib
    --test-executable $<TARGET_FILE:workshop_unit_controls_tests>)
add_test(NAME workshop_unit_controls_action_reset COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_unit_controls_tests.py
    --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_player_action_reset_tu15.json.zlib
    --test-executable $<TARGET_FILE:workshop_unit_controls_tests>)
if(TARGET halo2_physics_world_runtime)
    add_executable(workshop_unit_controls_hs_tests tests/workshop_unit_controls_hs_tests.cpp)
    target_compile_features(workshop_unit_controls_hs_tests PRIVATE cxx_std_20)
    target_link_libraries(workshop_unit_controls_hs_tests PRIVATE
        halo2_workshop_unit_controls_tu15 halo2_physics_world_runtime)
    add_test(NAME workshop_unit_controls_hs COMMAND workshop_unit_controls_hs_tests)
endif()
