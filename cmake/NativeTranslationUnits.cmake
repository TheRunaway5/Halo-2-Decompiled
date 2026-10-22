include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/NativeOperandSpecialization.cmake")

# Generated source remains the authenticated review/provenance input. Build
# products partition its bodies so a serial compiler stays within the cap.
function(halo2_partition_native_translation target source count)
    get_filename_component(stem "${source}" NAME_WE)
    halo2_native_operand_source(partition_source "${source}")
    set(directory "${CMAKE_CURRENT_BINARY_DIR}/native-translations/${stem}")
    set(outputs "${directory}/router.cpp")
    math(EXPR last "${count} - 1")
    foreach(index RANGE ${last})
        set(ordinal "00${index}")
        string(LENGTH "${ordinal}" length)
        math(EXPR offset "${length} - 3")
        string(SUBSTRING "${ordinal}" ${offset} 3 ordinal)
        list(APPEND outputs "${directory}/body_${ordinal}.cpp")
    endforeach()
    add_custom_command(OUTPUT ${outputs} "${directory}/common.hpp" "${directory}/manifest.json"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/split_native_translation_unit.py"
            --source "${partition_source}" --output "${directory}" --count ${count}
        DEPENDS "${partition_source}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/split_native_translation_unit.py"
        COMMENT "Partitioning unchanged native bodies from ${stem}" VERBATIM)
    target_sources(${target} PRIVATE ${outputs})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(NOT MSVC)
        set_property(SOURCE ${outputs} APPEND PROPERTY COMPILE_OPTIONS
            -fno-fast-math -ffp-contract=off -frounding-math)
    endif()
endfunction()
