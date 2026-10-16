#include "halo2/mcc_mono_distortion_definition.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <xmmintrin.h>
using namespace halo2;
using namespace halo2::tu15_mono_parameter_conversion;
using Json=nlohmann::json;
namespace {
void check(bool b,const char*why){if(!b)throw std::runtime_error(why);}
using Bank=MccSoundEffectDefinitions;
const Bank::EffectTemplate& mono(const Bank&b){for(const auto&t:b.templates())for(const auto&p:t.platforms)for(const auto&c:p.templates)if(c.input.text&&*c.input.text=="sfx_mono_distortion")return t;throw FormatError("Actual mono missing");}
const Bank::Instance& radio(const Bank&b){for(const auto&c:b.collections())for(const auto&e:c.effects)if(e.name.text&&*e.name.text=="radio_3d")return e.instances.at(0);throw FormatError("Actual radio missing");}
std::array<std::uint32_t,11> words(const Parameters&p){std::array<std::uint32_t,11> out{};out[0]=p.gain.bits;unsigned i=1;for(const auto&f:{p.before_clip,p.after_clip})for(auto w:{f.current_input,f.previous_input,f.older_input,f.previous_output,f.older_output})out[i++]=w.bits;return out;}
struct Change {
    std::span<const std::byte> raw;
    std::vector<std::byte> original;
    Change(const Cache&cache,std::uint32_t at,std::span<const std::byte> replacement):raw(cache.read_tag_bytes(at,replacement.size())),original(raw.begin(),raw.end()){std::memcpy(const_cast<std::byte*>(raw.data()),replacement.data(),replacement.size());}
    ~Change(){std::memcpy(const_cast<std::byte*>(raw.data()),original.data(),original.size());}
};
}
int main(int argc,char**argv)try{
 check(argc==4,"Expected actual maps, boundary reference and report");Json reference;std::ifstream(argv[2])>>reference;
 check(reference.at("passed")&&!reference.at("source_instructions_patched").get<bool>(),"Original reference not authenticated");
 auto cache=std::make_shared<Cache>(std::filesystem::path(argv[1])/"mainmenu.map",argv[1]);auto bank=std::make_shared<Bank>(cache);
 const auto&over=radio(*bank).override_records.at(0).components.at(0);check(over.sources.size()==4&&over.constants.size()==3,"Actual override layout differs");
 const auto&base=reference.at("function_context");tu15_effect_function::SpeakerGeometry geometry{};
 const auto directions=reference.at("speaker_geometry_words").get<std::array<std::uint32_t,15>>();for(unsigned i=0;i<15;i++)geometry.directions[i/3][i%3]=directions[i];
 FunctionContext context{{base.at("context_bits").get<std::array<std::uint32_t,7>>()},std::bit_cast<std::int32_t>(base.at("speaker").get<std::uint32_t>()),geometry,{0x023f,0x1f80}};
 unsigned count=0,guards=0,infinities=0;
 for(const auto&row:reference.at("cases")){
  const auto mask=row.at("mask").get<std::uint32_t>();const auto selectors=row.at("sources").get<std::array<std::uint8_t,4>>();const auto constants=row.at("constants").get<std::array<std::uint32_t,3>>();
  Change a(*cache,over.raw.source_address+0x18,std::as_bytes(std::span(&mask,1)));Change b(*cache,over.sources[0].raw.source_address,std::as_bytes(std::span(selectors)));
  std::array<std::unique_ptr<Change>,3> changes;for(unsigned i=0;i<3;i++)changes[i]=std::make_unique<Change>(*cache,over.constants[i].raw.source_address,std::as_bytes(std::span(&constants[i],1)));
  auto fresh=std::make_shared<Bank>(cache);auto compiled=CompiledMonoDefinition::compile(fresh,mono(*fresh).handle);context.fp.loaded_x87_control=row.at("cw").get<std::uint16_t>();
  const auto value=compiled->instance_with_functions(radio(*fresh),0,static_cast<FpMode>(context.fp.loaded_x87_control),context);
  const auto original=row.at("builder_words").get<std::array<std::uint32_t,11>>();
  check(std::equal(value.bounded_float_bits.begin(),value.bounded_float_bits.end(),original.begin()+4),"Compact selection/bounds differ from original builder");
  check(words(value.parameters)==row.at("converted_words").get<std::array<std::uint32_t,11>>(),"Compact conversion differs from original converter");
  infinities+=(constants[0]&0x7fffffff)==0x7f800000;++count;
 }
 context.fp.loaded_x87_control=0x023f;
 const auto reject=[&](const auto&fn){bool failed=false;try{fn();}catch(const FormatError&){failed=true;}check(failed,"Unowned selected function admitted");++guards;};
 for(std::uint8_t selector:{std::uint8_t(3),std::uint8_t(127),std::uint8_t(129),std::uint8_t(255)}){
  Change c(*cache,over.sources[0].raw.source_address,std::as_bytes(std::span(&selector,1)));auto fresh=std::make_shared<Bank>(cache);auto compiled=CompiledMonoDefinition::compile(fresh,mono(*fresh).handle);
  reject([&]{(void)compiled->instance_with_functions(radio(*fresh),0,FpMode::source_pretable_53,context);});
 }
 // Unknown function bytes must not be evaluated when the compact mask selects
 // no parameters. This is an actual Cache mutation restored before return.
 {const std::uint32_t mask=0;const std::uint8_t unsupported=1;
  Change a(*cache,over.raw.source_address+0x18,std::as_bytes(std::span(&mask,1)));Change b(*cache,over.functions[0].curve.source_address,std::as_bytes(std::span(&unsupported,1)));
  auto fresh=std::make_shared<Bank>(cache);auto compiled=CompiledMonoDefinition::compile(fresh,mono(*fresh).handle);
  check(compiled->instance_with_functions(radio(*fresh),0,FpMode::source_pretable_53,context).parameters==compiled->defaults(FpMode::source_pretable_53).parameters,"Unselected unsupported function evaluated");++guards;
 }
 check(count==reference.at("cases").size()&&infinities==42,"Boundary coverage differs");
 Json report={{"passed",true},{"original_builder_converter_boundary_matches",count},{"constant_infinity_source_clamps",infinities},{"compact_function_provenance_controls",guards},{"original_source_instructions_patched",false},{"Cache_mutations_scoped_and_restored",true},{"initializer45_ready",false}};
 std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
