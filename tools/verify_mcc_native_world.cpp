#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <iostream>

int main(int argc,char** argv)try {
    using namespace halo2;
    if(argc<3)throw FormatError(
        "usage: verify_mcc_native_world MCC.map PRIVATE_SAVE_DIRECTORY [--spawn] [--checkpoint] [--object-runtime] [--reference-xbe OFFLINE_REFERENCE.xbe]");
    bool spawn=false,checkpoint=false,object_runtime=false;std::filesystem::path reference;
    for(int n=3;n<argc;++n) {
        const std::string_view option(argv[n]);
        if(option=="--spawn" && !spawn)spawn=true;
        else if(option=="--checkpoint" && !checkpoint)checkpoint=true;
        else if(option=="--object-runtime" && !object_runtime)object_runtime=true;
        else if(option=="--reference-xbe" && reference.empty() && n+1<argc)reference=argv[++n];
        else throw FormatError("Unknown or repeated world probe option");
    }
    const Cache cache(argv[1]);
    if(!cache.is_mcc())throw FormatError("This integration probe requires an imported MCC cache");
    std::filesystem::create_directories(argv[2]);
    std::unique_ptr<PhysicsLiveWorldTu15> direct_world;
    std::unique_ptr<MultiplayerObjectRuntime> objects;
    if(object_runtime) {
        objects=reference.empty()?std::make_unique<MultiplayerObjectRuntime>(cache,0,argv[2]):
            std::make_unique<MultiplayerObjectRuntime>(reference,cache,0,argv[2]);
    } else {
        direct_world=reference.empty()?std::make_unique<PhysicsLiveWorldTu15>(cache,argv[2]):
            std::make_unique<PhysicsLiveWorldTu15>(cache,reference,argv[2]);
    }
    auto* world=objects?&objects->tu15_world():direct_world.get();
    const auto read=[&](std::uint32_t address){return Bytes(world->memory().view(address,4)).u32(0);};
    const auto index=read(0x556d90u),table=read(0x4f0414u);
    if(read(index+28u)!=0x74616773u || read(index+24u)!=cache.tags().size())
        throw FormatError("Converted MCC data did not publish the vanilla tag index");
    for(const auto& tag:cache.tags())if(!tag.handle().is_null()) {
        const auto row=table+tag.handle().index()*16u;
        if(read(row)!=tag.type().value || read(row+4u)!=tag.datum)
            throw FormatError("Converted vanilla tag directory changed authored group or salted identity");
    }
    std::cout<<std::unitbuf<<"PASS MCC vanilla tag/BSP publication: "<<cache.name()<<'\n';
    world->initialize_map();
    const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
    if(!scenario)throw FormatError("MCC world probe lost its scenario owner");
    const auto kind=Bytes(cache.read_tag_bytes(scenario->address,0x12)).u16(0x10);
    if(kind==0)world->configure_campaign_session(1,1,30,0x12345678u);
    else if(kind==1)world->configure_session(source_game_variant_default(0),1,30,0x12345678u);
    else throw FormatError("World probe requires a gameplay scenario");
    if(world->world().is_null() || world->world().value<=UINT32_MAX)
        throw FormatError("MCC gameplay world did not retain a native 64-bit allocation");
    std::cout<<"PASS MCC original gameplay world construction and session configuration\n";
    if(spawn) {
        const auto biped=std::ranges::find_if(cache.tags(),[](const Tag& tag) {
            return tag.group=="bipd" && tag.name=="objects\\characters\\masterchief\\masterchief_mp";
        });
        if(biped==cache.tags().end() || cache.spawns().empty())
            throw FormatError("Biped probe requires an authored multiplayer biped and spawn");
        const auto body=objects?objects->create_biped(biped->datum,cache.spawns().front().position,{1,0,0}):
            world->create_biped(biped->datum,cache.spawns().front().position,{1,0,0});
        if(body==0xffffffffu || world->object_body(body).value<=UINT32_MAX)
            throw FormatError("MCC biped creation did not retain its native body owner");
        std::cout<<"PASS MCC original biped creation and native body ownership\n";
    }
    if(checkpoint) {
        if(objects) {
            const auto saved=objects->authority_checkpoint();
            auto prepared=objects->prepare_authority(saved);
            if(!objects->commit_authority(std::move(prepared)))
                throw FormatError("MCC object checkpoint did not publish its prepared native owners");
        } else {
            const auto saved=world->authority_checkpoint();
            auto prepared=world->prepare_authority(saved);
            if(!world->commit_authority(std::move(prepared)))
                throw FormatError("MCC world checkpoint did not publish its prepared native owners");
        }
        if(world->world().value<=UINT32_MAX || read(0x556d90u)!=index || read(0x4f0414u)!=table)
            throw FormatError("MCC checkpoint changed vanilla tag bindings or narrowed native ownership");
        std::cout<<"PASS MCC native allocation checkpoint and vanilla tag rebinding\n";
    }
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
