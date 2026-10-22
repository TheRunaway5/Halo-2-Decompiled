#include "halo2/sound_gp_image_tu15.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <limits>
#include <openssl/evp.h>
#include <span>
#include <stdexcept>
#include <string>

namespace halo2 {namespace {
using Bytes=std::span<const std::byte>;
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(Bytes bytes,std::size_t at) {
  require(at<=bytes.size()&&bytes.size()-at>=4,"Truncated TU1.5 sound image word");
  std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[at+i])<<(8*i);return value;
}
Bytes range(Bytes bytes,std::size_t at,std::size_t count) {
  require(at<=bytes.size()&&count<=bytes.size()-at,"TU1.5 sound image extent is outside its backing");return bytes.subspan(at,count);
}
std::vector<std::uint32_t> words(Bytes bytes) {
  require(bytes.size()%4==0,"Unaligned TU1.5 DSP word buffer");std::vector<std::uint32_t> out;
  out.reserve(bytes.size()/4);for(std::size_t i=0;i<bytes.size();i+=4)out.push_back(word(bytes,i));return out;
}
struct Section {std::uint32_t va,extent,raw,length,flags,name;};
Bytes section_bytes(Bytes image,const Section &section) {
  require(section.length<=section.extent,"TU1.5 section raw size exceeds its virtual extent");return range(image,section.raw,section.length);
}
Bytes virtual_bytes(Bytes image,std::span<const Section> sections,std::uint32_t address,std::size_t count) {
  for(const auto &s:sections)if(address>=s.va&&std::uint64_t(address)-s.va+count<=s.length)
    return range(image,std::uint64_t(s.raw)+address-s.va,count);
  throw std::runtime_error("Required original sound bytes have no file-backed section");
}
std::string name_at(Bytes image,std::uint32_t base,std::uint32_t address) {
  require(address>=base,"TU1.5 section name precedes its image");std::string out;
  for(std::size_t i=address-base;i<image.size();++i){const auto c=std::to_integer<unsigned>(image[i]);if(!c)return out;require(c<128,"Invalid TU1.5 section name");out+=char(c);}
  throw std::runtime_error("Unterminated TU1.5 section name");
}
void authenticate(Bytes image) {
  constexpr char pin[]="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd";
  std::array<unsigned char,32> digest{};unsigned length{};
  require(EVP_Digest(image.data(),image.size(),digest.data(),&length,EVP_sha256(),nullptr)==1&&length==digest.size(),"Cannot authenticate owned sound executable");
  const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
  for(unsigned i=0;i<digest.size();++i)require(digest[i]==((digit(pin[i*2])<<4)|digit(pin[i*2+1])),"GP image preparation requires the authenticated TU1.5 executable");
}
struct DecoderState {
  std::uint32_t low=0x49de12ba,high=0x7fa49bca;
  // 38DF59's defined output words; discarded high SHLD work does not affect
  // these values. Every shift/XOR retains the source's wrapped 32-bit width.
  void advance() {
    const auto feedback=(((((low<<2)^low)<<1)^low)<<28)^high;
    const auto next_low=(low>>1)|(high<<31);
    high=(high>>1)|(feedback&0x80000000u);low=next_low;
  }
  std::array<std::uint8_t,8> bytes()const {
    std::array<std::uint8_t,8> result{};for(unsigned i=0;i<4;++i){result[i]=std::uint8_t(low>>(8*i));result[i+4]=std::uint8_t(high>>(8*i));}return result;
  }
};
DecoderState initialized_decoder() {
  DecoderState state;state.advance();for(unsigned i=0;i<80;++i)state.advance();return state;
}
void decode(std::span<std::byte> bytes,const std::array<std::uint8_t,8> &key) {
  auto state=initialized_decoder();const auto seed=state.bytes();state.advance();
  for(std::size_t i=0;i<bytes.size();++i) {
    const auto phase=i&7u;
    // F6 EB multiplies signed AL/BL. Only AL is consumed by this byte API;
    // the original full-register ABI remains in the generated CPU module.
    const auto product=std::bit_cast<std::int8_t>(key[phase])*std::bit_cast<std::int8_t>(seed[phase]);
    const auto difference=std::uint8_t(std::to_integer<unsigned>(bytes[i])-std::uint8_t(product));
    state.advance();bytes[i]=std::byte(seed[phase]^difference^std::uint8_t(state.low));
  }
}
} // namespace

SoundGpImageTu15 load_sound_gp_image_tu15(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file)&&file.tellg()>=0&&file.tellg()<=16*1024*1024,"Owned TU1.5 sound executable is unavailable");
  std::vector<std::byte> image(std::size_t(file.tellg()));file.seekg(0);
  require(bool(file.read(reinterpret_cast<char*>(image.data()),image.size())),"Truncated TU1.5 sound executable");authenticate(image);
  require(word(image,0)==0x48454258,"Invalid TU1.5 executable header");
  const auto base=word(image,0x104),count=word(image,0x11c),table=word(image,0x120);
  require(table>=base&&count<=128,"Invalid TU1.5 sound section table");std::vector<Section> sections;
  for(unsigned i=0;i<count;++i){const auto at=std::uint64_t(table)-base+std::uint64_t(i)*56;
    sections.push_back({word(image,at+4),word(image,at+8),word(image,at+12),word(image,at+16),word(image,at),word(image,at+20)});}
  Bytes resource;for(const auto &s:sections)if(name_at(image,base,s.name)=="DSPImage") {
    require(resource.empty(),"Ambiguous TU1.5 DSPImage resource");require((s.flags&2u)&&!(s.flags&1u),"TU1.5 DSPImage lacks its original read-only/preload flags");resource=section_bytes(image,s);
  }
  require(!resource.empty(),"Owned TU1.5 DSPImage is absent");SoundGpImageTu15 out{};
  // Original embedded GP control bytes, independently matched against the
  // actual uploaded control program. Their JSR operand locates effect code.
  out.control_program=words(virtual_bytes(image,sections,0x390c18,1476));
  require(out.control_program.at(0x37)==0x0bf080,"Original GP effect-call opcode differs");
  out.program_base_word=out.control_program.at(0x38);
  require(out.control_program.at(0x88)==0x56f400,"Original GP data-base instruction differs");
  for(unsigned i=0;i<6;++i)out.header[i]=word(resource,0x800+i*4);
  const auto program_bytes=std::uint64_t(out.header[1])*4,data_bytes=std::uint64_t(out.header[3])*4;
  require(std::uint64_t(out.program_base_word)+out.header[1]<=0x1000,"Original GP program exceeds PRAM");
  const auto metadata=0x818+program_bytes+data_bytes;
  const auto effects=word(resource,metadata);require(effects==15,"Unexpected TU1.5 GP effect count");
  out.user_scratch_bytes=word(resource,metadata+4);
  const auto seeds=metadata+8+std::uint64_t(effects)*32;
  range(resource,seeds,std::size_t(effects)*8);
  out.decoded_resource.assign(resource.begin(),resource.end());
  decode(std::span(out.decoded_resource).subspan(seeds,effects*8),initialized_decoder().bytes());
  std::size_t cursor=0x818;out.data_base_word=out.control_program.at(0x89)+out.header[0];
  require(out.header[2]==0x818+program_bytes,"Original GP data-source offset differs");
  out.user_scratch_base=std::numeric_limits<std::uint32_t>::max();
  for(unsigned index=0;index<effects;++index) {
    SoundGpEffectImageTu15 effect{};for(unsigned j=0;j<8;++j)effect.descriptor[j]=word(resource,metadata+8+index*32+j*4);
    const auto &d=effect.descriptor;
    require(d[0]==cursor&&d[1]%4==0&&d[2]>=out.header[2]&&d[3]%4==0,"Original GP descriptor layout differs");
    require(cursor+d[1]<=0x818+program_bytes,"Original GP payload exceeds its program span");
    std::array<std::uint8_t,8> key{};for(unsigned j=0;j<8;++j)key[j]=std::to_integer<std::uint8_t>(out.decoded_resource[seeds+index*8+j]);
    decode(std::span(out.decoded_resource).subspan(cursor,d[1]),key);
    effect.program_word=out.program_base_word+std::uint32_t((cursor-0x818)/4);
    effect.state_word=out.data_base_word+(d[2]-out.header[2])/4;
    require(word(out.decoded_resource,cursor)==0x65f400&&word(out.decoded_resource,cursor+4)==effect.state_word,"Decoded GP prologue/state binding differs");
    require(std::uint64_t(d[2])+d[3]<=metadata,"Original GP state exceeds its data span");
    require(std::uint64_t(d[6])+d[7]<=0x100000,"Original GP scratch extent exceeds its supported image");
    out.user_scratch_base=std::min(out.user_scratch_base,d[6]);out.minimum_scratch_bytes=std::max(out.minimum_scratch_bytes,d[6]+d[7]);
    out.effects.push_back(effect);cursor+=d[1];
  }
  require(cursor+4==0x818+program_bytes&&word(resource,cursor)==0x0c,"Original GP program footer differs");
  require(out.minimum_scratch_bytes>=out.user_scratch_base&&out.minimum_scratch_bytes-out.user_scratch_base<=out.user_scratch_bytes,"Original GP user-scratch allocation is insufficient");
  out.effect_program=words(range(out.decoded_resource,0x818,program_bytes));
  out.initial_data=words(range(out.decoded_resource,out.header[2],data_bytes));
  require(std::ranges::all_of(out.effect_program,[](auto v){return v<=0xffffff;}),"Decoded GP program exceeds its 24-bit words");
  require(std::ranges::all_of(out.initial_data,[](auto v){return v<=0xffffff;}),"Initial GP data exceeds its 24-bit words");
  return out;
}
} // namespace halo2
