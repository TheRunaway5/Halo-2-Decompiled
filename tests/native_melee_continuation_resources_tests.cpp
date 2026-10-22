#include "halo2/cache.hpp"
#include "halo2/native_melee_animation.hpp"
#include "halo2/original_data.hpp"
#include "halo2/simulation.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "runtime/world_continuation_bindings.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const FormatError &){rejected=true;}
    check(rejected,"Different admitted melee inputs were accepted");}
struct Directory {
    std::filesystem::path path;
    Directory(){auto name=(std::filesystem::temp_directory_path()/"halo2-melee-resources-XXXXXX").string();
        check(::mkdtemp(name.data()),"Cannot create private melee resource directory");path=name;}
    ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Strike {
    NativeMeleeAction action;std::uint32_t random{};std::vector<std::uint32_t> published;
};
Strike strike(NativeMeleeAnimation &owner,const LevelContent &content,std::uint32_t weapon){
    Unit unit;unit.exists=true;unit.definition=content.player_representations.at(2).biped;
    unit.weapon.definition=weapon;unit.body.grounded=true;
    Strike result;result.random=0x12345678;
    owner.update(unit,true,90,result.random,[&](auto activity){result.published.push_back(activity);});
    result.action=unit.melee_action;
    check(result.action.active()&&!result.published.empty(),"Actual melee resources never admitted a strike");
    return result;
}
void same(const Strike &a,const Strike &b){
    check(a.random==b.random&&a.published==b.published&&a.action.activity==b.action.activity&&
        a.action.started==b.action.started&&a.action.progress==b.action.progress&&a.action.impact==b.action.impact&&
        a.action.recovery==b.action.recovery&&a.action.duration==b.action.duration&&
        a.action.modifiers==b.action.modifiers&&a.action.speed_modifier==b.action.speed_modifier,
        "Replacement melee resources changed authored events, timing or RNG");
}
struct Trace {
    std::vector<NativeMeleeAction> states,impacts;
    std::vector<std::uint32_t> published;
    std::uint32_t random=0x12345678,action_word{};
};
auto action_values(const NativeMeleeAction &s){return std::tuple(s.activity,s.started,s.progress,s.impact,
    s.recovery,s.duration,s.modifiers,s.speed_modifier);}
Trace trace(NativeMeleeAnimation &owner,const LevelContent &content,int representation,
    std::uint32_t weapon,int kind){
    Unit unit;unit.exists=true;unit.definition=content.player_representations.at(representation).biped;
    check(unit.definition.datum!=null_datum,"MCC melee fixture has no actual player representation");
    unit.weapon.definition=weapon;unit.body.grounded=true;Trace result;
    for(unsigned step=0;step<256;++step){
        owner.update(unit,step==0,90+step,result.random,
            [&](auto activity){result.published.push_back(activity);},
            [&](const auto &impact){result.impacts.push_back(impact);},kind);
        result.states.push_back(unit.melee_action);
        if(step==0)check(unit.melee_action.active()&&unit.melee_action.duration>0,
            "Actual MCC melee resources never admitted a timed strike");
        if(!unit.melee_action.active())break;
    }
    check(!result.states.back().active()&&!result.published.empty(),"MCC melee animation did not finish its real clock");
    check(!result.impacts.empty(),"MCC melee animation lost its authored impact request");
    result.action_word=unit.source_action_word;return result;
}
void same(const Trace &a,const Trace &b){
    check(a.random==b.random&&a.published==b.published&&a.action_word==b.action_word&&
        a.states.size()==b.states.size()&&a.impacts.size()==b.impacts.size(),
        "MCC replacement changed animation/event/RNG suffix");
    for(unsigned n=0;n<a.states.size();++n)check(action_values(a.states[n])==action_values(b.states[n]),
        "MCC replacement changed an actual melee clock state");
    for(unsigned n=0;n<a.impacts.size();++n)check(action_values(a.impacts[n])==action_values(b.impacts[n]),
        "MCC replacement changed an authored melee impact");
}
int mcc(const std::filesystem::path &map){
    if(!std::filesystem::is_regular_file(map))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"MCC melee controls require the managed private scratch owner");
    Directory files;const auto shared=std::filesystem::absolute(map).parent_path();
    unsigned pinned{};
    for(const auto name:{"mainmenu.map","shared.map","single_player_shared.map"})
        if(std::filesystem::is_regular_file(shared/name)){
            std::filesystem::create_symlink(shared/name,files.path/name);++pinned;
        }
    check(pinned==3,"MCC melee fixture requires all three genuine MCC shared resource inputs");
    // Cache's MCC texture/sound loaders use these actual containers, and the
    // retained resource owner authenticates facial sectors in sounds_en.dat.
    // These links remain read-only inputs; all replacement controls below
    // remove a link before creating a fixture-owned file at that name.
    for(const auto name:{"textures.dat","sounds_en.dat","sounds_neutral.dat","sounds_remastered.dat"}){
        check(std::filesystem::is_regular_file(shared/name),"MCC melee fixture lacks an actual auxiliary resource container");
        std::filesystem::create_symlink(shared/name,files.path/name);
    }
    auto cache=std::make_shared<Cache>(map,files.path);
    check(cache->is_mcc()&&!cache->is_vista()&&cache->original_data()==OriginalData::native_tu15(),
        "MCC melee continuation must use the actual MCC cache with compiled operands and no XBE input");
    auto content=cache->load_content(0,true);
    check(cache->original_data()->melee_constants.has_value(),"Compiled native melee operands unavailable");
    const auto constants=*cache->original_data()->melee_constants;
    NativeMeleeAnimation owner(cache,content,constants);
    // This is the production continuation capture boundary that used to throw
    // for every MCC cache, despite its source resource owner supporting MCC.
    const auto binding=owner.authority_binding();
    {
        SourceRuntimeResources resources(*cache);
        std::map<std::uint32_t,int> representations;std::map<std::uint32_t,std::uint32_t> graphs;
        for(unsigned i=2;i<content.player_representations.size();++i)
            if(const auto id=content.player_representations[i].biped.datum;id!=null_datum){
                representations.emplace(id,int(i));graphs.emplace(id,cache->load_model_animation_graph(id));
            }
        check(binding==world_continuation::melee_binding(resources.identity(),constants,content.movement.forward,
            representations,graphs),"MCC melee binding differs from its authentic retained resource identity");
    }
    const auto weapon=cache->slayer_starting_weapon(5);std::uint32_t sword=null_datum;
    for(const auto &tag:cache->select_tags({.group=tag_groups::weapon,.require_address=true}))
        if(tag.name.ends_with("\\energy_blade")){check(sword==null_datum,"Duplicate authored MCC sword");sword=tag.datum;}
    check(weapon!=null_datum&&sword!=null_datum,"Actual MCC SMG/sword authored definitions unavailable");
    auto other_constants=constants;other_constants.frame_seconds_bits^=1;
    NativeMeleeAnimation changed_constants(cache,content,other_constants);
    check(changed_constants.authority_binding()!=binding,"MCC continuation omitted selected native operands");
    rejects([&]{changed_constants.prepare_authority_owner(content,binding);});
    struct Case{int representation;std::uint32_t weapon;int kind;Trace before;};
    std::vector<Case> cases;
    for(const int representation:{2,3})for(const auto &[selected,kind]:
        std::array<std::pair<std::uint32_t,int>,3>{{{weapon,1},{sword,1},{sword,2}}})
        cases.push_back({representation,selected,kind,trace(owner,content,representation,selected,kind)});
    for(const auto name:{"mainmenu.map","shared.map","single_player_shared.map"})
        check(std::filesystem::remove(files.path/name),"Cannot retire fixture-private shared resource link");
    check(owner.authority_binding()==binding,"MCC continuation reopened a changed private resource path");
    auto restored=owner.prepare_authority_owner(content,binding);
    check(restored->authority_binding()==binding,"MCC replacement lost its retained resource identity");
    for(const auto &c:cases)same(c.before,trace(*restored,content,c.representation,c.weapon,c.kind));
    auto changed=content;changed.movement.forward+=.125f;
    rejects([&]{owner.prepare_authority_owner(changed,binding);});
    auto wrong=binding;wrong[0]^=std::byte{1};rejects([&]{owner.prepare_authority_owner(content,wrong);});
    std::unique_ptr<NativeMeleeAnimation> missing_shared;
    try{missing_shared=std::make_unique<NativeMeleeAnimation>(cache,content,constants);}
    catch(const FormatError &){/* Strict eager dependency admission may reject the missing files. */}
    if(missing_shared){
        check(missing_shared->authority_binding()!=binding,"New MCC resource admission hid changed shared-file presence");
        rejects([&]{missing_shared->prepare_authority_owner(content,binding);});
    }
    // Invalid files are owned by this scratch directory, never the imported
    // MCC cache. A new owner must decode/reject them; old owners stay pinned.
    for(const auto name:{"mainmenu.map","shared.map","single_player_shared.map"}){
        std::ofstream replacement(files.path/name,std::ios::binary);replacement.put('x');
    }
    rejects([&]{NativeMeleeAnimation malformed(cache,content,constants);});
    auto again=restored->prepare_authority_owner(content,binding);
    for(const auto &c:cases)same(c.before,trace(*again,content,c.representation,c.weapon,c.kind));
    std::cout<<"PASS MCC melee immutable owner: exact resource digest, both representations, SMG/sword clocks and impact/RNG suffixes, retained replacement and changed-input rejection; cases="<<cases.size()<<'\n';
    return 0;
}
}
int main(int argc,char **argv)try {
    if(argc==3&&std::string_view(argv[1])=="--mcc-only")return mcc(argv[2]);
    check(argc==3,"Supply the owned TU1.5 XBE and Xbox map");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    const auto original=OriginalData::load_xbe(argv[1]);
    Directory files;
    const auto shared=std::filesystem::absolute(argv[2]).parent_path();
    for(const auto name:{"mainmenu.map","shared.map","single_player_shared.map"})
        if(std::filesystem::exists(shared/name))std::filesystem::create_symlink(shared/name,files.path/name);
    const auto cache=std::make_shared<Cache>(argv[2],files.path,original);
    auto content=cache->load_content(0,true);
    NativeMeleeAnimation owner(cache,content,*original->melee_constants);
    const auto binding=owner.authority_binding();
    std::map<std::uint32_t,int> representations;std::map<std::uint32_t,std::uint32_t> graphs;
    for(unsigned i=2;i<content.player_representations.size();++i)if(content.player_representations[i].biped.datum!=null_datum){
        const auto id=content.player_representations[i].biped.datum;
        representations.emplace(id,int(i));graphs.emplace(id,cache->load_model(id,false).animation);
        check(cache->load_model_animation_graph(id)==graphs.at(id),"Metadata-only model graph differs from full model import");
    }
    check(binding==world_continuation::melee_binding(*cache,*original->melee_constants,content.movement.forward,
        representations,graphs),"Pinned melee binding changed the canonical resource digest");
    const auto weapon=cache->slayer_starting_weapon(5);
    const auto before=strike(owner,content,weapon);
    // Remove only this fixture's symlinks before replacing files. Never write
    // through a link to the user's original caches.
    for(const auto name:{"mainmenu.map","shared.map","single_player_shared.map"}) {
        const auto path=files.path/name;std::filesystem::remove(path);
        std::ofstream replacement(path,std::ios::binary);replacement.put('x');
    }
    check(owner.authority_binding()==binding,"Melee continuation reopened changed shared-resource files");
    auto restored=owner.prepare_authority_owner(content,binding);
    check(restored->authority_binding()==binding,"Replacement melee owner lost the pinned resource identity");
    same(before,strike(*restored,content,weapon));
    auto changed=content;changed.movement.forward+=.125f;
    rejects([&]{owner.prepare_authority_owner(changed,binding);});
    auto wrong=binding;wrong[0]^=std::byte{1};rejects([&]{owner.prepare_authority_owner(content,wrong);});
    NativeMeleeAnimation different(cache,content,*original->melee_constants);
    check(different.authority_binding()!=binding,"A newly admitted resource owner hid changed file bytes");
    rejects([&]{different.prepare_authority_owner(content,binding);});
    std::filesystem::remove_all(files.path);
    auto again=restored->prepare_authority_owner(content,binding);
    same(before,strike(*again,content,weapon));
    std::cout<<"PASS pinned TU1.5 melee resources, canonical digest, restored authored strike/RNG and changed-input rejection\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
