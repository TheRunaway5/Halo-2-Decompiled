#pragma once

#include <array>
#include <string_view>

namespace halo2::macos::loading_converter_metadata {
inline constexpr std::string_view schema = "H2_MACOS_MCC_LOADING_CONVERTER_RUNTIME_V1";
inline constexpr std::string_view executable = "macos/loading-converter/bin/halo2_mcc_loading_audio";
inline constexpr std::array<std::string_view, 7> files{
    executable,
    "macos/loading-converter/lib/libogg.0.dylib",
    "macos/loading-converter/lib/libvorbis.0.dylib",
    "macos/loading-converter/licenses/Ogg.txt",
    "macos/loading-converter/licenses/Vorbis.txt",
    "macos/loading-converter/licenses/ww2ogg.txt",
    "external/ww2ogg/packed_codebooks_aoTuV_603.bin",
};
} // namespace halo2::macos::loading_converter_metadata
