include_guard(GLOBAL)
function(halo2_macos_bundle target name)
    if(NOT APPLE)
        return()
    endif()
    cmake_parse_arguments(BUNDLE "" "" "LICENSES" ${ARGN})
    set(_license_args)
    foreach(license IN LISTS BUNDLE_LICENSES)
        list(APPEND _license_args --license "${license}")
    endforeach()
    foreach(license HALO2_MOLTENVK_LICENSE HALO2_SDL_LICENSE)
        if(NOT EXISTS "${${license}}")
            # The selected SDL target owns this exact generated notice. Its
            # build dependency completes before the post-build bundle seal.
            if(NOT (license STREQUAL "HALO2_SDL_LICENSE" AND TARGET halo2_macos_sdl_sdk
                    AND "${${license}}" STREQUAL "${CMAKE_BINARY_DIR}/macos-dependencies/sdl/share/licenses/SDL2.txt"))
                message(FATAL_ERROR "Set ${license} to the dependency's license before bundling")
            endif()
        endif()
    endforeach()
    get_filename_component(_moltenvk_dir "${HALO2_MOLTENVK_LIBRARY}" DIRECTORY)
    string(REPLACE "_" "-" _bundle_id "${target}")
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE TRUE
        OUTPUT_NAME "${name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "net.halo2forever.${_bundle_id}"
        MACOSX_BUNDLE_BUNDLE_NAME "${name}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
        BUILD_RPATH "@executable_path/../Frameworks;${_moltenvk_dir}")
    set(_arch_args)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND _arch_args --arch "${arch}")
    endforeach()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/macos/bundle.py"
            --app "$<TARGET_BUNDLE_DIR:${target}>" ${_arch_args}
            --library "${HALO2_MOLTENVK_LIBRARY}"
            --license "MoltenVK=${HALO2_MOLTENVK_LICENSE}"
            --license "SDL2=${HALO2_SDL_LICENSE}"
            ${_license_args}
        COMMENT "Bundling, checking architecture slices and signing ${name}.app" VERBATIM)
endfunction()

function(halo2_macos_engine_bundle target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/data"
        VERBATIM)
    halo2_macos_bundle(${target} "Halo 2" LICENSES ${HALO2_MACOS_ENGINE_LICENSES})
endfunction()
