#include "halo2/mcc_gameplay_reference.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
struct Field {std::uint32_t address;TagGroup group;std::string_view donor,target;};
struct Case {TagGroup group;std::string_view name;std::vector<Field> fields;unsigned nested;};
const std::array cases{
    Case{tag_groups::weapon,"objects\\weapons\\rifle\\battle_rifle\\battle_rifle",{
        {0x126c,tag_groups::sound,"sound\\weapons\\battle_rifle\\zoom_in","sound\\weapons\\sniper_rifle\\zoom_in"},
        {0x1274,tag_groups::sound,"sound\\weapons\\battle_rifle\\zoom_out","sound\\weapons\\sniper_rifle\\zoom_out"}},0},
    Case{tag_groups::weapon,"objects\\vehicles\\h_turret_ap\\weapon\\h_turret_ap",{
        {0x1804,tag_groups::effect,"effects\\objects\\weapons\\fixed\\h_turret_mp\\h_turret_gun_fire",
            "effects\\objects\\vehicles\\warthog\\chaingun\\warthog_gun_fire"}},2},
    Case{tag_groups::weapon,"objects\\characters\\prophet\\weapons\\gravity_cannon\\gravity_cannon",{
        {0x1600,tag_groups::sound_looping,"sound\\weapons\\hunter_cannon\\hunter_cannon_loop\\hunter_cannon_gravitythrone_loop",
            "sound\\weapons\\hunter_cannon\\hunter_cannon_loop\\hunter_cannon_loop"}},1},
    Case{tag_groups::projectile,"objects\\characters\\sentinel_enforcer\\weapons\\rocket\\sentinel_enforcer_rocket",{
        {0x1054,tag_groups::material_effects,"effects\\materials\\objects\\weapons\\enforcer_rocket",
            "effects\\materials\\objects\\weapons\\rocket_launcher"}},0}
};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
void put(std::span<std::byte> bytes,unsigned offset,std::uint32_t value) {
    Bytes(bytes).require(offset,4);
    for(unsigned n=0;n<4;++n)bytes[offset+n]=std::byte(value>>(n*8));
}
struct Fixture {
    std::vector<std::byte> bytes=std::vector<std::byte>(0x1000);
    std::vector<MccGameplayReferenceIdentity> identities;
    std::optional<TagHandle> missing;
    bool wrong_target_salt=false;
    bool wrong_target_group=false;
    bool alias_source_index=false;
    explicit Fixture(const Case& c) {
        for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte(n*17u+0xa7u);
        if(c.nested) {put(bytes,c.nested==1?0x94:0x2d0,1);put(bytes,c.nested==1?0x98:0x2d4,0x1600);}
        if(c.nested==2){put(bytes,0x6e4,1);put(bytes,0x6e8,0x1800);}
        for(unsigned n=0;n<c.fields.size();++n) {
            const auto& f=c.fields[n];const TagHandle donor{0x81230001u+n},target{0xb4560005u+n};
            identities.push_back({donor,f.group,f.donor});identities.push_back({target,f.group,f.target});
            put(bytes,f.address-0x1000,f.group.value);put(bytes,f.address-0x1000+4,donor.value);
        }
    }
    MccGameplayReferenceAccess access() {
        return {
            .bytes=[&](std::uint32_t address,std::size_t size) {
                if(address<0x1000)throw FormatError("Fixture owner underflow");
                Bytes(bytes).require(address-0x1000,size);return std::span(bytes).subspan(address-0x1000,size);
            },
            .resolve=[&](TagHandle handle)->std::optional<MccGameplayReferenceIdentity> {
                const auto found=std::ranges::find(identities,handle,&MccGameplayReferenceIdentity::handle);
                if(found!=identities.end())return *found;
                if(alias_source_index)for(const auto& id:identities)if(id.handle.index()==handle.index())return id;
                return {};
            },
            .find=[&](TagGroup group,std::string_view name)->std::optional<MccGameplayReferenceIdentity> {
                const auto found=std::ranges::find_if(identities,[&](const auto& id){return id.group==group && id.name==name;});
                if(found==identities.end() || found->handle==missing)return {};
                auto result=*found;
                if(wrong_target_salt)result.handle.value^=0x10000u;
                if(wrong_target_group)result.group=tag_groups::bitmap;
                return result;
            }
        };
    }
};
void synthetic() {
    for(const auto& c:cases) {
        Fixture f(c);auto expected=f.bytes;
        for(unsigned n=0;n<c.fields.size();++n)put(expected,c.fields[n].address-0x1000+4,0xb4560005u+n);
        auto changed=convert_mcc_gameplay_references(c.group,c.name,0x1000,f.access());
        require(changed && changed->status==MccGameplayReferenceStatus::Converted && f.bytes==expected,
            "Known dependency tuple did not change only its exact local full-salt reference bytes");
        require(changed->field_count==c.fields.size(),"Reference receipt omitted a tuple field");
        for(unsigned n=0;n<c.fields.size();++n) {
            const auto& receipt=changed->fields[n];const auto& field=c.fields[n];
            require(receipt.address==field.address && receipt.before.group==field.group &&
                receipt.before.handle.value==0x81230001u+n && receipt.after.group==field.group &&
                receipt.after.handle.value==0xb4560005u+n && receipt.required_group==field.group &&
                receipt.required_name==field.target,"Reference receipt lost group, full salt, path or serialized address");
        }
        auto repeat=convert_mcc_gameplay_references(c.group,c.name,0x1000,f.access());
        require(repeat && repeat->status==MccGameplayReferenceStatus::AlreadyEquivalent && f.bytes==expected,
            "Converted reference tuple was not idempotent");
        Fixture missing(c);missing.missing=missing.identities.back().handle;const auto original=missing.bytes;
        auto required=convert_mcc_gameplay_references(c.group,c.name,0x1000,missing.access());
        require(required && required->status==MccGameplayReferenceStatus::RequiredDependencyUnavailable &&
            missing.bytes==original && required->fields[c.fields.size()-1].required_name==c.fields.back().target,
            "Missing local dependency did not report its explicit path atomically");
        Fixture bad_salt(c);bad_salt.wrong_target_salt=true;const auto before=bad_salt.bytes;bool rejected=false;
        try{(void)convert_mcc_gameplay_references(c.group,c.name,0x1000,bad_salt.access());}catch(const FormatError&){rejected=true;}
        require(rejected && before==bad_salt.bytes,"Resolver published a stale-salt target or partly wrote a tuple");
        Fixture bad_group(c);bad_group.wrong_target_group=true;const auto before_target_group=bad_group.bytes;rejected=false;
        try{(void)convert_mcc_gameplay_references(c.group,c.name,0x1000,bad_group.access());}catch(const FormatError&){rejected=true;}
        require(rejected && before_target_group==bad_group.bytes,"Resolver published a target of another group");
        for(const auto& field:c.fields) {
            Fixture authored(c);put(authored.bytes,field.address-0x1000+4,0x77770001u);
            const auto before_authored=authored.bytes;
            auto result=convert_mcc_gameplay_references(c.group,c.name,0x1000,authored.access());
            require(result && result->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized &&
                authored.bytes==before_authored,"An authored/stale-salt reference tuple was overwritten");
            Fixture group(c);put(group.bytes,field.address-0x1000,tag_groups::bitmap.value);
            const auto before_group=group.bytes;auto wrong=convert_mcc_gameplay_references(c.group,c.name,0x1000,group.access());
            require(wrong && wrong->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized &&
                group.bytes==before_group,"Reference conversion ignored the serialized group word");
        }
        Fixture custom(c);const auto before_custom=custom.bytes;
        require(!convert_mcc_gameplay_references(c.group,std::string(c.name)+"_custom",0x1000,custom.access()) &&
            !convert_mcc_gameplay_references(tag_groups::bitmap,c.name,0x1000,custom.access()) &&
            custom.bytes==before_custom,"Reference conversion reinterpreted a custom path or other group");
        Fixture short_owner(c);short_owner.bytes.resize(c.fields.back().address-0x1000+7);
        const auto before_short=short_owner.bytes;rejected=false;
        try{(void)convert_mcc_gameplay_references(c.group,c.name,0x1000,short_owner.access());}catch(const FormatError&){rejected=true;}
        require(rejected && short_owner.bytes==before_short,"A short metadata owner was partially written");
        if(c.nested) {
            Fixture authored(c);put(authored.bytes,c.nested==1?0x94:0x2d0,2);
            const auto before_count=authored.bytes;auto result=convert_mcc_gameplay_references(c.group,c.name,0x1000,authored.access());
            require(result && result->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized &&
                authored.bytes==before_count,"An authored nested block shape was overwritten");
            Fixture invalid(c);put(invalid.bytes,c.nested==1?0x98:0x2d4,0);
            const auto before_invalid=invalid.bytes;rejected=false;
            try{(void)convert_mcc_gameplay_references(c.group,c.name,0x1000,invalid.access());}catch(const FormatError&){rejected=true;}
            require(rejected && invalid.bytes==before_invalid,"A missing nested metadata owner was partially written");
        }
    }
    Fixture partial(cases[0]);put(partial.bytes,0x270,0xb4560005u);const auto before=partial.bytes;
    auto mixed=convert_mcc_gameplay_references(cases[0].group,cases[0].name,0x1000,partial.access());
    require(mixed && mixed->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && partial.bytes==before,
        "A mixed authored zoom tuple was completed without evidence");
    Fixture alias(cases[0]);alias.alias_source_index=true;put(alias.bytes,0x270,0x77770001u);
    const auto before_alias=alias.bytes;auto stale=convert_mcc_gameplay_references(cases[0].group,cases[0].name,0x1000,alias.access());
    require(stale && stale->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && alias.bytes==before_alias,
        "A resolver's matching low index was accepted without the original full salt");
    require(!convert_mcc_gameplay_references(tag_groups::projectile,
        "objects\\vehicles\\warthog\\turrets\\gauss\\weapon\\gauss_bullet",0x1000,{}),
        "Map-specific Gauss authoring was guessed from a tag name");
    std::cout<<"PASS 4 exact dependency profiles, full salt/group guards, nested owners, missing-local requirements and idempotence\n";
}
std::vector<std::uint32_t> locations(const Cache& cache,const Tag& tag,unsigned nested) {
    if(!nested) {
        if(tag.type()==tag_groups::projectile)return {tag.address+0x54};
        return {tag.address+0x26c,tag.address+0x274};
    }
    Bytes block(cache.read_tag_bytes(tag.address+(nested==1?0x94:0x2d0),8));
    require(block.u32(0)==1,"Unexpected retail outer block count");auto address=block.u32(4);
    if(nested==2){Bytes effect(cache.read_tag_bytes(address+0xe4,8));require(effect.u32(0)==1,"Unexpected retail firing effect count");address=effect.u32(4)+4;}
    return {address};
}
void actual(const std::filesystem::path& reference,const std::filesystem::path& donor) {
    const Cache xbox(reference,{},OriginalData::native_tu15()),mcc(donor);
    require(!xbox.is_mcc() && mcc.is_mcc(),"Reference integration requires actual Xbox and MCC banks");
    unsigned matched{},required{},map_gauss{};
    for(const auto& c:cases) {
        const auto* source=xbox.query_tag({.group=c.group,.name=c.name});
        const auto* converted=mcc.query_tag({.group=c.group,.name=c.name});
        if(!source || !converted)continue;
        const auto a=locations(xbox,*source,c.nested),b=locations(mcc,*converted,c.nested);
        bool missing=false;
        for(const auto& field:c.fields)if(!mcc.query_tag({.group=field.group,.name=field.target,.require_address=true,.minimum_size=1}))missing=true;
        for(unsigned n=0;n<a.size();++n) {
            const Bytes original(xbox.read_tag_bytes(a[n],8)),current(mcc.read_tag_bytes(b[n],8));
            const auto* expected=xbox.query_tag({.handle=TagHandle{original.u32(4)}});
            const auto* actual=mcc.query_tag({.handle=TagHandle{current.u32(4)}});
            require(expected && actual && current.u32(0)==original.u32(0),"Dependency has no full-salt identity or group changed");
            require(expected->name==c.fields[n].target && expected->type()==c.fields[n].group,
                "Reference map does not match the reviewed dependency target");
            require(actual->name==(missing?c.fields[n].donor:c.fields[n].target) && actual->type()==c.fields[n].group,
                "Cache did not publish the available vanilla dependency, or changed an unresolved tuple");
            missing?++required:++matched;
        }
    }
    // The Gauss null/non-null difference belongs to authenticated complete
    // map authoring, rather than the name-only reference profiles above.
    // Check the actual Cache publication against this exact original map.
    constexpr auto gauss="objects\\vehicles\\warthog\\turrets\\gauss\\weapon\\gauss_bullet";
    const auto *source=xbox.query_tag({.group=tag_groups::projectile,.name=gauss,.require_address=true});
    const auto *converted=mcc.query_tag({.group=tag_groups::projectile,.name=gauss,.require_address=true});
    if(source && converted) {
        const Bytes expected(xbox.read_tag_bytes(source->address+0x138,8));
        const Bytes actual(mcc.read_tag_bytes(converted->address+0x138,8));
        require(expected.u32(0)==actual.u32(0),"Map-specific Gauss flyby changed its original reference group");
        if(expected.u32(4)==0xffffffffu)
            require(actual.u32(4)==0xffffffffu,"Authenticated map retained a Gauss flyby absent in TU1.5");
        else {
            const auto *a=xbox.query_tag({.handle=TagHandle{expected.u32(4)}});
            const auto *b=mcc.query_tag({.handle=TagHandle{actual.u32(4)}});
            require(a && b && a->type()==b->type() && a->name==b->name,
                "Authenticated map lost its original non-null Gauss flyby identity");
        }
        ++map_gauss;
    }
    require(matched+required+map_gauss!=0,"No reviewed reference profile exists in this map pair");
    std::cout<<"PASS "<<mcc.name()<<": "<<matched<<" exact vanilla dependency identities / "<<required<<" missing-local requirements\n";
    std::cout<<"PASS "<<map_gauss<<" original map-specific Gauss flyby references\n";
}
}
int main(int argc,char** argv)try {
    synthetic();
    if(argc==3)actual(argv[1],argv[2]);
    else if(argc!=1)throw halo2::FormatError("usage: mcc_gameplay_reference_tests [XBOX.map MCC.map]");
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
