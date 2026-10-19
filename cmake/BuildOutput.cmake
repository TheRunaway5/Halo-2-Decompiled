# Keep the latest successfully linked local binaries in a predictable folder.
# Disposable validation must not retain its executables outside owned scratch.
set(_halo2_stage_default ON)
if(DEFINED ENV{HALO2_TEST_SCRATCH})
    set(_halo2_stage_default OFF)
endif()
option(HALO2_STAGE_BUILD_OUTPUT "Copy linked binaries to dist/builds/<platform>" ${_halo2_stage_default})
unset(_halo2_stage_default)

if(HALO2_STAGE_BUILD_OUTPUT)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
        foreach(_halo2_output_target halo2_native halo2_launcher halo2_title_launcher halo2_onboarding)
            add_custom_command(TARGET ${_halo2_output_target} POST_BUILD
                COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/stage_build_output.py
                    --root ${CMAKE_CURRENT_SOURCE_DIR}
                    --binary $<TARGET_FILE:${_halo2_output_target}>
                    --target ${_halo2_output_target}
                    --platform linux-x86_64
                COMMENT "Staging latest Linux x86_64 ${_halo2_output_target} binary"
                VERBATIM)
        endforeach()
        unset(_halo2_output_target)
    else()
        message(STATUS "Build output staging skipped: only Linux x86_64 is currently supported")
    endif()
endif()
