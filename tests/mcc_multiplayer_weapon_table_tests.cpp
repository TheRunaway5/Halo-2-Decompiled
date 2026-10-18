#include "halo2/mcc_gameplay_reference.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value) {
    Bytes(bytes).require(at,4);
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
// Authored identities from paired Lockout mulg tables, not imported Xbox
// datum values. Every fixture uses a different local salt/index and address.
constexpr std::array<std::string_view,20> paths{
    "objects\\weapons\\pistol\\magnum\\magnum",
    "objects\\weapons\\rifle\\smg\\smg",
    "objects\\weapons\\rifle\\shotgun\\shotgun",
    "objects\\weapons\\rifle\\battle_rifle\\battle_rifle",
    "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle",
    "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher",
    "objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol",
    "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle",
    "objects\\weapons\\rifle\\brute_plasma_rifle\\brute_plasma_rifle",
    "objects\\weapons\\pistol\\needler\\needler",
    "objects\\weapons\\rifle\\covenant_carbine\\covenant_carbine", "",
    "objects\\weapons\\support_low\\brute_shot\\brute_shot",
    "objects\\weapons\\rifle\\beam_rifle\\beam_rifle",
    "objects\\weapons\\melee\\energy_blade\\energy_blade",
    "objects\\weapons\\support_high\\flak_cannon\\flak_cannon",
    "objects\\characters\\sentinel_aggressor\\weapons\\beam\\sentinel_aggressor_beam",
    "objects\\weapons\\rifle\\smg_silenced\\smg_silenced",
    "scenarios\\objects\\covenant\\military\\scarab\\weapons\\main_gun\\scarab_main_gun_handheld",
    "objects\\weapons\\rifle\\gpmg\\gpmg"
};
struct Fixture {
    static constexpr unsigned root=0x1000,runtime=0x1200,bank=0x1800;
    std::vector<std::byte> bytes=std::vector<std::byte>(0x1000);
    std::array<MccGameplayReferenceIdentity,20> identities;
    bool alias_salt=false;
    Fixture() {
        for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(i*31u+19u);
        put(bytes,8,1);put(bytes,12,runtime);
        put(bytes,runtime-root+0x68,20);put(bytes,runtime-root+0x6c,bank);
        for(unsigned i=0;i<20;++i) {
            const TagHandle handle{0xa7410010u+i};
            identities[i]={handle,tag_groups::weapon,paths[i]};
            put(bytes,bank-root+i*8,paths[i].empty()?0xffffffffu:tag_groups::weapon.value);
            put(bytes,bank-root+i*8+4,paths[i].empty()?0xffffffffu:handle.value);
        }
    }
    MccGameplayReferenceAccess access() {
        return {.bytes=[&](std::uint32_t address,std::size_t size) {
            if(address<root)throw FormatError("Fixture owner underflow");
            Bytes(bytes).require(address-root,size);return std::span(bytes).subspan(address-root,size);
        },.resolve=[&](TagHandle handle)->std::optional<MccGameplayReferenceIdentity> {
            for(const auto& id:identities)
                if(id.handle==handle || (alias_salt && id.handle.index()==handle.index()))return id;
            return {};
        },.find={}};
    }
    auto convert(){return convert_mcc_multiplayer_weapon_table(tag_groups::multiplayer_globals,
        "multiplayer\\multiplayer_globals",root,access());}
};
void synthetic() {
    Fixture f;auto expected=f.bytes;
    put(expected,Fixture::runtime-Fixture::root+0x68,18);
    put(expected,Fixture::bank-Fixture::root+15*8+4,f.identities[5].handle.value);
    put(expected,Fixture::bank-Fixture::root+17*8,0xffffffffu);
    put(expected,Fixture::bank-Fixture::root+17*8+4,0xffffffffu);
    auto result=f.convert();
    require(result && result->status==MccGameplayReferenceStatus::Converted && f.bytes==expected,
        "Vanilla conversion changed bytes outside the exact count/two authored rows");
    require(result->before_count==20 && result->after_count==18 &&
        result->count_address==Fixture::runtime+0x68 && result->entries_address==Fixture::bank,
        "Weapon-table receipt lost the serialized owners/counts");
    result=f.convert();
    require(result && result->status==MccGameplayReferenceStatus::AlreadyEquivalent && f.bytes==expected,
        "Converted Xbox-equivalent table was not idempotent");
    unsigned negatives=0;
    const auto unchanged=[&](Fixture& bad) {
        const auto before=bad.bytes;const auto r=bad.convert();
        require(r && r->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && before==bad.bytes,
            "Unknown/stale authored tuple was partly overwritten");++negatives;
    };
    for(unsigned i=0;i<20;++i) {
        Fixture salt;salt.alias_salt=true;
        put(salt.bytes,Fixture::bank-Fixture::root+i*8+4,
            Bytes(salt.bytes).u32(Fixture::bank-Fixture::root+i*8+4)^0x10000u);
        unchanged(salt);
        Fixture group;put(group.bytes,Fixture::bank-Fixture::root+i*8,tag_groups::bitmap.value);unchanged(group);
        if(!paths[i].empty()) {
            Fixture path;path.identities[i].name="custom\\modded_weapon";unchanged(path);
            Fixture identity;identity.identities[i].group=tag_groups::bitmap;unchanged(identity);
        }
    }
    for(unsigned count:{0u,1u,17u,18u,19u,21u,0xffffffffu}) {
        Fixture count_bad;put(count_bad.bytes,Fixture::runtime-Fixture::root+0x68,count);unchanged(count_bad);
    }
    Fixture runtime_count;put(runtime_count.bytes,8,2);unchanged(runtime_count);
    // A duplicate path in a different local datum is not the original
    // duplicate reference, and could carry different authored gameplay data.
    Fixture duplicate;(void)duplicate.convert();duplicate.identities[15].name=paths[5];
    put(duplicate.bytes,Fixture::bank-Fixture::root+15*8+4,duplicate.identities[15].handle.value);
    unchanged(duplicate);
    Fixture custom;const auto original=custom.bytes;
    require(!convert_mcc_multiplayer_weapon_table(tag_groups::multiplayer_globals,
        "multiplayer\\multiplayer_globals_custom",Fixture::root,custom.access()) &&
        !convert_mcc_multiplayer_weapon_table(tag_groups::bitmap,
        "multiplayer\\multiplayer_globals",Fixture::root,custom.access()) && custom.bytes==original,
        "Custom paths/groups were treated as the retail table");
    for(unsigned cutoff:{15u,Fixture::runtime-Fixture::root+0x6fu,
                         Fixture::bank-Fixture::root+20*8-1}) {
        Fixture short_bank;short_bank.bytes.resize(cutoff);const auto before=short_bank.bytes;bool rejected=false;
        try{(void)short_bank.convert();}catch(const FormatError&){rejected=true;}
        require(rejected && before==short_bank.bytes,"A short metadata owner was partly written");++negatives;
    }
    Fixture overflow;put(overflow.bytes,12,0xfffffff0u);const auto before=overflow.bytes;bool rejected=false;
    try{(void)overflow.convert();}catch(const FormatError&){rejected=true;}
    require(rejected && before==overflow.bytes,"An overflowing runtime token was accepted");++negatives;
    std::cout<<"PASS original 18 weapon choices; local salted references, literal nulls, atomic conversion, idempotence; "
        <<negatives<<" mutation guards\n";
}
struct Table {
    std::vector<std::pair<TagGroup,std::string>> identities;
    std::vector<std::uint32_t> handles;
};
Table table(const Cache& cache) {
    const auto* globals=cache.find_tag(cache.globals_tag());require(globals,"Missing matg");
    const Bytes matg(cache.read_tag_bytes(globals->address,globals->size));
    const auto* mulg=cache.find_tag(TagHandle{matg.u32(0x16c)});
    require(mulg && mulg->type()==tag_groups::multiplayer_globals,"Missing mulg");
    const Bytes header(cache.read_tag_bytes(mulg->address,16));require(header.u32(8)==1,"Runtime count");
    const auto runtime=header.u32(12);const Bytes descriptor(cache.read_tag_bytes(runtime+0x68,8));
    Table result;const auto count=descriptor.u32(0),bank=descriptor.u32(4);
    require(count==18,"Ordinary Cache did not publish the original 18-entry table");
    for(unsigned i=0;i<count;++i) {
        const Bytes row(cache.read_tag_bytes(bank+i*8,8));const auto group=TagGroup{row.u32(0)};
        const auto datum=row.u32(4);result.handles.push_back(datum);
        if(datum==0xffffffffu){require(group.value==0xffffffffu,"Nonliteral null group");result.identities.emplace_back(group,"");}
        else {
            const auto* tag=cache.find_tag(TagHandle{datum});
            require(tag && tag->type()==group && group==tag_groups::weapon,"Stale/incorrect full weapon identity");
            result.identities.emplace_back(group,tag->name);
        }
    }
    const auto choices=cache.multiplayer_weapon_choices();
    require(std::ranges::equal(choices,result.handles),"Ordinary gameplay lookup did not use the converted vanilla table");
    return result;
}
}
int main(int argc,char** argv)try {
    synthetic();
    if(argc==1)return 0;
    if(argc==3 && std::string_view(argv[1])=="--mcc-only") {
        if(!std::filesystem::is_regular_file(argv[2]))return 77;
        const halo2::Cache mcc(argv[2]);require(mcc.is_mcc(),"Expected an actual MCC donor");
        const auto actual=table(mcc);
        for(unsigned i=0;i<18;++i) {
            auto expected=paths[i];
            if(i==15)expected=paths[5];
            if(i==17)expected={};
            require(actual.identities[i].second==expected,"MCC-only vanilla weapon identity differs");
        }
        std::cout<<"PASS MCC-only ordinary Cache publishes all 18 original weapon options\n";
        return 0;
    }
    if(argc!=3)throw halo2::FormatError("Expected optional Xbox/MCC paired map paths");
    const halo2::Cache xbox(argv[1],{},halo2::OriginalData::native_tu15()),mcc(argv[2]);
    require(!xbox.is_mcc() && mcc.is_mcc(),"Expected actual Xbox reference and MCC donor");
    require(table(xbox).identities==table(mcc).identities,"Converted ordinary MCC weapon options differ from Xbox");
    std::cout<<"PASS actual ordinary Cache weapon table and gameplay lookup equal Xbox identities\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
