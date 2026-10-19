#include "halo2/mcc_map_gameplay_vanilla.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <zlib.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
using Status=MccMapGameplayStatus;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::uint32_t u32(std::span<const std::byte> bytes,unsigned at) {
    require(at<=bytes.size() && bytes.size()-at>=4,"Test word is out of range");
    std::uint32_t result{};for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(bytes[at+n])<<(n*8);return result;
}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value) {
    require(at<=bytes.size() && bytes.size()-at>=4,"Test store is out of range");
    for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));
}
void put16(std::span<std::byte> bytes,unsigned at,std::uint16_t value) {
    require(at<=bytes.size() && bytes.size()-at>=2,"Test halfword is out of range");
    bytes[at]=std::byte(value);bytes[at+1]=std::byte(value>>8);
}
std::vector<std::byte> unhex(std::string_view text) {
    require(text.size()%2==0,"Invalid fixture hex");
    const auto digit=[](char c)->unsigned {
        if(c>='0'&&c<='9')return unsigned(c-'0');
        if(c>='a'&&c<='f')return unsigned(c-'a'+10);
        throw std::runtime_error("Invalid fixture hex digit");
    };
    std::vector<std::byte> bytes(text.size()/2);
    for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte(digit(text[n*2])*16+digit(text[n*2+1]));
    return bytes;
}
TagGroup group(std::string_view text) {
    require(text.size()==4,"Invalid fixture group");std::uint32_t code{};
    for(auto c:text)code=(code<<8)|static_cast<unsigned char>(c);
    return TagGroup{code};
}
MccMapGameplayIdentity identity(const Json& value) {
    return {{value.at("datum").get<std::uint32_t>()},group(value.at("group").get_ref<const std::string&>()),
        value.at("name").get_ref<const std::string&>(),value.at("address").get<std::uint32_t>(),value.at("size").get<std::uint32_t>()};
}
struct Fixture {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::map<std::uint32_t,MccMapGameplayIdentity> identities;
    std::map<std::string,std::uint32_t> roots;
    TagHandle scenario,globals;
    std::string scenario_path;
    std::set<std::uint32_t> missing;
    bool wrong_resolve=false,wrong_find=false,short_read=false,ambiguous=false;
    explicit Fixture(const Json& profile) {
        for(const auto& bank:profile.at("banks")) {
            const auto id=identity(bank.at("identity"));identities.emplace(id.handle.value,id);
            roots.emplace(bank.at("kind").get<std::string>(),id.source_token);
            const auto packed=unhex(bank.at("compressed_hex").get_ref<const std::string&>());
            std::vector<std::byte> raw(id.bytes);uLongf size=raw.size();
            require(uncompress(reinterpret_cast<Bytef*>(raw.data()),&size,
                reinterpret_cast<const Bytef*>(packed.data()),packed.size())==Z_OK,"Cannot decode retained fixture bank");
            raw.resize(size);banks.emplace(id.source_token,std::move(raw));
            if(bank.at("kind")=="scenario"){scenario=id.handle;scenario_path=id.name;}
            if(bank.at("kind")=="globals")globals=id.handle;
        }
        for(const auto& value:profile.at("extra_identities")) {
            const auto id=identity(value);identities.emplace(id.handle.value,id);
        }
    }
    std::span<std::byte> bytes(std::uint32_t token,std::size_t count) {
        auto bank=banks.upper_bound(token);
        require(bank!=banks.begin(),"Test source address precedes owned storage");--bank;
        const auto offset=std::uint64_t(token)-bank->first;
        require(offset<=bank->second.size() && count<=bank->second.size()-offset,"Test source address escapes its owned allocation");
        return std::span(bank->second).subspan(std::size_t(offset),count);
    }
    MccMapGameplaySource source() {
        return {scenario_path,scenario,globals,
            [&](TagHandle handle)->std::optional<MccMapGameplayIdentity> {
                if(missing.contains(handle.value))return {};
                const auto found=identities.find(handle.value);if(found==identities.end())return {};
                auto id=found->second;if(wrong_resolve)id.handle.value^=0x10000;return id;
            },
            [&](TagGroup requested,std::string_view path)->std::optional<MccMapGameplayIdentity> {
                if(ambiguous)return {};
                std::optional<MccMapGameplayIdentity> out;
                for(const auto& [handle,id]:identities)if(!missing.contains(handle) && id.group==requested && id.name==path) {
                    if(out)return {};
                    out=id;
                }
                if(out && wrong_find)out->group=tag_groups::bitmap;
                return out;
            },
            [&](std::uint32_t token,std::size_t count)->std::span<const std::byte> {
                auto result=bytes(token,count);if(short_read && !result.empty())result=result.first(result.size()-1);return result;
            }};
    }
};
void commit(Fixture& fixture,const MccMapGameplayPlan& plan) {
    std::vector<std::span<std::byte>> targets;
    for(const auto& write:plan.writes)targets.push_back(fixture.bytes(write.source_token,write.after.size()));
    preflight_mcc_map_gameplay_vanilla(plan,fixture.source().read);
    for(unsigned n=0;n<targets.size();++n)std::ranges::copy(plan.writes[n].after,targets[n].begin());
}
void commit(Fixture& fixture,std::span<const MccMapGameplayPlan> plans) {
    std::vector<std::pair<std::span<std::byte>,const MccMapGameplayWrite*>> targets;
    for(const auto& plan:plans)if(plan.status==Status::Ready)
        for(const auto& write:plan.writes)
            targets.emplace_back(fixture.bytes(write.source_token,write.after.size()),&write);
    preflight_mcc_map_gameplay_vanilla(plans,fixture.source().read);
    for(const auto& [target,write]:targets)std::ranges::copy(write->after,target.begin());
}
void qualified(const Json& profile) {
    Fixture fixture(profile);const auto before=fixture.banks;
    const auto plan=plan_mcc_map_gameplay_vanilla(fixture.source());
    require(fixture.banks==before,"Pure planner mutated owned source data");
    auto expected=before;
    unsigned changes=0;
    if(profile.at("original_reference").at("flyby").is_null()) {
        auto& bytes=expected.at(fixture.roots.at("gauss"));put(bytes,312,0xffffffff);put(bytes,316,0xffffffff);
        ++changes;
    }
    const auto& stop=profile.at("original_reference").at("flag_reset_stop_bits");
    if(!stop.is_null()) {
        const auto offset=profile.at("mulg_field_offset").get<unsigned>();
        const auto bits=std::uint32_t(std::stoul(stop.get<std::string>(),nullptr,16));
        if(u32(expected.at(fixture.roots.at("mulg")),offset)!=bits) {
            put(expected.at(fixture.roots.at("mulg")),offset,bits);++changes;
        }
    }
    require(plan.status==(changes?Status::Ready:Status::AlreadyEquivalent) && plan.writes.size()==changes,
        "Known map did not produce its independently authenticated original field targets");
    require(plan.guards.size()==(fixture.roots.contains("mulg")?4:2),"Map captured inappropriate required owners");
    for(const auto& write:plan.writes)require(write.after.size()==(write.field=="Gauss projectile/Flyby Sound"?8:4),
        "Planned field lost its serialized width");
    commit(fixture,plan);require(fixture.banks==expected,"Map publication changed unrelated bytes");
    const auto repeat=plan_mcc_map_gameplay_vanilla(fixture.source());
    require(repeat.status==Status::AlreadyEquivalent && repeat.writes.empty(),"Map conversion was not idempotent");
    preflight_mcc_map_gameplay_vanilla(repeat,fixture.source().read);
}
void preserve_custom(const Json& profile) {
    for(const auto& kind:{"scenario","mulg","gauss"}) {
        if(!Fixture(profile).roots.contains(kind))continue;
        Fixture fixture(profile);fixture.banks.at(fixture.roots.at(kind))[100]^=std::byte{1};const auto before=fixture.banks;
        const auto plan=plan_mcc_map_gameplay_vanilla(fixture.source());
        require(plan.status==Status::AuthoredTupleUnrecognized && plan.writes.empty() && plan.guards.empty() && fixture.banks==before,
            "Unrelated custom owner authoring was changed or admitted as stock");
    }
    Fixture renamed(profile);renamed.scenario_path+="_custom";
    require(plan_mcc_map_gameplay_vanilla(renamed.source()).status==Status::NoProfile,"Custom scenario path qualified a stock map");
    if(Fixture(profile).roots.contains("mulg")) {
        Fixture radius(profile);put(radius.banks.at(radius.roots.at("mulg")),30628,0x40e00000);
        require(plan_mcc_map_gameplay_vanilla(radius.source()).status==Status::AuthoredTupleUnrecognized,"Custom authored radius was rewritten");
        Fixture binding(profile);put(binding.banks.at(binding.roots.at("globals")),0x16c,0xe7224815);
        require(plan_mcc_map_gameplay_vanilla(binding.source()).status==Status::AuthoredTupleUnrecognized,"A foreign primary-global mulg binding was admitted");
    }
}
void owner_identity_controls(const Json& profile) {
    for(const auto& kind:{"scenario","gauss"})for(unsigned change=0;change<3;++change) {
        Fixture fixture(profile);
        auto it=std::ranges::find_if(fixture.identities,[&](const auto& item){
            return item.second.source_token==fixture.roots.at(kind);});
        require(it!=fixture.identities.end(),"Missing control owner");
        auto id=it->second;fixture.identities.erase(it);
        if(change==0)id.handle.value^=0x10000;
        if(change==1)id.source_token+=16;
        if(change==2)id.bytes+=4;
        fixture.identities.emplace(id.handle.value,id);
        if(std::string_view(kind)=="scenario")fixture.scenario=id.handle;
        const auto before=fixture.banks;
        const auto plan=plan_mcc_map_gameplay_vanilla(fixture.source());
        require(plan.status==Status::AuthoredTupleUnrecognized && plan.writes.empty() && plan.guards.empty() && fixture.banks==before,
            "Foreign salt, root or allocation size qualified as an authenticated owner");
    }
    Fixture stale(profile);const auto plan=plan_mcc_map_gameplay_vanilla(stale.source());
    stale.banks.at(stale.roots.at("gauss"))[100]^=std::byte{1};const auto before=stale.banks;
    bool rejected=false;try{commit(stale,plan);}catch(const std::exception&){rejected=true;}
    require(rejected && stale.banks==before,"Changed Gauss owner was partly published");
    if(!Fixture(profile).roots.contains("mulg")) {
        Fixture campaign(profile);auto source=campaign.source();source.globals={0xffffffff};
        const auto find=source.find;
        source.find=[&](TagGroup requested,std::string_view path)->std::optional<MccMapGameplayIdentity> {
            require(requested!=tag_groups::multiplayer_globals,"Campaign requested an absent multiplayer owner");
            return find(requested,path);
        };
        const auto globals=campaign.globals;
        const auto resolve=source.resolve;
        source.resolve=[&](TagHandle handle)->std::optional<MccMapGameplayIdentity> {
            require(handle!=globals && !handle.is_null(),"Campaign requested multiplayer primary globals");
            return resolve(handle);
        };
        const auto ready=plan_mcc_map_gameplay_vanilla(source);
        require(ready.status==Status::Ready && ready.writes.size()==1 && ready.guards.size()==2,
            "Campaign Gauss conversion depends on absent multiplayer globals/map ID");
    }
}
void failures(const Json& profile) {
    Fixture missing(profile);missing.missing.insert(0xe7224815);
    require(plan_mcc_map_gameplay_vanilla(missing.source()).status==Status::RequiredDependencyUnavailable,"Missing gameplay owner was admitted");
    Fixture ambiguous(profile);ambiguous.ambiguous=true;
    require(plan_mcc_map_gameplay_vanilla(ambiguous.source()).status==Status::RequiredDependencyUnavailable,"Ambiguous named owner was admitted");
    Fixture salt(profile);salt.wrong_resolve=true;
    require(plan_mcc_map_gameplay_vanilla(salt.source()).status==Status::InvalidMetadata,"Stale-salt resolver result was admitted");
    Fixture group_fixture(profile);group_fixture.wrong_find=true;
    require(plan_mcc_map_gameplay_vanilla(group_fixture.source()).status==Status::InvalidMetadata,"Wrong-group named owner was admitted");
    Fixture short_view(profile);short_view.short_read=true;
    require(plan_mcc_map_gameplay_vanilla(short_view.source()).status==Status::InvalidMetadata,"Short source view was admitted");
    Fixture sound(profile);sound.missing.insert(u32(sound.banks.at(sound.roots.at("gauss")),316));
    require(plan_mcc_map_gameplay_vanilla(sound.source()).status==Status::RequiredDependencyUnavailable,"Missing full-salt donor sound was admitted");
    Fixture header(profile);header.scenario_path="scenarios\\multi\\backwash\\backwash";
    require(plan_mcc_map_gameplay_vanilla(header.source()).status==Status::AuthoredTupleUnrecognized,"Header-only map identity qualified a different scenario bank");
    Fixture stale(profile);auto plan=plan_mcc_map_gameplay_vanilla(stale.source());
    stale.banks.at(stale.roots.at("scenario"))[100]^=std::byte{1};const auto before=stale.banks;
    bool rejected=false;try{commit(stale,plan);}catch(const std::exception&){rejected=true;}
    require(rejected && stale.banks==before,"Stale context was partly published");
    Fixture stale_binding(profile);plan=plan_mcc_map_gameplay_vanilla(stale_binding.source());
    put(stale_binding.banks.at(stale_binding.roots.at("globals")),0x16c,0xe7224815);const auto before_binding=stale_binding.banks;
    rejected=false;try{commit(stale_binding,plan);}catch(const std::exception&){rejected=true;}
    require(rejected && stale_binding.banks==before_binding,"Changed globals binding was partly published");
    Fixture overlap(profile);plan=plan_mcc_map_gameplay_vanilla(overlap.source());
    plan.writes.push_back(plan.writes.front());const auto before_overlap=overlap.banks;
    rejected=false;try{commit(overlap,plan);}catch(const std::exception&){rejected=true;}
    require(rejected && overlap.banks==before_overlap,"Overlapping writes were partly published");
    Fixture invalid_width(profile);plan=plan_mcc_map_gameplay_vanilla(invalid_width.source());plan.writes.front().after.pop_back();
    const auto before_width=invalid_width.banks;rejected=false;try{commit(invalid_width,plan);}catch(const std::exception&){rejected=true;}
    require(rejected && invalid_width.banks==before_width,"Invalid write width was partly published");
}
void material_qualified(const Json& profile) {
    Fixture fixture(profile);const auto before=fixture.banks;
    const auto plan=plan_mcc_material_gameplay_vanilla(fixture.source());
    require(plan.status==Status::Ready && plan.writes.size()==2 && plan.guards.size()==3,
        "Authenticated material tuple did not produce its two guarded halfwords");
    require(fixture.banks==before,"Material planning changed source bytes");
    auto expected=before;const auto root=fixture.roots.at("material_model");
    for(unsigned i=0;i<plan.writes.size();++i) {
        const auto& write=plan.writes[i];const auto offset=i?308u:288u;
        require(write.source_token==root+offset && write.before==std::vector<std::byte>(2) &&
            write.after==std::vector<std::byte>{std::byte{0x45},std::byte{0}},
            "Material correction lost its exact signed16 serialized width or target");
        put16(expected.at(root),offset,69);
    }
    commit(fixture,plan);require(fixture.banks==expected,"Material conversion changed unrelated bytes");
    const auto repeat=plan_mcc_material_gameplay_vanilla(fixture.source());
    require(repeat.status==Status::AlreadyEquivalent && repeat.writes.empty(),"Material conversion was not idempotent");
    preflight_mcc_map_gameplay_vanilla(repeat,fixture.source().read);
    // The original-authored values are independent of whether Gauss has been
    // converted; each component qualifies its own complete donor/target tuple.
    Fixture together(profile);const auto joint_before=together.banks;
    const std::array plans{plan_mcc_map_gameplay_vanilla(together.source()),
        plan_mcc_material_gameplay_vanilla(together.source())};
    require(plans[0].status==Status::Ready && plans[0].writes.size()==1 && plans[1].status==Status::Ready,
        "Material component suppressed the existing Floodzone Gauss profile");
    auto joint_expected=joint_before;
    put(joint_expected.at(together.roots.at("gauss")),312,0xffffffff);
    put(joint_expected.at(together.roots.at("gauss")),316,0xffffffff);
    put16(joint_expected.at(root),288,69);put16(joint_expected.at(root),308,69);
    commit(together,plans);require(together.banks==joint_expected,"Joint conversion changed bytes outside its three fields");
    const std::array repeats{plan_mcc_map_gameplay_vanilla(together.source()),
        plan_mcc_material_gameplay_vanilla(together.source())};
    require(repeats[0].status==Status::AlreadyEquivalent && repeats[1].status==Status::AlreadyEquivalent,
        "Composed authoring conversion was not idempotent");
    commit(together,repeats);require(together.banks==joint_expected,"Already-equivalent composition mutated data");
    const auto gauss_on_material_target=plan_mcc_map_gameplay_vanilla(fixture.source());
    require(gauss_on_material_target.status==Status::Ready,"Material target prevented independent Gauss conversion");
}
void material_preservation(const Json& profile) {
    const auto reject_changed=[&](Fixture& fixture) {
        const auto before=fixture.banks;const auto plan=plan_mcc_material_gameplay_vanilla(fixture.source());
        require(plan.status==Status::AuthoredTupleUnrecognized && plan.writes.empty() && plan.guards.empty() && fixture.banks==before,
            "Unknown material authoring was changed or admitted as vanilla");
        const auto gauss=plan_mcc_map_gameplay_vanilla(fixture.source());
        if(gauss.status==Status::Ready) {
            const std::array plans{gauss,plan};commit(fixture,plans);
            auto expected=before;put(expected.at(fixture.roots.at("gauss")),312,0xffffffff);
            put(expected.at(fixture.roots.at("gauss")),316,0xffffffff);
            require(fixture.banks==expected,"Rejected material component altered Gauss publication scope");
        }
    };
    for(const auto& kind:{"scenario","globals","material_model"}) {
        Fixture changed(profile);changed.banks.at(changed.roots.at(kind))[100]^=std::byte{1};reject_changed(changed);
    }
    for(const auto offset:{0x150u,0x154u,166884u-143360u,288u,308u,290u,252u}) {
        Fixture changed(profile);const bool globals=offset==0x150 || offset==0x154 || offset==166884u-143360u;
        changed.banks.at(changed.roots.at(globals?"globals":"material_model"))[offset]^=std::byte{1};
        reject_changed(changed);
    }
    for(const auto offset:{288u,308u}) {
        Fixture partial(profile);put16(partial.banks.at(partial.roots.at("material_model")),offset,69);reject_changed(partial);
        Fixture foreign(profile);put16(foreign.banks.at(foreign.roots.at("material_model")),offset,42);reject_changed(foreign);
    }
    for(const auto& kind:{"scenario","globals","material_model"})for(unsigned change=0;change<4;++change) {
        Fixture foreign(profile);auto it=std::ranges::find_if(foreign.identities,[&](const auto& item){return item.second.source_token==foreign.roots.at(kind);});
        require(it!=foreign.identities.end(),"Missing material identity control");auto id=it->second;foreign.identities.erase(it);
        if(change==0)id.handle.value^=0x10000;
        if(change==1)id.source_token+=16;
        if(change==2)id.bytes+=4;
        if(change==3)id.group=tag_groups::bitmap;
        foreign.identities.emplace(id.handle.value,id);
        if(std::string_view(kind)=="scenario")foreign.scenario=id.handle;
        if(std::string_view(kind)=="globals")foreign.globals=id.handle;
        const auto before=foreign.banks;const auto plan=plan_mcc_material_gameplay_vanilla(foreign.source());
        require((plan.status==Status::AuthoredTupleUnrecognized || plan.status==Status::RequiredDependencyUnavailable) &&
            plan.writes.empty() && plan.guards.empty() && foreign.banks==before,"Foreign material identity qualified");
    }
    Fixture renamed(profile);renamed.scenario_path+="_mod";
    require(plan_mcc_material_gameplay_vanilla(renamed.source()).status==Status::NoProfile,"Custom scenario qualified material correction");
    for(const auto& kind:{"globals","material_model"}) {
        Fixture missing(profile);const auto root=missing.roots.at(kind);
        const auto id=std::ranges::find_if(missing.identities,[&](const auto& item){return item.second.source_token==root;})->first;
        missing.missing.insert(id);const auto before=missing.banks;
        const auto material=plan_mcc_material_gameplay_vanilla(missing.source());
        const auto gauss=plan_mcc_map_gameplay_vanilla(missing.source());
        require(material.status==Status::RequiredDependencyUnavailable && gauss.status==Status::Ready,
            "Missing material dependency suppressed existing campaign Gauss conversion");
        const std::array plans{gauss,material};commit(missing,plans);
        auto expected=before;put(expected.at(missing.roots.at("gauss")),312,0xffffffff);put(expected.at(missing.roots.at("gauss")),316,0xffffffff);
        require(missing.banks==expected,"Missing material dependency changed other data");
    }
}
void material_failures(const Json& profile) {
    for(unsigned failure=0;failure<4;++failure) {
        Fixture bad(profile);auto source=bad.source();
        if(failure==0)bad.short_read=true;
        if(failure==1)bad.wrong_resolve=true;
        if(failure==2)bad.wrong_find=true;
        if(failure==3)source.read={};
        const auto before=bad.banks;const auto plan=plan_mcc_material_gameplay_vanilla(source);
        require(plan.status==Status::InvalidMetadata && plan.writes.empty() && plan.guards.empty() && bad.banks==before,
            "Malformed material callbacks were admitted");
    }
    Fixture ambiguous(profile);ambiguous.ambiguous=true;
    require(plan_mcc_material_gameplay_vanilla(ambiguous.source()).status==Status::RequiredDependencyUnavailable,
        "Ambiguous material owner was admitted");
    for(const auto& kind:{"scenario","globals","material_model"}) {
        Fixture stale(profile);const std::array plans{plan_mcc_map_gameplay_vanilla(stale.source()),
            plan_mcc_material_gameplay_vanilla(stale.source())};
        stale.banks.at(stale.roots.at(kind))[100]^=std::byte{1};const auto before=stale.banks;
        bool rejected=false;try{commit(stale,plans);}catch(const std::exception&){rejected=true;}
        require(rejected && stale.banks==before,"A stale later material component partly published Gauss");
    }
    Fixture overlapping(profile);const auto material=plan_mcc_material_gameplay_vanilla(overlapping.source());
    const std::array overlap{material,material};const auto before=overlapping.banks;
    bool rejected=false;try{commit(overlapping,overlap);}catch(const std::exception&){rejected=true;}
    require(rejected && overlapping.banks==before,"Cross-component overlapping writes were partly published");
    Fixture invalid(profile);auto bad=plan_mcc_material_gameplay_vanilla(invalid.source());bad.status=Status::InvalidMetadata;
    const std::array plans{plan_mcc_map_gameplay_vanilla(invalid.source()),bad};const auto invalid_before=invalid.banks;
    rejected=false;try{commit(invalid,plans);}catch(const std::exception&){rejected=true;}
    require(rejected && invalid.banks==invalid_before,"Invalid later component partly published Gauss");
    auto rejected_plan=material;rejected_plan.status=Status::AuthoredTupleUnrecognized;
    rejected=false;try{preflight_mcc_map_gameplay_vanilla(std::span{&rejected_plan,1},overlapping.source().read);}catch(const std::exception&){rejected=true;}
    require(rejected,"Rejected component retained hidden publication data");
}
void foot_state(Fixture& fixture,const Json& profile,std::string_view state) {
    auto& bytes=fixture.banks.at(fixture.roots.at("foot"));
    if(state=="seed") {
        const auto offset=profile.at("foot_owner_offset").get<unsigned>();
        put(bytes,offset,0xffffffff);put(bytes,offset+4,0xffffffff);
    } else if(state=="union")for(const auto& row:profile.at("foot_rows")) {
        const auto offset=row.at("owner_offset").get<unsigned>();
        put(bytes,offset,0xffffffff);put(bytes,offset+4,0xffffffff);
    }
}
std::set<std::string> foot_supports(const Json& profile,std::string_view state) {
    const auto& entry=profile.at("foot_states").at(std::string(state));std::set<std::string> out;
    for(const auto field:{"active_primary_support_ids","active_donor_secondary_support_ids"})
        for(const auto& id:entry.at(field))out.insert(id.get<std::string>());
    return out;
}
std::uint32_t support_handle(const Json& profile,const std::string& id) {
    return profile.at("foot_supports").at(id).at("identity").at("datum").get<std::uint32_t>();
}
std::string support_kind(const Json& profile,std::uint32_t handle) {
    return profile.at("support_kinds").at(std::to_string(handle)).get<std::string>();
}
void foot_qualified(const Json& profile) {
    for(const auto state:{"donor","seed","union"}) {
        Fixture fixture(profile);foot_state(fixture,profile,state);const auto before=fixture.banks;
        const auto& expected_state=profile.at("foot_states").at(state);
        const auto plan=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
        const auto count=expected_state.at("write_count").get<std::size_t>();
        require(plan.status==(count?Status::Ready:Status::AlreadyEquivalent) && plan.writes.size()==count &&
            plan.guards.size()==3+foot_supports(profile,state).size() && fixture.banks==before,
            "Authenticated D/seed/union foot tuple lost its exact writes or deduplicated active guards");
        std::set<std::uint32_t> tokens,expected_tokens,guards;
        for(const auto& value:expected_state.at("write_offsets"))expected_tokens.insert(fixture.roots.at("foot")+value.get<std::uint32_t>());
        for(const auto& write:plan.writes) {
            require(write.before.size()==8 && write.after==std::vector<std::byte>(8,std::byte{0xff}),
                "Batch foot write lost its exact serialized reference width");
            tokens.insert(write.source_token);
        }
        for(const auto& guard:plan.guards)require(guards.insert(guard.source_token).second,"Supporting owner guard was duplicated");
        require(tokens==expected_tokens,"Batch foot plan changed the wrong finite field set");
        Fixture wanted(profile);foot_state(wanted,profile,"union");
        commit(fixture,plan);require(fixture.banks==wanted.banks,"Batch foot publication changed unrelated complete-owner bytes");
        const auto repeat=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
        require(repeat.status==Status::AlreadyEquivalent && repeat.writes.empty(),"Union foot target was not idempotent");
        commit(fixture,repeat);require(fixture.banks==wanted.banks,"Union repeat changed source bytes");
        // Every now-inactive donor secondary can be absent and modified. The
        // planner must not read it, while all active primary metadata remains.
        const auto active=foot_supports(profile,"union");std::vector<std::pair<std::uint32_t,std::size_t>> inactive;
        for(const auto& [id,metadata]:profile.at("foot_supports").items())if(!active.contains(id)) {
            const auto handle=metadata.at("identity").at("datum").get<std::uint32_t>();
            const auto root=fixture.roots.at(support_kind(profile,handle));
            fixture.missing.insert(handle);fixture.banks.at(root)[0]^=std::byte{1};
            inactive.emplace_back(root,fixture.banks.at(root).size());
        }
        auto source=fixture.source();const auto read=source.read;
        source.read=[&](std::uint32_t at,std::size_t size) {
            for(const auto& [root,extent]:inactive)require(std::uint64_t(at)+size<=root || at>=std::uint64_t(root)+extent,
                "Union read its inactive donor secondary");
            return read(at,size);
        };
        require(plan_mcc_foot_gameplay_vanilla(source,fixture.globals).status==Status::AlreadyEquivalent,
            "Inactive secondary changes blocked union idempotence");
        fixture.banks.at(fixture.roots.at("primary_sound"))[0]^=std::byte{1};
        const auto custom=fixture.banks;const auto rejected=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
        require(rejected.status==Status::AuthoredTupleUnrecognized && rejected.writes.empty() && rejected.guards.empty() && fixture.banks==custom,
            "Union admitted modified active primary metadata");
    }
}
void foot_active_support_controls(const Json& profile) {
    for(const auto state:{"donor","seed","union"}) {
        Fixture fixture(profile);foot_state(fixture,profile,state);
        const auto active=foot_supports(profile,state);
        for(const auto& id:active) {
            const auto handle=support_handle(profile,id);const auto root=fixture.roots.at(support_kind(profile,handle));
            auto& bytes=fixture.banks.at(root);bytes[0]^=std::byte{1};
            auto plan=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
            require(plan.status==Status::AuthoredTupleUnrecognized && plan.guards.empty() && plan.writes.empty(),
                "Modified required sound metadata was admitted in a finite foot state");
            bytes[0]^=std::byte{1};fixture.missing.insert(handle);
            plan=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
            require(plan.status==Status::RequiredDependencyUnavailable && plan.guards.empty() && plan.writes.empty(),
                "Missing required sound metadata was admitted in a finite foot state");
            fixture.missing.erase(handle);
            auto node=fixture.identities.extract(handle);const auto old=node.mapped();
            node.key()^=0x10000;node.mapped().handle.value^=0x10000;fixture.identities.insert(std::move(node));
            plan=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
            require(plan.status==Status::RequiredDependencyUnavailable && plan.guards.empty() && plan.writes.empty(),
                "Foreign full-salt required support was admitted");
            fixture.identities.erase(old.handle.value^0x10000);fixture.identities.emplace(old.handle.value,old);
        }
        for(const auto& [id,metadata]:profile.at("foot_supports").items())if(!active.contains(id)) {
            const auto handle=metadata.at("identity").at("datum").get<std::uint32_t>();
            auto& bytes=fixture.banks.at(fixture.roots.at(support_kind(profile,handle)));bytes[0]^=std::byte{1};
            fixture.missing.insert(handle);const auto before=fixture.banks;
            const auto plan=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
            require(plan.status==(std::string_view(state)=="union"?Status::AlreadyEquivalent:Status::Ready) && fixture.banks==before,
                "An inactive sound metadata owner blocked the finite foot transition");
            fixture.missing.erase(handle);bytes[0]^=std::byte{1};
        }
    }
}
void foot_subsets_and_aliases(const Json& profile) {
    const auto& row=*std::ranges::find_if(profile.at("foot_rows"),[](const auto& value){return !value.at("narrow_seed_field").template get<bool>();});
    const auto offset=row.at("owner_offset").get<unsigned>();
    for(const auto state:{"donor","seed","union"}) {
        Fixture mixed(profile);foot_state(mixed,profile,state);auto& bytes=mixed.banks.at(mixed.roots.at("foot"));
        if(std::string_view(state)=="union") {
            const auto donor=unhex(row.at("secondary_donor_hex").get_ref<const std::string&>());
            std::ranges::copy(donor,bytes.begin()+offset);
        } else {put(bytes,offset,0xffffffff);put(bytes,offset+4,0xffffffff);}
        const auto before=mixed.banks;const auto plan=plan_mcc_foot_gameplay_vanilla(mixed.source(),mixed.globals);
        require(plan.status==Status::AuthoredTupleUnrecognized && plan.guards.empty() && plan.writes.empty() && mixed.banks==before,
            "An arbitrary mixed subset qualified as donor, seed or union");
    }
    for(const auto kind:{"foot","primary_sound","secondary_sound"}) {
        Fixture alias(profile);auto found=std::ranges::find_if(alias.identities,[&](const auto& entry){return entry.second.source_token==alias.roots.at(kind);});
        require(found!=alias.identities.end(),"Alias control is absent");found->second.bytes=0;
        const auto before=alias.banks;const auto plan=plan_mcc_foot_gameplay_vanilla(alias.source(),alias.globals);
        require(plan.status==Status::AuthoredTupleUnrecognized && plan.guards.empty() && plan.writes.empty() && alias.banks==before,
            "An unauthenticated alias extent was followed to a neighbouring owner");
    }
    Fixture ambiguous(profile);const auto handle=profile.at("primary_handle").get<std::uint32_t>();
    auto alias=ambiguous.identities.at(handle);alias.handle.value^=0x10000;
    ambiguous.identities.emplace(alias.handle.value,alias);
    const auto before=ambiguous.banks;const auto plan=plan_mcc_foot_gameplay_vanilla(ambiguous.source(),ambiguous.globals);
    require(plan.status==Status::RequiredDependencyUnavailable && plan.guards.empty() && plan.writes.empty() && ambiguous.banks==before,
        "Ambiguous named support aliases were admitted");
}
void foot_preservation(const Json& profile) {
    const auto reject=[&](Fixture& fixture) {
        const auto before=fixture.banks;const auto plan=plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals);
        require(plan.status==Status::AuthoredTupleUnrecognized && plan.guards.empty() && plan.writes.empty() && fixture.banks==before,
            "Unknown foot/support/context authoring was changed or admitted");
    };
    for(const auto kind:{"scenario","globals","foot","primary_sound","secondary_sound"}) {
        Fixture changed(profile);changed.banks.at(changed.roots.at(kind))[0]^=std::byte{1};reject(changed);
    }
    const auto offset=profile.at("foot_owner_offset").get<unsigned>();
    for(const auto at:{offset,offset+4,offset+8,offset+12,4u}) {
        Fixture custom(profile);custom.banks.at(custom.roots.at("foot"))[at]^=std::byte{1};reject(custom);
    }
    Fixture partial(profile);put(partial.banks.at(partial.roots.at("foot")),offset,0xffffffff);reject(partial);
    Fixture duplicate(profile);
    // An added duplicate default key is custom authoring, even when it would
    // be shadowed by the current first matching row.
    const auto root=duplicate.roots.at("foot");auto& bytes=duplicate.banks.at(root);
    const auto action=u32(bytes,4)-root;const auto sound=u32(bytes,action+12)-root;
    require(u32(bytes,action+8)>1,"Duplicate-key control needs an actual later row");
    put16(bytes,sound+24+0x14,0);reject(duplicate);
    Fixture bounds(profile);put(bounds.banks.at(bounds.roots.at("foot")),4,0xfffffff8);reject(bounds);
    Fixture renamed(profile);renamed.scenario_path+="_custom";
    require(plan_mcc_foot_gameplay_vanilla(renamed.source(),renamed.globals).status==Status::NoProfile,
        "Unknown scenario qualified a foot profile");
    Fixture binding(profile);auto foreign=binding.globals;foreign.value^=0x10000;
    const auto before=binding.banks;
    const auto plan=plan_mcc_foot_gameplay_vanilla(binding.source(),foreign);
    require(plan.status==Status::AuthoredTupleUnrecognized && plan.guards.empty() && plan.writes.empty() && binding.banks==before,
        "Foreign native group-first material globals qualified");
    require(plan_mcc_foot_gameplay_vanilla(binding.source(),{}).status==Status::RequiredDependencyUnavailable,
        "Absent native material globals qualified");
    for(const auto kind:{"scenario","globals","foot","primary_sound","secondary_sound"})for(unsigned change=0;change<4;++change) {
        Fixture foreign_owner(profile);const auto at=foreign_owner.roots.at(kind);
        auto it=std::ranges::find_if(foreign_owner.identities,[&](const auto& item){return item.second.source_token==at;});
        require(it!=foreign_owner.identities.end(),"Missing foot identity control");auto id=it->second;foreign_owner.identities.erase(it);
        if(change==0)id.handle.value^=0x10000;
        if(change==1)id.source_token+=16;
        if(change==2)id.bytes+=4;
        if(change==3)id.group=tag_groups::bitmap;
        foreign_owner.identities.emplace(id.handle.value,id);
        if(std::string_view(kind)=="scenario")foreign_owner.scenario=id.handle;
        if(std::string_view(kind)=="globals")foreign_owner.globals=id.handle;
        const auto bytes_before=foreign_owner.banks;
        const auto rejected=plan_mcc_foot_gameplay_vanilla(foreign_owner.source(),foreign_owner.globals);
        require((rejected.status==Status::AuthoredTupleUnrecognized || rejected.status==Status::RequiredDependencyUnavailable) &&
            rejected.guards.empty() && rejected.writes.empty() && foreign_owner.banks==bytes_before,"Foreign foot identity qualified");
    }
}
void foot_failures(const Json& profile) {
    for(const auto kind:{"scenario","globals","foot","primary_sound","secondary_sound"}) {
        Fixture missing(profile);const auto root=missing.roots.at(kind);
        const auto id=std::ranges::find_if(missing.identities,[&](const auto& item){return item.second.source_token==root;})->first;
        missing.missing.insert(id);const auto before=missing.banks;
        const auto plan=plan_mcc_foot_gameplay_vanilla(missing.source(),missing.globals);
        require(plan.status==Status::RequiredDependencyUnavailable && plan.guards.empty() && plan.writes.empty() && missing.banks==before,
            "Missing full-salt foot dependency was admitted");
    }
    for(unsigned failure=0;failure<5;++failure) {
        Fixture bad(profile);auto source=bad.source();
        if(failure==0)bad.short_read=true;
        if(failure==1)bad.wrong_resolve=true;
        if(failure==2)bad.wrong_find=true;
        if(failure==3)source.read={};
        if(failure==4)source.resolve={};
        const auto before=bad.banks;const auto plan=plan_mcc_foot_gameplay_vanilla(source,bad.globals);
        require(plan.status==Status::InvalidMetadata && plan.guards.empty() && plan.writes.empty() && bad.banks==before,
            "Malformed foot callbacks were admitted");
    }
    Fixture ambiguous(profile);ambiguous.ambiguous=true;
    require(plan_mcc_foot_gameplay_vanilla(ambiguous.source(),ambiguous.globals).status==Status::RequiredDependencyUnavailable,
        "Ambiguous foot authoring qualified");
}
void foot_composition(const Json& profile) {
    const auto plans_for=[](Fixture& fixture) {
        return std::array{plan_mcc_map_gameplay_vanilla(fixture.source()),
            plan_mcc_material_gameplay_vanilla(fixture.source()),
            plan_mcc_foot_gameplay_vanilla(fixture.source(),fixture.globals)};
    };
    Fixture seed(profile);auto expected=seed.banks;
    put(expected.at(seed.roots.at("gauss")),312,0xffffffff);put(expected.at(seed.roots.at("gauss")),316,0xffffffff);
    put16(expected.at(seed.roots.at("material_model")),288,69);put16(expected.at(seed.roots.at("material_model")),308,69);
    for(const auto& row:profile.at("foot_rows")) {
        const auto offset=row.at("owner_offset").get<unsigned>();
        put(expected.at(seed.roots.at("foot")),offset,0xffffffff);put(expected.at(seed.roots.at("foot")),offset+4,0xffffffff);
    }
    for(const auto state:{"donor","seed","union"})for(unsigned target_mask=0;target_mask<4;++target_mask) {
        Fixture mixed(profile);foot_state(mixed,profile,state);
        for(unsigned component=0;component<2;++component)if(target_mask&(1u<<component)) {
            const auto plans=plans_for(mixed);require(plans[component].status==Status::Ready,"Actual composed donor did not qualify");
            commit(mixed,plans[component]);
        }
        const auto plans=plans_for(mixed);
        for(unsigned component=0;component<3;++component)
            require(plans[component].status==((component==2?std::string_view(state)=="union":bool(target_mask&(1u<<component)))?Status::AlreadyEquivalent:Status::Ready),
                "One component's target state changed another component's admission");
        commit(mixed,plans);require(mixed.banks==expected,"Three-component publication changed unrelated bytes");
    }
    const std::array kinds{"gauss","material_model","foot"};
    for(unsigned component=0;component<3;++component) {
        Fixture custom(profile);custom.banks.at(custom.roots.at(kinds[component]))[100]^=std::byte{1};
        const auto before=custom.banks;const auto plans=plans_for(custom);
        require(plans[component].status==Status::AuthoredTupleUnrecognized,"Custom optional component was admitted");
        auto wanted=before;
        for(unsigned other=0;other<3;++other)if(other!=component) {
            require(plans[other].status==Status::Ready,"Custom optional owner suppressed an independent component");
            wanted.at(custom.roots.at(kinds[other]))=expected.at(custom.roots.at(kinds[other]));
        }
        commit(custom,plans);require(custom.banks==wanted,"Rejected component changed independent publication scope");
    }
    for(const auto kind:{"foot","primary_sound","secondary_sound"}) {
        Fixture stale(profile);const auto plans=plans_for(stale);
        stale.banks.at(stale.roots.at(kind))[0]^=std::byte{1};const auto before=stale.banks;
        bool rejected=false;try{commit(stale,plans);}catch(const std::exception&){rejected=true;}
        require(rejected && stale.banks==before,"Late third-component guard partly published earlier components");
    }
    Fixture bad(profile);auto plans=plans_for(bad);plans[2].status=Status::InvalidMetadata;const auto before=bad.banks;
    bool rejected=false;try{commit(bad,plans);}catch(const std::exception&){rejected=true;}
    require(rejected && bad.banks==before,"Invalid third component partly published earlier components");
    plans=plans_for(bad);plans[2].writes.push_back(plans[0].writes.front());
    plans[2].guards.push_back(plans[0].guards[1]);
    rejected=false;try{commit(bad,plans);}catch(const std::exception&){rejected=true;}
    require(rejected && bad.banks==before,"Cross-component overlap partly published data");
}
} // namespace
int main(int argc,char** argv)try {
    // Positive banks are authenticated, user-owned reference fixtures, never
    // fabricated payloads or bundled product assets. The disposable runner
    // supplies a fixture made from the retained offline evidence receipt.
    if(argc!=2 || !*argv[1]){std::cout<<"SKIP: supply authenticated map-gameplay fixture JSON\n";return 77;}
    Json fixture;std::ifstream input(argv[1]);require(bool(input),"Cannot open authenticated fixture");input>>fixture;
    const auto count=fixture.at("profiles").size();
    require(count==3 || count==28,"Fixture does not contain an authenticated context set");
    const auto lockout=std::ranges::find_if(fixture.at("profiles"),[](const auto& profile){return profile.at("map")=="lockout.map";});
    require(lockout!=fixture.at("profiles").end(),"Fixture lacks the retained transactional control context");
    for(const auto& profile:fixture.at("profiles")){qualified(profile);preserve_custom(profile);owner_identity_controls(profile);}
    failures(*lockout);
    const bool materials=fixture.contains("material_profile");
    if(materials){material_qualified(fixture.at("material_profile"));material_preservation(fixture.at("material_profile"));material_failures(fixture.at("material_profile"));}
    const bool feet=fixture.contains("foot_profiles");
    if(feet) {
        const auto& profiles=fixture.at("foot_profiles");require(profiles.size()==27,"Foot fixture lacks its authenticated27 contexts");
        std::set<std::string> owners;
        for(const auto& profile:profiles) {
            foot_qualified(profile);foot_subsets_and_aliases(profile);
            if(owners.insert(profile.at("owner_profile_id").get<std::string>()).second)
                foot_active_support_controls(profile);
        }
        require(owners.size()==16,"Batch fixture lost a distinct physical owner transition");
        const auto control=std::ranges::find_if(profiles,[](const auto& profile){return profile.at("map")=="lockout.map";});
        const auto composed=std::ranges::find_if(profiles,[](const auto& profile){return profile.at("map")=="06b_floodzone.map";});
        require(control!=profiles.end() && composed!=profiles.end(),"Foot fixtures lack transactional control contexts");
        foot_preservation(*control);foot_failures(*control);foot_composition(*composed);
    }
    std::cout<<"GREEN: "<<count<<" actual map tuples, idempotence, custom preservation, full owner identity, campaign isolation and transactional preflight\n";
    std::cout<<"Material component controls: "<<(materials?"GREEN exact original tuple, signed16 widths, independent Gauss composition and atomic preflight":"not supplied")<<'\n';
    std::cout<<"Foot component controls: "<<(feet?"GREEN27 D/seed/union contexts,2345fields,16owners, active supports, custom/subset/alias/key/bounds and three-component atomic composition":"not supplied")<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
