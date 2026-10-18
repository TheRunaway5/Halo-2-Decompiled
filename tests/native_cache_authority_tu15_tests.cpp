#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
using Checkpoint=PhysicsLiveWorldTu15::AuthorityCheckpoint;
using Region=SourceRuntimeMemory::CheckpointRegion;
void require(bool value,const char* why){if(!value)throw FormatError(why);}
Region& region(Checkpoint& checkpoint,std::uint32_t address) {
    const auto found=std::ranges::find(checkpoint.bank.regions,address,&Region::address);
    if(found==checkpoint.bank.regions.end())throw FormatError("Test checkpoint lost its expected owner");
    return *found;
}
void put(Checkpoint& checkpoint,std::uint32_t address,std::uint32_t value) {
    for(auto& owner:checkpoint.bank.regions)if(address>=owner.address &&
        std::uint64_t(address)-owner.address+4<=owner.bytes.size()) {
        const auto at=address-owner.address;
        for(unsigned n=0;n<4;++n)owner.bytes[at+n]=std::byte(value>>(n*8));
        return;
    }
    throw FormatError("Test checkpoint word has no retained owner");
}
}
int main(int argc,char** argv)try {
    using namespace halo2;
    if(argc<3 || !*argv[1] || !std::filesystem::is_regular_file(argv[1]) ||
        (argc>3 && *argv[3] && !std::filesystem::is_regular_file(argv[3]))) {
        std::cout<<"SKIP: supply an owned MCC multiplayer map, private save directory and optional offline TU1.5 XBE\n";
        return 77;
    }
    const Cache cache(argv[1]);
    require(cache.is_mcc(),"Authority regression requires an MCC cache");
    std::filesystem::create_directories(argv[2]);
    std::unique_ptr<PhysicsLiveWorldTu15> world;
    if(argc>3 && *argv[3])world=std::make_unique<PhysicsLiveWorldTu15>(cache,argv[3],argv[2]);
    else world=std::make_unique<PhysicsLiveWorldTu15>(cache,argv[2]);
    world->initialize_map();
    world->configure_session(source_game_variant_default(0),1,30,0x12345678u);
    const auto read=[&](std::uint32_t address){return Bytes(world->memory().view(address,4)).u32(0);};
    const auto base=read(0x556d90u),table=read(0x4f0414u);
    const auto original_world=world->world();
    const auto saved=world->authority_checkpoint();
    unsigned rejections{};
    const auto rejects=[&](const char* name,auto change) {
        auto corrupted=saved;change(corrupted);
        bool rejected=false;
        try{auto prepared=world->prepare_authority(corrupted);}
        catch(const std::exception&){rejected=true;}
        if(!rejected)throw FormatError(std::string("Accepted malformed MCC authority: ")+name);
        require(world->world()==original_world,"Rejected checkpoint changed the live world backing");
        require(world->authority_checkpoint().bank==saved.bank,"Rejected checkpoint changed live bank bytes or ownership");
        ++rejections;
    };
    rejects("missing native bank",[&](auto& image){
        std::erase_if(image.bank.regions,[&](const auto& owner){return owner.address==base;});
    });
    rejects("truncated native bank",[&](auto& image){region(image,base).bytes.pop_back();});
    rejects("split native bank",[&](auto& image){
        auto& owner=region(image,base);
        Region tail{base+4096,owner.access,owner.owner,0,
            SourceRuntimeSnapshotBytes(owner.bytes.begin()+4096,owner.bytes.end()),{}};
        owner.bytes=SourceRuntimeSnapshotBytes(owner.bytes.begin(),owner.bytes.begin()+4096);
        const auto found=std::ranges::find(image.bank.regions,base,&Region::address);
        image.bank.regions.insert(std::next(found),std::move(tail));
    });
    rejects("renamed native bank",[&](auto& image){region(image,base).owner="foreign gameplay allocation";});
    rejects("paged native bank",[&](auto& image){
        auto& owner=region(image,base);owner.page_bytes=4096;owner.committed.assign(owner.bytes.size()/4096,1);
    });
    rejects("read-only native bank",[&](auto& image){region(image,base).access=SourceRuntimeMemory::Access::ReadOnly;});
    for(const auto address:{0x55658cu,0x556d90u,0x4f0414u,0x474c20u,0x4ecc20u,0x4ecc1cu})
        rejects("vanilla publication binding",[&](auto& image){put(image,address,read(address)^4u);});
    rejects("index table binding",[&](auto& image){put(image,base+8u,table+16u);});
    rejects("index count",[&](auto& image){put(image,base+24u,read(base+24u)-1u);});
    rejects("tag salt",[&](auto& image){put(image,table+4u,read(table+4u)^0x10000u);});
    rejects("tag payload binding",[&](auto& image){put(image,table+8u,read(table+8u)+4u);});
    rejects("selected BSP binding",[&](auto& image){put(image,0x556d94u,read(0x556d94u)+16u);});
    auto prepared=world->prepare_authority(saved);
    require(world->commit_authority(std::move(prepared)),"Genuine MCC authority did not commit");
    require(world->world().value>UINT32_MAX,"Checkpoint narrowed the native world allocation");
    require(world->authority_checkpoint().bank==saved.bank,"Genuine checkpoint changed mutable definition fixups or ownership");
    require(read(0x556d90u)==base && read(0x4f0414u)==table,"Checkpoint changed vanilla tag publication");
    std::cout<<"PASS MCC authority: "<<rejections<<" malformed native owners/bindings rejected atomically; genuine mutable bank restored\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
