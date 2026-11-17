// The separate animated effect-light pool: a real effect whose light has
// brightness/colour animation blocks must create its light there (never in
// the ordinary pool), tick it as C2CB0 does (state, then C2B30 with the full
// light datum and the game tick) and retire it with its authored duration.
// Usage: MAP SHARED_MAPS UPDATE_XBE
#include "halo2/world_effects.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/source_effect_location.hpp"
#include <bit>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool b,const char* m){if(!b)throw FormatError(m);}
std::array<std::uint32_t,8> bits(const EffectLightState& s) {
    return {std::bit_cast<std::uint32_t>(s.size),std::bit_cast<std::uint32_t>(s.control),
        std::bit_cast<std::uint32_t>(s.diffuse.x),std::bit_cast<std::uint32_t>(s.diffuse.y),
        std::bit_cast<std::uint32_t>(s.diffuse.z),std::bit_cast<std::uint32_t>(s.specular.x),
        std::bit_cast<std::uint32_t>(s.specular.y),std::bit_cast<std::uint32_t>(s.brightness)};
}
}
int main(int argc,char** argv){try {
    check(argc==4,"Supply MAP SHARED_MAPS UPDATE_XBE");
    auto cache=std::make_shared<Cache>(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto content=cache->load_content(0,true);
    Mesh mesh;CollisionWorld collision(content.collision);
    NativeWorldEffects effects(*cache,mesh,collision,0x12345678);
    check(effects.animated_light_definitions().empty() && effects.animated_lights().empty(),
        "Animated light pool was populated before any effect was read");
    // Reading an effect graph is what admits its animated light definition.
    std::vector<std::uint32_t> candidates;std::set<std::uint32_t> known;unsigned graphs{};
    for(const auto& tag:cache->tags())if(tag.group=="effe") {
        (void)effects.try_preload_attached(tag.datum);
        for(const auto id:effects.animated_light_definitions())if(known.insert(id).second) {
            ++graphs;
            const auto definition=load_completed_effect_light(*cache,id);
            check(definition.animation && definition.radius>0,"Animated pool admitted a definition it cannot tick or draw");
            bool ordinary=true;try{(void)load_effect_light(*cache,id);}catch(const FormatError&){ordinary=false;}
            check(!ordinary,"An ordinary light definition entered the animated pool");
            if(definition.duration>0 && (candidates.empty() || candidates.back()!=tag.datum))
                candidates.push_back(tag.datum);
        }
    }
    check(!candidates.empty(),"No effect graph in this map owns a timed animated light");
    const auto ordinary_catalog=effects.light_definitions();
    for(const auto id:known)
        check(std::find(ordinary_catalog.begin(),ordinary_catalog.end(),id)==ordinary_catalog.end(),
            "An animated light entered the ordinary resident catalog");
    check(effects.lights().empty() && effects.animated_lights().empty(),"Preloading created a runtime light");
    WorldEffectFrame frame;frame.position={97.9881f,-175.637f,1.9f};frame.normal={0,0,1};frame.incoming={0,0,-1};
    const std::uint64_t birth=100;
    effects.advance(birth-1,1.f/30);
    // These graphs place their lights at named source locations of their
    // owner (cannon_flash, replace, core). Supply the declared world frame
    // under each string ID the graph asks for, as a completed source
    // instance does; probe in a disposable owner and use the first graph
    // that lights.
    const auto string_id=[&](const std::string& name)->std::uint32_t {
        for(std::uint32_t index=0;index<0x10000u;++index) {
            const auto id=std::uint32_t(name.size())<<24|index;
            try{if(cache->string_id(id)==name)return id;}catch(const FormatError&){}
        }
        throw FormatError("Effect location name has no string ID in this cache");
    };
    const std::uint32_t owner=0xe2340007u;
    Transform place;place.position=frame.position;
    std::vector<SourceEffectLocation> locations{{string_id("replace"),place}};
    std::set<std::string> named{"replace"};
    std::uint32_t effect=0xffffffffu;
    for(const auto candidate:candidates) {
        for(unsigned attempt=0;attempt<6 && effect==0xffffffffu;++attempt) {
            try {
                Mesh probe_mesh;NativeWorldEffects probe(*cache,probe_mesh,collision,0x12345678);
                (void)probe.try_preload_attached(candidate);
                probe.advance(birth-1,1.f/30);
                probe.spawn_source(candidate,0xffffffffu,birth,locations);
                bool lit=false;
                for(std::uint64_t tick=birth;tick<birth+40 && !lit;++tick) {
                    probe.advance(tick,1.f/30);lit=!probe.animated_lights().empty();
                }
                if(lit)effect=candidate;
                break;
            }catch(const FormatError& error) {
                const std::string text=error.what();const std::string prefix="World effect location has no native producer: ";
                if(text.rfind(prefix,0)!=0 || !named.insert(text.substr(prefix.size())).second)break;
                locations.push_back({string_id(text.substr(prefix.size())),place});
            }
        }
        if(effect!=0xffffffffu)break;
    }
    check(effect!=0xffffffffu,"No timed animated effect graph lights from its declared source locations");
    effects.spawn_source(effect,0xffffffffu,birth,locations);
    unsigned observed{},varied{};std::array<std::uint32_t,8> first{};bool retired=false;
    for(std::uint64_t tick=birth;tick<birth+120;++tick) {
        effects.advance(tick,1.f/30);
        for(const auto& light:effects.lights())
            check(!known.contains(light.definition),"An animated light was created in the ordinary pool");
        const auto lights=effects.animated_lights();
        if(lights.empty()) {if(observed)retired=true;continue;}
        check(!retired,"A retired animated light returned");
        for(const auto& light:lights) {
            check(known.contains(light.definition),"Animated pool holds an unknown definition");
            const auto definition=load_completed_effect_light(*cache,light.definition);
            auto expected=source_effect_light_state(definition,light.authored_control,
                std::bit_cast<std::int32_t>(std::uint32_t(tick)-light.birth),30,cache->original_data()->fade_curve);
            const auto plain=bits(expected);
            source_effect_light_animate(*definition.animation,light.identity,std::int32_t(tick),30,expected);
            check(bits(light.state)==bits(expected),
                "Animated light state differs from C2CB0 state followed by C2B30 at this tick");
            varied+=bits(expected)!=plain;
            if(!observed++)first=bits(light.state);
        }
    }
    // Every effect graph that owns an animated light and is now a complete
    // attached graph must run as an attachment: posed owner markers under
    // the names it asks for, ninety ticks, no declined owner.
    unsigned attached{},attached_lit{};
    for(const auto& tag:cache->tags())if(tag.group=="effe") {
        // The marker names these graphs place their parts at.
        std::set<std::string> names{"cannon_flash","replace","core"};
        NativeWorldEffects::ObjectMarkers posed;
        for(const auto& name:names)posed[name]={frame};
        for(unsigned attempt=0;attempt<8;++attempt) {
            Mesh one_mesh;NativeWorldEffects one(*cache,one_mesh,collision,0x12345678);
            const bool complete=one.try_preload_attached(tag.datum);
            if(one.animated_light_definitions().empty() || !complete)break;
            try {
                one.advance(birth-1,1.f/30);one.object_markers(owner,posed);
                one.spawn_attached(tag.datum,owner,0xffffffffu,birth);
                bool lit=false;
                for(std::uint64_t tick=birth;tick<birth+90;++tick) {
                    one.advance(tick,1.f/30);lit=lit || !one.animated_lights().empty();
                    for(const auto& light:one.lights())
                        check(!known.contains(light.definition),"An attached animated light entered the ordinary pool");
                }
                ++attached;attached_lit+=lit;
                std::cout<<"attached animated graph: "<<tag.name<<" markers="<<posed.size()<<" lit="<<lit<<'\n';
                break;
            }catch(const FormatError& error) {
                const std::string text=error.what();const std::string prefix="World effect location has no native producer: ";
                if(text.rfind(prefix,0)!=0 || !names.insert(text.substr(prefix.size())).second)
                    throw FormatError("Attached animated graph "+tag.name+" declined: "+text);
                posed[text.substr(prefix.size())]={frame};
            }
        }
    }
    check(attached && attached_lit,"No complete attached graph lit its animated light");
    check(observed>=3,"The timed animated light did not live for several ticks");
    check(varied,"The animation never changed the light's brightness or diffuse colour");
    check(retired,"The timed animated light outlived its authored duration");
    std::cout<<"PASS: "<<graphs<<" animated light definitions admitted from effect graphs; "<<observed
        <<" ticks matched C2CB0 state + C2B30, "<<varied<<" of them animated; ordinary pool untouched; retired on time; "
        <<attached<<" complete attached graphs ran, "<<attached_lit<<" lit\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
