include_guard(GLOBAL)
function(halo2_macos_steam_dependency)
    if(TARGET halo2_macos_steam_sdk)
        return()
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The Steam helper SDK requires explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/steam")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/steam-work")
    set(HALO2_MACOS_DOTNET_EXECUTABLE "dotnet" CACHE STRING "Pinned .NET SDK compiler for the private Steam helper")
    set(arguments)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arguments --arch "${arch}")
    endforeach()
    add_custom_command(OUTPUT "${prefix}/built.stamp"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_steam.py"
            --work "${work}" --prefix "${prefix}" --dotnet "${HALO2_MACOS_DOTNET_EXECUTABLE}" ${arguments}
        BYPRODUCTS "${prefix}/manifest.json"
        DEPENDS "${root}/tools/macos/build_steam.py" "${root}/tools/steam/Program.cs"
            "${root}/tools/steam/ContentProtocol.cs" "${root}/tools/steam/ContentSession.cs"
            "${root}/tools/steam/ContentRules.cs" "${root}/tools/steam/DownloaderAdapters.cs"
            "${root}/tools/steam/upstream.json"
            "${root}/tools/steam/Halo2Steam.csproj" "${root}/tools/steam/global.json"
            "${root}/tools/steam/packages.lock.json" "${root}/tools/steam/package-inputs.json"
            "${root}/tools/steam/content-profile.json" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Publishing the pinned Steam helper's private arm64/x86_64 runtimes" VERBATIM)
    add_custom_target(halo2_macos_steam_sdk DEPENDS "${prefix}/built.stamp")
    set(HALO2_MACOS_STEAM_PREFIX "${prefix}" PARENT_SCOPE)
endfunction()
