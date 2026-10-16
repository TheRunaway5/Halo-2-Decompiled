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
std::uint16_t cw(){std::uint16_t v;asm volatile("fnstcw %0":"=m"(v));return v;}
std::uint16_t sw(){std::uint16_t v;asm volatile("fnstsw %0":"=m"(v));return v;}
using Bank=MccSoundEffectDefinitions;
const Bank::EffectTemplate& mono(const Bank&b){for(const auto&t:b.templates())for(const auto&p:t.platforms)for(const auto&c:p.templates)if(c.input.text&&*c.input.text=="sfx_mono_distortion")return t;throw FormatError("Actual mono missing");}
const Bank::Instance& radio(const Bank&b){for(const auto&c:b.collections())for(const auto&e:c.effects)if(e.name.text&&*e.name.text=="radio_3d"){check(e.instances.size()==1,"Radio instance count differs");return e.instances[0];}throw FormatError("Actual radio missing");}
std::array<std::uint32_t,11> words(const Parameters&p){std::array<std::uint32_t,11> out{};out[0]=p.gain.bits;unsigned i=1;for(const auto&f:{p.before_clip,p.after_clip})for(auto w:{f.current_input,f.previous_input,f.older_input,f.previous_output,f.older_output})out[i++]=w.bits;return out;}
struct Change {
    std::span<const std::byte> raw;
    std::vector<std::byte> original;
    Change(const Cache&cache,std::uint32_t at,std::span<const std::byte> replacement):raw(cache.read_tag_bytes(at,replacement.size())),original(raw.begin(),raw.end()){std::memcpy(const_cast<std::byte*>(raw.data()),replacement.data(),replacement.size());}
    ~Change(){std::memcpy(const_cast<std::byte*>(raw.data()),original.data(),original.size());}
};
std::vector<std::byte> curve(std::string_view text){check(text.size()==232,"Expected source116-byte curve");std::vector<std::byte> out(116);for(unsigned i=0;i<116;i++)out[i]=std::byte(std::stoul(std::string(text.substr(2*i,2)),nullptr,16));return out;}
FunctionContext context(const Json&c,const tu15_effect_function::SpeakerGeometry&geometry){const auto raw=c.at("speaker").get<std::uint32_t>();return {{c.at("context_bits").get<std::array<std::uint32_t,7>>()},std::bit_cast<std::int32_t>(raw),geometry,{c.at("cw").get<std::uint16_t>(),0x1f80}};}
}
int main(int argc,char**argv)try{
    check(argc==5,"Expected actual map directory, source function report, combined original report and output");
    Json source,combined;std::ifstream(argv[2])>>source;std::ifstream(argv[3])>>combined;
    check(source.at("controls").size()==3498&&combined.at("rows").size()==3498,"Source control count differs");
    // Literal geometry is independently authenticated source data, supplied to
    // this offline fixture. No runtime geometry or clock owner is invented.
    tu15_effect_function::SpeakerGeometry geometry{};
    const std::array<std::uint32_t,15> geometry_words=source.at("speaker_geometry_words").get<std::array<std::uint32_t,15>>();
    for(unsigned i=0;i<15;i++)geometry.directions[i/3][i%3]=geometry_words[i];
    unsigned actual_calls=0,guards=0,finite_calls=0,nan_rejects=0,infinity_clamps=0;
    Json maps=Json::array();
    for(const char*name:{"mainmenu.map","00a_introduction.map","lockout.map"}){
        auto cache=std::make_shared<Cache>(std::filesystem::path(argv[1])/name,argv[1]);std::weak_ptr<const Cache> weak=cache;
        auto bank=std::make_shared<Bank>(cache);auto compiled=CompiledMonoDefinition::compile(bank,mono(*bank).handle);
        const auto&instance=radio(*bank);const auto&override=instance.override_records.at(0).components.at(0);
        check(override.parameter_mask==0x71&&override.sources.size()==4&&override.constants.size()==3&&override.functions.size()==1,"Actual radio override layout differs");
        const std::array<std::uint8_t,4> selectors{1,0,2,4};for(unsigned i=0;i<4;i++)check(override.sources[i].bits==selectors[i],"Actual compact selectors differ");
        const auto reject=[&](const auto&fn){const auto before_cw=cw(),before_sw=sw();const auto before_mx=_mm_getcsr();bool caught=false;try{fn();}catch(const FormatError&){caught=true;}check(caught&&cw()==before_cw&&sw()==before_sw&&_mm_getcsr()==before_mx,"Rejected bridge call changed FP or was accepted");++guards;};
        reject([&]{(void)compiled->instance(instance,0,FpMode::source_pretable_53);});
        auto ctx=context(source.at("controls")[0],geometry);
        reject([&]{auto mismatch=ctx;mismatch.fp.loaded_x87_control=0x037f;(void)compiled->instance_with_functions(instance,0,FpMode::source_pretable_53,mismatch);});
        reject([&]{auto mismatch=ctx;mismatch.fp.mxcsr=0x9fc0;(void)compiled->instance_with_functions(instance,0,FpMode::source_pretable_53,mismatch);});
        {const auto copied=instance;reject([&]{(void)compiled->instance_with_functions(copied,0,FpMode::source_pretable_53,ctx);});}
        reject([&]{(void)compiled->instance_with_functions(instance,99,FpMode::source_pretable_53,ctx);});
        // These use the actual untouched MCC curve/constant bank. Only gain is
        // compared to the original function-only observation; stock-vs-Xbox
        // constant differences are retained separately and never relabelled.
        for(const auto&c:source.at("controls"))if(c.at("curve_origin")=="mcc"&&c.at("label").get<std::string>().starts_with("mcc_rolloff_")){
            auto inputs=context(c,geometry);auto value=compiled->instance_with_functions(instance,0,static_cast<FpMode>(inputs.fp.loaded_x87_control),inputs);
            check(value.bounded_float_bits[0]==c.at("proposal_words")[4],"Actual MCC radio gain differs from original builder");++actual_calls;
        }
        if(std::string_view(name)=="mainmenu.map"){
            const auto&function=override.functions[0];const auto function_address=function.raw.source_address;
            const auto curve_address=function.curve.source_address;
            const std::array<std::uint32_t,3> original_constants{0x42a00000,0xc1f00000,0x3f000000};
            std::array<std::unique_ptr<Change>,3> constants;
            for(unsigned i=0;i<3;i++)constants[i]=std::make_unique<Change>(*cache,override.constants[i].raw.source_address,std::as_bytes(std::span(&original_constants[i],1)));
            for(std::size_t i=0;i<3498;i++){
                const auto&c=source.at("controls")[i];const auto&r=combined.at("rows")[i];
                const auto payload=curve(c.at("curve_hex").get<std::string>());Change curve_change(*cache,curve_address,payload);
                const auto selectors_word=(c.at("range_input").get<std::uint32_t>()&0xffff)<<16|(c.at("input").get<std::uint32_t>()&0xffff);
                const auto period=c.at("period_bits").get<std::uint32_t>();Change selector_change(*cache,function_address,std::as_bytes(std::span(&selectors_word,1)));Change period_change(*cache,function_address+0xc,std::as_bytes(std::span(&period,1)));
                auto fresh_bank=std::make_shared<Bank>(cache);auto fresh=CompiledMonoDefinition::compile(fresh_bank,mono(*fresh_bank).handle);auto inputs=context(c,geometry);
                const auto before_cw=cw(),before_sw=sw();const auto before_mx=_mm_getcsr();
                if(!r.at("converter_executed_until_first_setter").get<bool>()){
                    bool failed=false;try{(void)fresh->instance_with_functions(radio(*fresh_bank),0,static_cast<FpMode>(inputs.fp.loaded_x87_control),inputs);}catch(const FormatError&){failed=true;}
                    check(failed,"Unresolved original NaN converter path was admitted");++nan_rejects;
                }else{
                    const auto result=fresh->instance_with_functions(radio(*fresh_bank),0,static_cast<FpMode>(inputs.fp.loaded_x87_control),inputs);
                    const auto expected=c.at("proposal_words").get<std::array<std::uint32_t,11>>();std::array<std::uint32_t,7> expected_bits{};std::copy(expected.begin()+4,expected.end(),expected_bits.begin());
                    if(result.bounded_float_bits!=expected_bits||words(result.parameters)!=r.at("converted").at("words").get<std::array<std::uint32_t,11>>()){
                        std::cerr<<"Bridge mismatch index "<<i<<" label "<<c.at("label")<<" floats "<<Json(result.bounded_float_bits)<<" expected "<<Json(expected_bits)<<" words "<<Json(words(result.parameters))<<" expected "<<r.at("converted").at("words")<<'\n';throw FormatError("Complete native bridge differs from unchanged builder/converter");
                    }
                    ++finite_calls;const auto scalar=c.at("value_before_parameter_bounds_bits").get<std::uint32_t>();infinity_clamps+=(scalar&0x7fffffff)==0x7f800000;
                }
                check(cw()==before_cw&&sw()==before_sw&&_mm_getcsr()==before_mx,"Bridge altered caller FP environment");
            }
        }
        auto held=compiled->instance_with_functions(instance,0,FpMode::source_pretable_53,ctx);auto copied=held;compiled.reset();bank.reset();cache.reset();check(!weak.expired(),"Bridge lost exact MCC Cache");held.definition_owner.reset();check(!weak.expired()&&copied.definition_owner->definitions().collections().size()>0,"Copied bridge proposal lost definition owner");copied.definition_owner.reset();check(weak.expired(),"Bridge leaked the actual MCC Cache");maps.push_back({{"map",name},{"retained_lifetime",true}});
    }
    check(finite_calls==3480&&nan_rejects==18&&infinity_clamps==12,"Bridge coverage differs");
    Json report={{"passed",true},{"actual_MCC_calls",actual_calls},{"guards",guards},{"complete_original_builder_converter_matches",finite_calls},{"NaN_converter_paths_unresolved",nan_rejects},{"original_infinite_scalar_clamped_then_converted",infinity_clamps},{"maps",maps},{"stock_MCC_constants_silently_replaced",false},{"graph_or_installer_ready",false},{"initializer45_ready",false}};
    std::ofstream(argv[4])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
