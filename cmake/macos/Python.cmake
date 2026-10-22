include_guard(GLOBAL)
function(halo2_macos_python_dependency)
    if(TARGET halo2_macos_python_sdk)
        return()
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The Python runtime needs explicit macOS architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/python")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/python-work")
    set(arguments)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arguments --arch "${arch}")
    endforeach()
    set(HALO2_MACOS_PYTHON_INSTALLER "" CACHE FILEPATH "Optional verified caller-owned PSF installer input")
    if(HALO2_MACOS_PYTHON_INSTALLER)
        list(APPEND arguments --installer "${HALO2_MACOS_PYTHON_INSTALLER}")
    endif()
    add_custom_command(OUTPUT "${prefix}/built.stamp"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_python.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
        BYPRODUCTS "${prefix}/manifest.json" "${prefix}/python/bin/python3"
        DEPENDS "${root}/tools/macos/build_python.py" "${root}/tools/macos/python_inputs.json"
            "${root}/tools/macos/bundle.py" "${root}/src/macos/python_launcher.cpp"
            "${root}/include/halo2/macos/executable.hpp" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Assembling pinned universal Python, Tk, Pillow and certificate runtime" VERBATIM)
    add_custom_target(halo2_macos_python_sdk DEPENDS "${prefix}/built.stamp")
    set(HALO2_MACOS_PYTHON_PREFIX "${prefix}" PARENT_SCOPE)
endfunction()
