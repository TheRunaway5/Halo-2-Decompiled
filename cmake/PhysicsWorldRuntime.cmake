# A single native caller ABI connects the verified world/motion/solver/contact
# graph. The live match still needs its original memory and lower owners.
add_library(halo2_physics_world_runtime STATIC
    src/physics/runtime/physics_world_runtime_generated.cpp
    src/physics/native_constraint_schema_emitter_consumer_tu15.cpp
    src/physics/native_tu15_constraint_schema_emitter.cpp
    src/physics/native_tu15_constraint_producer.cpp src/physics/native_tu15_constraint_base.cpp)
target_include_directories(halo2_physics_world_runtime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(halo2_physics_world_runtime PUBLIC Threads::Threads)
target_include_directories(halo2_physics_world_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_physics_world_runtime PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(halo2_physics_world_runtime PRIVATE /fp:strict)
else()
    target_compile_options(halo2_physics_world_runtime PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_physics_world_runtime PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    # Keep every generated native path instrumented without optimizer inlining
    # across the complete world graph. Release separately checks optimized code.
    target_compile_options(halo2_physics_world_runtime PRIVATE -O0 -g1)
    target_link_options(halo2_physics_world_runtime PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_physics_world_runtime)
if(BUILD_TESTING)
    add_executable(physics_world_runtime_tests tests/physics_world_runtime_tests.cpp)
    target_link_libraries(physics_world_runtime_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    foreach(pair IN ITEMS "world_runtime|disc-original-verified" "world_runtime_tu15|u5-rebound-verified"
            "world_runtime_x87|disc-x87-verified" "world_runtime_x87_tu15|u5-x87-verified")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 test_name)
        list(GET fields 1 fixture_directory)
        add_test(NAME original_physics_${test_name} COMMAND physics_world_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/${fixture_directory}/fixture.json 0031c2e0)
        set_tests_properties(original_physics_${test_name} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME original_physics_world_runtime_retained_contacts COMMAND physics_world_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011/disc-original-v3/fixture.json 0031e8b0)
    add_test(NAME original_physics_world_runtime_retained_contacts_tu15 COMMAND physics_world_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011/u5-rebound-v3/fixture.json 0031e8b0)
    set_tests_properties(original_physics_world_runtime_retained_contacts original_physics_world_runtime_retained_contacts_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    foreach(pair IN ITEMS "world_listener|disc-original-verified" "world_listener_tu15|u5-rebound-verified"
            "world_listener_x87|disc-x87-verified" "world_listener_x87_tu15|u5-x87-verified")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 test_name)
        list(GET fields 1 fixture_directory)
        add_test(NAME original_physics_${test_name} COMMAND physics_world_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-listener-runtime-20261011/${fixture_directory}/fixture.json 00208fb0)
        set_tests_properties(original_physics_${test_name} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    foreach(pair IN ITEMS "world_event|disc-original-verified" "world_event_tu15|u5-rebound-verified"
            "world_event_x87|disc-x87-verified" "world_event_x87_tu15|u5-x87-verified")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 test_name)
        list(GET fields 1 fixture_directory)
        add_test(NAME original_physics_${test_name} COMMAND physics_world_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-event-runtime-20261011/${fixture_directory}/fixture.json)
        set_tests_properties(original_physics_${test_name} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    foreach(pair IN ITEMS "world_listener_manager_boundary|disc-original-v7"
            "world_listener_manager_boundary_tu15|u5-rebound-v7-strict")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 test_name)
        list(GET fields 1 fixture_directory)
        add_test(NAME original_physics_${test_name} COMMAND physics_world_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-listener-runtime-20261011/${fixture_directory}/fixture.json 0031f4b0)
        set_tests_properties(original_physics_${test_name} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    foreach(pair IN ITEMS "world_bootstrap|disc-original-verified" "world_bootstrap_tu15|u5-rebound-verified"
            "world_bootstrap_x87|disc-x87-verified" "world_bootstrap_x87_tu15|u5-x87-verified")
        string(REPLACE "|" ";" fields "${pair}")
        list(GET fields 0 test_name)
        list(GET fields 1 fixture_directory)
        add_test(NAME original_physics_${test_name} COMMAND physics_world_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011/${fixture_directory}/fixture.json)
        set_tests_properties(original_physics_${test_name} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
endif()
