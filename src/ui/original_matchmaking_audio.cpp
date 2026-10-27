#include "halo2/original_matchmaking_audio.hpp"
#include <array>
#include <openssl/evp.h>
#include <string_view>
#include <zlib.h>

namespace halo2 {
namespace {
struct OriginalCountdownAudioRecord {
    unsigned slot,rate,channels,codec,samples,sound_class;
    std::size_t offset,bytes;
    std::string_view name;
};
#include "ui/original_matchmaking_audio.inc"

const std::array<EncodedSound,2>& original_sounds() {
    static const auto sounds=[] {
        static_assert(original_countdown_audio_bytes==35028);
        static_assert(original_countdown_audio_compressed_bytes<=24000);
        const auto digit=[](char c)->unsigned {
            if(c>='0' && c<='9')return unsigned(c-'0');
            if(c>='a' && c<='f')return unsigned(c-'a'+10);
            throw FormatError("Invalid original countdown audio encoding");
        };
        std::vector<unsigned char> compressed;
        compressed.reserve(original_countdown_audio_compressed_bytes);
        for(const auto chunk:original_countdown_audio_chunks) {
            if(chunk.size()%2 || chunk.size()>4096 ||
               chunk.size()/2>original_countdown_audio_compressed_bytes-compressed.size())
                throw FormatError("Invalid original countdown audio chunk");
            for(std::size_t i=0;i<chunk.size();i+=2)
                compressed.push_back(static_cast<unsigned char>((digit(chunk[i])<<4)|digit(chunk[i+1])));
        }
        if(compressed.size()!=original_countdown_audio_compressed_bytes)
            throw FormatError("Truncated original countdown audio payload");
        std::vector<std::byte> bytes(original_countdown_audio_bytes);
        uLongf length=bytes.size();
        if(uncompress(reinterpret_cast<Bytef*>(bytes.data()),&length,compressed.data(),compressed.size())!=Z_OK || length!=bytes.size())
            throw FormatError("Invalid original countdown audio compression");
        std::array<unsigned char,32> digest{};unsigned count{};
        if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&count,EVP_sha256(),nullptr)!=1 ||
           count!=digest.size() || original_countdown_audio_sha256.size()!=digest.size()*2)
            throw FormatError("Cannot authenticate original countdown audio");
        constexpr char hex[]="0123456789abcdef";
        for(std::size_t i=0;i<digest.size();++i)
            if(original_countdown_audio_sha256[i*2]!=hex[digest[i]>>4] ||
               original_countdown_audio_sha256[i*2+1]!=hex[digest[i]&15])
                throw FormatError("Original countdown audio payload changed");
        std::array<EncodedSound,2> result;
        for(unsigned i=0;i<result.size();++i) {
            const auto &source=original_countdown_audio_records[i];
            if(source.slot!=(i?12u:10u) || source.rate!=44100 || source.channels!=1 || source.codec!=1 ||
               source.sound_class!=50 || source.offset>bytes.size() || source.bytes>bytes.size()-source.offset ||
               source.samples!=source.bytes)
                throw FormatError("Original countdown audio source descriptor changed");
            auto &sound=result[i];sound.name=source.name;sound.sample_rate=source.rate;
            sound.channels=source.channels;sound.codec=source.codec;sound.samples=source.samples;
            sound.sound_class=source.sound_class;
            sound.data.assign(bytes.begin()+source.offset,bytes.begin()+source.offset+source.bytes);
        }
        return result;
    }();
    return sounds;
}
} // namespace

bool original_matchmaking_audio_slot(unsigned slot) noexcept {return slot==10 || slot==12;}
EncodedSound load_original_matchmaking_audio(unsigned slot) {
    if(!original_matchmaking_audio_slot(slot))throw FormatError("Unknown original countdown sound role");
    return original_sounds()[slot==12];
}
} // namespace halo2
