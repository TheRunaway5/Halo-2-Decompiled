include_guard(GLOBAL)

# Generated source remains the authenticated review/provenance input. Build
# products partition its bodies so a serial compiler stays within the cap.
function(halo2_partition_native_translation target source count)
    get_filename_component(stem "${source}" NAME_WE)
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
            --source "${CMAKE_CURRENT_SOURCE_DIR}/${source}" --output "${directory}" --count ${count}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/split_native_translation_unit.py"
        COMMENT "Partitioning unchanged native bodies from ${stem}" VERBATIM)
    target_sources(${target} PRIVATE ${outputs})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endfunction()
