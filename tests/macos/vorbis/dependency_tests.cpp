#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Function>
void loaded_from(Function symbol, const std::filesystem::path& expected) {
    Dl_info info{};
    require(dladdr(reinterpret_cast<const void*>(symbol), &info) != 0 && info.dli_fname,
            "The actual codec image could not be identified");
    require(std::filesystem::equivalent(info.dli_fname, expected),
            "A codec loaded outside the supplied bundle");
    std::cout << "loaded " << info.dli_fname << '\n';
}
}

int main(int argc, char** argv) try {
    require(argc == 2, "Supply the actual bundle Frameworks directory");
    const std::filesystem::path directory(argv[1]);
    loaded_from(&oggpack_writeinit, directory / "libogg.0.dylib");
    loaded_from(&vorbis_info_init, directory / "libvorbis.0.dylib");
    loaded_from(&vorbis_encode_init_vbr, directory / "libvorbisenc.2.dylib");
    loaded_from(&ov_open_callbacks, directory / "libvorbisfile.3.dylib");

    oggpack_buffer writer{}, reader{};
    oggpack_writeinit(&writer);
    oggpack_write(&writer, 0x5a3, 11);
    oggpack_write(&writer, 0x1c, 5);
    oggpack_readinit(&reader, oggpack_get_buffer(&writer), oggpack_bytes(&writer));
    const bool bits = oggpack_read(&reader, 11) == 0x5a3 && oggpack_read(&reader, 5) == 0x1c
        && oggpack_read(&reader, 1) == -1;
    oggpack_writeclear(&writer);
    require(bits, "The real Ogg bitstream ABI did not round trip");

    vorbis_info info{};
    vorbis_info_init(&info);
    const int encoded = vorbis_encode_init_vbr(&info, 2, 44100, 0.3f);
    const bool profile = encoded == 0 && info.channels == 2 && info.rate == 44100;
    vorbis_info_clear(&info);
    require(profile, "The real Vorbis encoder ABI did not admit the stereo profile");

    unsigned reads{};
    ov_callbacks callbacks{};
    callbacks.read_func = [](void*, std::size_t, std::size_t, void* owner) -> std::size_t {
        ++*static_cast<unsigned*>(owner);
        return 0;
    };
    OggVorbis_File file{};
    require(ov_open_callbacks(&reads, &file, nullptr, 0, callbacks) < 0 && reads > 0,
            "The real Vorbisfile callback ABI did not reject an empty input");
    require(std::string(vorbis_version_string()).find("1.3.7") != std::string::npos,
            "The loaded Vorbis version differs from the pinned source");
    std::cout << "PASS " << vorbis_version_string() << ": actual Ogg, Vorbis, encoder and file APIs\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
