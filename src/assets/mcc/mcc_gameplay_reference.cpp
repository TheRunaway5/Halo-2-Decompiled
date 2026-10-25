#include "halo2/mcc_gameplay_reference.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
namespace {
enum class Location { Root, FirstAttachment, FirstFiringEffect };
struct Field {std::uint32_t offset;TagGroup group;std::string_view donor,target;};
struct Profile {
    TagGroup group;std::string_view name,label;Location location;
    std::array<Field,2> fields;std::uint8_t count;
};
// The paired owned Xbox maps and MCC donor maps are hash-pinned in
// tools/mcc-gameplay-reference-profiles.json. OriginalData::native_tu15()
// selects the authenticated TU1.5 postprocessors for the reference audit.
// These are authored dependency identities, not a claim that the referenced
// MCC sound/effect/material payload has already been proved equivalent.
constexpr std::array profiles{
    Profile{tag_groups::weapon,"objects\\weapons\\rifle\\battle_rifle\\battle_rifle",
        "tu15-battle-rifle-zoom-dependencies",Location::Root,{{
            {0x26c,tag_groups::sound,"sound\\weapons\\battle_rifle\\zoom_in","sound\\weapons\\sniper_rifle\\zoom_in"},
            {0x274,tag_groups::sound,"sound\\weapons\\battle_rifle\\zoom_out","sound\\weapons\\sniper_rifle\\zoom_out"}}},2},
    Profile{tag_groups::weapon,"objects\\vehicles\\h_turret_ap\\weapon\\h_turret_ap",
        "tu15-human-turret-firing-dependency",Location::FirstFiringEffect,{{
            {4,tag_groups::effect,"effects\\objects\\weapons\\fixed\\h_turret_mp\\h_turret_gun_fire",
                "effects\\objects\\vehicles\\warthog\\chaingun\\warthog_gun_fire"},{}}},1},
    Profile{tag_groups::weapon,"objects\\characters\\prophet\\weapons\\gravity_cannon\\gravity_cannon",
        "tu15-prophet-looping-dependency",Location::FirstAttachment,{{
            {0,tag_groups::sound_looping,"sound\\weapons\\hunter_cannon\\hunter_cannon_loop\\hunter_cannon_gravitythrone_loop",
                "sound\\weapons\\hunter_cannon\\hunter_cannon_loop\\hunter_cannon_loop"},{}}},1},
    Profile{tag_groups::projectile,"objects\\characters\\sentinel_enforcer\\weapons\\rocket\\sentinel_enforcer_rocket",
        "tu15-enforcer-material-dependency",Location::Root,{{
            {0x54,tag_groups::material_effects,"effects\\materials\\objects\\weapons\\enforcer_rocket",
                "effects\\materials\\objects\\weapons\\rocket_launcher"},{}}},1}
};
// Gauss flyby references deliberately remain outside these profiles: 22
// original maps use a literal FFFFFFFF/FFFFFFFF null, but six owned DLC maps
// retain the donor's scorpion sound. A name alone cannot authenticate which
// authoring variant a mod or resource bank should receive.
std::uint32_t add(std::uint32_t base,std::uint32_t offset) {
    if(offset>std::numeric_limits<std::uint32_t>::max()-base)
        throw FormatError("MCC gameplay reference address overflows its source token");
    return base+offset;
}
void word(std::span<std::byte> bytes,unsigned at,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));
}
}
std::optional<MccGameplayReferenceReceipt> convert_mcc_gameplay_references(
    TagGroup group,std::string_view name,std::uint32_t root,const MccGameplayReferenceAccess& access) {
    const auto p=std::ranges::find_if(profiles,[&](const auto& row){return row.group==group && row.name==name;});
    if(p==profiles.end())return {};
    MccGameplayReferenceReceipt receipt;receipt.profile=p->label;
    const auto& fields=p->fields;
    if(!access.bytes || !access.resolve || !access.find)
        throw FormatError("MCC gameplay reference conversion requires checked bank access");
    const auto view=[&](std::uint32_t address,std::size_t size) {
        auto bytes=access.bytes(address,size);
        if(bytes.size()!=size)throw FormatError("MCC gameplay reference reader returned a short view");
        return bytes;
    };
    auto owner=root;
    const auto first=[&](std::uint32_t descriptor)->std::optional<std::uint32_t> {
        const Bytes block(view(descriptor,8));
        // Every affected retail reference has exactly one element at each
        // selected nested level. An authored alternative layout is preserved.
        if(block.u32(0)!=1)return {};
        const auto address=block.u32(4);
        if(!address || address==0xffffffffu)throw FormatError("MCC gameplay reference block has no metadata owner");
        return address;
    };
    if(p->location!=Location::Root) {
        auto nested=first(add(owner,p->location==Location::FirstAttachment?0x94:0x2d0));
        if(nested && p->location==Location::FirstFiringEffect)nested=first(add(*nested,0xe4));
        if(!nested){receipt.status=MccGameplayReferenceStatus::AuthoredTupleUnrecognized;return receipt;}
        owner=*nested;
    }
    bool donor=true,target=true;
    std::array<std::span<std::byte>,2> views;
    receipt.field_count=p->count;
    const auto matches=[&](TagReference ref,TagGroup expected,std::string_view path) {
        if(ref.group!=expected || ref.handle.is_null())return false;
        const auto identity=access.resolve(ref.handle);
        return identity && identity->handle==ref.handle && identity->group==expected && identity->name==path;
    };
    for(unsigned n=0;n<p->count;++n) {
        const auto& field=fields[n];auto& result=receipt.fields[n];
        result.address=add(owner,field.offset);views[n]=view(result.address,8);const Bytes bytes(views[n]);
        result.before={TagGroup{bytes.u32(0)},TagHandle{bytes.u32(4)}};result.after=result.before;
        result.required_group=field.group;result.required_name=field.target;
        donor&=matches(result.before,field.group,field.donor);target&=matches(result.before,field.group,field.target);
    }
    if(target){receipt.status=MccGameplayReferenceStatus::AlreadyEquivalent;return receipt;}
    if(!donor){receipt.status=MccGameplayReferenceStatus::AuthoredTupleUnrecognized;return receipt;}
    std::array<TagReference,2> replacements;
    for(unsigned n=0;n<p->count;++n) {
        const auto& field=fields[n];
        const auto found=access.find(field.group,field.target);
        if(!found){receipt.status=MccGameplayReferenceStatus::RequiredDependencyUnavailable;return receipt;}
        if(found->handle.is_null() || !matches({found->group,found->handle},field.group,field.target))
            throw FormatError("MCC vanilla dependency resolver returned an invalid full tag identity");
        replacements[n]={field.group,found->handle};
    }
    // All selected descriptors, tuple guards and full target identities have
    // been checked before the first write. Missing dependencies never borrow
    // the low index of a tag in a different cache directory.
    for(unsigned n=0;n<p->count;++n) {
        word(views[n],0,replacements[n].group.value);word(views[n],4,replacements[n].handle.value);
        receipt.fields[n].after=replacements[n];
    }
    receipt.status=MccGameplayReferenceStatus::Converted;return receipt;
}

std::optional<MccMultiplayerWeaponTableReceipt> convert_mcc_multiplayer_weapon_table(
    TagGroup group,std::string_view name,std::uint32_t root,const MccGameplayReferenceAccess& access) {
    if(group!=tag_groups::multiplayer_globals || name!="multiplayer\\multiplayer_globals")return {};
    if(!access.bytes || !access.resolve)
        throw FormatError("MCC multiplayer weapon conversion requires checked bank access");
    // Paired authored mulg data: Xbox slot 15 repeats rocket_launcher and
    // slot 17 is null. MCC substitutes flak_cannon/smg_silenced and appends
    // scarab/gpmg. Reusing the authenticated slot-5 handle avoids importing
    // an Xbox datum or resolving a similarly named tag from another bank.
    constexpr std::array<std::string_view,20> donor{
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
        "objects\\weapons\\rifle\\covenant_carbine\\covenant_carbine",
        "",
        "objects\\weapons\\support_low\\brute_shot\\brute_shot",
        "objects\\weapons\\rifle\\beam_rifle\\beam_rifle",
        "objects\\weapons\\melee\\energy_blade\\energy_blade",
        "objects\\weapons\\support_high\\flak_cannon\\flak_cannon",
        "objects\\characters\\sentinel_aggressor\\weapons\\beam\\sentinel_aggressor_beam",
        "objects\\weapons\\rifle\\smg_silenced\\smg_silenced",
        "scenarios\\objects\\covenant\\military\\scarab\\weapons\\main_gun\\scarab_main_gun_handheld",
        "objects\\weapons\\rifle\\gpmg\\gpmg"
    };
    const auto view=[&](std::uint32_t address,std::size_t size) {
        if(!address || address==0xffffffffu || size>0x100000000ull-address)
            throw FormatError("MCC multiplayer weapon metadata has an invalid owner");
        auto bytes=access.bytes(address,size);
        if(bytes.size()!=size)throw FormatError("MCC multiplayer weapon reader returned a short view");
        return bytes;
    };
    MccMultiplayerWeaponTableReceipt result;
    result.status=MccGameplayReferenceStatus::AuthoredTupleUnrecognized;
    const Bytes header(view(add(root,8),8));
    if(header.u32(0)!=1)return result;
    const auto runtime=header.u32(4);
    auto descriptor=view(add(runtime,0x68),8);
    const Bytes block(descriptor);
    result.count_address=add(runtime,0x68);result.entries_address=block.u32(4);
    result.before_count=result.after_count=block.u32(0);
    if(result.before_count!=18 && result.before_count!=20)return result;
    auto entries=view(result.entries_address,result.before_count*8);
    // A malformed block cannot alias the count descriptor that is committed
    // after the row replacements. Validate every owner before the first write.
    const auto bank_end=std::uint64_t(result.entries_address)+entries.size();
    if(result.count_address<bank_end &&
        std::uint64_t(result.count_address)+8>result.entries_address)
        throw FormatError("MCC multiplayer weapon metadata owners overlap");
    const Bytes rows(entries);
    for(unsigned i=0;i<result.before_count;++i) {
        auto expected=donor[i];
        if(result.before_count==18 && i==15)expected=donor[5];
        if(result.before_count==18 && i==17)expected={};
        const TagReference ref{TagGroup{rows.u32(i*8)},TagHandle{rows.u32(i*8+4)}};
        if(expected.empty()) {
            if(ref.group.value!=0xffffffffu || ref.handle.value!=0xffffffffu)return result;
        } else {
            if(ref.group!=tag_groups::weapon || ref.handle.is_null())return result;
            const auto resolved=access.resolve(ref.handle);
            if(!resolved || resolved->handle!=ref.handle || resolved->group!=tag_groups::weapon ||
                resolved->name!=expected)return result;
        }
    }
    if(result.before_count==18) {
        if(rows.u32(15*8+4)!=rows.u32(5*8+4))return result;
        result.status=MccGameplayReferenceStatus::AlreadyEquivalent;return result;
    }
    const auto rocket_group=rows.u32(5*8),rocket_datum=rows.u32(5*8+4);
    word(entries,15*8,rocket_group);word(entries,15*8+4,rocket_datum);
    word(entries,17*8,0xffffffffu);word(entries,17*8+4,0xffffffffu);
    word(descriptor,0,18);
    result.after_count=18;result.status=MccGameplayReferenceStatus::Converted;
    return result;
}

std::optional<MccMultiplayerVehicleTableReceipt> convert_mcc_multiplayer_vehicle_table(
    TagGroup group,std::string_view name,std::uint32_t root,const MccGameplayReferenceAccess& access) {
    if(group!=tag_groups::multiplayer_globals || name!="multiplayer\\multiplayer_globals")return {};
    if(!access.bytes || !access.resolve)throw FormatError("MCC vehicle conversion requires checked bank access");
    // Authenticated Coagulation/Headlong/Zanzibar original and MCC census:
    // analysis/vehicles-tu15-goal-20261023/vehicle-runtime-table-census-v1.json.
    //15CB60's five-way Random retains choice4 and rerolls its original NONE.
    // Selecting from a filtered pool would change both weighting and RNG.
    constexpr std::array<std::string_view,14> donor{
        "objects\\vehicles\\warthog\\warthog","objects\\vehicles\\ghost\\ghost",
        "objects\\vehicles\\scorpion\\scorpion","objects\\vehicles\\wraith\\wraith",
        "objects\\vehicles\\banshee\\banshee","objects\\vehicles\\spectre\\spectre",
        "objects\\vehicles\\mongoose\\mongoose","objects\\vehicles\\h_turret_ap\\h_turret_ap",
        "","","objects\\vehicles\\c_turret_ap\\c_turret_ap",
        "objects\\vehicles\\creep\\creep_mp","objects\\vehicles\\falcon\\falcon",
        "objects\\vehicles\\gravity_throne\\gravity_throne_mp"};
    const auto view=[&](std::uint32_t address,std::size_t size) {
        if(!address || address==0xffffffffu || size>0x100000000ull-address)
            throw FormatError("MCC vehicle table has an invalid metadata owner");
        auto result=access.bytes(address,size);
        if(result.size()!=size)throw FormatError("MCC vehicle table reader returned a short view");
        return result;
    };
    MccMultiplayerVehicleTableReceipt result;
    result.status=MccGameplayReferenceStatus::AuthoredTupleUnrecognized;
    const Bytes header(view(add(root,8),8));if(header.u32(0)!=1)return result;
    result.count_address=add(header.u32(4),0x70);
    auto descriptor=view(result.count_address,8);const Bytes block(descriptor);
    result.entries_address=block.u32(4);result.before_count=result.after_count=block.u32(0);
    if(result.before_count!=11 && result.before_count!=14)return result;
    auto entries=view(result.entries_address,result.before_count*8u);const Bytes rows(entries);
    if(std::uint64_t(result.count_address)<std::uint64_t(result.entries_address)+entries.size() &&
        std::uint64_t(result.count_address)+8u>result.entries_address)
        throw FormatError("MCC vehicle table metadata owners overlap");
    for(unsigned i=0;i<result.before_count;++i) {
        const auto expected=result.before_count==11 && i==6?std::string_view{}:donor[i];
        const TagReference ref{TagGroup{rows.u32(i*8)},TagHandle{rows.u32(i*8+4)}};
        if(expected.empty()) {
            if(ref.group.value!=0xffffffffu || !ref.handle.is_null())return result;
        } else {
            if(ref.group!=tag_groups::vehicle || ref.handle.is_null())return result;
            const auto resolved=access.resolve(ref.handle);
            if(!resolved || resolved->handle!=ref.handle || resolved->group!=ref.group || resolved->name!=expected)return result;
        }
    }
    if(result.before_count==11){result.status=MccGameplayReferenceStatus::AlreadyEquivalent;return result;}
    word(entries,6*8,0xffffffffu);word(entries,6*8+4,0xffffffffu);word(descriptor,0,11);
    result.after_count=11;result.status=MccGameplayReferenceStatus::Converted;return result;
}

std::optional<MccBspInstanceTransformReceipt> convert_mcc_bsp_instance_transform(
    TagGroup group,std::string_view name,std::uint32_t root,
    const std::function<std::span<std::byte>(std::uint32_t,std::size_t)>& bytes) {
    if(group!=tag_groups::scenario_structure_bsp ||
        name!="scenarios\\solo\\03a_oldmombasa\\earthcity_3")return {};
    if(!bytes)throw FormatError("MCC instance transform conversion requires checked bank access");
    // Paired authored instance zero. Original16CF00 reads this same 0x58-byte
    // record for its sphere test and inverse transform144090. Restore the
    // exact float words before either collision or render views are published.
    constexpr std::array<std::uint32_t,13> before{
        0x3f800000u,0xb56fb6a1u,0x3f800000u,0,0xbf800000u,0xb56fb6a1u,0,0,0,
        0x3f800000u,0xc41ef873u,0x436f6625u,0xc16ba993u};
    constexpr std::array<std::uint32_t,13> after{
        0x3f800000u,0xb56fb6a7u,0x3f7fffffu,0,0xbf7fffffu,0xb56fb6a7u,0,0,0,
        0x3f800000u,0xc41ef86bu,0x436f6616u,0xc16ba9a4u};
    const auto view=[&](std::uint32_t address,std::size_t size) {
        if(!address || address==0xffffffffu || size>0x100000000ull-address)
            throw FormatError("MCC instance transform has an invalid metadata owner");
        auto result=bytes(address,size);
        if(result.size()!=size)throw FormatError("MCC instance transform reader returned a short view");
        return result;
    };
    MccBspInstanceTransformReceipt result{MccGameplayReferenceStatus::AuthoredTupleUnrecognized};
    const Bytes blocks(view(add(root,0x138),16));
    if(blocks.u32(0)!=43 || blocks.u32(8)!=243)return result;
    const auto definition=blocks.u32(4),instance=blocks.u32(12);
    if(Bytes(view(add(definition,0x5c),4)).u32(0)!=0x36138b96u)return result;
    auto record=view(instance,88);const Bytes data(record);result.address=instance;
    if(data.u32(52)!=0 || data.u32(56)!=0 || data.u32(76)!=0xf38a7e81u || data.u32(84)!=0)
        return result;
    bool donor=true,equivalent=true;
    for(unsigned i=0;i<before.size();++i) {
        donor&=data.u32(i*4)==before[i];equivalent&=data.u32(i*4)==after[i];
    }
    if(equivalent){result.status=MccGameplayReferenceStatus::AlreadyEquivalent;return result;}
    if(!donor)return result;
    for(unsigned i=0;i<after.size();++i)word(record,i*4,after[i]);
    result.status=MccGameplayReferenceStatus::Converted;return result;
}
} // namespace halo2
