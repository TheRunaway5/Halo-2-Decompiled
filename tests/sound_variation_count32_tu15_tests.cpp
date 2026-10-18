#include "halo2/sound_variation.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cfenv>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
#include <cstring>
#include <set>
using namespace halo2;
void need(bool b,const char*w){if(!b)throw FormatError(w);}
void put16(auto& a,unsigned at,unsigned n){a[at]=std::byte(n);a[at+1]=std::byte(n>>8);}
std::string hex(std::span<const std::byte>a){std::string s;for(auto b:a){auto n=std::to_integer<unsigned>(b);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
#if defined(__i386__) || defined(__x86_64__)
int main(int argc,char**argv)try{
 if(argc!=2)return 77;std::ifstream f(argv[1]);if(!f)return 77;nlohmann::json cases;f>>cases;
 unsigned total{},count32{},lower{},draws{},controls{};nlohmann::json mismatches=nlohmann::json::array();
 const auto set_cw=[](std::uint16_t cw){asm volatile("fldcw %0"::"m"(cw));};
 std::fenv_t saved;std::fegetenv(&saved);const unsigned saved_mxcsr=_mm_getcsr();
 for(const auto& c:cases){
  const int count=c.at("count");SoundVariationDefinition d;put16(d.range,10,unsigned(count));d.sound_class[10]=std::byte(c.at("class_flags10").get<unsigned>());d.permutations.resize(unsigned(count));
  for(unsigned n=0;n<d.permutations.size();++n)put16(d.permutations[n],2,c.at("chances")[n].get<unsigned>());
  SoundRangeState state{std::int8_t(c.at("last").get<int>()),std::int8_t(c.at("forced").get<int>()),c.at("used").get<std::uint32_t>()};const auto before=state;SoundLocalRandom random(c.at("seed"));
  set_cw(c.at("cw").get<std::uint16_t>());_mm_setcsr(c.at("mxcsr"));const auto selected=source_sound_permutation(d,state,random);std::fesetenv(&saved);_mm_setcsr(saved_mxcsr);
  const auto&e=c.at("expected");std::array<std::byte,16> bank;bank.fill(std::byte{0xa5});const unsigned usedindex=c.at("encoded_used_index"),offset=usedindex>>3;const bool persistent=(c.at("class_flags10").get<unsigned>()&0x20)==0;
  if(persistent){need(offset+4<=bank.size(),"Probe usedword escapes fixed oraclefixture");for(unsigned n=0;n<4;++n)bank[offset+n]=std::byte(before.used>>(8*n));if(count>1&&before.forced==-1){for(unsigned n=0;n<unsigned(count+7)/8;++n)bank[offset+n]=std::byte(state.used>>(8*n));}}
  auto seed=c.at("seed").get<std::uint32_t>();bool stores=true;for(const auto&s:random.stores){stores&=s.before==seed&&(s.instruction==0x21b281u||s.instruction==0x21b349u);seed=seed*0x19660du+0x3c6ef35fu;stores&=s.after==seed;}
  bool same=selected==e.at("selected")&&int(state.last)==e.at("last")&&int(state.forced)==e.at("forced")&&random.seed==e.at("seed")&&random.stores.size()==e.at("rng_draws")&&hex(bank)==e.at("used_bank_hex").get<std::string>()&&stores;
  if(persistent)same&=state.used==e.at("used_word");else same&=state.used==before.used;
  if(!same)mismatches.push_back({{"id",c.at("id")},{"label",c.at("label")},{"count",count},{"selected",selected},{"last",int(state.last)},{"forced",int(state.forced)},{"used",state.used},{"bank",hex(bank)},{"seed",random.seed},{"draws",random.stores.size()},{"expected",e}});
  ++total;count==32?++count32:++lower;draws+=random.stores.size();controls+=c.at("label")=="floating-control";
 }
 unsigned rejected{};for(int count:{-1,0,33,64}){SoundVariationDefinition d;put16(d.range,10,unsigned(count));d.permutations.resize(count>0?unsigned(count):0);SoundRangeState state{-1,-1,0x12345678};SoundLocalRandom random(1);try{source_sound_permutation(d,state,random);throw std::logic_error("unsupported domain accepted");}catch(const FormatError&){need(random.seed==1&&random.stores.empty()&&state.last==-1&&state.forced==-1&&state.used==0x12345678,"Rejected domain touched state/RNG");++rejected;}}
 SoundVariationDefinition shortd;put16(shortd.range,10,32);shortd.permutations.resize(31);SoundRangeState state{-1,-1,0};SoundLocalRandom random(1);try{source_sound_permutation(shortd,state,random);throw std::logic_error("truncated permutation list accepted");}catch(const FormatError&){need(random.seed==1&&random.stores.empty(),"Truncated range touched RNG");++rejected;}
 need(total==1131&&count32==438&&lower==693&&controls==96,"Original functional matrix is incomplete");
 std::cout<<nlohmann::json{{"cases",total},{"count32_cases",count32},{"lower_cases",lower},{"floating_control_cases",controls},{"native_rng_stores",draws},{"rejected_domains",rejected},{"mismatches",mismatches},{"scope","Function-level selected/last/forced/full16B used-bank writeback/RNG-state and draw-count equality to real TU1.5 CPU. Existing source RNG IDs are canonicalTU1.1 labels mapped to primary TU15 stores21FB81/21FC49. Native compiler GP/flags/x87 instruction state and elapsed/callback parity not claimed."}}.dump(2)<<'\n';return mismatches.empty()?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

#else
// The fixture preserves original x87 and SSE controls; unsupported hosts skip it.
int main(){return 77;}
#endif
