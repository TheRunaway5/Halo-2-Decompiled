# Component-only library. World scheduling/presentation/script admission are
# deliberately separate integration work; this does not link it into gameplay.
add_library(halo2_workshop_titles_tu15 STATIC EXCLUDE_FROM_ALL
    src/campaign/source_workshop_titles_tu15_generated.cpp)
target_include_directories(halo2_workshop_titles_tu15 PRIVATE include src)
target_compile_features(halo2_workshop_titles_tu15 PRIVATE cxx_std_20)
target_compile_options(halo2_workshop_titles_tu15 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
foreach(kind IN ITEMS title title_lifetime)
    add_executable(workshop_${kind}_tests tests/workshop_${kind}_tests.cpp)
    target_include_directories(workshop_${kind}_tests PRIVATE include)
    target_link_libraries(workshop_${kind}_tests PRIVATE halo2_workshop_titles_tu15 nlohmann_json::nlohmann_json)
    target_compile_features(workshop_${kind}_tests PRIVATE cxx_std_20)
    add_test(NAME workshop_${kind} COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/workshop_title_tests.py
        --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_${kind}_tu15.json.zlib
        --test-executable $<TARGET_FILE:workshop_${kind}_tests>)
endforeach()
