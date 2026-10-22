include_guard(GLOBAL)
function(halo2_macos_network_observation target)
    find_library(HALO2_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(HALO2_SYSTEMCONFIGURATION_FRAMEWORK SystemConfiguration REQUIRED)
    target_sources(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../src/macos/native_network_observation.cpp")
    target_link_libraries(${target} PRIVATE
        "${HALO2_COREFOUNDATION_FRAMEWORK}" "${HALO2_SYSTEMCONFIGURATION_FRAMEWORK}")
endfunction()
