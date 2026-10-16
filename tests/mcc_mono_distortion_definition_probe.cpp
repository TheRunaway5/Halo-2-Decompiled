#include "halo2/mcc_mono_distortion_definition.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <xmmintrin.h>
using namespace halo2;using namespace halo2::tu15_mono_parameter_conversion;using Json=nlohmann::json;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
std::array<std::uint32_t,11> words(const Parameters&p){std::array<std::uint32_t,11> o{};o[0]=p.gain.bits;unsigned i=1;for(const auto&f:{p.before_clip,p.after_clip})for(auto v:{f.current_input,f.previous_input,f.older_input,f.previous_output,f.older_output})o[i++]=v.bits;return o;}
std::uint16_t cw(){std::uint16_t x;asm volatile("fnstcw %0":"=m"(x));return x;}
std::uint16_t sw(){std::uint16_t x;asm volatile("fnstsw %0":"=m"(x));return x;}
using Bank=MccSoundEffectDefinitions;
const Bank::EffectTemplate& mono(const Bank&b){for(const auto&t:b.templates())for(const auto&p:t.platforms)for(const auto&c:p.templates)if(c.input.text&&*c.input.text=="sfx_mono_distortion")return t;throw std::runtime_error("Actual mono template missing");}
const Bank::Instance& effect_instance(const Bank&b,std::string_view name){for(const auto&c:b.collections())for(const auto&e:c.effects)if(e.name.text&&*e.name.text==name){check(e.instances.size()==1,"Actual effect instance count differs");return e.instances[0];}throw std::runtime_error("Actual effect missing");}
}
int main(int argc,char**argv)try{
 check(argc==4,"Expected imported map directory, hardware original report and output report");Json reference;std::ifstream(argv[2])>>reference;
 const auto mode=FpMode::source_pretable_53;std::size_t guards=0,compared=0;Json maps=Json::array(),unconverted=Json::array();
 std::array<std::uint32_t,7> default_bits{};std::optional<ConvertedMonoParameters> retained;
 for(const char*name:{"mainmenu.map","00a_introduction.map","lockout.map"}){
  auto cache=std::make_shared<Cache>(std::filesystem::path(argv[1])/name,argv[1]);std::weak_ptr<const Cache> weak=cache;
  auto bank=std::make_shared<Bank>(cache);auto compiled=CompiledMonoDefinition::compile(bank,mono(*bank).handle);
  auto defaults=compiled->defaults(mode);default_bits=defaults.bounded_float_bits;
  const auto match=[&](const ConvertedMonoParameters& actual){const auto i=std::ranges::find_if(reference.at("conversion_controls"),[&](const auto&r){return r.at("cw")==0x023f&&r.at("input_bits")==Json(actual.bounded_float_bits);});check(i!=reference.at("conversion_controls").end(),"Actual MCC conversion lacks genuine original control");check(words(actual.parameters)==i->at("words").template get<std::array<std::uint32_t,11>>(),"Actual MCC proposal differs from genuine original");++compared;};match(defaults);
  check(defaults.parameters.gain.bits==3003121&&defaults.parameters.gain.bits!=3000000,"Converted default was replaced with authored initial gain");
  for(const char*effect:{"radio_default_in","radio_default_out","radio_default_loop","radio_covy_in","radio_covy_loop","radio_covy_out","radio_default_low","radio_covy_low"}){
   const auto& instance=effect_instance(*bank,effect);match(compiled->instance(instance,0,mode));check(compiled->instance(instance,std::nullopt,mode).parameters==defaults.parameters,"No override did not use source defaults");
  }
  const auto reject=[&](const auto&fn){bool did=false;try{fn();}catch(const std::exception&){did=true;}check(did,"Unresolved MCC definition accepted");++guards;};
  reject([&]{(void)compiled->instance(effect_instance(*bank,"radio_3d"),0,mode);});
  {const auto fake=effect_instance(*bank,"radio_default_in");reject([&]{(void)compiled->instance(fake,0,mode);});}
  reject([&]{(void)compiled->instance(effect_instance(*bank,"radio_default_in"),99,mode);});
  reject([&]{(void)CompiledMonoDefinition::compile({},compiled->identity());});
  reject([&]{(void)CompiledMonoDefinition::compile(bank,TagHandle{0xffffffff});});
  reject([&]{(void)compiled->defaults(static_cast<FpMode>(0x007f));});
  if(std::string_view(name)=="mainmenu.map"){
   for(const auto&t:bank->templates())if(t.handle!=compiled->identity()){bool rejected=false;try{(void)CompiledMonoDefinition::compile(bank,t.handle);}catch(const FormatError&){rejected=true;}check(rejected,"Other effect algorithm silently converted as mono");unconverted.push_back(t.name);++guards;}
   const auto id=compiled->identity();const auto&item=compiled->source_template();const auto&inst=effect_instance(*bank,"radio_default_in");const auto&root=inst.override_records[0];const auto&override=root.components[0];
   const auto mutate=[&](std::uint32_t address,std::uint32_t value,unsigned width,const auto&fn){auto raw=cache->read_tag_bytes(address,width);std::array<std::byte,4> saved{};std::ranges::copy(raw,saved.begin());for(unsigned i=0;i<width;i++)const_cast<std::byte*>(raw.data())[i]=std::byte(value>>(8*i));try{fn();}catch(...){std::memcpy(const_cast<std::byte*>(raw.data()),saved.data(),width);throw;}std::memcpy(const_cast<std::byte*>(raw.data()),saved.data(),width);check(std::ranges::equal(raw,std::span(saved).first(width)),"Private metadata control failed to restore bytes");};
   for(const auto&change:std::vector<std::pair<std::uint32_t,std::uint32_t>>{
    {item.raw.source_address,0xffffffff},{item.raw.source_address+8,0},
    {item.components[0].raw.source_address,0},{item.components[0].raw.source_address+4,0x7fc12345},
    {item.components[0].raw.source_address+8,0xbf800000},{item.components[0].raw.source_address+12,0x40000000},
    {item.components[3].raw.source_address+8,0},{item.components[3].raw.source_address+12,0x7f800000}})
    mutate(change.first,change.second,4,[&]{reject([&]{auto altered=std::make_shared<Bank>(cache);(void)CompiledMonoDefinition::compile(altered,id);});});
   //228CD0 overwrites the declared effect mask through197620; packed
   //header offset/size and nonselector flag bits do not enter mono arithmetic.
   for(const auto [at,value]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{{item.raw.source_address+4,0xffffffff},{item.raw.source_address+8,0x101},{item.raw.source_address+12,0x00300010}}})
    mutate(at,value,4,[&]{auto altered=std::make_shared<Bank>(cache);auto c=CompiledMonoDefinition::compile(altered,id);check(c->defaults(mode).parameters==defaults.parameters,"Unconsumed declaration metadata changed mono arithmetic");++compared;});
   for(const auto&change:std::vector<std::tuple<std::uint32_t,std::uint32_t,unsigned>>{
    {override.constants[0].raw.source_address,0x7fc12345,4},{override.sources[0].raw.source_address,0xff,1},
    {override.sources[0].raw.source_address,1,1},{override.sources[0].raw.source_address,0x7e,1},
    {override.raw.source_address+0x10,0,4},{root.raw.source_address,0,4}}){
     const auto[at,bits,width]=change;mutate(at,bits,width,[&]{reject([&]{auto altered=std::make_shared<Bank>(cache);auto c=CompiledMonoDefinition::compile(altered,id);(void)c->instance(effect_instance(*altered,"radio_default_in"),0,mode);});});
   }
   for(const auto [value,expected]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{{0xbf800000,0},{0x40000000,0x3f800000},{0x80000000,0x80000000}}})
    mutate(override.constants[0].raw.source_address,value,4,[&]{auto altered=std::make_shared<Bank>(cache);auto c=CompiledMonoDefinition::compile(altered,id);check(c->instance(effect_instance(*altered,"radio_default_in"),0,mode).bounded_float_bits[0]==expected,"Source finite bounds/signed zero changed");++compared;});
   const auto&function_override=effect_instance(*bank,"radio_3d").override_records[0].components[0];
   mutate(function_override.raw.source_address+0x18,0,4,[&]{auto altered=std::make_shared<Bank>(cache);auto c=CompiledMonoDefinition::compile(altered,id);check(c->instance(effect_instance(*altered,"radio_3d"),0,mode).parameters==defaults.parameters,"Unselected function was evaluated");++compared;});
   mutate(root.raw.source_address+0x10,0,4,[&]{auto altered=std::make_shared<Bank>(cache);auto c=CompiledMonoDefinition::compile(altered,id);check(c->instance(effect_instance(*altered,"radio_default_in"),0,mode).parameters==defaults.parameters,"Unselected component did not retain defaults");++compared;});
  }
  retained=compiled->defaults(mode);auto detached=*retained;defaults.definition_owner.reset();compiled.reset();bank.reset();cache.reset();check(!weak.expired(),"Compiled definition lost actual MCC cache");check(retained->definition_owner->source_template().components[0].default_bits==default_bits[0],"Retained typed source template expired");retained.reset();check(!weak.expired()&&detached.parameters.gain.bits==3003121,"Copied proposal lost MCC owner");detached.definition_owner.reset();check(weak.expired(),"Private converter leaked resolving cache");maps.push_back({{"map",name},{"retained_cache_lifetime",true}});
 }
 const auto reject_math=[&](const auto&fn){const auto before_cw=cw(),before_sw=sw();const auto mx=_mm_getcsr();bool failed=false;try{fn();}catch(const std::invalid_argument&){failed=true;}check(failed&&cw()==before_cw&&sw()==before_sw&&_mm_getcsr()==mx,"Math rejection mutated caller FP context");++guards;};
 for(unsigned i=0;i<7;i++)for(std::uint32_t bad:{0x7f800000u,0xff800000u,0x7fc12345u}){auto values=default_bits;values[i]=bad;reject_math([&]{(void)convert_bounded_values(values,mode);});}
 for(std::uint64_t bits:{UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff0000000000001),UINT64_C(0x7ff8000000001234)})reject_math([&]{(void)quantize_finite_fraction(std::bit_cast<double>(bits),mode);});
 {const auto saved_mx=_mm_getcsr();for(std::uint32_t incoming:{0x1f80u,0x9fc0u}){_mm_setcsr(incoming);auto values=default_bits;values[0]=0x80000001;reject_math([&]{(void)convert_bounded_values(values,mode);});}_mm_setcsr(saved_mx);}
 for(unsigned i=0;i<7;i++)for(float value:{-100000.f,100000.f}){auto values=default_bits;values[i]=std::bit_cast<std::uint32_t>(value);reject_math([&]{(void)convert_bounded_values(values,mode);});}
 for(std::uint16_t value:{std::uint16_t(0x007f),std::uint16_t(0x0b7f),std::uint16_t(0xffff)})reject_math([&]{(void)convert_bounded_values(default_bits,static_cast<FpMode>(value));});
 {asm volatile("fld1");reject_math([&]{(void)convert_bounded_values(default_bits,mode);});long double retained_stack;asm volatile("fstpt %0":"=m"(retained_stack));check(retained_stack==1,"Rejected conversion changed existing x87 stack value");}
 std::fenv_t environment;check(std::fegetenv(&environment)==0,"Cannot save test FP context");
 for(std::uint16_t input_cw:{std::uint16_t(0x027f),std::uint16_t(0x037f),std::uint16_t(0x077f),std::uint16_t(0x0b7f),std::uint16_t(0x0f7f)})for(auto mode:{FpMode::source_pretable_53,FpMode::double_53,FpMode::extended_64}){
  asm volatile("fldcw %0"::"m"(input_cw));const auto mx=std::uint32_t(0x9fc0);_mm_setcsr(mx);std::feraiseexcept(FE_INEXACT);const auto status=sw();const auto actual_cw=cw();const auto actual_mx=_mm_getcsr();(void)convert_bounded_values(default_bits,mode);if(cw()!=actual_cw||sw()!=status||_mm_getcsr()!=actual_mx)std::cerr<<"FP actual/before "<<cw()<<"/"<<actual_cw<<" "<<sw()<<"/"<<status<<" "<<_mm_getcsr()<<"/"<<actual_mx<<"\n";check(cw()==actual_cw&&sw()==status&&_mm_getcsr()==actual_mx,"Successful conversion did not restore caller FP context");++compared;
 }
 std::fesetenv(&environment);
 Json report={{"passed",true},{"maps",maps},{"original_parameter_proposals_and_controls",compared},{"guards",guards},{"other_definitions_unconverted",unconverted},{"chorus_missing_definition_remains_unresolved",true},{"install_ack",false},{"graph_ready",false},{"authored_initial_gain",3000000},{"converted_default_gain",3003121}};std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
