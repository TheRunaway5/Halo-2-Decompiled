# Record the final graph, including modules added after the launcher targets.
# This is evidence about build reachability, not runtime feature acceptance.
function(halo2_record_engine_wiring)
    get_property(_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/engine-targets.txt" "")
    foreach(_target IN LISTS _targets)
        get_target_property(_sources ${_target} SOURCES)
        get_target_property(_links ${_target} LINK_LIBRARIES)
        get_target_property(_dependencies ${_target} MANUALLY_ADDED_DEPENDENCIES)
        file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/engine-targets.txt"
            "${_target}|${_links}|${_dependencies}|${_sources}\n")
    endforeach()
endfunction()
cmake_language(DEFER CALL halo2_record_engine_wiring)
if(BUILD_TESTING)
    add_test(NAME launcher_engine_promotion COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_build_launcher_engine.py")
endif()
