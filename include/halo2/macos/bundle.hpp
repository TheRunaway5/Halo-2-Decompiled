#pragma once
#include "halo2/macos/executable.hpp"
#include <filesystem>
#include <stdexcept>
#if defined(__APPLE__) || defined(_WIN32)
#include <SDL.h>
#endif

namespace halo2 {
inline std::filesystem::path runtime_shader_directory(const std::filesystem::path &fallback) {
#ifdef _WIN32
    // Cross-built Windows executables cannot use the Linux compiler's
    // absolute build path. Their complete shader bundle is beside the EXE.
    if(char *base=SDL_GetBasePath()) {
        const auto directory=std::filesystem::path(reinterpret_cast<const char8_t*>(base));
        SDL_free(base);
        return directory / "shaders";
    }
#endif
    const auto contents = macos_bundle_contents();
    return contents.empty() ? fallback : contents / "Resources/shaders";
}
inline void configure_bundled_moltenvk() {
#ifdef __APPLE__
    const auto contents = macos_bundle_contents();
    if(contents.empty()) return;
    const auto library = contents / "Frameworks/libMoltenVK.dylib";
    if(!std::filesystem::is_regular_file(library))
        throw std::runtime_error("The application bundle is missing libMoltenVK.dylib");
    if(SDL_setenv("SDL_VULKAN_LIBRARY", library.c_str(), 1)!=0)
        throw std::runtime_error("Could not select the bundled MoltenVK library");
#endif
}
}
