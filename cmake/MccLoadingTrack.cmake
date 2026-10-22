# The supplied PCM's MCC provenance belongs to the loader/importer. This owner
# and component test neither import assets nor authorize a legacy tone fallback.
target_sources(halo2_systems PRIVATE src/platform/media/native_mcc_loading_audio.cpp)
if(NOT MSVC)
    set_source_files_properties(src/platform/media/native_mcc_loading_audio.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
    add_executable(native_mcc_loading_audio_tests tests/native_mcc_loading_audio_tests.cpp)
    target_link_libraries(native_mcc_loading_audio_tests PRIVATE halo2_systems SDL2::SDL2 OpenSSL::Crypto)
    add_test(NAME native_mcc_loading_audio COMMAND native_mcc_loading_audio_tests)
    set_tests_properties(native_mcc_loading_audio PROPERTIES
        TIMEOUT 60 SKIP_RETURN_CODE 77 ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        LABELS "mcc-loading-component")
    set(HALO2_MCC_LOADING_AUDIO_TEST_PCM "" CACHE FILEPATH
        "Explicit owned canonical WAV from authenticated MCC loading media; never a legacy tone")
    if(NOT "${HALO2_MCC_LOADING_AUDIO_TEST_PCM}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_MCC_LOADING_AUDIO_TEST_PCM}" OR
            NOT EXISTS "${HALO2_MCC_LOADING_AUDIO_TEST_PCM}")
            message(FATAL_ERROR "HALO2_MCC_LOADING_AUDIO_TEST_PCM requires an existing absolute owned WAV")
        endif()
        add_test(NAME native_mcc_loading_audio_mcc COMMAND native_mcc_loading_audio_tests
            --mcc-pcm "${HALO2_MCC_LOADING_AUDIO_TEST_PCM}")
        set_tests_properties(native_mcc_loading_audio_mcc PROPERTIES
            TIMEOUT 90 SKIP_RETURN_CODE 77 ENVIRONMENT "SDL_AUDIODRIVER=dummy"
            LABELS "selected-mcc-native;mcc-loading-component")
    endif()
    set(HALO2_MCC_LOADING_V3_TEST_CACHE "" CACHE PATH
        "Explicit owned prepared MCC loading V3 cache for component admission; empty disables this check")
    if(NOT "${HALO2_MCC_LOADING_V3_TEST_CACHE}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_MCC_LOADING_V3_TEST_CACHE}" OR
            NOT EXISTS "${HALO2_MCC_LOADING_V3_TEST_CACHE}/presentation/loading-v3/manifest.json")
            message(FATAL_ERROR "HALO2_MCC_LOADING_V3_TEST_CACHE requires an existing absolute prepared V3 cache")
        endif()
        if(NOT TARGET mcc_loading_presentation_tests)
            message(FATAL_ERROR "The explicit V3 admission check requires mcc_loading_presentation_tests")
        endif()
        if(NOT IS_ABSOLUTE "${HALO2_MCC_FRONTEND_TEST_CACHE}" OR
            NOT EXISTS "${HALO2_MCC_FRONTEND_TEST_CACHE}/halo2/h2_maps_win64_dx11/mainmenu.map")
            message(FATAL_ERROR "The explicit V3 admission check requires an absolute prepared MCC frontend cache")
        endif()
        add_test(NAME mcc_loading_presentation_v3_imported COMMAND mcc_loading_presentation_tests
            "${HALO2_MCC_FRONTEND_TEST_CACHE}" "${CMAKE_CURRENT_SOURCE_DIR}"
            --v3-cache "${HALO2_MCC_LOADING_V3_TEST_CACHE}")
        set_tests_properties(mcc_loading_presentation_v3_imported PROPERTIES
            TIMEOUT 120 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;mcc-loading-component")
    endif()
endif()
