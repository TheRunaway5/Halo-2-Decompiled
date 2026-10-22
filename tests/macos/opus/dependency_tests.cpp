#include "halo2/mcc_audio.hpp"
#include <algorithm>
#include <cmath>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opus/opus.h>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void packets(unsigned channels){
    int error{};
    std::unique_ptr<OpusEncoder,decltype(&opus_encoder_destroy)> encoder(
        opus_encoder_create(48000,int(channels),OPUS_APPLICATION_RESTRICTED_LOWDELAY,&error),opus_encoder_destroy);
    check(encoder && error==OPUS_OK,"Native Opus encoder allocation failed");
    check(opus_encoder_ctl(encoder.get(),OPUS_SET_BITRATE(128000))==OPUS_OK,"Opus bitrate setup failed");
    EncodedSound sound;sound.name="Mac Opus dependency";sound.sound_class=5;
    sound.codec=5;sound.channels=channels;sound.sample_rate=48000;sound.samples=960;
    for(unsigned block=0;block<4;++block){
        std::vector<opus_int16> input(240*channels);
        for(unsigned frame=0;frame<240;++frame)for(unsigned channel=0;channel<channels;++channel)
            input[frame*channels+channel]=opus_int16((int((frame+block*240)*(channel+3)%127)-63)*180);
        unsigned char packet[1275];const auto bytes=opus_encode(encoder.get(),input.data(),240,packet,sizeof(packet));
        check(bytes>0 && opus_packet_get_nb_samples(packet,bytes,48000)==240,"Opus did not encode the actual 5ms packet");
        const auto length=std::uint16_t(block==3?-bytes:bytes);
        sound.data.push_back(std::byte(length));sound.data.push_back(std::byte(length>>8));
        for(int i=0;i<bytes;++i)sound.data.push_back(std::byte(packet[i]));
    }
    sound.packet_chunks.push_back({unsigned(sound.data.size()),960*channels*2,true});
    const auto decoded=decode_mcc_opus(sound);
    check(decoded.frames()==960 && decoded.name==sound.name && decoded.sound_class==5,
          "Production MCC decoder lost native Opus duration/identity");
    check(std::ranges::all_of(decoded.stereo,[](float sample){return std::isfinite(sample)&&std::abs(sample)<=1;}),
          "Native Opus PCM is not finite normalized output");
    check(std::ranges::any_of(decoded.stereo,[](float sample){return sample!=0;}),"Nonzero Opus input became silence");
    bool distinct=false;
    for(unsigned i=0;i<960;++i)distinct|=decoded.stereo[i*2]!=decoded.stereo[i*2+1];
    check(distinct==(channels==2),"Production MCC mono duplication/stereo mapping differs");
}
}
int main(int argc,char **argv)try{
    check(argc==2,"Supply the expected SDK Opus library");
    Dl_info loaded{};check(::dladdr(reinterpret_cast<void*>(&opus_decoder_create),&loaded)!=0 && loaded.dli_fname,
                          "Cannot identify the loaded Opus decoder");
    check(std::filesystem::equivalent(loaded.dli_fname,argv[1]),"MCC decode loaded a different Opus library");
    check(std::string(opus_get_version_string()).starts_with("libopus 1.6.1"),"Pinned Opus runtime version differs");
    packets(1);packets(2);
    std::cout<<"PASS actual SDK Opus runtime "<<loaded.dli_fname<<"; nonzero mono/stereo 5ms packets through production MCC framing/decode\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
