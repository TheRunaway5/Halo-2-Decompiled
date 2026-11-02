#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/source_transparent_group_parts.hpp"
#include <algorithm>
#include <iostream>
#include <optional>

namespace {
using namespace halo2;
void require(bool ok,const char *message){if(!ok)throw FormatError(message);}
template<class F>void rejects(F &&action){bool rejected{};try{action();}catch(const FormatError&){rejected=true;}require(rejected,"Transparent native pass guard accepted an unsupported owner");}
}
int main(int argc,char **argv)try {
    using namespace halo2;
    if(argc!=4 || !std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1],argv[2]);if(!cache.is_mcc())return 77;
    // Exact tuple observed at the ordinary scene02 rejection. This is a
    // regression input, never a production shader/part selection rule.
    const auto pass=load_menu_world_source_pass(cache,0xe7ad0637u,{0,1,0,0});
    require(pass.implementation_address()==0x1d4325cu,"Captured Cache pass address changed");
    PhysicsLiveWorldTu15 world(cache,argv[3]);auto &memory=world.memory();
    const auto backing=world.resident_cache_address(cache,pass.implementation_address(),306);
    const auto address=memory.source_address(backing,306);
    require(backing.value>UINT32_MAX && address==0x9137b1dcu,
        "Captured transparent descriptor is not the current native allocation");
    require(std::ranges::equal(memory.view(backing,306),pass.implementation_record()),
        "Actual complete resident pass differs from its Cache preimage");
    require(pass.implementation_address()!=address,"MCC address-domain RED control disappeared");
    unsigned minted{};
    const SourceTransparentGroupNativePassOwner current=[&](const MenuWorldSourcePass &selected) {
        ++minted;return source_shadow_raw_pass_owner_tu15(world,cache,selected);
    };
    validate_source_transparent_group_pass_owner(cache,pass,address,current);
    rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,address);});
    rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,pass.implementation_address());});
    rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,address^4u,current);});
    rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,pass.implementation_address(),current);});
    // A change at the LAST byte must fail a fresh mint, proving the complete
    //306B check rather than a matching prefix/address or a cached capsule.
    const auto last=memory.view(address+305u,1).front();const std::array altered{last^std::byte{1}};
    memory.write(address+305u,altered);
    rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,address,current);});
    memory.write(address+305u,std::span(&last,1));
    validate_source_transparent_group_pass_owner(cache,pass,address,current);
    std::optional<MenuWorldSourcePass> other;
    for(unsigned layer:{3u,8u,7u,2u,16u,15u,21u,10u})try {
        other=load_menu_world_source_pass(cache,pass.shader(),{0,layer,0,0});break;
    }catch(const FormatError&){}
    if(other) {
        const SourceTransparentGroupNativePassOwner wrong=[&](const MenuWorldSourcePass &) {
            return source_shadow_raw_pass_owner_tu15(world,cache,*other);
        };
        rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,address,wrong);});
    }else rejects([&]{(void)load_menu_world_source_pass(cache,pass.shader(),{0,31,0,0});});
    const Cache foreign(argv[1],argv[2]);
    const auto foreign_pass=load_menu_world_source_pass(foreign,pass.shader(),pass.choice());
    rejects([&]{validate_source_transparent_group_pass_owner(cache,foreign_pass,address,current);});
    const SourceTransparentGroupNativePassOwner foreign_owner=[&](const MenuWorldSourcePass &) {
        return source_shadow_raw_pass_owner_tu15(world,foreign,foreign_pass);
    };
    rejects([&]{validate_source_transparent_group_pass_owner(cache,pass,address,foreign_owner);});
    require(minted==5,"Transparent resolver cached a native pass capsule across validations");
    // Actual first pStencil descriptor, independently observed in V9. Its
    // layer12 implementation has the same logical/resident address boundary.
    const auto projected=load_menu_world_source_pass(cache,3801088282u,{0,12,0,0});
    require(projected.implementation_address()==30469740u,
        "Captured projected Cache implementation changed");
    const auto projected_backing=world.resident_cache_address(cache,projected.implementation_address(),306);
    const auto projected_address=memory.source_address(projected_backing,306);
    require(projected_address!=projected.implementation_address(),
        "Projected logical/resident RED control disappeared");
    auto projected_current=[&]{return source_shadow_raw_pass_owner_tu15(world,cache,projected);};
    require(projected_current().matches(projected,projected_address),
        "Actual projected descriptor lost its fresh resident implementation owner");
    require(!projected_current().matches(projected,projected.implementation_address()) &&
            !projected_current().matches(projected,projected_address^4u) &&
            !projected_current().matches(pass,address),
        "Projected owner accepted a logical, wrong or unrelated pass pointer");
    const auto projected_last=memory.view(projected_address+305u,1).front();
    const std::array projected_altered{projected_last^std::byte{1}};
    memory.write(projected_address+305u,projected_altered);
    rejects([&]{(void)projected_current();});
    memory.write(projected_address+305u,std::span(&projected_last,1));
    require(projected_current().matches(projected,projected_address),
        "Projected owner did not reauthenticate the restored final byte");
    const auto projected_foreign=load_menu_world_source_pass(foreign,projected.shader(),projected.choice());
    const SourceTransparentGroupNativePassOwner projected_foreign_owner=[&](const MenuWorldSourcePass&) {
        return source_shadow_raw_pass_owner_tu15(world,foreign,projected_foreign);
    };
    rejects([&]{validate_source_transparent_group_pass_owner(cache,projected,projected_address,projected_foreign_owner);});
    std::cout<<"PASS actual MCC transparent tuple: Cache=0x1d4325c resident=0x9137b1dc; complete306B, fresh mint="<<minted
        <<", wrong selected choice="<<bool(other)<<", missing provider/wrong pointer/foreign Cache/damaged preimage rejected\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
