#include "halo2/source_script_effect_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto content=cache.load_content(0,false);const auto &named=content.object_names.at(26);
    const auto placed=std::ranges::find_if(content.objects,[&](const auto &object) {
        return object.type==named.type && object.placement==named.placement;
    });
    if(placed==content.objects.end() || placed->definition!=0xe201008cu)
        throw FormatError("Actual opening explosion placement differs");
    auto path=(std::filesystem::temp_directory_path()/"halo2-opening-effects-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source effect test storage unavailable");
    struct Cleanup {std::string path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}}cleanup{path};
    std::cout<<"PHASE source-construction\n"<<std::flush;
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);
    auto &world=objects.tu15_world();world.initialize_map();
    world.configure_campaign_session(1,1,30,0x12345678);
    const auto interface=Bytes(world.memory().view(0x51df50u,4)).u32(0);
    if(!interface || Bytes(world.memory().view(interface,4)).u32(0)!=0x3f800000u ||
        world.memory().view(interface+5u,1)[0]!=std::byte{})
        throw FormatError("Original new-map interface did not consume the delivered campaign options");
    source_campaign_cinematic_control_tu15(world,true);
    std::cout<<"PHASE actual-scenery-birth\n"<<std::flush;
    const auto object=objects.create_scenario_object(*placed,SourceScenarioBirth::runtime);
    if(object==null_datum)throw FormatError("Original opening scenery constructor returned NONE");
    const auto &memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    source_campaign_cinematic_lod_tu15(world,object,true);
    if(!(word(objects.source_object_body(object)+4u)&0x80000u))
        throw FormatError("Original cinematic LOD admission did not reach its object");
    source_campaign_cinematic_lod_tu15(world,object,false);
    if(word(objects.source_object_body(object)+4u)&0x80000u)
        throw FormatError("Original cinematic LOD release did not reach its object");
    source_campaign_cinematic_lod_tu15(world,object,true);
    const auto effect_pool=word(0x4f720cu);
    if(!effect_pool)throw FormatError("Original effect pool is absent");
    const auto cinematic=word(0x51df50u);
    if(!cinematic || memory.view(cinematic+5u,1)[0]!=std::byte{1} || memory.view(0x4922a8u,1)[0]!=std::byte{})
        throw FormatError("Original cinematic control or CPU-only renderer admission differs");
    const auto definitions=[&] {
        std::map<std::uint32_t,unsigned> result;
        const auto data=word(effect_pool+0x44u),bitmap=word(effect_pool+0x48u),high=word(effect_pool+0x38u);
        for(unsigned i=0;i<high;++i)if(word(bitmap+i/32u*4u)&(1u<<(i%32u)))
            ++result[word(data+i*400u+4u)];
        return result;
    };
    for(const auto effect:{0xea070892u,0xea03088eu}) {
        const auto before=definitions()[effect];
        std::cout<<"PHASE script-request "<<std::hex<<effect<<" object="<<object<<std::dec<<'\n'<<std::flush;
        source_script_marker_effect_tu15(world,effect,object,0x130008fdu);
        const auto after=definitions()[effect];
        if(after!=before+1u)throw FormatError("Original script effect did not retain one actual allocation");
        std::cout<<"ALLOCATED "<<std::hex<<effect<<std::dec<<" count="<<after<<'\n';
    }
    // Explicit authored native campaign admission, separate from production
    // campaign ownership and from Xbox framebuffer/particle appearance proof.
    SourceWorldParticleDrawsTu15 graphics(cache,content.objects);
    if(!graphics.preload_world(0xea03088eu) || !graphics.active())
        throw FormatError("Actual opening particle resource census did not admit its script effect");
    const auto storage_before=word(0x4c2aa8u);
    objects.render_model_view([&](auto &source){graphics.prepare_storage(source);});
    if(!word(0x4c2aa8u))throw FormatError("Actual opening transparent particle storage is absent");
    std::cout<<"STORAGE before="<<std::hex<<storage_before<<" after="<<word(0x4c2aa8u)<<std::dec<<'\n';
    const auto saved=objects.authority_checkpoint();
    const auto copy=[&](std::uint32_t at,unsigned count) {
        const auto bytes=memory.view(at,count);return std::vector<std::byte>(bytes.begin(),bytes.end());
    };
    const auto saved_interface=copy(cinematic,0x24u);
    const auto saved_renderer=copy(word(0x515e2cu),0x34u);
    const auto saved_effects=copy(word(effect_pool+0x44u),word(effect_pool+0x38u)*400u);
    const auto retire=[&] {
    objects.destroy_object(object);world.call_actual(0x179cd0u);
    // Original17A4A0 frees an effect without a particle chain immediately.
    // With a live chain it clears particle emission, marks retirement and
    // retains the effect until those actual particles finish. Deletion
    // detaches the owner; it must not truncate the remaining particle life.
    if(definitions()[0xea070892u] || definitions()[0xea03088eu]!=1u)
        throw FormatError("Original opening effect retirement admission differs");
    const auto data=word(effect_pool+0x44u),bitmap=word(effect_pool+0x48u),high=word(effect_pool+0x38u);
    for(unsigned i=0;i<high;++i)if(word(bitmap+i/32u*4u)&(1u<<(i%32u))) {
        const auto effect=data+i*400u;
        if(word(effect+4u)==0xea03088eu &&
            (word(effect+0x48u)!=null_datum ||
             (Bytes(memory.view(effect+2u,2)).u16(0)&0x44u)!=0x44u ||
             word(effect+0xfcu)==null_datum))
            throw FormatError("Original particle effect did not detach and retain its retiring chain");
    }
    unsigned entered{},completed{};
    const SourceFirstPersonPhaseObserverTu15 observe=[&](bool before,const auto &,const auto &bank,auto trace,auto) {
        if(before)++entered;else ++completed;
        const auto data=word(effect_pool+0x44u),bitmap=word(effect_pool+0x48u),high=word(effect_pool+0x38u);
        unsigned active{},chains{};
        std::cout<<"CAMPAIGN_FRAME "<<(before?"before":"completed")<<" trace_pcs="<<trace.size()<<" rows=";
        for(unsigned i=0;i<high;++i)if(word(bitmap+i/32u*4u)&(1u<<(i%32u))) {
            ++active;const auto row=data+i*400u;
            chains+=word(row+0xfcu)!=null_datum;
            std::cout<<std::hex<<row<<':';
            constexpr char digits[]="0123456789abcdef";
            for(auto byte:bank.view(row,400u)) {
                const auto value=std::to_integer<unsigned>(byte);std::cout<<digits[value>>4]<<digits[value&15u];
            }
            std::cout<<';'<<std::dec;
        }
        std::cout<<" active="<<active<<" chains="<<chains<<'\n'<<std::flush;
        if(before && (!active || !chains))throw FormatError("Campaign particle frame did not enter with an actual live script chain");
        if(!before && trace.empty())throw FormatError("Actual campaign particle frame completed without source execution evidence");
    };
    for(unsigned tick=0;tick<240;++tick) {
        world.step_objects();
        objects.advance_effect_frame(1.f/30.f,tick==0?observe:SourceFirstPersonPhaseObserverTu15{});
    }
    if(entered!=1 || completed!=1)throw FormatError("Actual campaign179DF0 did not complete exactly once under observation");
    if(definitions()[0xea070892u] || definitions()[0xea03088eu])
        throw FormatError("Original opening particle lifetime did not release its retired effect");
    source_campaign_cinematic_control_tu15(world,false);
    if(memory.view(cinematic+5u,1)[0]!=std::byte{})throw FormatError("Original cinematic stop did not release its admission");
    };
    retire();
    if(!objects.commit_authority(objects.prepare_authority(saved)))
        throw FormatError("Original cinematic/effect checkpoint did not publish");
    if(copy(cinematic,0x24u)!=saved_interface || copy(word(0x515e2cu),0x34u)!=saved_renderer ||
        copy(word(effect_pool+0x44u),word(effect_pool+0x38u)*400u)!=saved_effects ||
        definitions()[0xea070892u]!=1u || definitions()[0xea03088eu]!=1u ||
        !(word(objects.source_object_body(object)+4u)&0x80000u))
        throw FormatError("Original cinematic/effect checkpoint lost retained control, identity or effect bytes");
    retire();
    std::cout<<"PASS actual opening scenery birth, marker lookup, LOD set/clear, two original script-effect allocations, authored transparent storage, 480 source game/effect-frame phases, two observed active-chain179DF0 completions, retirement, cinematic stop and source checkpoint restoration\n";
    std::cout<<"LIMIT no custom-animation bridge, per-view collection, lens queries/draws, full movie or Xbox pixels/audio\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
