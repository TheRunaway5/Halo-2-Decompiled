#include "halo2/movie.hpp"
#include <nlohmann/json.hpp>
#include <dlfcn.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
std::string loaded(const void* symbol, const std::filesystem::path& frameworks) {
    Dl_info info{};
    require(dladdr(symbol, &info) && info.dli_fname, "Cannot identify loaded playback library");
    const auto path = std::filesystem::canonical(info.dli_fname);
    require(path.parent_path() == std::filesystem::canonical(frameworks), "Playback borrowed a host library");
    return path.string();
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 3, "Supply synthetic movie and bundled framework directories");
        using namespace halo2;
        nlohmann::json report;
        report["schema"] = "H2_MACOS_FFMPEG_PLAYBACK_V1";
        report["runtime_license"] = avcodec_license();
        report["runtime_configuration"] = avcodec_configuration();
        report["loaded"] = {loaded(reinterpret_cast<const void*>(&avcodec_version), argv[2]),
            loaded(reinterpret_cast<const void*>(&avformat_version), argv[2]),
            loaded(reinterpret_cast<const void*>(&avutil_version), argv[2]),
            loaded(reinterpret_cast<const void*>(&swresample_version), argv[2]),
            loaded(reinterpret_cast<const void*>(&swscale_version), argv[2])};
        // Verify actual runtime availability, including the source-specific
        // Xbox codec that cannot be substituted with WAV IMA ADPCM.
        for (const auto id : {AV_CODEC_ID_H264, AV_CODEC_ID_HEVC, AV_CODEC_ID_AV1,
             AV_CODEC_ID_BINKVIDEO, AV_CODEC_ID_AAC, AV_CODEC_ID_BINKAUDIO_DCT,
             AV_CODEC_ID_BINKAUDIO_RDFT, AV_CODEC_ID_WMAV1, AV_CODEC_ID_WMAV2,
             AV_CODEC_ID_PCM_S16BE, AV_CODEC_ID_PCM_S16LE,
             AV_CODEC_ID_ADPCM_IMA_XBOX, AV_CODEC_ID_ADPCM_IMA_WAV}) {
            const auto* codec = avcodec_find_decoder(id);
            require(codec, "Missing engine playback decoder");
            report["decoders"].push_back(codec->name);
        }
        require(std::string(avcodec_find_decoder(AV_CODEC_ID_AV1)->name) == "libdav1d", "AV1 lacks software fallback");
        void* dav1d = dlsym(RTLD_DEFAULT, "dav1d_open");
        require(dav1d, "The real dav1d library was not loaded");
        report["loaded"].push_back(loaded(dav1d, argv[2]));
        require(av_hwdevice_find_type_by_name("videotoolbox") == AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
                "Apple hardware decoding is absent");
        EncodedSound adpcm{"Xbox ADPCM", 48000, 2, 1, 64, std::vector<std::byte>(72)};
        const auto pcm = decode_sound(adpcm);
        require(pcm.frames() == 64 && pcm.rate == 48000 &&
            std::all_of(pcm.stereo.begin(), pcm.stereo.end(), [](float x) { return x == 0; }),
            "The production Xbox ADPCM path changed its 64-sample block");
        adpcm.data.pop_back();
        bool rejected = false;
        try { decode_sound(adpcm); } catch (const FormatError&) { rejected = true; }
        require(rejected, "Truncated Xbox ADPCM was accepted");
        report["xbox_adpcm_frames"] = pcm.frames();
        bool passed = true;
        for (const char* name : {"h264", "hevc", "av1"}) {
          try {
            const auto path = std::filesystem::path(argv[1]) / (std::string(name) + ".mp4");
            Movie software(path, MovieDecodeMode::Software);
            Movie automatic(path);
            require(software.frame_count() == 6 && software.frame_rate() == 30 &&
                software.image().width == 64 && software.image().height == 64, "Synthetic movie layout/timing changed");
            bool equal = true;
            for (unsigned frame = 0; frame < 6; ++frame) {
                const double seconds = (frame + .1) / 30;
                software.advance(seconds); automatic.advance(seconds);
                require(software.frame_number() == frame + 1 && automatic.frame_number() == frame + 1,
                        "Production movie timing lost a frame");
                equal &= software.image().pixels == automatic.image().pixels;
            }
            const auto audio = decode_movie_audio(path);
            require(audio.rate == 48000 && audio.frames() >= 9600 && audio.frames() <= 11000 &&
                std::all_of(audio.stereo.begin(), audio.stereo.end(), [](float x) { return std::isfinite(x); }) &&
                std::any_of(audio.stereo.begin(), audio.stereo.end(), [](float x) { return std::abs(x) > .01f; }),
                "Production AAC movie audio failed");
            report["movies"][name] = {{"frames", software.frame_count()}, {"audio_frames", audio.frames()},
                {"automatic_device", automatic.decode_device()}, {"hardware_matches_software_pixels", equal}};
            if (!equal) {
                report["movies"][name]["error"] = "Automatic movie pixels differ from software decoding";
                passed = false;
            }
          } catch (const std::exception& error) {
            report["movies"][name]["error"] = error.what();
            passed = false;
          }
        }
        report["passed"] = passed;
        std::cout << report.dump() << '\n';
        return passed ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
