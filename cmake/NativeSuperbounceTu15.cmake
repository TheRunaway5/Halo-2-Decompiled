# Complete native collector lifetime for the late Turf crouch-contact path.
target_sources(halo2_physics_world_runtime PRIVATE
    src/physics/collision/native_tu15_scoped_contact_query.cpp)
# Explicit acceptance target: only an actual uncommanded launch can pass.
if(BUILD_TESTING AND UNIX AND NOT APPLE)
    add_executable(native_superbounce_tu15_tests EXCLUDE_FROM_ALL tests/native_superbounce_tu15_tests.cpp)
    target_link_libraries(native_superbounce_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(native_superbounce_runtime_tu15_tests EXCLUDE_FROM_ALL
        tests/native_superbounce_runtime_tu15_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(native_superbounce_runtime_tu15_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
    target_compile_definitions(native_superbounce_runtime_tu15_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
    add_dependencies(native_superbounce_runtime_tu15_tests halo2_shaders)
    set(HALO2_SUPERBOUNCE_MCC_TURF "" CACHE FILEPATH "Prepared MCC Turf map for native collision regression")
    set(HALO2_SUPERBOUNCE_MCC_LOCKOUT "" CACHE FILEPATH "Prepared MCC Lockout map for native crouch priming control")
    if(HALO2_SUPERBOUNCE_MCC_TURF)
        add_test(NAME native_tu15_turf_late_crouch_contact
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/run_test.py --
                $<TARGET_FILE:native_superbounce_tu15_tests> --replay
                "${HALO2_SUPERBOUNCE_MCC_TURF}"
                "${PROJECT_SOURCE_DIR}/tests/fixtures/turf_late_crouch_contact_tu15.json"
                "@HALO2_TEST_SCRATCH@/turf-contact.json")
        set_tests_properties(native_tu15_turf_late_crouch_contact PROPERTIES TIMEOUT 120)
        add_test(NAME native_tu15_turf_superbounce
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/run_test.py --
                $<TARGET_FILE:native_superbounce_tu15_tests> --match-replay
                "${HALO2_SUPERBOUNCE_MCC_TURF}"
                "${PROJECT_SOURCE_DIR}/tests/fixtures/superbounce_turf_native_match_tu15.json"
                "@HALO2_TEST_SCRATCH@/turf-superbounce.json")
        set_tests_properties(native_tu15_turf_superbounce PROPERTIES TIMEOUT 120)
        add_test(NAME native_tu15_turf_superbounce_rendered
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/run_test.py --
                $<TARGET_FILE:native_superbounce_runtime_tu15_tests>
                "${HALO2_SUPERBOUNCE_MCC_TURF}"
                "${PROJECT_SOURCE_DIR}/tests/fixtures/superbounce_turf_native_match_tu15.json"
                "${CMAKE_CURRENT_BINARY_DIR}/turf-superbounce-rendered.json")
        set_tests_properties(native_tu15_turf_superbounce_rendered PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    if(HALO2_SUPERBOUNCE_MCC_LOCKOUT)
        add_test(NAME native_tu15_lockout_crouch_prime
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/run_test.py --
                $<TARGET_FILE:native_superbounce_tu15_tests> --replay
                "${HALO2_SUPERBOUNCE_MCC_LOCKOUT}"
                "${PROJECT_SOURCE_DIR}/tests/fixtures/superbounce_lockout_crouch_prime_tu15.json"
                "@HALO2_TEST_SCRATCH@/lockout-prime.json")
        set_tests_properties(native_tu15_lockout_crouch_prime PROPERTIES TIMEOUT 120)
    endif()
endif()
