#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
constexpr std::uint32_t sound_id=0xe2000002u;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(Buffer& b,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(8*i));}
void half(Buffer& b,std::size_t at,std::uint16_t value){for(unsigned i=0;i<2;++i)b.at(at+i)=std::byte(value>>(8*i));}
void save(const std::filesystem::path& p,const Buffer& b){std::ofstream o(p,std::ios::binary);o.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));check(bool(o),"Fixture write failed");}
template<class F>void rejects(F f){try{f();}catch(const FormatError&){return;}throw std::runtime_error("Malformed MCC codec metadata admitted");}
Buffer fixture(unsigned rate,unsigned codec_index,unsigned channels) {
  Buffer b(8192);constexpr auto magic=0x100000u-4096;
  word(b,0,0x68656164);word(b,4,13);word(b,8,b.size());word(b,16,1024);word(b,32,4);
  word(b,36,2048);word(b,40,512);word(b,44,2304);word(b,720,1024+magic);
  word(b,1024,0x400000+1056);word(b,1028,4);word(b,1032,0x400000+1104);
  word(b,1036,0xffffffffu);word(b,1040,0xe0000000u);word(b,1048,4);word(b,1052,0x74616773);
  constexpr std::uint32_t groups[]{0x6d617467,0x736e636c,0x736e6421,0x75676821};
  constexpr std::uint32_t addresses[]{0x100000,0x100240,0x100300,0x100340};
  constexpr std::uint32_t sizes[]{0x22c,0x80,0x1c,0x400};
  for(unsigned i=0;i<4;++i){
    word(b,1056+i*12,groups[i]);word(b,1060+i*12,0xffffffffu);word(b,1064+i*12,0xffffffffu);
    word(b,1104+i*16,groups[i]);word(b,1108+i*16,0xe0000000u+i*0x1000001u);
    word(b,1112+i*16,addresses[i]);word(b,1116+i*16,sizes[i]);word(b,2304+i*4,i*64);
    const auto name="fixture\\"+std::to_string(i);for(unsigned n=0;n<name.size();++n)b[2048+i*64+n]=std::byte(name[n]);
  }
  auto block=[&](std::size_t at,unsigned count,std::uint32_t address){word(b,4096+at,count);word(b,4100+at,address);};
  block(0xc0,1,0x100200);word(b,4096+0x204,0xe1000001u);word(b,4096+0x20c,0xe1000001u);
  block(0x240,1,0x100248);b[4096+0x248+10]=std::byte{0x20};
  b[4096+0x303]=std::byte(codec_index);b[4096+0x30a]=std::byte{1};
  block(0x340,8,0x1003b8);
  for(unsigned i=0;i<8;++i){b[4096+0x3b8+i*3]=std::byte(rate);b[4096+0x3b9+i*3]=std::byte(channels==1?0:1);b[4096+0x3ba+i*3]=std::byte{3};}
  block(0x348,1,0x100400);block(0x350,1,0x100440);block(0x360,1,0x100450);
  block(0x368,1,0x100460);block(0x370,1,0x100470);block(0x3a0,1,0x100490);
  half(b,4096+0x418,200);half(b,4096+0x41a,200);
  half(b,4096+0x456,0x8000);half(b,4096+0x458,0x7fff);
  b[4096+0x464]=std::byte{0xff};b[4096+0x465]=std::byte{0xff};half(b,4096+0x46a,1);
  block(0x478,1,0x100480);word(b,4096+0x480,480*channels);half(b,4096+0x486,1);
  block(0x490,1,0x100498);word(b,4096+0x49c,480*channels);word(b,4096+0x4a0,480*channels);
  return b;
}
Buffer pcm_bytes(unsigned channels){Buffer b(480*channels);for(unsigned frame=0;frame<240;++frame)for(unsigned c=0;c<channels;++c)half(b,(frame*channels+c)*2,std::uint16_t(std::int16_t((int(frame%31)-15)*1000+int(c)*100)));return b;}
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}};
std::vector<float> expected_pcm(const Cache& cache,unsigned category,const PcmSound& pcm) {
  const auto constants=*cache.original_data()->sound_driver_parameter_constants;
  const auto pitch=source_sound_driver_pitch(category,std::bit_cast<std::uint32_t>(200.f),constants,{0x027f,0x1f80});
  const auto relative=source_sound_relative_rate(pitch,category,constants,{0x027f,0x1f80});
  SdlSoundPlatform platform(manual);const auto voice=platform.create_voice(sound_id,{relative,0,false,{{0,0,1},{1,1,1}}});
  const auto samples=std::make_shared<const std::vector<float>>(pcm.stereo);
  check(platform.submit(voice,{samples,0,pcm.frames(),1})==SdlSoundPlatform::Submission::Accepted,"Expected packet rejected");
  return platform.render(64);
}
}
int main(int argc,char** argv){
  const auto root=std::filesystem::temp_directory_path()/("halo2-mcc-rate-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(root);unsigned cases=0;
    constexpr unsigned rates[]{22050,44100,32000,48000};
    for(unsigned category=0;category<4;++category)for(unsigned channels:{1,2}){
      std::vector<float> first;save(root/"sounds_neutral.dat",pcm_bytes(channels));
      for(unsigned index:{0,1,2,4,6,7}){
        const auto bytes=fixture(category,index,channels);save(root/"shared.map",bytes);Cache cache(root/"shared.map");
        const auto definition=cache.load_sound_variation(sound_id);
        check(definition.sample_rate_index==category && std::to_integer<unsigned>(definition.sound[3])==index,"MCC codec index was not resolved without changing the raw record");
        check(std::ranges::equal(definition.sound,std::span(bytes).subspan(4096+0x300,20)),"MCC raw snd! metadata changed");
        const auto encoded=cache.load_sound(sound_id);check(encoded.sample_rate==rates[category] && encoded.channels==channels && encoded.codec==3,"MCC codec format changed");
        const auto pcm=decode_sound(encoded);check(pcm.rate==48000 && pcm.frames()>64,"Decoded PCM is not the adapter's 48 kHz domain");
        MultiplayerAudio audio(cache,0x12345678,manual);audio.impulse(sound_id);const auto rendered=audio.render(64);
        check(rendered==expected_pcm(cache,category,pcm),"MCC rate category/pitch was not consumed exactly once by the PCM adapter");
        if(first.empty())first=rendered;else check(first==rendered,"Equivalent MCC codec rows changed playback pitch");
        check(std::ranges::any_of(rendered,[](float x){return x!=0;}),"Regression fixture produced no signal");++cases;
      }
    }
    auto bad=fixture(1,8,1);save(root/"shared.map",bad);{Cache cache(root/"shared.map");rejects([&]{(void)cache.load_sound_variation(sound_id);});}
    bad=fixture(4,0,1);save(root/"shared.map",bad);{Cache cache(root/"shared.map");rejects([&]{(void)cache.load_sound_variation(sound_id);});}
    bad=fixture(1,0,1);word(bad,4096+0x340,0);save(root/"shared.map",bad);{Cache cache(root/"shared.map");rejects([&]{(void)cache.load_sound_variation(sound_id);});}
    unsigned imported=0,opus=0;
    for(int a=1;a<argc;++a){
      Cache cache(argv[a]);
      for(const auto& tag:cache.tags()){
        unsigned category;
        if(tag.group!="snd!" || !tag.address)continue;
        if(tag.name=="sound\\weapons\\battle_rifle\\zoom_in" ||
           tag.name=="sound\\ui\\teleport_loop\\teleporter_loop\\loop" ||
           tag.name=="sound\\ui\\main_menu_music\\menu_music\\in" ||
           tag.name=="sound\\ambience\\sound_scenery\\lockout_offinthecanyon\\details\\lockout_offinthecanyon" ||
           tag.name=="sound\\ambience\\lockout\\particulate_wind\\details\\particulate_wind")category=1;
        else if(tag.name=="sound\\characters\\masterchief\\mc_warthog_p_exit" ||
                tag.name=="sound_remastered\\ui\\shield_pop")category=3;
        else continue;
        const auto definition=cache.load_sound_variation(tag.datum);
        const auto encoded=cache.load_sound(tag.datum);
        check(definition.sample_rate_index==category && encoded.sample_rate==rates[category],"Imported MCC semantic rate differs from its codec owner");
        if(encoded.codec==5){const auto decoded=decode_sound(encoded);check(decoded.rate==48000 && decoded.frames()>0,"Imported Opus did not retain the native PCM rate");++opus;}
        ++imported;
      }
    }
    if(argc>1)check(imported>=unsigned(argc-1),"Imported MCC fixture did not exercise any selected sounds");
    std::filesystem::remove_all(root);std::cout<<cases<<" MCC mono/stereo codec-row, semantic-rate and native PCM pitch cases; 3 malformed metadata controls; "<<imported<<" imported sound owners/"<<opus<<" actual Opus decodes passed\n";return 0;
  }catch(const std::exception& e){std::filesystem::remove_all(root);std::cerr<<e.what()<<'\n';return 1;}
}
