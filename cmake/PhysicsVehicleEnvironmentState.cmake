# TU1.5-only environment state and force-delivery composition.
add_library(halo2_physics_vehicle_environment_state STATIC src/physics/vehicles/physics_vehicle_environment_state_tu15_generated.cpp)
target_include_directories(halo2_physics_vehicle_environment_state PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_physics_vehicle_environment_state PUBLIC cxx_std_20)
target_link_libraries(halo2_physics_vehicle_environment_state PUBLIC halo2_physics_vehicle_constraint_cleanup)
if(MSVC)
    target_compile_options(halo2_physics_vehicle_environment_state PRIVATE /fp:strict)
else()
    target_compile_options(halo2_physics_vehicle_environment_state PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_physics_vehicle_environment_state PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_physics_vehicle_environment_state PUBLIC -fsanitize=address,undefined)
endif()
if(TARGET halo2_systems)
    target_link_libraries(halo2_systems PUBLIC halo2_physics_vehicle_environment_state)
endif()
if(BUILD_TESTING)
    add_executable(physics_vehicle_environment_state_tests tests/physics_vehicle_environment_state_tests.cpp)
    target_link_libraries(physics_vehicle_environment_state_tests PRIVATE halo2_physics_vehicle_environment_state nlohmann_json::nlohmann_json)
    add_test(NAME original_physics_vehicle_environment_state_tu15 COMMAND physics_vehicle_environment_state_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/vehicle-environment-state-tu15-20261011/hardware-verified/u5-native.json)
    set_tests_properties(original_physics_vehicle_environment_state_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_FIRST_PERSON_EFFECT_RETIREMENT_TU15_FIXTURE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/first_person_effect_retirement_tu15.json"
        CACHE FILEPATH "Original TU1.5 effect-retirement cases for the first-person dispatch seam")
    add_test(NAME original_first_person_effect_retirement_tu15 COMMAND physics_vehicle_environment_state_tests
        "${HALO2_FIRST_PERSON_EFFECT_RETIREMENT_TU15_FIXTURE}" first-person-dispatch)
    set_tests_properties(original_first_person_effect_retirement_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_FIRST_PERSON_VECTOR_PACK_TU15_FIXTURE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/first_person_vector_pack_tu15.json"
        CACHE FILEPATH "Original TU1.5 vector-packing cases for the first-person dispatch seam")
    add_test(NAME original_first_person_vector_pack_tu15 COMMAND physics_vehicle_environment_state_tests
        "${HALO2_FIRST_PERSON_VECTOR_PACK_TU15_FIXTURE}" first-person-vector-pack)
    set_tests_properties(original_first_person_vector_pack_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_FIRST_PERSON_PART_RETIREMENT_TU15_FIXTURE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/first_person_part_retirement_tu15.json"
        CACHE FILEPATH "Original TU1.5 effect-part release cases for the first-person dispatch seam")
    add_test(NAME original_first_person_part_retirement_tu15 COMMAND physics_vehicle_environment_state_tests
        "${HALO2_FIRST_PERSON_PART_RETIREMENT_TU15_FIXTURE}" first-person-part-retirement)
    set_tests_properties(original_first_person_part_retirement_tu15 PROPERTIES SKIP_RETURN_CODE 77)
endif()
