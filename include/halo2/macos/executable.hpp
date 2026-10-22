#pragma once
#include <filesystem>
#ifdef __APPLE__
#include <cstdint>
#include <mach-o/dyld.h>
#include <stdexcept>
#include <vector>
#endif

namespace halo2 {
inline std::filesystem::path macos_executable_path() {
#ifdef __APPLE__
    std::uint32_t bytes = 0;
    if (_NSGetExecutablePath(nullptr, &bytes) != -1 || !bytes)
        throw std::runtime_error("Could not determine the Darwin executable path size");
    std::vector<char> buffer(bytes);
    for (;;) {
        bytes = static_cast<std::uint32_t>(buffer.size());
        if (_NSGetExecutablePath(buffer.data(), &bytes) == 0)
            return std::filesystem::canonical(buffer.data());
        if (bytes <= buffer.size())
            throw std::runtime_error("Could not resolve the Darwin executable path");
        buffer.resize(bytes);
    }
#else
    return {};
#endif
}
inline std::filesystem::path macos_bundle_contents() {
#ifdef __APPLE__
    const auto directory = macos_executable_path().parent_path();
    if (directory.filename() == "MacOS" && directory.parent_path().filename() == "Contents")
        return directory.parent_path();
#endif
    return {};
}
}
