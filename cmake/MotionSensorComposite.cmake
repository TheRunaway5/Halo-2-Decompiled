# The implementation is portable. This exact FP/COW differential currently
# authenticates the native x86 MXCSR/x87 environment used by the Linux runtime.
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64|i[3-6]86)$")
    add_executable(motion_sensor_composite_tests
        tests/motion_sensor_composite_tests.cpp
        tests/motion_sensor_composite_reference.cpp
        src/ui/hud/motion_sensor.cpp)
    target_include_directories(motion_sensor_composite_tests PRIVATE include)
    target_compile_features(motion_sensor_composite_tests PRIVATE cxx_std_20)
    target_compile_options(motion_sensor_composite_tests PRIVATE -ffp-contract=off)
    if(HALO2_SANITIZERS)
        target_compile_options(motion_sensor_composite_tests PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(motion_sensor_composite_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME motion_sensor_composite_differential COMMAND motion_sensor_composite_tests)
    set_tests_properties(motion_sensor_composite_differential PROPERTIES TIMEOUT 120)
endif()
