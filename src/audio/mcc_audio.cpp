#include "halo2/mcc_audio.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <opus/opus.h>

namespace halo2 {
PcmSound decode_mcc_opus(const EncodedSound& input) {
    if(input.codec!=5 || input.sample_rate!=48000 || !input.channels || input.channels>2 ||
       !input.samples || input.samples>48000u*600u || input.packet_chunks.empty())
        throw FormatError("Invalid MCC Opus sound format/extent");
    int error{};
    std::unique_ptr<OpusDecoder,decltype(&opus_decoder_destroy)> decoder(
        opus_decoder_create(48000,int(input.channels),&error),opus_decoder_destroy);
    if(error!=OPUS_OK || !decoder)throw FormatError("Cannot create MCC Opus decoder");
    PcmSound out;out.name=input.name;out.sound_class=input.sound_class;
    std::array<opus_int16,480> pcm{};
    std::size_t at=0;
    for(const auto& chunk:input.packet_chunks) {
        if(!chunk.bytes || !chunk.pcm_bytes || chunk.pcm_bytes%(input.channels*2))
            throw FormatError("Invalid MCC Opus chunk PCM extent");
        if(at>input.data.size() || chunk.bytes>input.data.size()-at)
            throw FormatError("MCC Opus chunk escapes its owned bytes");
        const auto end=at+chunk.bytes;
        const auto unit=input.channels*480u;
        // Original 769480 rounds a chunk's PCM allocation up to a 5ms block.
        const auto expected=(std::uint64_t(chunk.pcm_bytes)+unit-1)/unit*unit;
        if(expected>128u*1024u*1024u)throw FormatError("Excessive MCC Opus chunk allocation");
        std::size_t decoded=0;
        bool last=false;
        while(at<end) {
            if(last || end-at<2)throw FormatError("Trailing/truncated MCC Opus packet header");
            const auto length_word=std::uint16_t(std::to_integer<unsigned>(input.data[at])|
                (std::to_integer<unsigned>(input.data[at+1])<<8));
            const auto signed_length=std::bit_cast<std::int16_t>(length_word);at+=2;
            const auto length=unsigned(signed_length<0?-int(signed_length):signed_length);
            if(!length || length>end-at)throw FormatError("MCC Opus packet escapes chunk");
            const auto packet=reinterpret_cast<const unsigned char*>(input.data.data()+at);
            const auto frames=opus_decode(decoder.get(),packet,int(length),pcm.data(),240,0);
            if(frames<0)throw FormatError(std::string("Invalid MCC Opus packet: ")+opus_strerror(frames));
            decoded+=std::size_t(frames)*input.channels*2;
            if(decoded>expected || out.frames()+unsigned(frames)>48000u*600u)
                throw FormatError("MCC Opus decoded extent exceeds declaration");
            for(int n=0;n<frames;++n) {
                out.stereo.push_back(float(pcm[std::size_t(n)*input.channels])/32768.f);
                out.stereo.push_back(float(pcm[std::size_t(n)*input.channels+input.channels-1])/32768.f);
            }
            at+=length;last=signed_length<0;
        }
        if(decoded!=expected)throw FormatError("MCC Opus chunk PCM extent differs from decoded packets");
        if(chunk.reset_after && opus_decoder_ctl(decoder.get(),OPUS_RESET_STATE)!=OPUS_OK)
            throw FormatError("Cannot release MCC Opus chunk state");
    }
    if(at!=input.data.size() || out.frames()<input.samples)
        throw FormatError("MCC Opus permutation extent differs from metadata");
    out.stereo.resize(std::size_t(input.samples)*2);
    return out;
}
}
