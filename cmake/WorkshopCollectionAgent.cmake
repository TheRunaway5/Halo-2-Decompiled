add_library(halo2_native_collection_contact_component STATIC EXCLUDE_FROM_ALL
    src/physics/native_tu15_collection_contact_agent.cpp
    src/physics/native_tu15_collection_filter.cpp)
target_include_directories(halo2_native_collection_contact_component PUBLIC include)
target_compile_features(halo2_native_collection_contact_component PUBLIC cxx_std_20)
target_compile_options(halo2_native_collection_contact_component PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
add_executable(workshop_collection_agent_tests tests/workshop_collection_agent_tests.cpp
    src/physics/native_tu15_contact_agent_factory.cpp
    src/physics/native_tu15_array_reserve.cpp src/physics/native_tu15_array_growth.cpp)
target_link_libraries(workshop_collection_agent_tests PRIVATE halo2_native_collection_contact_component)
target_compile_features(workshop_collection_agent_tests PRIVATE cxx_std_20)
if(HALO2_SANITIZERS AND NOT MSVC)
    foreach(target halo2_native_collection_contact_component workshop_collection_agent_tests)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()
add_test(NAME workshop_collection_agent COMMAND workshop_collection_agent_tests)
add_executable(workshop_collection_filter_tests tests/workshop_collection_filter_tests.cpp
    src/physics/native_tu15_activation_services.cpp src/physics/native_tu15_contact_agent_factory.cpp)
target_link_libraries(workshop_collection_filter_tests PRIVATE
    halo2_native_collection_contact_component nlohmann_json::nlohmann_json ZLIB::ZLIB)
target_compile_features(workshop_collection_filter_tests PRIVATE cxx_std_20)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(workshop_collection_filter_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(workshop_collection_filter_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME workshop_collection_filter COMMAND workshop_collection_filter_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_collection_filter_tu15.json.zlib)
