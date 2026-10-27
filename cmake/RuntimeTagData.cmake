include_guard(GLOBAL)

set(_halo2_custom_asset_default ON)
if(DEFINED ENV{HALO2_TEST_SCRATCH})
    set(_halo2_custom_asset_default OFF)
endif()
option(HALO2_STAGE_CUSTOM_ASSETS "Stage verified custom asset sources with the engine build"
    ${_halo2_custom_asset_default})
unset(_halo2_custom_asset_default)

function(halo2_stage_tag_data target source)
    # Ordinary runtime definitions remain available to component tests even
    # when their disposable build omits the much larger source asset bundle.
    # The macOS engine instead copies data under Resources before signing.
    if(NOT APPLE)
        add_custom_target(${target}_tag_data
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${source}" "$<TARGET_FILE_DIR:${target}>/data/tags"
            VERBATIM)
        add_dependencies(${target} ${target}_tag_data)
    endif()
    if(NOT HALO2_STAGE_CUSTOM_ASSETS)
        return()
    endif()
    # This dependency runs even when no source relink is needed. Asset-only
    # updates must refresh the source payload delivered with the build.
    set(HALO2_CUSTOM_ASSET_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH
        "Owned custom asset sources to include with this build")
    set(_custom_destination "${CMAKE_CURRENT_BINARY_DIR}/custom-assets")
    add_custom_target(${target}_custom_assets
        COMMAND ${Python3_EXECUTABLE} -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/custom_assets.py"
            --root "${HALO2_CUSTOM_ASSET_ROOT}" --destination "${_custom_destination}"
        BYPRODUCTS "${_custom_destination}/custom-assets.json"
        COMMENT "Verifying and staging custom textures, maps and game modes"
        VERBATIM)
    if(APPLE)
        # Changed resources require relinking and the existing post-build
        # bundle seal. Unchanged verified receipts retain their timestamp.
        set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
            "${_custom_destination}/custom-assets.json")
    endif()
    add_dependencies(${target} ${target}_custom_assets)
endfunction()
