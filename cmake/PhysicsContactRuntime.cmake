target_sources(halo2_systems PRIVATE src/physics_contact_runtime_generated.cpp)
if(MSVC)
    set_source_files_properties(src/physics_contact_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics_contact_runtime_generated.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(physics_contact_runtime_tests tests/physics_contact_runtime_tests.cpp)
    target_link_libraries(physics_contact_runtime_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_contact_runtime COMMAND physics_contact_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-contact-runtime-20261011/disc-original-verified/fixture.json 00313800)
    add_test(NAME original_physics_contact_runtime_tu15 COMMAND physics_contact_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-contact-runtime-20261011/u5-rebound-verified/fixture.json 00313800)
    set_tests_properties(original_physics_contact_runtime original_physics_contact_runtime_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    foreach(pair IN ITEMS "world_contact_runtime|disc-original-v3" "world_contact_runtime_tu15|u5-rebound-v3"
            "world_contact_x87|disc-x87-v3" "world_contact_x87_tu15|u5-x87-v3")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 test_name)
        list(GET fields 1 fixture_directory)
        add_test(NAME original_physics_${test_name} COMMAND physics_contact_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011/${fixture_directory}/fixture.json)
        set_tests_properties(original_physics_${test_name} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/PhysicsWorldRuntime.cmake)
