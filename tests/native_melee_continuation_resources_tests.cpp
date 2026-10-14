#include "halo2/cache.hpp"
#include "halo2/native_melee_animation.hpp"
#include "halo2/original_data.hpp"
#include "halo2/simulation.hpp"
#include "world_continuation_bindings.hpp"
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
}
int main(int argc,char **argv)try {
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
