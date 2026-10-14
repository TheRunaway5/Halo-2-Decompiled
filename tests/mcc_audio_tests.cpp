#include "halo2/mcc_audio.hpp"
#include <algorithm>
#include <iostream>

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
}
int main(){try{
    for(unsigned channels:{1,2}) {
        const auto s=sound(channels);const auto pcm=decode_mcc_opus(s);
        check(pcm.frames()==480 && pcm.sound_class==5 && pcm.name=="fixture","MCC decoded duration/identity changed");
        check(std::ranges::all_of(pcm.stereo,[](float v){return v==0;}),"MCC silence decoded incorrectly");
    }
    auto s=sound();s.samples=479;check(decode_mcc_opus(s).frames()==479,"Authored PCM tail was not retained");
    s=sound();s.data[0]=std::byte{0xfd};s.data[1]=std::byte{0xff};rejects([&]{decode_mcc_opus(s);});
    s=sound();s.data[5]=std::byte{4};s.data[6]=std::byte{0};rejects([&]{decode_mcc_opus(s);});
    s=sound();s.packet_chunks[0].pcm_bytes=480;rejects([&]{decode_mcc_opus(s);});
    s=sound();s.data.pop_back();rejects([&]{decode_mcc_opus(s);});
    s=sound();s.packet_chunks.clear();rejects([&]{decode_mcc_opus(s);});
    s=sound();s.sample_rate=44100;rejects([&]{decode_mcc_opus(s);});
    std::cout<<"MCC signed Opus framing, stereo/mono, tail and extent checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
