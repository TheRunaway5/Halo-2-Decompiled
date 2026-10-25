#include "halo2/mcc_gameplay_reference.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value) {
    Bytes(bytes).require(at,4);for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
constexpr std::array<std::string_view,14> paths{
    "objects\\vehicles\\warthog\\warthog","objects\\vehicles\\ghost\\ghost",
    "objects\\vehicles\\scorpion\\scorpion","objects\\vehicles\\wraith\\wraith",
    "objects\\vehicles\\banshee\\banshee","objects\\vehicles\\spectre\\spectre",
    "objects\\vehicles\\mongoose\\mongoose","objects\\vehicles\\h_turret_ap\\h_turret_ap",
    "","","objects\\vehicles\\c_turret_ap\\c_turret_ap","objects\\vehicles\\creep\\creep_mp",
    "objects\\vehicles\\falcon\\falcon","objects\\vehicles\\gravity_throne\\gravity_throne_mp"};
struct Fixture {
    static constexpr unsigned root=0x1000,runtime=0x1200,bank=0x1800;
    std::vector<std::byte> bytes=std::vector<std::byte>(0x1000,std::byte{0xa5});
    std::array<MccGameplayReferenceIdentity,14> identities;
    bool alias_salt{};
    Fixture() {
        put(bytes,8,1);put(bytes,12,runtime);put(bytes,runtime-root+0x70,14);put(bytes,runtime-root+0x74,bank);
        for(unsigned i=0;i<paths.size();++i) {
            identities[i]={TagHandle{0xabcd0100u+i},tag_groups::vehicle,paths[i]};
            put(bytes,bank-root+i*8,paths[i].empty()?0xffffffffu:tag_groups::vehicle.value);
            put(bytes,bank-root+i*8+4,paths[i].empty()?0xffffffffu:identities[i].handle.value);
        }
    }
    MccGameplayReferenceAccess access() {
        return {.bytes=[&](std::uint32_t at,std::size_t size){
            if(at<root)throw FormatError("Fixture owner underflow");Bytes(bytes).require(at-root,size);
            return std::span(bytes).subspan(at-root,size);
        },.resolve=[&](TagHandle handle)->std::optional<MccGameplayReferenceIdentity>{
            for(const auto& id:identities)if(id.handle==handle || (alias_salt && id.handle.index()==handle.index()))return id;
            return {};
        },.find={}};
    }
    auto convert(){return convert_mcc_multiplayer_vehicle_table(tag_groups::multiplayer_globals,
        "multiplayer\\multiplayer_globals",root,access());}
};
}
int main()try {
    Fixture f;auto expected=f.bytes;
    put(expected,Fixture::runtime-Fixture::root+0x70,11);
    put(expected,Fixture::bank-Fixture::root+6*8,0xffffffffu);put(expected,Fixture::bank-Fixture::root+6*8+4,0xffffffffu);
    const auto converted=f.convert();
    check(converted && converted->status==MccGameplayReferenceStatus::Converted &&
        converted->before_count==14 && converted->after_count==11 && f.bytes==expected,
        "Vehicle projection changed another owner or retained MCC's cut Random choice");
    check(f.convert()->status==MccGameplayReferenceStatus::AlreadyEquivalent && f.bytes==expected,
        "Vehicle projection is not idempotent");
    unsigned negatives{};
    const auto unchanged=[&](Fixture& bad){const auto before=bad.bytes;const auto r=bad.convert();
        check(r && r->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && bad.bytes==before,
            "Unknown vehicle tuple was partly overwritten");++negatives;};
    for(unsigned i=0;i<paths.size();++i) {
        Fixture salt;salt.alias_salt=true;put(salt.bytes,Fixture::bank-Fixture::root+i*8+4,
            Bytes(salt.bytes).u32(Fixture::bank-Fixture::root+i*8+4)^0x10000u);unchanged(salt);
        Fixture group;put(group.bytes,Fixture::bank-Fixture::root+i*8,tag_groups::bitmap.value);unchanged(group);
        if(!paths[i].empty()) {
            Fixture path;path.identities[i].name="custom\\vehicle";unchanged(path);
            Fixture identity;identity.identities[i].group=tag_groups::bitmap;unchanged(identity);
        }
    }
    for(unsigned count:{0u,1u,10u,11u,12u,13u,15u,0xffffffffu}) {
        Fixture bad;put(bad.bytes,Fixture::runtime-Fixture::root+0x70,count);unchanged(bad);
    }
    Fixture custom;const auto before=custom.bytes;
    check(!convert_mcc_multiplayer_vehicle_table(tag_groups::multiplayer_globals,"custom",Fixture::root,custom.access()) &&
        !convert_mcc_multiplayer_vehicle_table(tag_groups::bitmap,"multiplayer\\multiplayer_globals",Fixture::root,custom.access()) &&
        custom.bytes==before,"Custom owner was projected as the retail vehicle table");
    for(unsigned cutoff:{15u,Fixture::runtime-Fixture::root+0x77u,Fixture::bank-Fixture::root+14*8-1u}) {
        Fixture bad;bad.bytes.resize(cutoff);const auto original=bad.bytes;bool rejected{};
        try{(void)bad.convert();}catch(const FormatError&){rejected=true;}
        check(rejected && bad.bytes==original,"Truncated vehicle owner was partially mutated");++negatives;
    }
    std::cout<<"PASS vehicle projection and idempotence; rejection controls="<<negatives<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
