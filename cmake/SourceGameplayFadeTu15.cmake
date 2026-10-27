if(MSVC)
    set_property(SOURCE src/gameplay/multiplayer/source_gameplay_fade_tu15.cpp APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
else()
    set_property(SOURCE src/gameplay/multiplayer/source_gameplay_fade_tu15.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off -frounding-math)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_gameplay_fade_tu15.cpp
    src/gameplay/multiplayer/source_gameplay_fade_journal_tu15.cpp)
if(BUILD_TESTING)
    add_executable(source_gameplay_fade_transport_tests tests/source_gameplay_fade_transport_tests.cpp)
    target_link_libraries(source_gameplay_fade_transport_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_gameplay_fade_transport COMMAND source_gameplay_fade_transport_tests)
    set_tests_properties(native_gameplay_fade_transport PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    add_executable(source_gameplay_fade_tu15_tests tests/source_gameplay_fade_tu15_tests.cpp
        src/gameplay/multiplayer/source_gameplay_fade_tu15.cpp
        src/gameplay/multiplayer/source_gameplay_fade_journal_tu15.cpp src/gameplay/units/source_player_lives.cpp)
    target_compile_features(source_gameplay_fade_tu15_tests PRIVATE cxx_std_20)
    target_include_directories(source_gameplay_fade_tu15_tests PRIVATE include)
    target_link_libraries(source_gameplay_fade_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(source_gameplay_fade_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
    add_test(NAME original_gameplay_fade_tu15 COMMAND source_gameplay_fade_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/gameplay_fade_tu15_original.json")
endif()
