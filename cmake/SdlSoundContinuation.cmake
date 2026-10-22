# Platform foundation only; this does not wire receipt/global audio adoption.
if(BUILD_TESTING)
    add_executable(sdl_sound_continuation_tests tests/sdl_sound_continuation_tests.cpp)
    target_link_libraries(sdl_sound_continuation_tests PRIVATE halo2_sound_platform)
    target_compile_features(sdl_sound_continuation_tests PRIVATE cxx_std_20)
    add_test(NAME native_sdl_sound_continuation COMMAND sdl_sound_continuation_tests)
    set_tests_properties(native_sdl_sound_continuation PROPERTIES
        TIMEOUT 30 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        LABELS "audio-continuation;component")
    if(UNIX)
        add_test(NAME native_sdl_sound_continuation_foreign_held COMMAND
            sdl_sound_continuation_tests --foreign-held-parent)
        set_tests_properties(native_sdl_sound_continuation_foreign_held PROPERTIES
            TIMEOUT 15 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy"
            LABELS "audio-continuation;contract-failure")
    endif()
endif()
