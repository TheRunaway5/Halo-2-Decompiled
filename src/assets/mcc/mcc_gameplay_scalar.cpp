#include "halo2/mcc_gameplay_scalar.hpp"
#include <algorithm>

namespace halo2 {
namespace {
struct Field {std::uint32_t offset,donor,target;std::uint8_t bytes;};
struct Profile {
    TagGroup group;
    std::string_view name,label;
    std::array<Field,3> fields;
    std::uint8_t count;
};
// Offline authority: authenticated English TU1.5 XBE SHA256
//21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd.
// Original disc Lockout SHA25614c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9
// and each original campaign/DLC map are checked against their owned import
// manifests before the offset-aware reference audit. The TU1.5 multiplayer
// postprocessors165B30/11F060 do not modify any field selected below.
//
// D18C0 resolves the equipped weapon through the vanilla directory and reads
// its exact binary32 pyramid angles at D1A26/D1A4A. No degree conversion or
// approximation is appropriate: each observed donor differs by one ULP.
// Gauss proj+128 is its reporting cause; original19B8F0 masks six bits and
// dispatches only0..40. MCC's42 would become unknown instead of Xbox's36.
// Flak's complete authored movement tuple is preserved in six original
// campaign maps (first04a_gasgiant SHA256
//9cb7a3da993bf1314dd9b61874ea2c18636b8114f56253263d35e729a170118e).
// This restores those raw reference fields; it does not claim a measured
// equipped-flak movement trace or establish other flak behavior.
constexpr std::array profiles{
    Profile{tag_groups::weapon,"objects\\weapons\\melee\\energy_blade\\energy_blade",
        "tu15-energy-blade-melee-pyramid",{{{0x1b0,0x3ed6dfe4,0x3ed6dfe3,4},
            {0x1b4,0x3e76f125,0x3e76f125,4},{}}},2},
    Profile{tag_groups::weapon,"objects\\weapons\\melee\\energy_blade\\energy_blade_useless",
        "tu15-energy-blade-useless-melee-pyramid",{{{0x1b0,0x3eb30fe9,0x3eb30fe9,4},
            {0x1b4,0x3e4dc8f4,0x3e4dc8f5,4},{}}},2},
    Profile{tag_groups::weapon,"objects\\weapons\\support_high\\flak_cannon\\flak_cannon",
        "tu15-flak-cannon-authored-movement",{{{0x22c,0,2,2},
            {0x230,0,0x3e800000,4},{0x234,0,0x3e800000,4}}},3},
    Profile{tag_groups::projectile,"objects\\vehicles\\warthog\\turrets\\gauss\\weapon\\gauss_bullet",
        "tu15-gauss-projectile-reporting-cause",{{{0x128,0x2a,0x24,1},{},{}}},1},
    // These four whole flag words are stable authored vanilla values across
    // all 88 matching reference observations. Restore their exact data; no
    // behavior or meaning is inferred for the differing bits 7 and 14.
    Profile{tag_groups::weapon,"objects\\weapons\\multiplayer\\ball\\ball",
        "tu15-ball-authored-flags",{{{0x12c,0x00340058,0x003400d8,4},{},{}}},1},
    Profile{tag_groups::weapon,"objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle",
        "tu15-sniper-rifle-authored-flags",{{{0x12c,0x00000020,0x00004020,4},{},{}}},1},
    Profile{tag_groups::weapon,"objects\\weapons\\rifle\\beam_rifle\\beam_rifle",
        "tu15-beam-rifle-authored-flags",{{{0x12c,0x00000820,0x00004820,4},{},{}}},1},
    Profile{tag_groups::weapon,"objects\\weapons\\rifle\\beam_rifle\\beam_rifle_noplayer",
        "tu15-beam-rifle-noplayer-authored-flags",{{{0x12c,0x20000820,0x20004820,4},{},{}}},1}
};
// MCC18090AC30 uses its resolved jpt root at18090AEFC/18090AF05.
//18090B318's added +0C bit2 gates direct area delivery18090EB80.
// The original TU1.5D9A30/D9ED8/D9FD0 branch has only the game-mode gate,
// then callsDA0D0 regardless of that added flag. Their existing falloff bit0
// remains unchanged. The compiled table restores only153 explicitly matched
// retail flag words; it is never a blanket clear on arbitrary damage tags.
// Full source/input hashes and named-map observations accompany the offline
// manifest. MCC's .pdata chained unwind fragments must be combined when
// inspecting these function bodies; a prologue fragment is not the body.
#include "assets/mcc/mcc_gameplay_scalar_damage_profiles.inc"
// Actual TU1.5 17AAB0 selects [4F3CD8] or +4 using effe bit2. These
// independently inventoried named destruction effects differ in that bit
// between original and MCC data, in both directions. Restore exact authored
// words before the original per-tick and presentation event consumers run.
// Sound/particle/event-part conversion remains a separate owner.
#include "assets/mcc/mcc_destruction_effect_scalar_profiles.inc"
}
std::optional<MccGameplayScalarReceipt> convert_mcc_gameplay_scalar(
    TagGroup group,std::string_view name,std::span<std::byte> definition) {
    const Profile* profile=nullptr;
    for(const auto candidates:{std::span<const Profile>(profiles),std::span<const Profile>(damage_profiles),
        std::span<const Profile>(destruction_effect_profiles)}) {
        const auto found=std::ranges::find_if(candidates,[&](const auto& value){
            return value.group==group && value.name==name;
        });
        if(found!=candidates.end()){profile=&*found;break;}
    }
    if(!profile)return {};
    MccGameplayScalarReceipt receipt;
    receipt.profile=profile->label;receipt.field_count=profile->count;
    bool donor=true,target=true;
    const Bytes data(definition);
    for(unsigned n=0;n<profile->count;++n) {
        const auto& field=profile->fields[n];data.require(field.offset,field.bytes);
        std::uint32_t value{};
        for(unsigned i=0;i<field.bytes;++i)
            value|=std::to_integer<std::uint32_t>(definition[field.offset+i])<<(i*8);
        receipt.fields[n]={field.offset,value,value,field.bytes};
        donor&=value==field.donor;target&=value==field.target;
    }
    if(target){receipt.status=MccGameplayScalarStatus::AlreadyEquivalent;return receipt;}
    if(!donor){receipt.status=MccGameplayScalarStatus::AuthoredTupleUnrecognized;return receipt;}
    receipt.status=MccGameplayScalarStatus::Converted;
    for(unsigned n=0;n<profile->count;++n) {
        const auto& field=profile->fields[n];receipt.fields[n].after=field.target;
        if(field.donor==field.target)continue;
        for(unsigned i=0;i<field.bytes;++i)definition[field.offset+i]=std::byte(field.target>>(i*8));
    }
    return receipt;
}
} // namespace halo2
