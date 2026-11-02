#include "halo2/mcc_audio.hpp"
#include <algorithm>
#include <iostream>
#include <array>
#include <bit>
#include <memory>
#include <opus/opus.h>

using namespace halo2;
namespace {
void check(bool v,const char* text){if(!v)throw std::runtime_error(text);}
template<class F> void rejects(F f){try{f();}catch(const FormatError&){return;}throw std::runtime_error("Invalid MCC packet admitted");}
EncodedSound sound(unsigned channels=1) {
    EncodedSound s;s.name="fixture";s.codec=5;s.channels=channels;s.sample_rate=48000;s.samples=480;s.sound_class=5;
    // Two valid 5ms Opus silence packets; signed final length is -3.
    for(unsigned v:{3,0,0xe8,0xff,0xfe,0xfd,0xff,0xe8,0xff,0xfe})s.data.push_back(std::byte(v));
    s.packet_chunks.push_back({10,480*channels*2,true});return s;
}
// Exercise actual compressed, non-silent packets. The reference models the
// original per-buffer submission length, independently of the native chunk
// framing/rounding loop. Decoder state still consumes the full padded packet.
void submitted_chunks(unsigned channels,bool reset,unsigned tail) {
    int error{};
    std::unique_ptr<OpusEncoder,decltype(&opus_encoder_destroy)> encoder(
        opus_encoder_create(48000,int(channels),OPUS_APPLICATION_AUDIO,&error),opus_encoder_destroy);
    check(error==OPUS_OK && bool(encoder),"Cannot create reference encoder");
    std::unique_ptr<OpusDecoder,decltype(&opus_decoder_destroy)> decoder(
        opus_decoder_create(48000,int(channels),&error),opus_decoder_destroy);
    check(error==OPUS_OK && bool(decoder),"Cannot create reference decoder");
    EncodedSound s;s.name="submitted chunks";s.codec=5;s.channels=channels;s.sample_rate=48000;s.sound_class=5;
    std::vector<float> reference;
    constexpr std::array<unsigned,4> logical{31,239,1,173};
    unsigned seed=0x6172;
    for(unsigned block=0;block<logical.size();++block) {
        std::array<opus_int16,480> input{},pcm{};
        for(unsigned i=0;i<240*channels;++i) {
            seed=seed*1664525u+1013904223u;
            input[i]=opus_int16(int((seed>>16)&0x7fff)-16384);
        }
        std::array<unsigned char,1500> packet{};
        const auto bytes=opus_encode(encoder.get(),input.data(),240,packet.data(),int(packet.size()));
        check(bytes>0 && bytes<32768,"Reference Opus packet encoding failed");
        const auto frames=opus_decode(decoder.get(),packet.data(),bytes,pcm.data(),240,0);
        check(frames==240,"Reference packet duration changed");
        const auto header=std::bit_cast<std::uint16_t>(std::int16_t(-bytes));
        s.data.push_back(std::byte(header&255));s.data.push_back(std::byte(header>>8));
        for(int i=0;i<bytes;++i)s.data.push_back(std::byte(packet[i]));
        s.packet_chunks.push_back({unsigned(bytes+2),logical[block]*channels*2,reset});
        s.samples+=logical[block];
        for(unsigned frame=0;frame<logical[block];++frame) {
            reference.push_back(float(pcm[frame*channels])/32768.f);
            reference.push_back(float(pcm[frame*channels+channels-1])/32768.f);
        }
        if(reset)check(opus_decoder_ctl(decoder.get(),OPUS_RESET_STATE)==OPUS_OK,"Reference reset failed");
    }
    check(tail<s.samples,"Invalid reference tail");s.samples-=tail;reference.resize(std::size_t(s.samples)*2);
    const auto actual=decode_mcc_opus(s);
    check(actual.stereo==reference,"Logical chunk submission leaked padding or changed decoder state");
    std::vector<PcmSound::ChunkBoundary> boundaries;std::size_t bytes{},frames{};
    for(const auto& c:s.packet_chunks){bytes+=c.bytes;frames+=c.pcm_bytes/(channels*2);
        boundaries.push_back({bytes,std::min(frames,std::size_t(s.samples))});}
    check(actual.chunk_boundaries==boundaries,"MCC emitted chunk map lost padding trim or final sample extent");
    check(std::ranges::any_of(reference,[](float v){return v!=0;}),"Chunk reference lost non-silent witness");
    auto short_declaration=s;short_declaration.samples=31+239+1+174;
    rejects([&]{decode_mcc_opus(short_declaration);});
}
}
int main(){try{
    for(unsigned channels:{1,2}) {
        const auto s=sound(channels);const auto pcm=decode_mcc_opus(s);
        check(pcm.frames()==480 && pcm.sound_class==5 && pcm.name=="fixture","MCC decoded duration/identity changed");
        check(std::ranges::all_of(pcm.stereo,[](float v){return v==0;}),"MCC silence decoded incorrectly");
    }
    for(unsigned channels:{1,2})for(bool reset:{false,true})for(unsigned tail:{0u,17u})
        submitted_chunks(channels,reset,tail);
    auto s=sound();s.samples=479;check(decode_mcc_opus(s).frames()==479,"Authored PCM tail was not retained");
    s=sound();s.data[0]=std::byte{0xfd};s.data[1]=std::byte{0xff};rejects([&]{decode_mcc_opus(s);});
    s=sound();s.data[5]=std::byte{4};s.data[6]=std::byte{0};rejects([&]{decode_mcc_opus(s);});
    s=sound();s.packet_chunks[0].pcm_bytes=480;rejects([&]{decode_mcc_opus(s);});
    s=sound();s.data.pop_back();rejects([&]{decode_mcc_opus(s);});
    s=sound();s.packet_chunks.clear();rejects([&]{decode_mcc_opus(s);});
    s=sound();s.sample_rate=44100;rejects([&]{decode_mcc_opus(s);});
    std::cout<<"MCC signed Opus framing, eight non-silent chunk submission/state cases, stereo/mono, tail and extent checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
