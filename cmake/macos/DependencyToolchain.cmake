# A separate configure is required per architecture: upstream dependency CPU
# detection uses CMAKE_SYSTEM_PROCESSOR, not the host's architecture list.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES HALO2_DEPENDENCY_ARCH)
set(CMAKE_SYSTEM_NAME Darwin)
if(HALO2_DEPENDENCY_ARCH STREQUAL "arm64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif(HALO2_DEPENDENCY_ARCH STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
else()
    message(FATAL_ERROR "Set HALO2_DEPENDENCY_ARCH to arm64 or x86_64")
endif()
set(CMAKE_OSX_ARCHITECTURES "${HALO2_DEPENDENCY_ARCH}" CACHE STRING "Dependency architecture" FORCE)
