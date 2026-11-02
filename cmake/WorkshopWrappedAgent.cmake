add_executable(workshop_wrapped_agent_tests tests/workshop_wrapped_agent_tests.cpp
    src/physics/native_tu15_wrapped_contact_agent.cpp
    src/physics/native_tu15_reversed_transformed_contact_agent.cpp
    src/physics/native_tu15_geometry.cpp
    src/physics/native_tu15_contact_agent_factory.cpp)
target_include_directories(workshop_wrapped_agent_tests PRIVATE include)
target_compile_features(workshop_wrapped_agent_tests PRIVATE cxx_std_20)
target_compile_options(workshop_wrapped_agent_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(workshop_wrapped_agent_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(workshop_wrapped_agent_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME workshop_wrapped_agent COMMAND workshop_wrapped_agent_tests)
