include_guard(GLOBAL)
function(halo2_observe_vulkan_lens_retirement target)
    # Executable --wrap observes references linked from static libraries, not
    # calls already resolved inside shared-library dependencies.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
        AND NOT BUILD_SHARED_LIBS)
        target_sources(${target} PRIVATE tests/helpers/vulkan_lens_retirement_observer.cpp)
        target_compile_definitions(${target} PRIVATE HALO2_VULKAN_LENS_RETIREMENT_OBSERVER=1)
        foreach(entry vkDeviceWaitIdle vkDestroyBuffer vkFreeMemory vkDestroyPipeline
            vkDestroyPipelineLayout vkDestroyQueryPool vkGetQueryPoolResults)
            target_link_options(${target} PRIVATE "LINKER:--wrap=${entry}")
        endforeach()
    endif()
endfunction()
