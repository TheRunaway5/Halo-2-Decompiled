add_executable(workshop_shape_type_tests tests/workshop_shape_type_tests.cpp
    src/physics/native_tu15_activation_services.cpp src/physics/native_tu15_contact_agent_factory.cpp)
target_include_directories(workshop_shape_type_tests PRIVATE include)
target_compile_features(workshop_shape_type_tests PRIVATE cxx_std_20)
target_compile_options(workshop_shape_type_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-ffunction-sections;-fdata-sections>)
target_link_options(workshop_shape_type_tests PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--gc-sections>
    $<$<PLATFORM_ID:Darwin>:-Wl,-dead_strip>)
add_test(NAME workshop_shape_types COMMAND workshop_shape_type_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_shape_types_tu15.tsv)
