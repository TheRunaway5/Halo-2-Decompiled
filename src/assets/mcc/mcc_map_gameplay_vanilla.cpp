#include "halo2/mcc_map_gameplay_vanilla.hpp"
#include <algorithm>
#include <array>
#include <new>
#include <openssl/evp.h>
#include <stdexcept>

namespace halo2 {
namespace {
using Span=std::span<const std::byte>;
using Digest=std::array<unsigned char,32>;
constexpr Digest hex(std::string_view value) {
    Digest out{};
    const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(unsigned n=0;n<32;++n)out[n]=static_cast<unsigned char>(digit(value[n*2])*16+digit(value[n*2+1]));
    return out;
}
struct Profile {
    std::string_view label,path;
    std::uint32_t scenario_handle,scenario_root,scenario_bytes,scenario_type,map_id;
    Digest scenario;
    std::uint32_t gauss_handle,gauss_root,gauss_bytes;
    Digest gauss_donor,gauss_target,mulg_target;
    bool null_gauss;
};
// Fresh owned MCC contexts and authenticated offline-original field reads:
// all-contexts-v2/inventory.json SHA256
// adf81cc7efaf9e74c2432cb033c6340008e9e9f0dbec26052630ef985de99453.
// The generated table contains identities/hashes/scalars, never asset banks.
// Original map hashes/field reads accompany that receipt. TU1.5 XBE SHA256
// 21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd:
// 24D5F0 resolves globals+16C -> mulg+0C -> runtime+534; 24D618 loads
// constants+D8, 24D649 squares it, and 24D71E/24D721 compare distance.
// Twenty-two map contexts use null Gauss flybys; six retain the donor identity.
// Campaign Gauss owners are local and have no resident multiplayer allocation.
constexpr auto mulg_donor=hex("162e7d0a735fa8d40c118ff587005b0b9b2ae1b3ac0500dded694b164cda5ee4");
constexpr std::array profiles{
#include "assets/mcc/mcc_map_gameplay_profiles.inc"
};
struct TeleporterPatch {std::uint32_t offset,before,after;};
struct TeleporterProfile {
    std::string_view label,path;
    std::uint32_t count;
    Digest donor,target;
    std::span<const TeleporterPatch> patches;
};
#include "gameplay/world/mcc_teleporter_profiles.inc"
struct FootOwner {
    std::uint32_t handle,root,bytes;
    Digest hash;
};
struct FootSupport {
    FootOwner owner;
    std::string_view name;
};
struct FootRow {
    std::uint32_t action,material_key,material_token,action_token,sounds,sound_token,row_ordinal,row_token;
    Digest material,descriptor;
    Digest row_donor,row_target;
    std::uint32_t primary,secondary;
    bool seed;
};
struct FootDefinition {
    FootOwner owner;
    Digest seed,target;
    std::uint32_t actions,action_token;
    std::span<const FootRow> rows;
    std::span<const FootSupport> supports;
};
struct FootProfile {
    std::string_view label,path;
    FootOwner scenario,globals;
    std::uint32_t scenario_type,map_id,materials,material_token;
    const FootDefinition* definition;
};
// Exact original-backed90 authored action/material classes in27 contexts:
// batch preparation53704c20ab64fdc5e05733c6f96a21d22187a5cdbe51fcc9c8e55f1a3f1ca82e.
// Generic TU1.5 wrapper18B1D0 /359-instruction consumer authority qualifies
// first signed material-key selection. Original CPU evidence remains one
// Lockout seed; no all-row CPU, sample-bank, playback or live parity claim.
// Three complete owner states admit donor -> union, prior seed -> union and
// idempotent union. Arbitrary mixed subsets and custom authoring stay unknown.
#include "gameplay/weapons/mcc_foot_gameplay_profiles.inc"
constexpr std::string_view mulg_path="multiplayer\\multiplayer_globals";
constexpr std::string_view gauss_path="objects\\vehicles\\warthog\\turrets\\gauss\\weapon\\gauss_bullet";
constexpr std::string_view scorpion_path="sound\\vehicles\\scorpion\\scorpion_projectile_up";
struct Rejected : std::runtime_error {
    MccMapGameplayStatus status;const char* reason;
    Rejected(MccMapGameplayStatus value,const char* text):std::runtime_error(text),status(value),reason(text){}
};
[[noreturn]] void reject(MccMapGameplayStatus status,const char* reason){throw Rejected{status,reason};}
std::uint32_t add(std::uint32_t address,std::uint64_t bytes) {
    if(bytes>0xffffffffu-address)reject(MccMapGameplayStatus::InvalidMetadata,"Source token overflows");
    return address+std::uint32_t(bytes);
}
Span checked(const std::function<Span(std::uint32_t,std::size_t)>& read,std::uint32_t address,std::size_t bytes) {
    if(!address || address==0xffffffffu || bytes>0x100000000ull-address)
        reject(MccMapGameplayStatus::InvalidMetadata,"Invalid source allocation extent");
    const auto result=read(address,bytes);
    if(result.size()!=bytes)reject(MccMapGameplayStatus::InvalidMetadata,"Source reader returned a short view");
    return result;
}
std::uint32_t word(Span bytes,std::size_t at) {
    if(at>bytes.size() || bytes.size()-at<4)reject(MccMapGameplayStatus::InvalidMetadata,"Word escapes source allocation");
    std::uint32_t result{};
    for(unsigned n=0;n<4;++n)result|=std::to_integer<std::uint32_t>(bytes[at+n])<<(n*8);
    return result;
}
Digest digest(Span bytes) {
    Digest out{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),out.data(),&size,EVP_sha256(),nullptr)!=1 || size!=out.size())
        reject(MccMapGameplayStatus::InvalidMetadata,"Cannot fingerprint source allocation");
    return out;
}
bool same(const MccMapGameplayIdentity& a,const MccMapGameplayIdentity& b) {
    return a.handle==b.handle && a.group==b.group && a.name==b.name &&
        a.source_token==b.source_token && a.bytes==b.bytes;
}
void expected(const MccMapGameplayIdentity& tag,TagGroup group,std::string_view name,
    std::uint32_t datum,std::uint32_t root,std::uint32_t bytes) {
    if(tag.group!=group || tag.name!=name || tag.handle.value!=datum ||
        tag.source_token!=root || tag.bytes!=bytes)
        reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Directory identity differs from authenticated authoring");
}
MccMapGameplayWrite write(std::uint32_t address,Span before,std::uint32_t after,std::string field) {
    MccMapGameplayWrite out{address,{before.begin(),before.end()},std::vector<std::byte>(before.size()),std::move(field)};
    for(std::size_t n=0;n<out.after.size();++n)out.after[n]=std::byte(after>>((n%4)*8));
    return out;
}
} // namespace

MccMapGameplayPlan plan_mcc_map_gameplay_vanilla(const MccMapGameplaySource& source) {
    MccMapGameplayPlan out;
    const auto profile=std::ranges::find(profiles,source.scenario_path,&Profile::path);
    if(profile==profiles.end()){out.reason="Scenario has no authenticated map profile";return out;}
    out.profile=profile->label;
    try {
        if(!source.read || !source.resolve || !source.find)
            reject(MccMapGameplayStatus::InvalidMetadata,"Checked map source callbacks are required");
        const auto resolve=[&](TagHandle handle) {
            const auto value=source.resolve(handle);
            if(!value)reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Required resident directory owner is absent");
            if(handle.is_null() || value->handle!=handle)
                reject(MccMapGameplayStatus::InvalidMetadata,"Resolver did not round-trip a full salted handle");
            return *value;
        };
        const auto find=[&](TagGroup group,std::string_view name) {
            const auto value=source.find(group,name);
            if(!value)reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Required unique named owner is absent");
            if(value->group!=group || value->name!=name || !same(*value,resolve(value->handle)))
                reject(MccMapGameplayStatus::InvalidMetadata,"Named owner does not round-trip its full identity");
            return *value;
        };
        const auto scenario=resolve(source.scenario);
        expected(scenario,tag_groups::scenario,profile->path,profile->scenario_handle,
            profile->scenario_root,profile->scenario_bytes);
        if(!same(scenario,find(tag_groups::scenario,profile->path)))
            reject(MccMapGameplayStatus::InvalidMetadata,"Primary scenario differs from its unique directory identity");
        const auto scenario_bytes=checked(source.read,scenario.source_token,scenario.bytes);
        if(digest(scenario_bytes)!=profile->scenario)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Scenario bank differs from authenticated map authoring");
        if(word(scenario_bytes,0x10)%65536!=profile->scenario_type)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Scenario differs from the authenticated gameplay type");
        const auto gauss=find(tag_groups::projectile,gauss_path);
        expected(gauss,tag_groups::projectile,gauss_path,profile->gauss_handle,
            profile->gauss_root,profile->gauss_bytes);
        const auto gauss_bytes=checked(source.read,gauss.source_token,gauss.bytes);
        const auto gauss_hash=digest(gauss_bytes);
        if(gauss_hash!=profile->gauss_donor && gauss_hash!=profile->gauss_target)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete Gauss owner differs from its donor and target tuples");
        if(word(gauss_bytes,312)!=0xffffffffu || word(gauss_bytes,316)!=0xffffffffu) {
            const auto sound=resolve({word(gauss_bytes,316)});
            if(word(gauss_bytes,312)!=tag_groups::sound.value || sound.group!=tag_groups::sound || sound.name!=scorpion_path ||
                !sound.source_token || !sound.bytes)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Gauss donor flyby identity differs");
        }
        out.guards={{scenario.source_token,{scenario_bytes.begin(),scenario_bytes.end()}},
            {gauss.source_token,{gauss_bytes.begin(),gauss_bytes.end()}}};
        if(profile->scenario_type==1) {
            const auto level=word(scenario_bytes,0x39c);
            const auto level_id=word(checked(source.read,add(level,0x14),4),0);
            if(word(checked(source.read,level_id,4),0)!=profile->map_id)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Scenario map ID differs from its authenticated bank");
            const auto mulg=find(tag_groups::multiplayer_globals,mulg_path);
            expected(mulg,tag_groups::multiplayer_globals,mulg_path,0xf3de54d1,0x01188edc,31408);
            const auto globals=resolve(source.globals);
            if(globals.group!=tag_groups::globals || globals.name!="globals\\globals" || globals.bytes<0x170 ||
                word(checked(source.read,add(globals.source_token,0x16c),4),0)!=mulg.handle.value)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Primary globals does not bind the authenticated multiplayer owner");
            const auto mulg_bytes=checked(source.read,mulg.source_token,mulg.bytes);
            const auto mulg_hash=digest(mulg_bytes);
            if(mulg_hash!=mulg_donor && mulg_hash!=profile->mulg_target)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete multiplayer owner differs from its donor and target tuples");
            out.guards.push_back({mulg.source_token,{mulg_bytes.begin(),mulg_bytes.end()}});
            const auto binding=checked(source.read,add(globals.source_token,0x16c),4);
            out.guards.push_back({add(globals.source_token,0x16c),{binding.begin(),binding.end()}});
            if(mulg_hash!=profile->mulg_target) {
                // Whole-bank hashes authenticate this exact constants+D8 word.
                out.writes.push_back(write(add(mulg.source_token,30628),mulg_bytes.subspan(30628,4),0x40800000,
                    "Multiplayer Constants/Flag Reset Stop Distance"));
            }
        }
        if(gauss_hash!=profile->gauss_target) {
            if(!profile->null_gauss)reject(MccMapGameplayStatus::InvalidMetadata,"Profile requests an unauthenticated flyby replacement");
            out.writes.push_back(write(add(gauss.source_token,312),gauss_bytes.subspan(312,8),0xffffffff,
                "Gauss projectile/Flyby Sound"));
        }
        out.status=out.writes.empty()?MccMapGameplayStatus::AlreadyEquivalent:MccMapGameplayStatus::Ready;
        out.reason=out.writes.empty()?"Authenticated original-equivalent map tuple":"Authenticated map-specific original authoring restoration";
        preflight_mcc_map_gameplay_vanilla(out,source.read);
    } catch(const Rejected& error) {
        out.status=error.status;out.reason=error.reason;out.guards.clear();out.writes.clear();
    } catch(const std::bad_alloc&) {throw;
    } catch(const std::exception& error) {
        out.status=MccMapGameplayStatus::InvalidMetadata;out.reason=error.what();out.guards.clear();out.writes.clear();
    }
    return out;
}

MccMapGameplayPlan plan_mcc_teleporter_gameplay_vanilla(const MccMapGameplaySource& source) {
    MccMapGameplayPlan out;
    const auto profile=std::ranges::find(teleporter_profiles,source.scenario_path,&TeleporterProfile::path);
    if(profile==teleporter_profiles.end())return out;
    out.profile=profile->label;
    try {
        if(!source.resolve || !source.find || !source.read)
            reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Teleporter profile needs its resident directory owner");
        const auto scenario=source.resolve(source.scenario);
        const auto named=source.find(tag_groups::scenario,source.scenario_path);
        if(!scenario || !named || !same(*scenario,*named) || scenario->handle!=source.scenario ||
           scenario->group!=tag_groups::scenario || scenario->name!=source.scenario_path)
            reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Teleporter scenario identity is missing or ambiguous");
        if(scenario->bytes<0x120u)
            reject(MccMapGameplayStatus::InvalidMetadata,"Teleporter scenario root is truncated");
        const auto descriptor=checked(source.read,add(scenario->source_token,0x118u),8);
        const auto count=word(descriptor,0),address=word(descriptor,4);
        if(count!=profile->count)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete teleporter flag bank has different authoring");
        const auto bytes=std::uint64_t(count)*32u;
        if(address<scenario->source_token || std::uint64_t(address)+bytes>
            std::uint64_t(scenario->source_token)+scenario->bytes)
            reject(MccMapGameplayStatus::InvalidMetadata,"Teleporter flag bank escapes its actual scenario owner");
        const auto bank=checked(source.read,address,std::size_t(bytes));const auto observed=digest(bank);
        if(observed!=profile->donor && observed!=profile->target)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete teleporter flag bank differs from donor and target");
        out.guards.push_back({add(scenario->source_token,0x118u),{descriptor.begin(),descriptor.end()}});
        out.guards.push_back({address,{bank.begin(),bank.end()}});
        if(observed==profile->donor)for(const auto& patch:profile->patches) {
            if(word(bank,patch.offset)!=patch.before)
                reject(MccMapGameplayStatus::InvalidMetadata,"Teleporter profile guard is inconsistent");
            out.writes.push_back(write(add(address,patch.offset),bank.subspan(patch.offset,4),patch.after,
                "Original Xbox netgame teleporter position/facing"));
        }
        out.status=out.writes.empty()?MccMapGameplayStatus::AlreadyEquivalent:MccMapGameplayStatus::Ready;
        out.reason="Authenticated original Xbox teleporter flag records";
        preflight_mcc_map_gameplay_vanilla(out,source.read);
    } catch(const Rejected& error) {
        out.status=error.status;out.reason=error.reason;out.guards.clear();out.writes.clear();
    } catch(const std::bad_alloc&) {throw;
    } catch(const std::exception& error) {
        out.status=MccMapGameplayStatus::InvalidMetadata;out.reason=error.what();out.guards.clear();out.writes.clear();
    }
    return out;
}

MccMapGameplayPlan plan_mcc_material_gameplay_vanilla(const MccMapGameplaySource& source) {
    // Authenticated all43 MCC / 38 offline-original context inventory:
    // correction-guards-v1/source-guards.json SHA256
    // 5f6e542f27d69ec6ec60ee956dcde7e6eaccb145b889937a54671019bf53450e.
    // Selected native model-collision control SHA256 77673653a14c03dc18acfee4
    // 101e45e919733da74050b56ad79bd7833bdb0ce1 proves the two authored
    // material rows supply the 592/360 collision surfaces. No table reorder.
    constexpr std::string_view path="scenarios\\solo\\06b_floodzone\\06b_floodzone";
    constexpr std::string_view model_path="scenarios\\objects\\solo\\sentinelhq\\door_key_tunnel_rusted\\door_key_tunnel_rusted";
    constexpr auto scenario_hash=hex("4b5f0ae1ad6a32d68b5f55407e8602e57970e9e5436ca344c49837e18d8df2fc");
    constexpr auto globals_hash=hex("1379c7dd38f4d4e0dce043f84788161af254fee93a8bd0d73e7f70d52b368378");
    constexpr auto material_hash=hex("ed9a4e33c3e65541ba9ede26f72bc60dba6af96867ec0a5e8310332238751626");
    constexpr auto donor_hash=hex("262196e2d6cac5b5493604bf1b93c677a60fe96fd1e576d8fbb23a7ba4741ecc");
    constexpr auto target_hash=hex("c4d04d364566ba57d24348fd5a01343994a995abffab261bd304b46ad2355db3");
    MccMapGameplayPlan out;
    if(source.scenario_path!=path){out.reason="Scenario has no authenticated material profile";return out;}
    out.profile="tu15-06b_floodzone-material-gameplay";
    try {
        if(!source.read || !source.resolve || !source.find)
            reject(MccMapGameplayStatus::InvalidMetadata,"Checked material source callbacks are required");
        const auto resolve=[&](TagHandle handle) {
            const auto value=source.resolve(handle);
            if(!value)reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Required material owner is absent");
            if(handle.is_null() || value->handle!=handle)
                reject(MccMapGameplayStatus::InvalidMetadata,"Material resolver did not round-trip a full salted handle");
            return *value;
        };
        const auto find=[&](TagGroup group,std::string_view name) {
            const auto value=source.find(group,name);
            if(!value)reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Required unique material owner is absent");
            if(value->group!=group || value->name!=name || !same(*value,resolve(value->handle)))
                reject(MccMapGameplayStatus::InvalidMetadata,"Named material owner does not round-trip its full identity");
            return *value;
        };
        const auto scenario=resolve(source.scenario);
        expected(scenario,tag_groups::scenario,path,3782868996u,206512,853240);
        if(!same(scenario,find(tag_groups::scenario,path)))
            reject(MccMapGameplayStatus::InvalidMetadata,"Primary material scenario differs from its named identity");
        const auto scenario_bytes=checked(source.read,scenario.source_token,scenario.bytes);
        if(digest(scenario_bytes)!=scenario_hash)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Material scenario bank differs from authenticated authoring");
        const auto globals=resolve(source.globals);
        expected(globals,tag_groups::globals,"globals\\globals",3782541312u,143360,49548);
        if(!same(globals,find(tag_groups::globals,"globals\\globals")))
            reject(MccMapGameplayStatus::InvalidMetadata,"Primary material globals differs from its named identity");
        const auto globals_bytes=checked(source.read,globals.source_token,globals.bytes);
        if(digest(globals_bytes)!=globals_hash)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete material globals differs from authenticated authoring");
        if(word(globals_bytes,0x150)!=183 || word(globals_bytes,0x154)!=154464 ||
            digest(globals_bytes.subspan(166884-globals.source_token,0xb4))!=material_hash)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Global material69 binding differs from authenticated authoring");
        const auto model=find(tag_groups::model,model_path);
        expected(model,tag_groups::model,model_path,4231600833u,21440260,1600);
        const auto model_bytes=checked(source.read,model.source_token,model.bytes);
        const auto model_hash=digest(model_bytes);
        if(model_hash!=donor_hash && model_hash!=target_hash)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete material model differs from donor and target tuples");
        if(word(model_bytes,0x58)!=3 || word(model_bytes,0x5c)!=21440512)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Model material descriptor differs from authenticated authoring");
        out.guards={{scenario.source_token,{scenario_bytes.begin(),scenario_bytes.end()}},
            {globals.source_token,{globals_bytes.begin(),globals_bytes.end()}},
            {model.source_token,{model_bytes.begin(),model_bytes.end()}}};
        if(model_hash==donor_hash)for(const auto offset:{288u,308u}) {
            const auto before=model_bytes.subspan(offset,2);
            out.writes.push_back({add(model.source_token,offset),{before.begin(),before.end()},
                {std::byte{0x45},std::byte{0}},"Rusted door/Global Material Index"});
        }
        out.status=out.writes.empty()?MccMapGameplayStatus::AlreadyEquivalent:MccMapGameplayStatus::Ready;
        out.reason=out.writes.empty()?"Authenticated original-equivalent collision materials":
            "Authenticated original collision material restoration";
        preflight_mcc_map_gameplay_vanilla(out,source.read);
    } catch(const Rejected& error) {
        out.status=error.status;out.reason=error.reason;out.guards.clear();out.writes.clear();
    } catch(const std::bad_alloc&) {throw;
    } catch(const std::exception& error) {
        out.status=MccMapGameplayStatus::InvalidMetadata;out.reason=error.what();out.guards.clear();out.writes.clear();
    }
    return out;
}

MccMapGameplayPlan plan_mcc_foot_gameplay_vanilla(const MccMapGameplaySource& source,
    TagHandle selected_material_globals) {
    constexpr std::string_view foot_path="effects\\materials\\objects\\characters\\masterchief";
    MccMapGameplayPlan out;
    const auto profile=std::ranges::find(foot_profiles,source.scenario_path,&FootProfile::path);
    if(profile==foot_profiles.end()){out.reason="Scenario has no authenticated foot profile";return out;}
    out.profile=profile->label;
    try {
        if(!source.read || !source.resolve || !source.find)
            reject(MccMapGameplayStatus::InvalidMetadata,"Checked foot source callbacks are required");
        if(selected_material_globals.is_null())
            reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Native material globals is absent");
        if(selected_material_globals!=source.globals || source.globals.value!=profile->globals.handle ||
            source.scenario.value!=profile->scenario.handle)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Primary or native-selected foot context differs");
        const auto resolve=[&](TagHandle handle) {
            const auto value=source.resolve(handle);
            if(!value)reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Required foot owner is absent");
            if(handle.is_null() || value->handle!=handle)
                reject(MccMapGameplayStatus::InvalidMetadata,"Foot resolver did not round-trip a full salted handle");
            return *value;
        };
        const auto load=[&](const FootOwner& owner,TagGroup group,std::string_view name,bool hash=true) {
            const auto tag=resolve({owner.handle});
            expected(tag,group,name,owner.handle,owner.root,owner.bytes);
            const auto named=source.find(group,name);
            if(!named)reject(MccMapGameplayStatus::RequiredDependencyUnavailable,"Required unique foot owner is absent");
            if(named->group!=group || named->name!=name || !same(*named,resolve(named->handle)))
                reject(MccMapGameplayStatus::InvalidMetadata,"Named foot owner did not round-trip its identity");
            if(!same(tag,*named))
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Named foot owner differs from its authenticated identity");
            const auto bytes=checked(source.read,tag.source_token,tag.bytes);
            if(hash && digest(bytes)!=owner.hash)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete supporting foot owner differs from authenticated authoring");
            return std::pair{tag,bytes};
        };
        const auto view=[](const MccMapGameplayIdentity& owner,Span bytes,std::uint32_t token,std::size_t count) {
            if(token<owner.source_token || std::uint64_t(token)-owner.source_token>bytes.size() ||
                count>bytes.size()-(std::uint64_t(token)-owner.source_token))
                reject(MccMapGameplayStatus::InvalidMetadata,"Foot descriptor escapes its authenticated owner");
            return bytes.subspan(token-owner.source_token,count);
        };
        const auto [scenario,scenario_bytes]=load(profile->scenario,tag_groups::scenario,profile->path);
        if((word(scenario_bytes,0x10)&0xffffu)!=profile->scenario_type)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot scenario type differs");
        if(profile->scenario_type==1) {
            const auto level=word(scenario_bytes,0x39c);
            const auto descriptor=view(scenario,scenario_bytes,add(level,0x10),8);
            if(word(scenario_bytes,0x398)!=1 || word(descriptor,0)!=1 ||
                word(view(scenario,scenario_bytes,word(descriptor,4),4),0)!=profile->map_id)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot scenario map ID differs");
        }
        const auto [globals,globals_bytes]=load(profile->globals,tag_groups::globals,"globals\\globals");
        if(word(globals_bytes,0x150)!=profile->materials || word(globals_bytes,0x154)!=profile->material_token)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot global-material descriptor differs");
        const auto& definition=*profile->definition;
        const auto [foot,foot_bytes]=load(definition.owner,TagGroup{"foot"},foot_path,false);
        const auto foot_hash=digest(foot_bytes);
        enum class State {Donor,Seed,Union};
        const auto state=foot_hash==definition.target?State::Union:
            foot_hash==definition.seed?State::Seed:State::Donor;
        if(state==State::Donor && foot_hash!=definition.owner.hash)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Complete Chief foot owner differs from donor, seed and union");
        if(word(foot_bytes,0)!=definition.actions || !definition.actions || word(foot_bytes,4)!=definition.action_token)
            reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot action descriptor differs");
        std::vector<bool> needed(definition.supports.size());
        for(const auto& expected_row:definition.rows) {
            if(expected_row.action>=definition.actions || expected_row.material_key>=profile->materials ||
                expected_row.material_key>=32768 || expected_row.primary>=definition.supports.size() ||
                expected_row.secondary>=definition.supports.size())
                reject(MccMapGameplayStatus::InvalidMetadata,"Authenticated foot row profile is inconsistent");
            const auto material_token=add(profile->material_token,std::uint64_t(expected_row.material_key)*0xb4);
            if(material_token!=expected_row.material_token ||
                digest(view(globals,globals_bytes,material_token,0xb4))!=expected_row.material)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot global-material row binding differs");
            const auto action_token=add(definition.action_token,std::uint64_t(expected_row.action)*24);
            const auto action=view(foot,foot_bytes,action_token,24);
            if(action_token!=expected_row.action_token || digest(action)!=expected_row.descriptor ||
                word(action,8)!=expected_row.sounds || !expected_row.sounds || word(action,12)!=expected_row.sound_token)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot authored sound descriptor differs");
            const auto sounds=view(foot,foot_bytes,expected_row.sound_token,std::size_t(expected_row.sounds)*24);
            std::size_t selected=expected_row.sounds;
            for(std::size_t n=0;n<expected_row.sounds;++n) {
                auto key=std::int32_t(word(sounds,n*24+0x14)&0xffffu);
                if(key>=32768)key-=65536;
                if(key==std::int32_t(expected_row.material_key)){selected=n;break;}
            }
            if(selected!=expected_row.row_ordinal || selected>=expected_row.sounds ||
                add(expected_row.sound_token,selected*24)!=expected_row.row_token)
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot first matching signed material key differs");
            const auto row=sounds.subspan(selected*24,24);
            const bool target=state==State::Union || (state==State::Seed && expected_row.seed);
            const auto& primary=definition.supports[expected_row.primary].owner;
            const auto& secondary=definition.supports[expected_row.secondary].owner;
            if(digest(row)!=(target?expected_row.row_target:expected_row.row_donor) ||
                word(row,0)!=tag_groups::sound.value || word(row,4)!=primary.handle ||
                word(row,8)!=(target?0xffffffffu:tag_groups::sound.value) ||
                word(row,12)!=(target?0xffffffffu:secondary.handle))
                reject(MccMapGameplayStatus::AuthoredTupleUnrecognized,"Foot selected row or sound references differ");
            needed[expected_row.primary]=true;
            if(!target) {
                // A seed-null row does not remove this dependency while any
                // other admitted row still references the same sound owner.
                needed[expected_row.secondary]=true;
                out.writes.push_back(write(add(expected_row.row_token,8),row.subspan(8,8),0xffffffff,
                    "Chief foot/Authenticated Secondary Sound"));
            }
        }
        std::vector<std::pair<MccMapGameplayIdentity,Span>> supports;
        for(std::size_t n=0;n<needed.size();++n)if(needed[n]) {
            const auto& support=definition.supports[n];
            supports.push_back(load(support.owner,tag_groups::sound,support.name));
        }
        out.guards={{scenario.source_token,{scenario_bytes.begin(),scenario_bytes.end()}},
            {globals.source_token,{globals_bytes.begin(),globals_bytes.end()}},
            {foot.source_token,{foot_bytes.begin(),foot_bytes.end()}}};
        for(const auto& [support,bytes]:supports)
            out.guards.push_back({support.source_token,{bytes.begin(),bytes.end()}});
        if(state!=State::Union && out.writes.empty()) {
            reject(MccMapGameplayStatus::InvalidMetadata,"Authenticated foot transition has no remaining fields");
        }
        out.status=state==State::Union?MccMapGameplayStatus::AlreadyEquivalent:MccMapGameplayStatus::Ready;
        out.reason=state==State::Union?"Authenticated original-equivalent Chief secondary references":
            "Authenticated finite Chief secondary-reference restoration";
        preflight_mcc_map_gameplay_vanilla(out,source.read);
    } catch(const Rejected& error) {
        out.status=error.status;out.reason=error.reason;out.guards.clear();out.writes.clear();
    } catch(const std::bad_alloc&) {throw;
    } catch(const std::exception& error) {
        out.status=MccMapGameplayStatus::InvalidMetadata;out.reason=error.what();out.guards.clear();out.writes.clear();
    }
    return out;
}

void preflight_mcc_map_gameplay_vanilla(const MccMapGameplayPlan& plan,
    const std::function<Span(std::uint32_t,std::size_t)>& read) {
    const auto require=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
    require(bool(read),"Map gameplay preflight requires a checked reader");
    require(plan.status==MccMapGameplayStatus::Ready || plan.status==MccMapGameplayStatus::AlreadyEquivalent,
        "Map gameplay plan is not qualified for publication");
    require(!plan.guards.empty(),"Map gameplay plan has no authenticated guards");
    for(std::size_t n=0;n<plan.guards.size();++n) {
        const auto& guard=plan.guards[n];
        require(!guard.bytes.empty(),"Map gameplay guard is empty");
        const auto observed=checked(read,guard.source_token,guard.bytes.size());
        require(std::ranges::equal(observed,guard.bytes),"Map gameplay source changed before publication");
        for(std::size_t m=0;m<n;++m) {
            const auto& other=plan.guards[m];
            require(std::uint64_t(guard.source_token)+guard.bytes.size()<=other.source_token ||
                std::uint64_t(other.source_token)+other.bytes.size()<=guard.source_token,"Map gameplay guard owners overlap");
        }
    }
    for(std::size_t n=0;n<plan.writes.size();++n) {
        const auto& change=plan.writes[n];
        require(!change.before.empty() && change.before.size()==change.after.size(),"Map gameplay write has invalid width");
        const auto end=std::uint64_t(change.source_token)+change.before.size();
        require(end<=0x100000000ull,"Map gameplay write source token overflows");
        const auto owner=std::ranges::find_if(plan.guards,[&](const auto& guard){
            return change.source_token>=guard.source_token && end<=std::uint64_t(guard.source_token)+guard.bytes.size();});
        require(owner!=plan.guards.end(),"Map gameplay write escapes authenticated owners");
        const auto expected=Span(owner->bytes).subspan(change.source_token-owner->source_token,change.before.size());
        require(std::ranges::equal(expected,change.before),"Map gameplay write differs from its captured owner");
        for(std::size_t m=0;m<n;++m) {
            const auto& other=plan.writes[m];
            require(end<=other.source_token || std::uint64_t(other.source_token)+other.before.size()<=change.source_token,
                "Map gameplay writes overlap");
        }
    }
}

void preflight_mcc_map_gameplay_vanilla(std::span<const MccMapGameplayPlan> plans,
    const std::function<Span(std::uint32_t,std::size_t)>& read) {
    for(std::size_t n=0;n<plans.size();++n) {
        const auto& plan=plans[n];
        if(plan.status==MccMapGameplayStatus::InvalidMetadata)
            throw std::runtime_error("Map gameplay component has invalid metadata: "+plan.reason);
        if(plan.status!=MccMapGameplayStatus::Ready && plan.status!=MccMapGameplayStatus::AlreadyEquivalent) {
            if(!plan.guards.empty() || !plan.writes.empty())
                throw std::runtime_error("Unqualified map gameplay component contains publication data");
            continue;
        }
        if((plan.status==MccMapGameplayStatus::AlreadyEquivalent)!=plan.writes.empty())
            throw std::runtime_error("Map gameplay component status differs from its writes");
        preflight_mcc_map_gameplay_vanilla(plan,read);
        for(const auto& change:plan.writes)for(std::size_t m=0;m<n;++m)
            for(const auto& other:plans[m].writes)
                if(std::uint64_t(change.source_token)+change.before.size()>other.source_token &&
                    std::uint64_t(other.source_token)+other.before.size()>change.source_token)
                    throw std::runtime_error("Map gameplay component writes overlap");
    }
}
} // namespace halo2
