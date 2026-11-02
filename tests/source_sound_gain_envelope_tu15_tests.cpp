#include "halo2/source_sound_gain_envelope_tu15.hpp"
#include "halo2/sound_variation.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
int main(int argc,char** argv)try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 using Json=nlohmann::json;
 const auto need=[](bool value,const char* why){if(!value)throw FormatError(why);};
 need(argc==2,"Supply original gain-envelope reference");const auto fixture=Json::parse(std::ifstream(argv[1]));
 need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
  fixture.at("every_executed_site_has_rooted_provenance").get<bool>() && fixture.at("source_instruction_patches").empty() &&
  fixture.at("substituted_calls").empty(),"Gain envelope reference authority differs");
 const auto metadata=[](const Json& row){const auto& m=row.at("metadata");return SourceSoundGainEnvelopeTu15{
  m.at("class_lower"),m.at("class_upper"),m.at("scale_lower"),m.at("scale_upper")};};
 unsigned scalar{},combined{},adapter{};
 for(const auto& row:fixture.at("scalar")){
  const auto m=metadata(row);const SoundDriverFloatingPointContext fp{row.at("fp_control"),row.at("mxcsr")};
  std::uint16_t before,after;asm volatile("fnstcw %0":"=m"(before));const auto csr=_mm_getcsr();
  const auto value=source_sound_gain_envelope_tu15(m,row.at("control"),fp);asm volatile("fnstcw %0":"=m"(after));
  need(before==after && csr==_mm_getcsr(),"Gain envelope leaked caller FP controls/status");
  if(value!=row.at("expected"))throw FormatError("Native gain envelope differs at "+std::to_string(scalar)+
   " expected="+row.at("expected").dump()+" actual="+Json(value).dump());
  if(fp.x87_control_word==0x027f){
   SoundVariationDefinition definition;
   const auto put=[](auto& b,unsigned at,std::uint32_t word){for(unsigned n=0;n<4;++n)b[at+n]=std::byte(word>>(8*n));};
   put(definition.sound_class,0x20,m.class_lower);put(definition.sound_class,0x24,m.class_upper);
   put(definition.scale,0,m.scale_lower);put(definition.scale,4,m.scale_upper);
   need(std::bit_cast<std::uint32_t>(source_sound_control_gain(definition,std::bit_cast<float>(row.at("control").get<std::uint32_t>())))==value,
    "Production sound metadata adapter differs from original gain envelope");++adapter;
  }++scalar;
 }
 for(const auto& row:fixture.at("combined")){
  const auto result=source_loop_combined_gain_tu15(row.at("amplitudes").get<std::array<std::uint32_t,4>>(),metadata(row),
   row.at("control"),row.at("parent_gain"),row.at("ducked_class_gain"),{row.at("fp_control"),row.at("mxcsr")});
  if(result!=row.at("expected").get<std::array<std::uint32_t,4>>())
   throw FormatError("Native four-bin gain publication differs at "+std::to_string(combined));++combined;
 }
 need(scalar==768 && combined==120 && adapter==192,"Original gain envelope coverage differs");
 std::cout<<"PASS "<<scalar<<" original gain envelopes, "<<adapter<<" actual production metadata calls and "<<combined
  <<" four-bin publications; independent source gains retained; caller FP restored\n";return 0;
#else
 (void)argc;(void)argv;return 77;
#endif
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
