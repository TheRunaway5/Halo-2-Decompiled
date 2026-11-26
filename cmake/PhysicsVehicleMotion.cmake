# Compose vehicle state construction, class motion and verified helper owners.
# Remaining solver, animation, activation and CRT owners stay explicit.
add_library(halo2_physics_vehicle_motion STATIC src/physics/vehicles/physics_vehicle_motion_generated.cpp)
target_include_directories(halo2_physics_vehicle_motion PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_physics_vehicle_motion PUBLIC cxx_std_20)
target_link_libraries(halo2_physics_vehicle_motion PUBLIC halo2_physics_vehicle_update)
if(MSVC)
    target_compile_options(halo2_physics_vehicle_motion PRIVATE /fp:strict)
else()
    target_compile_options(halo2_physics_vehicle_motion PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_physics_vehicle_motion PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_physics_vehicle_motion PUBLIC -fsanitize=address,undefined)
endif()
if(TARGET halo2_systems)
    target_link_libraries(halo2_systems PUBLIC halo2_physics_vehicle_motion)
endif()
if(BUILD_TESTING)
    add_executable(physics_vehicle_motion_tests tests/physics_vehicle_motion_tests.cpp)
    add_executable(native_vehicle_marker_emission_instruction_tests
        tests/native_vehicle_marker_emission_instruction_tests.cpp)
    target_include_directories(native_vehicle_marker_emission_instruction_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(native_vehicle_marker_emission_instruction_tests PRIVATE cxx_std_20)
    add_test(NAME native_vehicle_marker_emission_instruction COMMAND native_vehicle_marker_emission_instruction_tests)
    add_executable(native_vehicle_marker_matrix_instruction_tests
        tests/native_vehicle_marker_emission_instruction_tests.cpp)
    target_include_directories(native_vehicle_marker_matrix_instruction_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(native_vehicle_marker_matrix_instruction_tests PRIVATE cxx_std_20)
    target_compile_definitions(native_vehicle_marker_matrix_instruction_tests PRIVATE HALO2_TEST_VEHICLE_MARKER_MATRIX)
    add_test(NAME native_vehicle_marker_matrix_instruction COMMAND native_vehicle_marker_matrix_instruction_tests)
    target_link_libraries(physics_vehicle_motion_tests PRIVATE halo2_physics_vehicle_motion nlohmann_json::nlohmann_json)
    foreach(image IN ITEMS disc u5)
        add_test(NAME original_physics_vehicle_motion_${image} COMMAND physics_vehicle_motion_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/vehicle-motion-20261011/original-verified/${image}.json)
        set_tests_properties(original_physics_vehicle_motion_${image} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i.86)$")
        add_test(NAME native_tu15_vehicle_wheel_offset_20e414
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_cross_product_20e481
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --cross-product --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_force_20e4f4
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --force --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_response_20e6c3
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --response --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_output_20e91f
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --output-stage --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_scratch_reset_20ea32
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --scratch-reset --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_curve_20e84a
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --curve --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_contact_output_20f6e0
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --contact-output --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_contact_projection_20ee92
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_wheel_offset_20e414.py"
                --contact-projection --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_contact_query_20f3a8
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_contact_query_20f3a8.py"
                --compiler "${CMAKE_CXX_COMPILER}")
        add_test(NAME native_tu15_vehicle_wheel_contact_collection_20ea64
            COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_contact_collection_20ea64.py"
                --compiler "${CMAKE_CXX_COMPILER}")
    endif()
endif()
