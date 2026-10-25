# Complete original owners reached by the actual Midship BV-agent witness.
add_library(halo2_vehicle_bv_agent_tu15 STATIC
    src/physics/collision/source_vehicle_bv_agent_tu15_generated.cpp)
target_include_directories(halo2_vehicle_bv_agent_tu15 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(halo2_vehicle_bv_agent_tu15 PUBLIC cxx_std_20)
target_link_libraries(halo2_vehicle_bv_agent_tu15 PUBLIC halo2_physics_world_runtime)
if(MSVC)
    target_compile_options(halo2_vehicle_bv_agent_tu15 PRIVATE /fp:strict)
else()
    target_compile_options(halo2_vehicle_bv_agent_tu15 PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_vehicle_bv_agent_tu15 PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_vehicle_bv_agent_tu15 PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_vehicle_bv_agent_tu15)
if(BUILD_TESTING)
    add_executable(vehicle_bv_agent_tu15_tests EXCLUDE_FROM_ALL tests/vehicle_bv_agent_tu15_tests.cpp)
    target_link_libraries(vehicle_bv_agent_tu15_tests PRIVATE halo2_vehicle_bv_agent_tu15 nlohmann_json::nlohmann_json)
    add_test(NAME original_vehicle_bv_agent_tu15 COMMAND vehicle_bv_agent_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vehicle_bv_agent_original_tu15.json
        ${CMAKE_CURRENT_BINARY_DIR}/vehicle-bv-agent-original.json)
    set_tests_properties(original_vehicle_bv_agent_tu15 PROPERTIES TIMEOUT 120)
endif()
