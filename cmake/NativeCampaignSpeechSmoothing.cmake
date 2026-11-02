include_guard(GLOBAL)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_library(native_campaign_speech_smoothing_fixture OBJECT
        src/audio/native_campaign_speech_smoothing_tu15.cpp
        src/audio/native_campaign_speech_smoothing_tu15_binding.cpp
        src/audio/native_campaign_speech_envelope_tu15.cpp
        src/audio/native_campaign_speech_envelope_tu15_binding.cpp
        src/audio/native_tu15_sound_kernels_unit_speech.cpp
        src/audio/native_tu15_sound_manager.cpp
        tests/native_campaign_speech_envelope_manager_groups.cpp)
    set(_speech_smoothing_targets native_campaign_speech_smoothing_fixture)
    foreach(_kind IN ITEMS core composition observer)
        if(_kind STREQUAL "core")
            set(_target native_campaign_speech_smoothing_tu15_tests)
            set(_source tests/native_campaign_speech_smoothing_tu15_tests.cpp)
            set(_fixture native_campaign_speech_smoothing_tu15.json.gz)
            set(_test native_campaign_speech_smoothing_tu15)
        else()
            set(_target native_campaign_speech_smoothing_${_kind}_tu15_tests)
            set(_source tests/native_campaign_speech_smoothing_${_kind}_tu15_tests.cpp)
            set(_test native_campaign_speech_smoothing_${_kind}_tu15)
            if(_kind STREQUAL "composition")
                set(_fixture native_campaign_speech_smoothing_composition_tu15.json.gz)
            else()
                set(_fixture native_campaign_speech_smoothing_tu15.json.gz)
            endif()
        endif()
        add_executable(${_target} ${_source} $<TARGET_OBJECTS:native_campaign_speech_smoothing_fixture>)
        list(APPEND _speech_smoothing_targets ${_target})
        add_test(NAME ${_test}
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_campaign_screen_lifecycle_tu15.py"
            --probe "$<TARGET_FILE:${_target}>" --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${_fixture}")
        set_tests_properties(${_test} PROPERTIES TIMEOUT 60)
    endforeach()
    foreach(_target IN LISTS _speech_smoothing_targets)
        target_include_directories(${_target} PRIVATE include src)
        target_compile_features(${_target} PRIVATE cxx_std_20)
        target_compile_options(${_target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
        if(HALO2_SANITIZERS AND NOT MSVC)
            target_compile_options(${_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${_target} PRIVATE -fsanitize=address,undefined)
        endif()
    endforeach()
endif()
