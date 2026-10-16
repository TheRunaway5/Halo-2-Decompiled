#include "halo2/mcc_effect_parameter_function.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <xmmintrin.h>
using namespace halo2;using namespace halo2::tu15_effect_function;using Json=nlohmann::json;
namespace {
void check(bool b,const char*r){if(!b)throw std::runtime_error(r);}
std::uint16_t cw(){std::uint16_t x;asm volatile("fnstcw %0":"=m"(x));return x;}
std::uint16_t sw(){std::uint16_t x;asm volatile("fnstsw %0":"=m"(x));return x;}
const MccSoundEffectDefinitions::Function& radio(const MccSoundEffectDefinitions& bank){for(const auto&c:bank.collections())for(const auto&e:c.effects)if(e.name.text&&*e.name.text=="radio_3d")return e.instances.at(0).override_records.at(0).components.at(0).functions.at(0);throw FormatError("Actual MCC radio curve missing");}
}
int main(int argc,char**argv)try{
 check(argc==4,"Expected actual MCC map directory, original hardware report and output");Json original;std::ifstream(argv[2])>>original;std::size_t guards=0,cases=0;Json maps=Json::array();
 const auto reject=[&](const auto&fn){const auto before_cw=cw(),before_sw=sw();const auto before_mx=_mm_getcsr();bool rejected=false;try{fn();}catch(const FormatError&){rejected=true;}check(rejected&&cw()==before_cw&&sw()==before_sw&&_mm_getcsr()==before_mx,"Rejected effect function mutated caller FP context");++guards;};
 for(const char*name:{"mainmenu.map","00a_introduction.map","lockout.map"}){
  auto cache=std::make_shared<Cache>(std::filesystem::path(argv[1])/name,argv[1]);std::weak_ptr<const Cache> weak=cache;auto bank=std::make_shared<MccSoundEffectDefinitions>(cache);auto compiled=CompiledParameterFunction::compile(bank,radio(*bank));
  check(compiled->values().lower_bits==0x3eb74bc7&&compiled->values().upper_bits==0,"MCC curve was silently replaced by original Xbox values");
  for(const auto&control:original.at("controls"))if(control.at("curve_origin")=="mcc"&&control.at("label").template get<std::string>().starts_with("mcc_rolloff_")){
   const Inputs inputs{control.at("context_bits").template get<std::array<std::uint32_t,7>>()};const auto result=compiled->evaluate(inputs,-1,nullptr,{control.at("cw").template get<std::uint16_t>(),0x1f80});check(result.values.value_bits==control.at("proposal_words")[4],"Actual MCC retained function differs from unchanged original builder");++cases;
  }
  {auto foreign=radio(*bank);reject([&]{(void)CompiledParameterFunction::compile(bank,foreign);});}
  reject([&]{(void)CompiledParameterFunction::compile({},radio(*bank));});
  const Inputs inputs{{0,0,0x3f800000,0x3f400000,0x3f800000,0,0}};
  reject([&]{(void)compiled->evaluate(inputs,0,nullptr,{0x023f,0x1f80});});
  for(const ExplicitFp fp:std::array<ExplicitFp,4>{{{0x007f,0x1f80},{0x0b7f,0x1f80},{0x023f,0x9fc0},{0x023f,0x1fa0}}})reject([&]{(void)compiled->evaluate(inputs,-1,nullptr,fp);});
  if(std::string_view(name)=="mainmenu.map"){
   const auto&source=radio(*bank);const auto curve=source.curve.source_address;const auto mutate=[&](std::uint32_t address,std::uint32_t value,unsigned width,const auto&fn){auto raw=cache->read_tag_bytes(address,width);std::array<std::byte,4> saved{};std::ranges::copy(raw,saved.begin());for(unsigned i=0;i<width;i++)const_cast<std::byte*>(raw.data())[i]=std::byte(value>>(8*i));try{fn();}catch(...){std::memcpy(const_cast<std::byte*>(raw.data()),saved.data(),width);throw;}std::memcpy(const_cast<std::byte*>(raw.data()),saved.data(),width);check(std::ranges::equal(raw,std::span(saved).first(width)),"Private metadata guard did not restore bytes");};
   for(auto[index,width,value]:std::array<std::tuple<std::uint32_t,unsigned,std::uint32_t>,10>{{{curve,1,10},{curve+0x34,4,0x7fc12345},{curve+0x38,4,0x7f800000},{curve+4,4,0x7fc12345},{source.raw.source_address,2,0xffff},{source.raw.source_address,2,7},{source.raw.source_address+4,4,1},{source.raw.source_address+4,4,0},{source.raw.source_address+8,4,0},{source.raw.source_address+8,4,0xabcdef00}}})mutate(index,value,width,[&]{reject([&]{auto altered=std::make_shared<MccSoundEffectDefinitions>(cache);auto c=CompiledParameterFunction::compile(altered,radio(*altered));(void)c->evaluate(inputs,-1,nullptr,{0x023f,0x1f80});});});
   const auto expected=compiled->evaluate(inputs,-1,nullptr,{0x023f,0x1f80}).values.value_bits;
   for(auto[index,width,value]:std::array<std::tuple<std::uint32_t,unsigned,std::uint32_t>,6>{{{curve+0x14,4,0x7fc12345},{curve+0x64,4,0x7fc12345},{source.raw.source_address+2,2,0xffff},{source.raw.source_address+12,4,0x7fc12345},{curve+1,1,2},{curve+1,1,1}}})mutate(index,value,width,[&]{auto altered=std::make_shared<MccSoundEffectDefinitions>(cache);auto c=CompiledParameterFunction::compile(altered,radio(*altered));check(c->evaluate(inputs,-1,nullptr,{0x023f,0x1f80}).values.value_bits==expected,"Unconsumed curve/control fields changed parameter value");++cases;});
   // A valid ranged bank with the actual zero range input remains admitted.
  }
  auto result=compiled->evaluate(inputs,-1,nullptr,{0x023f,0x1f80});auto copied=result;compiled.reset();bank.reset();cache.reset();check(!weak.expired()&&copied.definition_owner->source().curve.payload.bytes.size()==116,"Returned function result lost actual resolving MCC owner");result.definition_owner.reset();check(!weak.expired(),"Copied result owner unexpectedly expired");copied.definition_owner.reset();check(weak.expired(),"Compiled function leaked the resolving MCC cache");maps.push_back({{"map",name},{"retained_MCC_curve_lifetime",true}});
 }
 // Meaningful direct admission guards, independent of raw metadata parsing.
 const auto&row=original.at("controls")[0];const auto curve_hex=row.at("curve_hex").get<std::string>();std::array<std::byte,116> bytes{};for(unsigned i=0;i<116;i++)bytes[i]=std::byte(std::stoul(curve_hex.substr(2*i,2),nullptr,16));const Bytes b(bytes);
 CubicParameterValues base{3,0,0x3f800000,b.u32(4),b.u32(8),0,{{{b.u32(0x34),b.u32(0x38),b.u32(0x3c),b.u32(0x40)}, {}}}};
 const Inputs inputs{{0,0,0x3f800000,0x3f400000,0x3f800000,0,0}};
 for(std::uint32_t bad:{0x7f800000u,0xff800000u,0x7fc12345u}){auto values=inputs;values.words[3]=bad;reject([&]{(void)evaluate_cubic_values(base,values,-1,nullptr,{0x023f,0x1f80});});}
 for(std::int16_t index:{std::int16_t(-1),std::int16_t(7),std::int16_t(32767)}){auto f=base;f.input=index;reject([&]{(void)evaluate_cubic_values(f,inputs,-1,nullptr,{0x023f,0x1f80});});}
 for(std::uint32_t period:{0u,0x80000000u,0x7fc12345u,0x7f800000u}){auto f=base;f.input=1;f.period_bits=period;reject([&]{(void)evaluate_cubic_values(f,inputs,-1,nullptr,{0x023f,0x1f80});});}
 {asm volatile("fld1");reject([&]{(void)evaluate_cubic_values(base,inputs,-1,nullptr,{0x023f,0x1f80});});long double kept;asm volatile("fstpt %0":"=m"(kept));check(kept==1,"Occupied-stack rejection lost actual caller value");}
 std::fenv_t saved;std::fegetenv(&saved);for(std::uint16_t incoming:{std::uint16_t(0x027f),std::uint16_t(0x037f),std::uint16_t(0x0b7f)})for(std::uint16_t supplied:{std::uint16_t(0x023f),std::uint16_t(0x027f),std::uint16_t(0x037f)}){asm volatile("fldcw %0"::"m"(incoming));_mm_setcsr(0x9fc0);std::feraiseexcept(FE_INEXACT);const auto before_cw=cw(),before_sw=sw();const auto before_mx=_mm_getcsr();(void)evaluate_cubic_values(base,inputs,-1,nullptr,{supplied,0x1f80});check(cw()==before_cw&&sw()==before_sw&&_mm_getcsr()==before_mx,"Function evaluation lost caller FP environment");++cases;}std::fesetenv(&saved);
 Json report={{"passed",true},{"maps",maps},{"guards",guards},{"retained_proposals_and_controls",cases},{"MCC_original_curve_equivalence_claimed",false},{"other_function_algorithms_unresolved",true},{"live_clock_FP_and_geometry_owners_unresolved",true},{"Ready",false}};std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
