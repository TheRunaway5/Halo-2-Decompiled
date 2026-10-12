#include "halo2/vista_audio_framing.hpp"
#include "halo2/audio.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const std::string& reason) {
  if(!value)throw FormatError(reason);
}
std::vector<std::string> split(const std::string& text,char delimiter) {
  std::vector<std::string> out;std::istringstream s(text);std::string part;
  while(std::getline(s,part,delimiter))out.push_back(part);
  return out;
}
unsigned digit(char c) {
  if(c>='0'&&c<='9')return unsigned(c-'0');
  if(c>='a'&&c<='f')return unsigned(c-'a'+10);
  throw FormatError("nonhex audio framing fixture");
}
std::vector<std::byte> unhex(const std::string& text) {
  if(text=="-")return {};
  check(text.size()%2==0,"odd audio framing fixture hex");
  std::vector<std::byte> out(text.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte((digit(text[i*2])<<4)|digit(text[i*2+1]));
  return out;
}
std::uint32_t u32(std::span<const std::byte> bytes,std::size_t at) {
  std::uint32_t n=0;
  for(unsigned i=0;i<4;++i)n|=std::to_integer<std::uint32_t>(bytes[at+i])<<(i*8);
  return n;
}
std::vector<std::byte> read_chunk(const std::filesystem::path& path,std::size_t at,std::size_t n) {
  std::ifstream s(path,std::ios::binary);check(bool(s),"open original PC shared map");
  s.seekg(std::streamoff(at));std::vector<std::byte> bytes(n);
  s.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(n));
  check(bool(s),"read original PC sound resource");return bytes;
}
template<class F> void reject(F fn,const char* reason) {
  bool rejected=false;try{fn();}catch(const FormatError&){rejected=true;}
  check(rejected,reason);
}
}
int main(int argc,char** argv)try {
  if(argc!=3){std::cerr<<"usage: vista_audio_framing_tests <original.tsv> <Vista shared.map>\n";return 2;}
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  std::ifstream input(argv[1]);std::string line;unsigned cases=0,accepted=0,rejected=0,stalled=0,decoded=0;
  check(bool(std::getline(input,line))&&line=="# H2V_AUDIO_FRAMING_ORIGINAL_V1","audio framing fixture identity");
  const std::array<std::string,3> names{"round_over","game_over","slayer"};
  const std::array<std::size_t,3> offsets{0x075EDE00,0x07692E00,0x07BFBC00};
  const std::array<std::size_t,3> lengths{22387,26846,17928};
  const std::array<std::size_t,3> counts{5,6,4};
  while(std::getline(input,line)) {
    if(line.empty()||line[0]=='#')continue;
    const auto f=split(line,'\t');check(f.size()==7,"audio framing fixture row width");
    const auto bytes=unhex(f[1]);const bool valid=f[2]=="1",nonprogress=f[3]=="1";
    ++cases;
    if(!valid||nonprogress) {
      reject([&]{(void)parse_vista_serialized_media(bytes);},"source invalid/nonprogress framing admitted");
      if(nonprogress)++stalled;else ++rejected;
      continue;
    }
    ++accepted;const auto media=parse_vista_serialized_media(bytes);
    const auto expected_media=unhex(f[4]),expected_format=unhex(f[5]);
    check(expected_media.size()==72,"missing original codec input media");
    check(std::equal(media.major_type.begin(),media.major_type.end(),expected_media.begin()),"major GUID mismatch");
    check(std::equal(media.subtype.begin(),media.subtype.end(),expected_media.begin()+16),"subtype GUID mismatch");
    check(std::equal(media.format_type.begin(),media.format_type.end(),expected_media.begin()+44),"format GUID mismatch");
    check(media.fixed_size_samples==u32(expected_media,32)&&
          media.temporal_compression==u32(expected_media,36)&&
          media.sample_size==u32(expected_media,40),"original media fields mismatch");
    check(u32(expected_media,60)==0&&u32(expected_media,64)==media.format.size()&&
          u32(expected_media,68)==64,"original media pointer reconstruction mismatch");
    check(std::equal(media.format.begin(),media.format.end(),expected_format.begin(),expected_format.end()),
          "original codec format bytes mismatch");
    const auto packets=f[6]=="-"?std::vector<std::string>{}:split(f[6],',');
    check(packets.size()==media.packets.size(),"original packet count mismatch");
    std::size_t extent=64+media.format.size();
    for(std::size_t i=0;i<packets.size();++i) {
      const auto packet=split(packets[i],':');check(packet.size()==3&&packet[2].size()==64,"packet fixture shape");
      check(media.packets[i].offset==std::stoull(packet[0])&&
            media.packets[i].bytes.size()==std::stoull(packet[1]),"original packet boundary mismatch");
      check(media.packets[i].offset==extent&&media.packets[i].bytes.data()==bytes.data()+extent,
            "packet input storage/order mismatch");
      extent+=media.packets[i].bytes.size();
    }
    check(extent==bytes.size(),"packet framing drops input payload");
    const auto actual=std::find(names.begin(),names.end(),f[0]);
    if(actual==names.end())continue;
    const auto index=std::size_t(actual-names.begin());
    check(bytes==read_chunk(argv[2],offsets[index],lengths[index]),"actual original resource differs from fixture");
    check(is_vista_serialized_audio(bytes)&&media.packets.size()==counts[index]&&
          media.format_tag==0x161&&media.channels==2&&media.sample_rate==44100&&
          media.average_bytes_per_second==12003&&media.block_align==4459&&
          media.bits_per_sample==16&&media.extra.size()==10,"actual PC announcer format mismatch");
    EncodedSound encoded;encoded.name="Vista/"+f[0];encoded.codec=4;
    encoded.sample_rate=media.sample_rate;encoded.channels=media.channels;
    encoded.sound_class=50;encoded.data=bytes;
    const auto pcm=decode_vista_serialized_sound(encoded);
    const auto repeated=decode_vista_serialized_sound(encoded);
    const auto routed=decode_sound(encoded);
    check(pcm.name==encoded.name&&pcm.sound_class==50&&pcm.rate==48000&&
          pcm.stereo.size()%2==0&&pcm.frames()>0&&pcm.frames()<48000ull*600&&
          pcm.stereo==repeated.stereo&&pcm.stereo==routed.stereo,
          "PC cue deterministic decode/metadata/routing mismatch");
    float peak=0;for(float sample:pcm.stereo) {
      check(std::isfinite(sample)&&std::abs(sample)<=1,"invalid PC decoded sample");peak=std::max(peak,std::abs(sample));
    }
    check(peak>0,"PC announcer decoded to silence");
    std::cout<<f[0]<<" packets="<<media.packets.size()<<" frames48k="<<pcm.frames()<<" peak="<<peak<<'\n';
    auto malformed=encoded;malformed.channels=1;
    reject([&]{(void)decode_vista_serialized_sound(malformed);},"mismatched PC channels admitted");
    malformed=encoded;malformed.sample_rate=48000;
    reject([&]{(void)decode_vista_serialized_sound(malformed);},"mismatched PC rate admitted");
    malformed=encoded;malformed.data[16]^=std::byte{1};
    reject([&]{(void)decode_vista_serialized_sound(malformed);},"unsupported PC codec subtype admitted");
    malformed=encoded;malformed.data[0]^=std::byte{1};
    check(!is_vista_serialized_audio(malformed.data),"unknown container recognized as PC audio");
    reject([&]{(void)decode_vista_serialized_sound(malformed);},"unknown container admitted to PC decoder");
    ++decoded;
  }
  check(cases==38&&decoded==3&&accepted>0&&rejected>0&&stalled==1,"original framing fixture coverage");
  const std::array<std::byte,16> asf{std::byte{0x30},std::byte{0x26},std::byte{0xb2},std::byte{0x75}};
  check(!is_vista_serialized_audio(asf),"Xbox ASF recognized as PC serialized media");
  std::cout<<"Vista original framing "<<cases<<" cases, "<<accepted<<" accepted, "<<rejected
           <<" invalid, "<<stalled<<" explicit nonprogress reject; "<<decoded<<" actual cues decoded\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
