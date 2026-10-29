include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayDelete.cmake")
if(NOT HALO2_NATIVE_TU15_ALLOCATE_SOURCE_ADDED)
    target_sources(halo2_native_tu15_data_array_delete PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../src/core/native_tu15_data_array_allocate.cpp")
    set(HALO2_NATIVE_TU15_ALLOCATE_SOURCE_ADDED TRUE)
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_data_array_allocate_tests tests/native_tu15_data_array_allocate_tests.cpp)
    target_include_directories(native_tu15_data_array_allocate_tests PRIVATE include)
    target_compile_features(native_tu15_data_array_allocate_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_data_array_allocate_tests PRIVATE
        nlohmann_json::nlohmann_json halo2_native_tu15_data_array_delete)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_data_array_allocate_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_data_array_allocate_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_data_array_allocate
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
        --probe "$<TARGET_FILE:native_tu15_data_array_allocate_tests>"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_data_array_allocate.json.gz")
endif()
