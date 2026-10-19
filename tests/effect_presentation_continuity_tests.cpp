#include "halo2/effect_presentation_continuity.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_authority_codec.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F &&call){bool rejected=false;try{call();}catch(const FormatError&){rejected=true;}
    check(rejected,"Invalid presentation cut was accepted");}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply TU15_XBE TURF_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.local_players=1;options.mute=true;
    options.shared_maps=options.map.parent_path().parent_path().parent_path()/"disc/maps";
    // Resolve the actual authored choice, never a guessed weapon-option index.
    options.starting_weapon=0;
    {
        Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(argv[1]));
        for(unsigned choice=3;choice<=20;++choice)try {
            const auto datum=cache.slayer_starting_weapon(choice);
            const auto tag=cache.query_tag({.handle=TagHandle{datum}});
            if(tag && tag->name.ends_with("\\plasma_pistol")){options.starting_weapon=choice;break;}
        }catch(const FormatError&){}
        check(options.starting_weapon!=0,"No authored Plasma Pistol choice");
    }
    MultiplayerPreparedWorld source(options);const auto player=source.match.join("Presentation continuity");
    check(source.cache->original_data()->image==OriginalData::Image::TitleUpdate15,"Requires TU1.5");
    const std::array<unsigned,1> views{player};source.bind_source_local_views(views);
    std::uint32_t primary=0x10203040,auxiliary=0x12345678;
    const SourceEffectRandom random{primary,auxiliary};Mesh mesh;
    NativeWorldEffects effects(*source.cache,mesh,source.match.simulation().collision(),random);
    NativeMuzzleEffects muzzle(*source.cache,random);
    SourceMapEffects map(*source.cache,source.content.objects,effects);
    std::vector<std::uint32_t> definitions;
    for(const auto group:{tag_groups::weapon,tag_groups::projectile})
        for(const auto &tag:source.cache->select_tags({.group=group,.require_address=true,.minimum_size=0x9c}))
            definitions.push_back(tag.datum);
    SourceCombatAttachments combat(*source.cache,effects,definitions);
    EffectPresentationContinuity presentation(muzzle,effects,map,combat,random);
    EffectPresentationContinuity foreign(muzzle,effects,map,combat,random);
    const auto initial=source.match.state();std::uint32_t named=null_datum;
    for(const auto &root:initial.source_scene_roots) {
        const auto tag=source.cache->load_source_tag_metadata(root.definition);if(tag.size()<0x9c)continue;
        const Bytes body(tag.bytes_at(tag.source_address(),0x9c));if(!body.u32(0x94))continue;
        const Bytes rows(source.cache->read_tag_bytes(body.u32(0x98),std::size_t(body.u32(0x94))*24));
        for(const auto &e:root.attached_effects)if(rows.u32(e.slot*24+16) || rows.u32(e.slot*24+20)){
            named=root.object;break;}
        if(named!=null_datum)break;
    }
    check(named!=null_datum,"No actual named map attachment owner");
    auto vitality=source.source_objects->object_vitality(named);vitality.feedback.body={.75f,.75f,30};
    source.source_objects->deliver_vitality(named,vitality);
    std::array<Controls,multiplayer_capacity> controls{};
    const auto synchronize=[&] {
        const auto state=source.match.state();std::vector<std::vector<std::uint8_t>> permutations;
        permutations.reserve(source.match.simulation().scene().objects().size());std::vector<SourceSceneEffectOwner> owners;
        for(const auto &object:source.match.simulation().scene().objects())if(object.handle!=null_datum) {
            const auto type=source.content.objects.at(object.placement).type;
            if((type!=6 && type!=7 && type!=11) || !object.solved_nodes || object.solved_nodes->empty())continue;
            permutations.push_back(source.source_objects->object_model_regions(object.handle).selection.permutations);
            owners.push_back({object.placement,object.handle,object.created_tick,*object.solved_nodes,permutations.back(),object.position});
        }
        map.synchronize(owners,state.tick,state.source_scene_roots);
        (void)combat.synchronize(*source.source_objects,state.tick);
        effects.advance(state.tick,tick_seconds);muzzle.advance(state.tick,tick_seconds);
    };
    const auto step=[&] {
        source.source_objects->publish_native_observer_position(0,source.source_objects->object_transform(named).position+Vec3{0,0,1});
        source.match.step(controls);synchronize();
    };
    for(unsigned i=0;i<45;++i)step();
    check(effects.diagnostics().particles>0,"Named source function has no live particles");
    const auto active=combat.checkpoint().active_instances();
    check(active>0,"No actual native weapon/projectile attachment instances were captured");
    AuthorityTransferContext context;context.host=0;context.peers=3;context.epoch=2;context.membership_revision=1;
    context.session[0]=std::byte{1};const auto binding=source_authority_binding(context);
    SourceMatchAuthority authority(source.match,*source.source_objects);const auto native_cut=authority.checkpoint(binding);
    const auto bytes=encode_source_authority(native_cut);
    ValidatedAuthorityCut cut;cut.peer=0;cut.context=context;cut.seal.id=7;cut.seal.tick=source.match.state().tick;cut.seal.bytes=bytes.size();
    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char*>(cut.seal.digest.data()));
    const auto snapshot=presentation.capture(context);const auto births=map.stats().spawned;
    auto wrong_tick=cut;++wrong_tick.seal.tick;
    rejects([&]{(void)presentation.checkpoint(wrong_tick);});
    const std::array accepted_random{primary,auxiliary};const auto accepted_counts=effects.diagnostics();
    MuzzleEffectView view{};view.forward={1,0,0};view.up={0,0,1};view.width=640;view.height=480;view.orientation={1,0,0,0,1,0,0,0,1};
    const auto accepted_draws=effects.draw(view,0,tick_seconds);
    for(unsigned i=0;i<10;++i)step();
    // Discard only presentation owners; actual native allocations stay valid.
    for(const auto &object:source.source_objects->live_objects())
        if(object.type==2 || object.type==5)effects.remove_object(object.object);
    rejects([&]{(void)combat.synchronize(*source.source_objects,source.match.state().tick);});
    rejects([&]{(void)combat.checkpoint();});
    map.synchronize({},source.match.state().tick); // Explicit discarded presentation future.
    // A duplicated source root is rejected before a successful synchronization.
    const auto roots=source.match.state().source_scene_roots;
    check(!roots.empty(),"No source roots for rejection control");
    std::array duplicate{roots.front(),roots.front()};
    rejects([&]{map.synchronize({},source.match.state().tick,duplicate);});
    rejects([&]{(void)map.checkpoint();});
    // Bind after the live world/caches advanced and faulted. No recapture is
    // permitted to replace the owned state of the earlier completed frame.
    const auto checkpoint=presentation.bind(snapshot,cut);
    rejects([&]{(void)presentation.bind(snapshot,wrong_tick);});
    auto wrong_generation=cut;++wrong_generation.context.epoch;
    rejects([&]{(void)presentation.bind(snapshot,wrong_generation);});
    const std::array future_random{primary,auxiliary};
    rejects([&]{(void)foreign.prepare_restore(checkpoint,cut,{},{});});
    for(unsigned change=0;change<10;++change) {
        auto wrong=cut;
        switch(change){case 0:++wrong.peer;break;case 1:wrong.context.session[1]=std::byte{2};break;
        case 2:++wrong.context.epoch;break;case 3:++wrong.context.membership_revision;break;case 4:wrong.context.peers=7;break;
        case 5:wrong.context.host=1;break;case 6:++wrong.seal.id;break;case 7:++wrong.seal.tick;break;
        case 8:++wrong.seal.bytes;break;default:wrong.seal.digest[1]=std::byte{3};break;}
        rejects([&]{(void)presentation.prepare_restore(checkpoint,wrong,{},{});});
    }
    {
        auto native=authority.prepare(native_cut,binding);
        auto prepared=presentation.prepare_restore(checkpoint,cut,{},{});
        check(primary==future_random[0] && auxiliary==future_random[1],"Preparation changed shared visual RNG");
        auto wrong=cut;++wrong.context.epoch;
        check(!presentation.commit_restore(std::move(prepared),wrong) && !foreign.commit_restore(std::move(prepared),cut),
            "Wrong cut/owner published a grouped restore");
        bool rejected=false,foreign_ready=false;std::thread other([&]{
            try{(void)presentation.checkpoint(cut);}catch(const FormatError&){rejected=true;}
            foreign_ready=presentation.restore_ready(prepared,cut);
        });other.join();check(rejected && !foreign_ready,"Capture/publication crossed its executor");
        check(presentation.restore_ready(prepared,cut),"Grouped prepared state was consumed by rejection");
        check(authority.commit(std::move(native)),"Native accepted cut did not publish");
        check(presentation.commit_restore(std::move(prepared),cut) && !presentation.commit_restore(std::move(prepared),cut),
            "Grouped restore lost or reused publication");
    }
    check(primary==accepted_random[0] && auxiliary==accepted_random[1],"Grouped restore lost shared RNG");
    check(source.match.state().tick==cut.seal.tick && map.stats().spawned==births &&
        combat.checkpoint().active_instances()==active,"Accepted native/cache state was not restored together");
    check(effects.diagnostics().particles==accepted_counts.particles,"Grouped restore lost live world particles");
    const auto restored_draws=effects.draw(view,0,tick_seconds);
    check(restored_draws.size()==accepted_draws.size(),"Grouped restore changed particle draw count");
    for(unsigned i=0;i<restored_draws.size();++i)check(restored_draws[i].vertex.attributes==accepted_draws[i].vertex.attributes &&
        restored_draws[i].vertex.constants==accepted_draws[i].vertex.constants,"Grouped restore changed completed particle inputs");
    synchronize();check(map.stats().spawned==births,"Accepted cache synchronization replayed map births");
    check(effects.diagnostics().effects==accepted_counts.effects,"Accepted combat/map cache duplicated a live effect");
    for(unsigned i=0;i<20;++i)step();
    std::cout<<"PASS grouped native/effect restore tick="<<cut.seal.tick<<" map_births="<<births<<" combat_instances="<<active
        <<" particles="<<accepted_counts.particles<<"; 20 continuation ticks, exact cut fields, faulted-cache recovery, executor fencing and shared RNG\n";
    std::cout<<"LIMIT local encoded native cut and in-process preparation; context is fixture metadata, no quorum/transport/runtime/GPU/device migration; first-person presenter is empty\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
