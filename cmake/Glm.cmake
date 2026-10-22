include_guard(GLOBAL)
find_package(glm REQUIRED CONFIG)
# GLM's supported header target preserves the API without introducing a
# host-architecture compiled libglm into universal engine/test binaries.
add_library(halo2_glm INTERFACE)
if(TARGET glm::glm-header-only)
    target_link_libraries(halo2_glm INTERFACE glm::glm-header-only)
else()
    get_target_property(glm_type glm::glm TYPE)
    if(NOT glm_type STREQUAL "INTERFACE_LIBRARY")
        message(FATAL_ERROR "GLM must export its header-only target")
    endif()
    target_link_libraries(halo2_glm INTERFACE glm::glm)
endif()
