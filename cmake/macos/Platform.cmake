include_guard(GLOBAL)
# Link the bundled implementation directly. SDL must open this same dylib;
# mixing loader and ICD handles is invalid. No system Vulkan install is needed.
find_path(Vulkan_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include")
set(Vulkan_INCLUDE_DIRS "${Vulkan_INCLUDE_DIR}")
find_library(HALO2_MOLTENVK_LIBRARY NAMES MoltenVK
    HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/../MoltenVK/dynamic/dylib/macOS")
if(NOT Vulkan_INCLUDE_DIR OR NOT HALO2_MOLTENVK_LIBRARY MATCHES "\\.dylib$")
    message(FATAL_ERROR "macOS requires Vulkan headers and a dynamic libMoltenVK.dylib. Set Vulkan_INCLUDE_DIR and HALO2_MOLTENVK_LIBRARY.")
endif()
if(NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan SHARED IMPORTED GLOBAL)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${HALO2_MOLTENVK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIR}")
endif()
set(HALO2_MOLTENVK_LICENSE "" CACHE FILEPATH "LICENSE from the bundled MoltenVK distribution")
set(HALO2_SDL_LICENSE "" CACHE FILEPATH "License for the bundled SDL library")
include("${CMAKE_CURRENT_LIST_DIR}/Bundle.cmake")
