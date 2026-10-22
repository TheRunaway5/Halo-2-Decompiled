#pragma once
#include "halo2/macos/executable.hpp"
#include <filesystem>
#include <stdexcept>
#ifdef __APPLE__
#include <SDL.h>
#endif

namespace halo2 {
inline std::filesystem::path runtime_shader_directory(const std::filesystem::path &fallback) {
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
