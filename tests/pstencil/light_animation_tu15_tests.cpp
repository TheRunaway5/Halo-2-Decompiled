// Dumps the native C2B30 light-animation kernel over real animated sphere
// definitions as a TSV fixture. tools/pstencil/oracle_light_animation_tu15.py
// executes the unchanged TU1.5 C2B30 on every row and requires identical
// output bits. Usage: OUTPUT.tsv MAP...
#include "halo2/effect_light.hpp"
#include "halo2/effect_light_lifetime.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
std::string hex(std::span<const std::byte> bytes) {
    std::ostringstream out;out<<std::hex<<std::setfill('0');
    for(const auto b:bytes)out<<std::setw(2)<<std::to_integer<unsigned>(b);
    const auto text=out.str();return text.empty()?std::string("-"):text;
}
std::uint32_t bits(float v){return std::bit_cast<std::uint32_t>(v);}
}
int main(int argc,char** argv){try {
    if(argc<3){std::cerr<<"usage: OUTPUT.tsv MAP...\n";return 2;}
    std::ofstream output(argv[1]);if(!output)throw FormatError("cannot write light animation fixture");
    output<<std::hex;
    std::set<std::string> seen;unsigned rows{},brightness{},color{};
    constexpr std::uint32_t identities[]{0x00010000u,0xe1230005u,0x7fff0123u,0xffffffffu,1u,0x80000000u};
    constexpr std::int32_t ticks[]{0,1,7,29,30,31,617,1799,1800,54321,1000000};
    constexpr float controls[]{0.f,.25f,1.f};
    constexpr float brights[]{1.f,2.5f};
    constexpr Vec3 colors[]{{1,1,1},{.72f,.69f,1.f},{.18f,.78f,1.f}};
    for(int index=2;index<argc;++index) {
        Cache cache(argv[index]);
        for(const auto& tag:cache.tags()) {
            if(tag.group!="ligh" || !seen.insert(tag.name).second)continue;
            EffectLightDefinition definition;
            try{definition=load_completed_effect_light(cache,tag.datum);}catch(const FormatError&){continue;}
            if(!definition.animation)continue;
            const auto& animation=*definition.animation;
            brightness+=!animation.brightness.empty();color+=!animation.color.empty();
            for(const auto identity:identities)for(const auto tick:ticks)for(const auto control:controls)
            for(const auto bright:brights)for(const auto base:colors) {
                EffectLightState state;state.control=control;state.brightness=bright;state.diffuse=base;
                state.specular={.5f,.25f,.125f};state.size=3.f;
                source_effect_light_animate(animation,identity,tick,30,state);
                if(bits(state.specular.x)!=bits(.5f) || bits(state.size)!=bits(3.f) || bits(state.control)!=bits(control))
                    throw FormatError("Light animation changed a field C2B30 does not own");
                output<<tag.name<<'\t'<<unsigned(animation.synchronized)<<'\t'<<hex(animation.brightness)<<'\t'
                    <<hex(animation.color)<<'\t'<<identity<<'\t'<<std::bit_cast<std::uint32_t>(tick)<<'\t'
                    <<bits(control)<<'\t'<<bits(bright)<<'\t'<<bits(base.x)<<'\t'<<bits(base.y)<<'\t'<<bits(base.z)<<'\t'
                    <<bits(state.brightness)<<'\t'<<bits(state.diffuse.x)<<'\t'<<bits(state.diffuse.y)<<'\t'
                    <<bits(state.diffuse.z)<<'\n';
                ++rows;
            }
        }
    }
    std::cout<<std::dec<<seen.size()<<" light definitions, "<<brightness<<" brightness and "<<color
        <<" colour animations, "<<rows<<" native rows\n";
    if(!rows || !brightness || !color)throw FormatError("Light animation fixture lacks a brightness or colour animation");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
