#include "halo2/mcc_sound_timing_reference.hpp"
#include "audio/mcc_sound_geometry_apply.hpp"
#include "audio/mcc_sound_timing_reference_profile.hpp"
#include "audio/mcc_sound_vanilla_reference_profile.hpp"
#include "audio/mcc_sound_stored_time_apply.hpp"
#include "audio/mcc_sound_stored_time_reference_profile.hpp"
#include "audio/mcc_sound_class_qualified.hpp"
#include "audio/mcc_sound_permutation_time_reference_profile.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/physics_contact_record.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <mutex>
#include <openssl/evp.h>
#include <set>
#include <tuple>
#include <thread>

namespace halo2 {
namespace {
using Digest=MccSoundVanillaDigest;
using mcc_sound_timing_detail::TimingOwner;
thread_local bool executing_timing{};
constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
constexpr std::array<unsigned,11> offsets{8,16,24,32,40,48,56,88,96,104,112};
class Hash {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
    Hash(){if(!ctx||EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)!=1)throw FormatError("Timing digest initialization failed");}
    void bytes(std::span<const std::byte> s){if(!s.empty()&&EVP_DigestUpdate(ctx.get(),s.data(),s.size())!=1)throw FormatError("Timing digest failed");}
    void word(std::uint32_t x){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(x>>(8*i));bytes(b);}
    void wide(std::uint64_t x){word(std::uint32_t(x));word(std::uint32_t(x>>32));}
    void text(std::string_view s){if(s.size()>0x100000)throw FormatError("Timing identity string is excessive");word(std::uint32_t(s.size()));bytes(std::as_bytes(std::span(s.data(),s.size())));}
    Digest finish(){Digest d;unsigned n=0;if(EVP_DigestFinal_ex(ctx.get(),reinterpret_cast<unsigned char*>(d.data()),&n)!=1||n!=32)throw FormatError("Timing digest finalization failed");return d;}
};
Digest digest(std::span<const std::byte> s){Hash h;h.bytes(s);return h.finish();}
void require(bool value,const char*why){if(!value)throw FormatError(why);}
std::uint32_t add(std::uint32_t a,std::uint64_t b){require(b<=0xffffffffull-a,"Timing native address wraps");return a+std::uint32_t(b);}
int signed16(unsigned x){return std::bit_cast<std::int16_t>(std::uint16_t(x));}
void word(std::vector<std::byte>& data,unsigned at,unsigned value,unsigned width=4){Bytes(data).require(at,width);for(unsigned i=0;i<width;++i)data[at+i]=std::byte(value>>(i*8));}
struct Reader {
    const MccSoundVanillaSource& source;
    std::map<std::uint32_t,const Tag*> tags;
    explicit Reader(const MccSoundVanillaSource&s):source(s){
        require(s.metadata.read&&s.metadata.relocate&&s.metadata.gestalt_owner&&s.string_id,"Timing needs complete typed source owners");
        require(s.metadata.tags.size()<=65536,"Timing source directory is excessive");
        for(std::size_t i=0;i<s.metadata.tags.size();++i){const auto&t=s.metadata.tags[i];if(t.handle().is_null())continue;require(t.handle().index()==i&&tags.emplace(t.datum,&t).second,"Timing source namespace has a foreign or duplicate salt/index");}
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const{require(at&&at!=0xffffffffu&&n<=128u*1024*1024&&std::uint64_t(at)+n<=0x100000000ull,"Timing source extent is invalid");const auto s=source.metadata.read(at,n);require(s.size()==n,"Timing source extent is partial");return s;}
    const Tag& tag(TagHandle h,std::string_view group)const{const auto i=tags.find(h.value);require(!h.is_null()&&i!=tags.end()&&i->second->address&&i->second->group==group,"Timing lost a full-salt typed tag owner");return *i->second;}
    TagBlock block(std::span<const std::byte>b,unsigned at,unsigned stride)const{const Bytes x(b);const auto n=x.u32(at),p=x.u32(at+4);require(n<=0x100000&&std::uint64_t(n)*stride<=128u*1024*1024,"Timing block is excessive");if(n)(void)read(p,std::size_t(n)*stride);return {std::int32_t(n),p};}
    std::span<const std::byte> row(const TagBlock&b,unsigned n,unsigned stride)const{require(n<std::uint32_t(b.count),"Timing index escapes its owning bank");return read(add(b.address,std::uint64_t(n)*stride),stride);}
    std::pair<unsigned,std::string> name(const TagBlock&b,unsigned index)const{require(signed16(index)>=0,"Timing import-name index is negative");const auto sid=Bytes(row(b,index,4)).u32(0);auto text=source.string_id(sid);require(!text.empty(),"Timing import name is absent");return {sid,std::move(text)};}
};
Digest plan_digest(const MccSoundRuntimePlan&p){
    Hash h;h.word(p.address);h.wide(p.bytes.size());h.bytes(p.bytes);
    for(const auto&b:p.blocks){h.word(std::uint32_t(b.count));h.word(b.address);}
    h.wide(p.patches.size());for(const auto&x:p.patches){h.word(x.owner.value);h.word(x.source_address);h.wide(x.before.size());h.bytes(x.before);h.wide(x.after.size());h.bytes(x.after);}
    h.wide(p.gestalts.size());for(const auto&g:p.gestalts){h.word(g.owner.value);for(auto x:g.first)h.word(x);for(auto x:g.count)h.word(x);}
    h.wide(p.chunks.size());for(const auto&c:p.chunks){h.word(c.gestalt.value);h.word(c.source_address);h.word(c.native_address);h.word(c.source_index);h.word(c.file_offset);h.word(c.encoded_bytes);h.word(c.decoded_bytes);h.word(c.reset_after);}
    h.wide(p.sounds.size());for(const auto&s:p.sounds){h.word(s.sound.value);h.word(s.gestalt.value);h.word(s.codec.sample_rate);h.word(s.codec.encoding);h.word(s.codec.compression);h.word(unsigned(s.file));h.word(s.remastered_index);h.bytes(s.mcc_extension);h.word(s.source_root);h.word(s.native_root);h.word(s.source_flags);h.word(s.native_flags);}
    h.wide(p.unused_flag_indices.size());for(const auto&u:p.unused_flag_indices){h.word(u.gestalt.value);h.word(u.range_index);h.word(u.encoded_index);h.wide(u.users.size());for(const auto&x:u.users){h.word(x.sound.value);h.word(x.mcc_class.value);h.word(x.native_class.value);h.word(x.class_index);h.word(x.mcc_flags);h.word(x.native_flags);}}
    h.wide(p.native_pcm_duration.size());for(auto x:p.native_pcm_duration)h.word(x.value);
    h.word(bool(p.authored_flags));if(p.authored_flags){h.wide(p.authored_flags->corrections.size());for(const auto&c:p.authored_flags->corrections){h.word(c.sound.value);h.word(c.gestalt.value);h.word(c.source_root);h.word(c.source_flags);h.word(c.native_before);h.word(c.native_after);}h.wide(p.authored_flags->owners.size());for(const auto&o:p.authored_flags->owners){h.word(o.gestalt.value);h.word(unsigned(o.status));}}
    return h.finish();
}
// Reuse the exact immutable V5 flag contract without changing its exports.
#include "audio/mcc_sound_vanilla_reference_profiles.inc"
namespace permutation_policy {
using Owner=mcc_sound_permutation_time_detail::Owner;
#include "audio/mcc_sound_permutation_time_reference_profiles.inc"
static mcc_sound_permutation_time_detail::Policy canonical(){
    return {permutation_time_owners,std::as_bytes(std::span(permutation_time_records,sizeof(permutation_time_records)-1))};
}
}
struct KeyRecord {Digest key,payload;MccSoundOriginalTimingOperands original;std::array<std::int16_t,5> pitch;};
struct Projection {
    MccSoundRuntimePlan native;
    std::vector<MccSoundTimingStoredReference> stored;
    std::vector<MccSoundTimingPermutationReference> permutations;
};
std::vector<MccSoundTimingStoredReference> stored_catalog(
    const MccSoundVanillaSource&s,const MccSoundRuntimePlan&native,
    const MccSoundStoredTimePlan&proof){
    std::vector<MccSoundTimingStoredReference> out;
    std::set<unsigned> sounds,roots;
    for(const auto&result:proof.sounds()){
        if(result.status!=MccSoundStoredTimeStatus::Authenticated&&
           result.status!=MccSoundStoredTimeStatus::AlreadyEquivalent)continue;
        const auto owner=std::ranges::find(proof.owners(),result.gestalt,&MccSoundVanillaOwnerResult::gestalt);
        require(owner!=proof.owners().end()&&owner->status==MccSoundVanillaOwnerStatus::Authenticated,
            "Stored-time catalog lost its complete authenticated owner");
        require(result.sound.index()<s.metadata.tags.size(),"Stored-time catalog lost its sound index");
        const auto&t=s.metadata.tags[result.sound.index()];
        const auto def=std::ranges::find(native.sounds,result.sound,&MccSoundRuntimeDefinition::sound);
        require(t.handle()==result.sound&&t.group=="snd!"&&
            def!=native.sounds.end()&&def->gestalt==result.gestalt&&def->source_root==t.address,
            "Stored-time catalog full-salt source/native owner differs");
        const auto patch=std::ranges::find(native.patches,t.address,&MccSoundRuntimePatch::source_address);
        require(patch!=native.patches.end()&&patch->owner==result.sound&&patch->before.size()==20&&patch->after.size()==20,
            "Stored-time catalog lacks a complete native header");
        const auto actual_ms=std::bit_cast<std::int32_t>(Bytes(patch->after).u32(16));
        const auto correction=std::ranges::find(proof.corrections(),result.sound,&MccSoundStoredTimeCorrection::sound);
        if(result.status==MccSoundStoredTimeStatus::Authenticated){
            require(correction!=proof.corrections().end()&&correction->gestalt==result.gestalt&&
                correction->source_root==t.address&&correction->native_root==def->native_root,
                "Stored-time catalog lost its original word receipt");
            // A generic Source baseline remains valid metadata, but its
            // unconverted stored words never become original-time authority.
            if(actual_ms!=correction->original_ms)continue;
        }else require(correction==proof.corrections().end()&&
            actual_ms==std::bit_cast<std::int32_t>(Bytes(patch->before).u32(16)),
            "AlreadyEquivalent stored-time catalog word differs");
        require(sounds.insert(result.sound.value).second&&roots.insert(def->native_root).second,
            "Stored-time catalog repeats a sound/native root");
        MccSoundTimingStoredReference reference{result.sound,result.gestalt,t.address,def->native_root,actual_ms,{}};
        std::ranges::copy(patch->after,reference.expected_native_header.begin());
        out.push_back(reference);
    }
    return out;
}
std::vector<MccSoundTimingPermutationReference> permutation_catalog(
    const MccSoundVanillaSource&s,const MccSoundRuntimePlan&native,
    const MccSoundPermutationTimePlan&proof,std::span<const KeyRecord> refs,
    std::span<const std::byte> permutation_records){
    require(!permutation_records.empty()&&permutation_records.size()%68==0&&permutation_records.size()/68<=100000,
        "Permutation catalog policy extent differs");
    struct Identity {Digest key,payload;std::uint16_t source,original;};
    std::vector<Identity> identities;
    for(std::size_t at=0;at<permutation_records.size();at+=68){Identity identity;
        std::ranges::copy(permutation_records.subspan(at,32),identity.key.begin());
        std::ranges::copy(permutation_records.subspan(at+32,32),identity.payload.begin());
        identity.source=Bytes(permutation_records).u16(at+64);identity.original=Bytes(permutation_records).u16(at+66);
        require(identities.empty()||identities.back().key<identity.key,"Permutation catalog policy is duplicated/unsorted");
        identities.push_back(identity);
    }
    Reader r(s);std::vector<MccSoundTimingPermutationReference> out;
    std::set<std::tuple<unsigned,unsigned,unsigned>> unique;
    for(const auto&result:proof.permutations()){
        if(result.status!=MccSoundPermutationTimeStatus::Authenticated&&
           result.status!=MccSoundPermutationTimeStatus::AlreadyEquivalent)continue;
        const auto owner=std::ranges::find(proof.owners(),result.gestalt,&MccSoundVanillaOwnerResult::gestalt);
        require(owner!=proof.owners().end()&&owner->status==MccSoundVanillaOwnerStatus::Authenticated,
            "Permutation catalog lost its complete authenticated owner");
        const auto&t=r.tag(result.sound,"snd!");const auto&g=r.tag(result.gestalt,"ugh!");
        const auto snd=r.read(t.address,28),root=r.read(g.address,120);
        const auto ranges=r.block(root,40,12),perms=r.block(root,48,16),names=r.block(root,24,4);
        const int first=signed16(Bytes(snd).u16(8));const auto count=std::to_integer<unsigned>(snd[10]);
        require(first>=0&&count<128&&result.range<count&&std::uint64_t(first)+count<=std::uint32_t(ranges.count),
            "Permutation catalog pitch interval differs");
        const auto ri=unsigned(first)+result.range;const auto range=r.row(ranges,ri,12);
        const int pf=signed16(Bytes(range).u16(8)),pc=signed16(Bytes(range).u16(10));
        require(pf>=0&&pc>=0&&result.permutation<unsigned(pc)&&std::uint64_t(pf)+pc<=std::uint32_t(perms.count)&&
            unsigned(pf)+result.permutation==result.source_permutation,"Permutation catalog source interval differs");
        const auto permutation=r.row(perms,result.source_permutation,16);
        const auto key=mcc_sound_timing_detail::named_key(t.name,r.name(names,Bytes(range).u16(0)).second,
            r.name(names,Bytes(permutation).u16(0)).second);
        const auto ref=std::ranges::lower_bound(refs,key,{},&KeyRecord::key);
        require(ref!=refs.end()&&ref->key==key,"Permutation catalog lacks the proven named identity");
        const auto identity=std::ranges::lower_bound(identities,key,{},&Identity::key);
        require(identity!=identities.end()&&identity->key==key&&identity->payload==ref->payload&&
            identity->source==Bytes(permutation).u16(6),"Permutation catalog policy identity differs from completed proof");
        const auto definition=std::ranges::find(native.sounds,result.sound,&MccSoundRuntimeDefinition::sound);
        const auto layout=std::ranges::find(native.gestalts,result.gestalt,&MccSoundRuntimeGestalt::owner);
        require(definition!=native.sounds.end()&&definition->gestalt==result.gestalt&&definition->source_root==t.address&&
            layout!=native.gestalts.end()&&ri<layout->count[4]&&result.source_permutation<layout->count[5],
            "Permutation catalog source/native owner differs");
        const auto nr=add(native.blocks[4].address,std::uint64_t(layout->first[4]+ri)*12);
        const auto np=add(native.blocks[5].address,std::uint64_t(layout->first[5]+result.source_permutation)*16);
        const auto native_range=Bytes(native.bytes).slice(nr-native.address,12);
        const auto native_permutation=Bytes(native.bytes).slice(np-native.address,16);
        const auto patch=std::ranges::find(native.patches,t.address,&MccSoundRuntimePatch::source_address);
        require(patch!=native.patches.end()&&patch->owner==result.sound&&patch->before.size()==20&&patch->after.size()==20&&
            signed16(Bytes(patch->after).u16(8))+result.range==layout->first[4]+ri&&
            signed16(Bytes(native_range).u16(8))+result.permutation==layout->first[5]+result.source_permutation&&
            result.permutation<Bytes(native_range).u16(10),"Permutation catalog merged rows differ");
        const auto correction=std::ranges::find_if(proof.corrections(),[&](const auto&c){return c.gestalt==result.gestalt&&c.source_index==result.source_permutation;});
        const auto original=correction==proof.corrections().end()?Bytes(permutation).u16(6):correction->original_ms;
        require(correction!=proof.corrections().end()||result.status==MccSoundPermutationTimeStatus::AlreadyEquivalent,
            "Permutation catalog lost its original U16 receipt");
        require(original==identity->original,"Permutation catalog original U16 differs from completed policy");
        // Generic B/BS remains a valid metadata plan, but only its independently
        // proven already-original rows may authorize the selected-time branch.
        if(Bytes(native_permutation).u16(6)!=original)continue;
        require(unique.emplace(result.sound.value,result.range,result.permutation).second,
            "Permutation catalog repeats a named sound selector");
        MccSoundTimingPermutationReference row{result.sound,result.gestalt,t.address,definition->native_root,
            ri,result.source_permutation,nr,np,result.range,result.permutation,std::uint16_t(original),key,ref->payload,{},{},{}};
        std::ranges::copy(patch->after,row.expected_native_header.begin());
        std::ranges::copy(native_range,row.expected_native_range.begin());
        std::ranges::copy(native_permutation,row.expected_native_permutation.begin());out.push_back(row);
    }
    return out;
}
Projection expected_projection(const MccSoundVanillaSource&source,const MccSoundVanillaSource&s,const MccSoundRuntimePlan&actual,bool require_stored,
    std::span<const KeyRecord> refs,const mcc_sound_permutation_time_detail::Policy* fixture_policy,bool classes,std::span<const MccSoundGeometryResource> geometry){
    const auto p_policy=fixture_policy?*fixture_policy:permutation_policy::canonical();
    const auto permutation_proof=[&](const MccSoundRuntimePlan&base){
        return classes?mcc_sound_permutation_time_detail::build_class_qualified(source,s,base,p_policy):
            mcc_sound_permutation_time_detail::build(s,s,base,p_policy);
    };
    const auto stored_proof=[&](const MccSoundRuntimePlan&base){return classes?
        mcc_sound_stored_time_detail::plan_class_qualified(source,s,base):plan_mcc_sound_stored_times(s,base);};
    auto expected=plan_mcc_sound_runtime_delivery(s.metadata,actual.address);
    const auto flags=mcc_sound_reference_detail::plan(s,raw_globals_sound_flag_profiles());
    require(actual.authored_flags||flags.corrections.empty(),"Timing native plan omitted required vanilla sound corrections");
    if(actual.authored_flags){expected.authored_flags=std::make_shared<const MccSoundVanillaFlagPlan>(flags);for(const auto&c:flags.corrections){
        auto i=std::ranges::find(expected.sounds,c.sound,&MccSoundRuntimeDefinition::sound);require(i!=expected.sounds.end()&&i->native_flags==c.native_before,"Timing vanilla flag projection lost a target");i->native_flags=c.native_after;
        auto p=std::ranges::find(expected.patches,c.source_root,&MccSoundRuntimePatch::source_address);require(p!=expected.patches.end()&&p->owner==c.sound&&Bytes(p->after).u16(0)==c.native_before,"Timing vanilla flag root preimage differs");word(p->after,0,c.native_after,2);
    }}
    if(classes)expected=mcc_sound_class_detail::stage_qualified(source.metadata,expected);
    if(!require_stored && !geometry.empty())mcc_sound_geometry_detail::apply(expected,geometry);
    const auto baseline=plan_digest(expected);
    const auto supplied=plan_digest(actual);
    // The generic Source API retains its original baseline contract, including
    // its later per-owner resource diagnostics. Cache callers require the
    // complete authenticated stored-time result instead.
    if(!require_stored&&baseline==supplied){
        // Preserve generic baseline diagnostics when a complete stored-time
        // proof is unavailable. Only a successful independent full proof may
        // retain the subset whose native words are already original.
        std::vector<MccSoundTimingStoredReference> admitted;
        try{const auto proof=stored_proof(expected);admitted=stored_catalog(s,expected,proof);}
        catch(const std::exception&){}
        std::vector<MccSoundTimingPermutationReference> permutations;
        try{const auto proof=permutation_proof(expected);permutations=permutation_catalog(s,expected,proof,refs,p_policy.records);}
        catch(const std::exception&){}
        return {std::move(expected),std::move(admitted),std::move(permutations)};
    }
    // Preserve the earlier generic Source fully-restored stored-root
    // contract. It carries no permission for the selected U16 time branch.
    if(!require_stored){
        auto legacy=expected;const auto proof=stored_proof(expected);
        mcc_sound_stored_time_detail::apply(legacy,classes?source.metadata:s.metadata,proof);
        if(plan_digest(legacy)==supplied){
            std::vector<MccSoundTimingPermutationReference> permutations;
            try{const auto p=permutation_proof(expected);permutations=permutation_catalog(s,legacy,p,refs,p_policy.records);}
            catch(const std::exception&){}
            return {std::move(legacy),stored_catalog(s,actual,proof),std::move(permutations)};
        }
    }
    // Cache publication requires the complete ordered conversion. Rebuild
    // both native child-bank U16 values and stored root values together;
    // whole-bank equality rejects any base/mixed/foreign projection.
    const auto permutation_times=permutation_proof(expected);
    mcc_sound_permutation_time_detail::apply(expected,classes?source.metadata:s.metadata,permutation_times);
    auto restored=expected;const auto stored=stored_proof(expected);
    mcc_sound_stored_time_detail::apply(restored,classes?source.metadata:s.metadata,stored);
    mcc_sound_geometry_detail::apply(restored,geometry);
    require(plan_digest(restored)==supplied,
        "Timing supplied metadata differs from complete flags/permutation/stored projection");
    auto admitted=stored_catalog(s,restored,stored);
    auto permutations=permutation_catalog(s,restored,permutation_times,refs,p_policy.records);
    return {std::move(restored),std::move(admitted),std::move(permutations)};
}

KeyRecord record(std::span<const std::byte>b){require(b.size()==81,"Timing policy row extent differs");KeyRecord r;std::ranges::copy(b.first(32),r.key.begin());std::ranges::copy(b.subspan(32,32),r.payload.begin());const Bytes x(b);r.original={x.u32(64),std::to_integer<std::uint8_t>(b[68]),std::to_integer<std::uint8_t>(b[69]),std::to_integer<std::uint8_t>(b[70])};require(r.original.encoded_bytes&&r.original.sample_rate<3&&r.original.encoding<3&&r.original.compression<5,"Timing reference format exceeds original domains");for(unsigned i=0;i<5;++i)r.pitch[i]=std::int16_t(signed16(x.u16(71+i*2)));return r;}
std::vector<KeyRecord> records(const mcc_sound_timing_detail::Policy&p){require(!p.records.empty()&&p.records.size()%81==0&&p.records.size()/81<=100000,"Timing policy extent is invalid");std::vector<KeyRecord> out;for(std::size_t n=0;n<p.records.size();n+=81){auto r=record(p.records.subspan(n,81));require(out.empty()||out.back().key<r.key,"Timing policy key is duplicated or unsorted");out.push_back(r);}std::set<Digest> unique;for(const auto&key:p.unavailable_pitch)require(unique.insert(key).second&&std::ranges::binary_search(out,key,{},&KeyRecord::key),"Timing pitch exclusion is duplicated or absent");return out;}
struct NativeTag {TagHandle handle;std::uint32_t group,root,size;};
struct Check {std::uint32_t address;std::vector<std::byte> bytes;bool mutable_range{};};
struct Detail {MccSoundVanillaPayload payload;std::vector<Check> checks;};
struct Epoch {};
}
struct MccSoundTimingPlan::Impl {
    std::vector<MccSoundTimingBinding> bindings;
    std::vector<MccSoundTimingGap> gaps;
    std::vector<MccSoundVanillaOwnerResult> owners;
    std::vector<MccSoundTimingStoredReference> stored;
    std::vector<MccSoundTimingPermutationReference> permutations;
    MccSoundRuntimePlan native;
    std::vector<NativeTag> tags;
    std::vector<Detail> details;
    std::vector<Check> roots;
    std::map<std::uint32_t,NativeTag> tag_index;
    std::map<std::uint32_t,Check> root_index;
    TagHandle globals,primary_owner;
    std::uint32_t count{},globals_root{},globals_child{},primary_gestalt{},permutation_bank{};
    std::array<std::byte,36> globals_row;
};
struct MccSoundTimingMemoryOwner::Impl {const std::thread::id thread{std::this_thread::get_id()};std::mutex mutex;std::shared_ptr<const SourcePhysicsContactMemory> memory;std::shared_ptr<const Epoch> epoch;};
struct MccSoundTimingBoundPlan::Impl {MccSoundTimingPlan plan;std::weak_ptr<const MccSoundTimingMemoryOwner> owner;std::shared_ptr<const Epoch> epoch;std::uint32_t index,table;};
struct MccSoundTimingResourceProof::Impl {MccSoundTimingBoundPlan bound;std::size_t binding;std::shared_ptr<const EncodedSound> resource;};
struct MccSoundTimingAccess {
    using Plan=MccSoundTimingPlan::Impl;
    using Bound=MccSoundTimingBoundPlan::Impl;
    using Proof=MccSoundTimingResourceProof::Impl;
    static const auto& get(const MccSoundTimingPlan&p){return p.impl_;}
    static const auto& get(const MccSoundTimingBoundPlan&p){return p.impl_;}
    static const auto& get(const MccSoundTimingResourceProof&p){return p.impl_;}
    static MccSoundTimingPlan plan(std::shared_ptr<const Plan> p){return MccSoundTimingPlan(std::move(p));}
    static MccSoundTimingBoundPlan bound(std::shared_ptr<const Bound>p){return MccSoundTimingBoundPlan(std::move(p));}
    static MccSoundTimingResourceProof proof(std::shared_ptr<const Proof>p){return MccSoundTimingResourceProof(std::move(p));}
    struct Locked {std::unique_lock<std::mutex> lock;std::shared_ptr<const SourcePhysicsContactMemory> memory;std::shared_ptr<const Epoch> epoch;};
    static bool on_thread(const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner){return owner&&owner->impl_->thread==std::this_thread::get_id();}
    static Locked lock(const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner){require(!executing_timing,"Timing execution callback cannot reenter a lease operation");require(on_thread(owner),"Timing memory lease operation is on a foreign thread");require(bool(owner),"Timing needs an actual memory owner");auto&i=*owner->impl_;std::unique_lock guard(i.mutex);return {std::move(guard),i.memory,i.epoch};}
};
MccSoundTimingPlan::MccSoundTimingPlan(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
std::span<const MccSoundTimingBinding>MccSoundTimingPlan::bindings()const noexcept{return impl_->bindings;}
std::span<const MccSoundTimingGap>MccSoundTimingPlan::gaps()const noexcept{return impl_->gaps;}
std::span<const MccSoundVanillaOwnerResult>MccSoundTimingPlan::owners()const noexcept{return impl_->owners;}
std::span<const MccSoundTimingStoredReference>MccSoundTimingPlan::stored_references()const noexcept{return impl_->stored;}
std::span<const MccSoundTimingPermutationReference>MccSoundTimingPlan::permutation_references()const noexcept{return impl_->permutations;}
MccSoundTimingBoundPlan::MccSoundTimingBoundPlan(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
MccSoundTimingResourceProof::MccSoundTimingResourceProof(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
MccSoundTimingMemoryOwner::MccSoundTimingMemoryOwner(std::shared_ptr<const SourcePhysicsContactMemory>memory):impl_(std::make_unique<Impl>()){rebind(std::move(memory));}
MccSoundTimingMemoryOwner::~MccSoundTimingMemoryOwner()=default;
void MccSoundTimingMemoryOwner::rebind(std::shared_ptr<const SourcePhysicsContactMemory>m){require(!executing_timing,"Timing execution callback cannot rebind a lease");require(impl_->thread==std::this_thread::get_id(),"Timing memory rebind is on a foreign thread");require(bool(m),"Timing memory ownership is null");auto epoch=std::make_shared<const Epoch>();std::lock_guard guard(impl_->mutex);impl_->memory=std::move(m);impl_->epoch=std::move(epoch);}
void MccSoundTimingMemoryOwner::invalidate()noexcept{std::lock_guard guard(impl_->mutex);impl_->epoch.reset();impl_->memory.reset();}
namespace {
using Access=MccSoundTimingAccess;
std::span<const std::byte> native_bytes(const Access::Plan&p,const MccSoundVanillaSource&s,std::uint32_t at,std::size_t size){if(at>=p.native.address&&std::uint64_t(at)-p.native.address+size<=p.native.bytes.size())return Bytes(p.native.bytes).slice(at-p.native.address,size);for(const auto&x:p.native.patches){const auto a=s.metadata.relocate(x.source_address,x.after.size());if(at>=a&&std::uint64_t(at)-a+size<=x.after.size())return Bytes(x.after).slice(at-a,size);}throw FormatError("Timing native input has no projected owner");}
void add_check(Detail&d,const Access::Plan&p,const MccSoundVanillaSource&s,std::uint32_t a,unsigned n,bool mutable_range=false){const auto bytes=native_bytes(p,s,a,n);d.checks.push_back({a,{bytes.begin(),bytes.end()},mutable_range});}
MccSoundVanillaPayload payload_facts(const Reader&r,const MccSoundRuntimeDefinition&def,const Bytes&snd,const Bytes&perm,std::span<const std::byte>g,const Digest&sha){
    const auto localized=r.block(perm.slice(0,16),8,8);require(localized.count>0,"Timing permutation lacks English localized info");const Bytes info(r.row(localized,0,8));const auto languages=r.block(g,0x60,8);require(languages.count>0,"Timing source lacks English chunks");const auto chunks=r.block(r.row(languages,0,8),0,16);const auto first=info.u16(4),count=info.u16(6);require(signed16(first)>=0&&signed16(count)>0&&std::uint64_t(first)+count<=std::uint32_t(chunks.count),"Timing chunk interval escapes English owner");
    const std::array rates{22050u,44100u,32000u,48000u},channels{1u,2u,2u,4u};require(def.codec.sample_rate<rates.size()&&def.codec.encoding<channels.size(),"Timing actual codec is unsupported");MccSoundVanillaPayload out{sha,0,def.codec.encoding==2?4u:def.codec.compression,rates[def.codec.sample_rate],channels[def.codec.encoding],0,std::to_integer<unsigned>(snd.slice(2,1)[0]),{}};require(out.codec<=5&&out.sound_class<54,"Timing actual compression/class differs");const auto pcm=info.u32(0);require(pcm&&(out.codec==4||pcm%(out.channels*2)==0),"Timing actual PCM/container extent differs");out.samples=out.codec==4?0:pcm/(out.channels*2);require(out.samples<=out.sample_rate*600ull,"Timing actual resource is excessive");
    for(unsigned i=0;i<count;++i){const Bytes row(r.row(chunks,first+i,16));const auto n=row.u32(4)&0x1fffffffu;require(n&&n<=128u*1024*1024-out.encoded_bytes,"Timing actual encoded extent is invalid");out.encoded_bytes+=n;out.packets.push_back({n,row.u32(8),bool(row.u32(4)&0x20000000u)});}return out;
}
}
namespace mcc_sound_timing_detail {
Digest named_key(std::string_view path,std::string_view range,std::string_view permutation){Hash h;constexpr char domain[]="H2_TU15_SOUND_TIMING_KEY_V1";h.bytes(std::as_bytes(std::span(domain,sizeof(domain))));h.text(path);h.text(range);h.text(permutation);return h.finish();}
MccSoundTimingPlan build_impl(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,const MccSoundRuntimePlan&native,const Policy&policy,bool require_stored,const mcc_sound_permutation_time_detail::Policy* fixture_policy=nullptr,bool classes=false,std::span<const MccSoundGeometryResource> geometry={}){
    Reader r(source);const auto refs=records(policy);auto out=std::make_shared<Access::Plan>();std::set<std::tuple<unsigned,unsigned,std::string,Digest>> unique;
    for(const auto&p:policy.owners)require(unique.emplace(p.handle.value,p.root,p.path,p.metadata).second,"Timing policy owner is ambiguous");
    for(const auto&[_,t]:r.tags)if(t->address&&t->group=="ugh!"){
        bool known=false,authenticated=false;Digest actual{};std::string failure;
        try{for(const auto&p:policy.owners)if(p.handle==t->handle()){known=true;if(p.root==t->address&&p.path==t->name){if(actual==Digest{})actual=mcc_sound_reference_detail::metadata_digest(fingerprint,t->handle());if(actual==p.metadata){require(!authenticated,"Timing complete owner has multiple profiles");authenticated=true;}}}}catch(const std::exception&e){failure=e.what();authenticated=false;}
        out->owners.push_back({t->handle(),authenticated?MccSoundVanillaOwnerStatus::Authenticated:known?MccSoundVanillaOwnerStatus::Custom:MccSoundVanillaOwnerStatus::Unmatched,authenticated?"Complete canonical raw sound metadata matches":known?(failure.empty()?"Complete canonical raw owner differs":failure):"No finite timing owner profile"});
    }
    try{auto projected=expected_projection(source,fingerprint,native,require_stored,refs,fixture_policy,classes,geometry);out->native=std::move(projected.native);out->stored=std::move(projected.stored);out->permutations=std::move(projected.permutations);}catch(const std::exception&e){for(auto&o:out->owners){out->gaps.push_back({{},o.gestalt,0,0,MccSoundTimingGapKind::InvalidNativeProjection,e.what()});o.status=MccSoundVanillaOwnerStatus::Custom;}return Access::plan(out);}
    out->count=std::uint32_t(source.metadata.tags.size());
    const Tag* globals=nullptr;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> native_prefixes;
    for(const auto&[_,t]:r.tags)if(t->address&&(t->group=="snd!"||t->group=="ugh!"||t->group=="matg"||t->group=="sncl")){
        const auto root=source.metadata.relocate(t->address,t->size);require(root&&root!=0xffffffffu&&std::uint64_t(root)+t->size<=0x100000000ull,"Timing native directory extent differs");const auto prefix=t->group=="snd!"?20u:t->group=="ugh!"?88u:t->group=="matg"?0xc8u:8u;
        require(root==source.metadata.relocate(t->address,prefix),"Timing native directory/prefix root owners differ");
        require(std::uint64_t(root)+prefix<=native.address||std::uint64_t(native.address)+native.bytes.size()<=root,"Timing native metadata root aliases the merged bank");
        native_prefixes.emplace_back(root,prefix);
        const NativeTag projected{t->handle(),t->type().value,root,t->size};out->tags.push_back(projected);out->tag_index.emplace(t->datum,projected);
        if(t->group=="matg"){require(!globals,"Timing globals namespace is ambiguous");globals=t;out->globals=t->handle();out->globals_root=root;}
    }
    std::ranges::sort(native_prefixes);
    for(std::size_t i=1;i<native_prefixes.size();++i)require(std::uint64_t(native_prefixes[i-1].first)+native_prefixes[i-1].second<=native_prefixes[i].first,"Timing distinct typed native root prefixes alias");
    for(const auto&patch:out->native.patches){const auto i=r.tags.find(patch.owner.value);if(i!=r.tags.end()&&(i->second->group=="snd!"||i->second->group=="ugh!")){Check check{source.metadata.relocate(patch.source_address,patch.after.size()),patch.after,false};require(out->root_index.emplace(check.address,check).second,"Timing projected native root aliases another owner");out->roots.push_back(std::move(check));}}
    require(globals,"Timing globals owner is absent");const auto sg=r.block(r.read(globals->address,0xc8),0xc0,44);require(sg.count==1,"Timing SoundGlobals is not singular");out->globals_child=source.metadata.relocate(sg.address,44);out->globals_row=project_mcc_sound_globals(r.row(sg,0,44));const TagHandle primary{Bytes(out->globals_row).u32(32)};out->primary_owner=primary;const auto&pt=r.tag(primary,"ugh!");out->primary_gestalt=source.metadata.relocate(pt.address,88);out->permutation_bank=native.blocks[5].address;
    for(auto&owner:out->owners){
        if(owner.status!=MccSoundVanillaOwnerStatus::Authenticated){out->gaps.push_back({{},owner.gestalt,0,0,owner.status==MccSoundVanillaOwnerStatus::Custom?MccSoundTimingGapKind::CustomOwner:MccSoundTimingGapKind::UnknownOwner,owner.reason});continue;}
        std::vector<MccSoundTimingBinding> staged;std::vector<Detail> details;std::vector<MccSoundTimingGap> gaps;
        try{
            require(bool(source.payload),"Timing owner has no real encoded resource callback");const auto&g=r.tag(owner.gestalt,"ugh!");const auto gr=r.read(g.address,120);std::array<TagBlock,11> banks;for(unsigned i=0;i<11;++i)banks[i]=r.block(gr,offsets[i],i==8?8:i==10?332:strides[i]);const auto layout=std::ranges::find(native.gestalts,owner.gestalt,&MccSoundRuntimeGestalt::owner);require(layout!=native.gestalts.end(),"Timing native gestalt interval owner is absent");std::set<std::tuple<std::string,std::string,std::string>> names;
            for(const auto&def:native.sounds)if(def.gestalt==owner.gestalt){
                const auto&t=r.tag(def.sound,"snd!");require(def.source_root==t.address&&source.metadata.gestalt_owner(t.handle())==owner.gestalt,"Timing sound native/source binding differs");const auto sr=r.read(t.address,28);const Bytes snd(sr);const auto first=snd.u16(8);const auto count=std::to_integer<unsigned>(sr[10]);require(signed16(first)>=0&&count<128&&std::uint64_t(first)+count<=std::uint32_t(banks[4].count),"Timing sound pitch interval differs");std::set<std::string> range_names;
                for(unsigned rp=0;rp<count;++rp){const auto ri=first+rp;const auto rr=r.row(banks[4],ri,12);const Bytes range(rr);const auto rn=r.name(banks[2],range.u16(0));require(range_names.insert(rn.second).second,"Timing sound pitch names are ambiguous");const auto pf=range.u16(8),pc=range.u16(10);require(signed16(pf)>=0&&signed16(pc)>=0&&std::uint64_t(pf)+pc<=std::uint32_t(banks[5].count),"Timing source permutation interval differs");std::set<std::string> permutation_names;
                    for(unsigned pp=0;pp<pc;++pp){const auto pi=pf+pp;const auto pr=r.row(banks[5],pi,16);const Bytes perm(pr);const auto pn=r.name(banks[2],perm.u16(0));require(permutation_names.insert(pn.second).second&&names.emplace(t.name,rn.second,pn.second).second,"Timing full named key is ambiguous");const auto key=named_key(t.name,rn.second,pn.second);const auto ref=std::ranges::lower_bound(refs,key,{},&KeyRecord::key);
                        if(ref==refs.end()||ref->key!=key){gaps.push_back({t.handle(),owner.gestalt,ri,pi,MccSoundTimingGapKind::NoOriginalKey,"No exact original named timing tuple"});continue;}
                        if(std::ranges::find(policy.unavailable_pitch,key)!=policy.unavailable_pitch.end()){gaps.push_back({t.handle(),owner.gestalt,ri,pi,MccSoundTimingGapKind::NaturalPitchNotRestored,"Original natural pitch is not restored for this exact known key"});continue;}
                        const auto play=snd.u16(6),pitch=range.u16(2);const auto scale=std::to_integer<unsigned>(sr[11]);require(signed16(play)>=0&&scale<128&&signed16(pitch)>=0,"Timing original pitch operands lack typed owners");const Bytes playback(r.row(banks[0],play,56)),scaling(r.row(banks[1],scale,20)),params(r.row(banks[3],pitch,10));const std::array<std::int16_t,5> pitch_values{std::int16_t(signed16(params.u16(0))),std::int16_t(signed16(playback.u16(24))),std::int16_t(signed16(playback.u16(26))),std::int16_t(signed16(scaling.u16(8))),std::int16_t(signed16(scaling.u16(10)))};require(pitch_values==ref->pitch,"Timing authored pitch operands differ from the original tuple");
                        const auto expected=payload_facts(r,def,snd,perm,gr,ref->payload);require(source.payload(t.handle(),rp,pp)==expected,"Actual timing encoded payload/packet/format differs from the finite policy");
                        const auto nr=add(native.blocks[4].address,std::uint64_t(layout->first[4]+ri)*12),np=add(native.blocks[5].address,std::uint64_t(layout->first[5]+pi)*16);staged.push_back({t.handle(),owner.gestalt,t.address,def.native_root,ri,pi,nr,np,rp,pp,t.name,rn.second,pn.second,key,ref->payload,ref->original});Detail detail{expected,{}};add_check(detail,*out,source,def.native_root,20);add_check(detail,*out,source,nr,12,true);add_check(detail,*out,source,np,16);add_check(detail,*out,source,add(native.blocks[0].address,std::uint64_t(layout->first[0]+play)*56),56);add_check(detail,*out,source,add(native.blocks[1].address,std::uint64_t(layout->first[1]+scale)*20),20);add_check(detail,*out,source,add(native.blocks[3].address,std::uint64_t(layout->first[3]+pitch)*10),10);add_check(detail,*out,source,add(native.blocks[2].address,std::uint64_t(layout->first[2]+range.u16(0))*4),4);add_check(detail,*out,source,add(native.blocks[2].address,std::uint64_t(layout->first[2]+perm.u16(0))*4),4);details.push_back(std::move(detail));
                    }
                }
            }
            out->bindings.insert(out->bindings.end(),staged.begin(),staged.end());out->details.insert(out->details.end(),std::make_move_iterator(details.begin()),std::make_move_iterator(details.end()));out->gaps.insert(out->gaps.end(),gaps.begin(),gaps.end());
        }catch(const std::exception&e){owner.status=MccSoundVanillaOwnerStatus::Custom;owner.reason=e.what();out->gaps.push_back({{},owner.gestalt,0,0,MccSoundTimingGapKind::EncodedResourceMismatch,e.what()});}
    }
    std::erase_if(out->stored,[&](const auto&reference){
        const auto owner=std::ranges::find(out->owners,reference.gestalt,&MccSoundVanillaOwnerResult::gestalt);
        return owner==out->owners.end()||owner->status!=MccSoundVanillaOwnerStatus::Authenticated;
    });
    std::erase_if(out->permutations,[&](const auto&reference){
        const auto owner=std::ranges::find(out->owners,reference.gestalt,&MccSoundVanillaOwnerResult::gestalt);
        return owner==out->owners.end()||owner->status!=MccSoundVanillaOwnerStatus::Authenticated;
    });
    require(out->bindings.size()==out->details.size(),"Timing private binding owner count differs");return Access::plan(out);
}
MccSoundTimingPlan build(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,const MccSoundRuntimePlan&native,const Policy&policy){
    return build_impl(source,fingerprint,native,policy,false);
}
MccSoundTimingPlan build_with_permutation_times(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,
    const MccSoundRuntimePlan&native,const Policy&policy,const mcc_sound_permutation_time_detail::Policy&permutations){
    return build_impl(source,fingerprint,native,policy,false,&permutations);
}
} // namespace mcc_sound_timing_detail
namespace {
std::vector<std::byte> read_memory(const SourcePhysicsContactMemory&m,std::uint32_t address,std::size_t size){require(address&&address!=0xffffffffu&&size<=128u*1024*1024&&std::uint64_t(address)+size<=0x100000000ull,"Timing current memory extent is invalid");std::vector<std::byte> bytes(size);m.read(address,bytes);return bytes;}
std::uint32_t memory_word(const SourcePhysicsContactMemory&m,std::uint32_t at){return Bytes(read_memory(m,at,4)).u32(0);}
void check_bytes(const SourcePhysicsContactMemory&m,const Check&c){const auto bytes=read_memory(m,c.address,c.bytes.size());for(std::size_t i=0;i<bytes.size();++i)if(!(c.mutable_range&&(i==4||i==5)))require(bytes[i]==c.bytes[i],"Timing current immutable native metadata changed");}
void check_tag(const SourcePhysicsContactMemory&m,std::uint32_t table,const NativeTag&t){const auto bytes=read_memory(m,add(table,std::uint64_t(t.handle.index())*16),16);const Bytes row(bytes);require(row.u32(0)==t.group&&row.u32(4)==t.handle.value&&row.u32(8)==t.root&&row.u32(12)==t.size,"Timing current runtime tag table lost its full-salt typed owner");}
std::pair<unsigned,unsigned> current_index(const SourcePhysicsContactMemory&m,const Access::Plan&p){const auto index=memory_word(m,0x556d90),table=memory_word(m,0x4f0414);const auto bytes=read_memory(m,index,32);const Bytes header(bytes);require(header.u32(8)==table&&header.u32(16)==p.globals.value&&header.u32(24)==p.count&&header.u32(28)==0x74616773,"Timing current runtime index/header differs");return {index,table};}
void globals_current(const SourcePhysicsContactMemory&m,const Access::Plan&p){const auto bytes=read_memory(m,add(p.globals_root,0xc0),8);const Bytes desc(bytes);require(desc.u32(0)==1&&desc.u32(4)==p.globals_child,"Timing current SoundGlobals descriptor changed");require(std::ranges::equal(read_memory(m,p.globals_child,36),p.globals_row),"Timing current SoundGlobals full references changed");require(memory_word(m,0x52d664)==p.primary_gestalt,"Timing current merged Legacy sound gestalt changed");}
void check_binding_owners(const SourcePhysicsContactMemory&m,const Access::Plan&p,unsigned table,const MccSoundTimingBinding&b){
    const std::array handles{b.sound,b.gestalt,p.globals,p.primary_owner};
    for(unsigned n=0;n<handles.size();++n){bool duplicate=false;for(unsigned j=0;j<n;++j)duplicate|=handles[n]==handles[j];if(duplicate)continue;const auto t=p.tag_index.find(handles[n].value);require(t!=p.tag_index.end(),"Timing binding lost its exact expected tag owner");check_tag(m,table,t->second);}
    const auto owner=p.tag_index.find(b.gestalt.value);require(owner!=p.tag_index.end(),"Timing source gestalt root is absent");
    for(const auto address:{p.primary_gestalt,owner->second.root}){const auto root=p.root_index.find(address);require(root!=p.root_index.end(),"Timing merged root has no complete projected owner");check_bytes(m,root->second);}
}
void check_inputs(const SourcePhysicsContactMemory&m,const Access::Plan&p,std::size_t i){for(const auto&c:p.details.at(i).checks)check_bytes(m,c);}
bool resource_matches(const EncodedSound&s,const MccSoundTimingBinding&b,const MccSoundVanillaPayload&p){if(s.name!=b.path||s.data.size()!=p.encoded_bytes||s.sample_rate!=p.sample_rate||s.channels!=p.channels||s.codec!=p.codec||s.samples!=p.samples||s.sound_class!=p.sound_class||s.packet_chunks.size()!=p.packets.size())return false;for(std::size_t i=0;i<p.packets.size();++i){const auto&a=s.packet_chunks[i];const auto&x=p.packets[i];if(a.bytes!=x.bytes||a.pcm_bytes!=x.pcm_bytes||a.reset_after!=x.reset_after)return false;}return digest(s.data)==p.sha256;}
bool same_owner(const Access::Bound&bound,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner,const Access::Locked&scope){const auto expected=bound.owner.lock();return expected&&expected.get()==owner.get()&&scope.memory&&scope.epoch&&scope.epoch==bound.epoch;}
#include "audio/mcc_sound_timing_reference_profiles.inc"
mcc_sound_timing_detail::Policy canonical_timing_policy(){return {timing_known_owners,std::as_bytes(std::span(timing_reference_bytes,sizeof(timing_reference_bytes)-1)),timing_unavailable_pitch_keys};}
}
MccSoundTimingPlan build_mcc_sound_timing_reference(const MccSoundVanillaSource&s,const MccSoundRuntimePlan&p){return mcc_sound_timing_detail::build(s,s,p,canonical_timing_policy());}
MccSoundTimingPlan build_mcc_sound_timing_reference(const Cache&c,const MccSoundRuntimePlan&p,const std::function<std::uint32_t(std::uint32_t,std::size_t)>&relocate){
    require(c.is_mcc(),"Timing reference requires MCC source ownership");
    const MccSoundVanillaSource s{{c.tags(),[&](auto a,auto n){return c.read_tag_bytes(a,n);},[&](TagHandle h){return c.mcc_sound_gestalt_owner(h).handle();},relocate},[&](auto id){return c.string_id(id);},[&](TagHandle h,unsigned pitch,unsigned permutation){const auto sound=c.load_sound(h.value,pitch,permutation);MccSoundVanillaPayload out{digest(sound.data),sound.data.size(),sound.codec,sound.sample_rate,sound.channels,sound.samples,sound.sound_class,{}};for(const auto&chunk:sound.packet_chunks)out.packets.push_back({chunk.bytes,chunk.pcm_bytes,chunk.reset_after});return out;}};
    const auto geometry=plan_mcc_sound_geometry_resources(c);
    if(c.is_custom_mcc())
        return mcc_sound_timing_detail::build_impl(s,s,p,canonical_timing_policy(),false,nullptr,false,geometry);
    bool moved=false;for(const auto&t:c.retained_tag_allocations())for(const auto&a:c.tags())if(a.address&&a.group=="matg"&&(t.group=="matg"||t.datum==a.datum))moved=true;
    if(!moved)return mcc_sound_timing_detail::build_impl(s,s,p,canonical_timing_policy(),true,nullptr,true,geometry);
    try{const auto tags=mcc_sound_reference_detail::raw_globals_namespace(s,c.retained_tag_allocations());auto canonical=s;canonical.metadata.tags=tags;return mcc_sound_timing_detail::build_impl(s,canonical,p,canonical_timing_policy(),true,nullptr,true,geometry);}
    catch(const FormatError&e){auto rejected=std::make_shared<Access::Plan>();for(const auto&t:c.tags())if(t.address&&t.group=="ugh!"){rejected->owners.push_back({t.handle(),MccSoundVanillaOwnerStatus::Custom,e.what()});rejected->gaps.push_back({{},t.handle(),0,0,MccSoundTimingGapKind::CustomOwner,e.what()});}return Access::plan(rejected);}
}
MccSoundTimingBoundPlan bind_mcc_sound_timing_memory(const MccSoundTimingPlan&plan,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner){
    const auto&p=Access::get(plan);require(p&&!p->bindings.empty(),"Timing plan has no admitted reference bindings");auto scope=Access::lock(owner);require(scope.memory&&scope.epoch,"Timing memory lease is invalid");const auto&memory=*scope.memory;const auto[index,table]=current_index(memory,*p);for(const auto&t:p->tags)check_tag(memory,table,t);globals_current(memory,*p);for(const auto&root:p->roots)check_bytes(memory,root);for(std::size_t i=0;i<p->bindings.size();++i)check_inputs(memory,*p,i);return Access::bound(std::make_shared<const Access::Bound>(Access::Bound{plan,owner,scope.epoch,index,table}));
}
MccSoundTimingResourceProof authenticate_mcc_sound_timing_resource(const MccSoundTimingBoundPlan&bound,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner,TagHandle sound,unsigned pitch,unsigned permutation,std::shared_ptr<const EncodedSound>resource){
    const auto&b=Access::get(bound);require(bool(b),"Timing bound plan is absent");auto scope=Access::lock(owner);require(same_owner(*b,owner,scope),"Timing memory binding is stale or foreign");const auto&p=*Access::get(b->plan);const auto i=std::ranges::find_if(p.bindings,[&](const auto&x){return x.sound==sound&&x.relative_pitch==pitch&&x.relative_permutation==permutation;});require(i!=p.bindings.end()&&resource,"Timing resource has no full binding/owner");const auto n=std::size_t(i-p.bindings.begin());require(current_index(*scope.memory,p)==std::pair{b->index,b->table},"Timing runtime namespace moved without rebinding");globals_current(*scope.memory,p);check_binding_owners(*scope.memory,p,b->table,*i);check_inputs(*scope.memory,p,n);require(resource_matches(*resource,*i,p.details[n].payload),"Actual immutable timing resource differs from its full named binding");return Access::proof(std::make_shared<const Access::Proof>(Access::Proof{bound,n,std::move(resource)}));
}
namespace {
using TimingStatus=MccSoundTimingLookupStatus;
std::optional<TimingStatus> timing_entry_failure(const MccSoundTimingBoundPlan&bound,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner,const MccSoundTimingEntry&e){
    if(e.pc!=0x21f590||e.return_address!=0x21f6f8||e.stack[0]!=e.return_address||e.stack[1]!=e.edi||e.stack[2]!=e.eax)return TimingStatus::WrongCaller;
    if(!Access::get(bound)||!owner)return TimingStatus::StaleOrForeignOwner;
    if(!Access::on_thread(owner))return TimingStatus::WrongThread;
    if(executing_timing)return TimingStatus::ReentrantLeaseOperation;
    return {};
}
MccSoundTimingLookup timing_lookup_locked(const MccSoundTimingBoundPlan&bound,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner,const MccSoundTimingEntry&e,const MccSoundTimingResourceProof&proof,const Access::Locked&scope){
    using Status=TimingStatus;const auto fail=[](Status status){return MccSoundTimingLookup{status,{}};};
    const auto&b=Access::get(bound);if(!same_owner(*b,owner,scope))return fail(Status::StaleOrForeignOwner);
    const auto&r=Access::get(proof);if(!r||Access::get(r->bound)!=b)return fail(Status::StaleOrForeignResource);const auto&p=*Access::get(b->plan);
    const auto i=std::ranges::find_if(p.bindings,[&](const auto&x){return x.native_root==e.esi&&x.native_range==e.edi&&std::uint64_t(e.edx)+e.ecx==x.native_permutation;});if(i==p.bindings.end())return fail(Status::NoReference);const auto n=std::size_t(i-p.bindings.begin());if(n!=r->binding)return fail(Status::StaleOrForeignResource);if(e.ecx%16||e.edx!=p.permutation_bank||e.ebx!=p.primary_gestalt||e.eax!=p.details[n].payload.encoded_bytes)return fail(Status::WrongRangeOrPermutation);
    try{
        const auto&m=*scope.memory;if(current_index(m,p)!=std::pair{b->index,b->table})return fail(Status::WrongTagOwner);globals_current(m,p);check_binding_owners(m,p,b->table,*i);check_inputs(m,p,n);const auto stack_bytes=read_memory(m,e.esp,12);const Bytes stack(stack_bytes);for(unsigned j=0;j<3;++j)if(stack.u32(j*4)!=e.stack[j])return fail(Status::WrongCaller);if(!resource_matches(*r->resource,*i,p.details[n].payload))return fail(Status::StaleOrForeignResource);
    }catch(const std::exception&){return fail(Status::ChangedNativeMetadata);}
    return {Status::Reference,i->original};
}
struct TimingExecutionGuard {
    TimingExecutionGuard(){require(!executing_timing,"Timing execution callback is reentrant");executing_timing=true;}
    ~TimingExecutionGuard(){executing_timing=false;}
};
} // namespace
MccSoundTimingLookup lookup_mcc_sound_original_timing(const MccSoundTimingBoundPlan&bound,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner,const MccSoundTimingEntry&e,const MccSoundTimingResourceProof&proof){
    if(const auto failure=timing_entry_failure(bound,owner,e))return {*failure,{}};
    auto scope=Access::lock(owner);return timing_lookup_locked(bound,owner,e,proof,scope);
}
MccSoundTimingLookupStatus with_mcc_sound_original_timing(const MccSoundTimingBoundPlan&bound,const std::shared_ptr<const MccSoundTimingMemoryOwner>&owner,const MccSoundTimingEntry&e,const MccSoundTimingResourceProof&proof,const std::function<void(const MccSoundOriginalTimingOperands&)>&callback){
    require(bool(callback),"Timing execution callback is absent");
    // A callback may release its caller's handles. Retain the object containing
    // the locked mutex and the actual resource proof until after unlocking.
    const auto retained_owner=owner;const auto retained_bound=bound;const auto retained_proof=proof;const auto retained_callback=callback;
    if(const auto failure=timing_entry_failure(retained_bound,retained_owner,e))return *failure;
    auto scope=Access::lock(retained_owner);const auto result=timing_lookup_locked(retained_bound,retained_owner,e,retained_proof,scope);
    if(result.status==TimingStatus::Reference){TimingExecutionGuard execution;retained_callback(*result.original);}
    return result.status;
}
} // namespace halo2

namespace halo2 {
struct MccSoundTimingReadLease::Impl {
    std::shared_ptr<const MccSoundTimingMemoryOwner> owner;
    std::shared_ptr<const Epoch> epoch;
};
struct MccSoundTimingOwnedReader::Impl {
    const std::thread::id thread{std::this_thread::get_id()};
    std::weak_ptr<const SourcePhysicsContactMemory> memory;
    bool active=false; // Accessed only on the immutable creating thread.
};
struct MccSoundTimingOwnedReadAccess {
    using Lease=MccSoundTimingReadLease::Impl;
    using Reader=MccSoundTimingOwnedReader::Impl;
    static const auto&get(const MccSoundTimingReadLease&lease){return lease.impl_;}
    static MccSoundTimingReadLease lease(std::shared_ptr<const Lease> p){return MccSoundTimingReadLease(std::move(p));}
    static MccSoundTimingOwnedReader reader(std::shared_ptr<Reader>p){return MccSoundTimingOwnedReader(std::move(p));}
};
struct MccSoundTimingHeldRead::Impl {
    const std::thread::id thread{std::this_thread::get_id()};
    const MccSoundTimingReadLease lease;
    const std::shared_ptr<const MccSoundTimingMemoryOwner> owner;
    MccSoundTimingAccess::Locked scope;
    static std::shared_ptr<const MccSoundTimingMemoryOwner> owner_for(
        const MccSoundTimingReadLease& lease){
        const auto& state=MccSoundTimingOwnedReadAccess::get(lease);
        require(state&&state->owner,"Held timing read requires an actual lease owner");
        return state->owner;
    }
    explicit Impl(const MccSoundTimingReadLease& input)
        :lease(input),owner(owner_for(input)),scope(MccSoundTimingAccess::lock(owner)){
        const auto& state=MccSoundTimingOwnedReadAccess::get(lease);
        require(scope.memory&&scope.epoch&&scope.epoch==state->epoch&&
                !scope.epoch.owner_before(state->epoch)&&!state->epoch.owner_before(scope.epoch),
                "Held timing read lease is stale or invalidated");
        // All potentially throwing preparation is finished. Keep the existing
        // timing execution guard active for this entire retained lock scope.
        executing_timing=true;
    }
    ~Impl(){
        if(thread!=std::this_thread::get_id())std::terminate();
        // Retained memory/epoch/owner destruction occurs after unlocking.
        scope.lock.unlock();executing_timing=false;
    }
};
MccSoundTimingHeldRead::MccSoundTimingHeldRead(std::unique_ptr<Impl> p):impl_(std::move(p)){}
MccSoundTimingHeldRead::~MccSoundTimingHeldRead(){
    if(impl_&&impl_->thread!=std::this_thread::get_id())std::terminate();
}
bool MccSoundTimingHeldRead::matches(const MccSoundTimingReadLease& lease)const noexcept{
    if(!impl_||impl_->thread!=std::this_thread::get_id())return false;
    const auto& state=MccSoundTimingOwnedReadAccess::get(lease);
    return state&&state->owner&&impl_->scope.lock.owns_lock()&&executing_timing&&
        state->owner.get()==impl_->owner.get()&&
        !state->owner.owner_before(impl_->owner)&&!impl_->owner.owner_before(state->owner)&&
        state->epoch==impl_->scope.epoch&&!state->epoch.owner_before(impl_->scope.epoch)&&
        !impl_->scope.epoch.owner_before(state->epoch);
}
MccSoundTimingHeldRead hold_mcc_sound_timing_read_lease(const MccSoundTimingReadLease& lease){
    return MccSoundTimingHeldRead(std::make_unique<MccSoundTimingHeldRead::Impl>(lease));
}
namespace {
thread_local const void* executing_owned_reader{};
struct OwnedReadExecutionGuard {
    TimingExecutionGuard execution;
    MccSoundTimingOwnedReadAccess::Reader&reader;
    explicit OwnedReadExecutionGuard(MccSoundTimingOwnedReadAccess::Reader&r):reader(r){
        require(!executing_owned_reader,"Owned timing reader is reentrant");
        reader.active=true;executing_owned_reader=&reader;
    }
    ~OwnedReadExecutionGuard(){reader.active=false;executing_owned_reader=nullptr;}
};
}
MccSoundTimingReadLease::MccSoundTimingReadLease(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
MccSoundTimingOwnedReader::MccSoundTimingOwnedReader(std::shared_ptr<Impl>p):impl_(std::move(p)){}
std::vector<std::byte>MccSoundTimingOwnedReader::read(std::uint32_t address,std::size_t size)const{
    const auto state=impl_;
    require(state&&state->thread==std::this_thread::get_id(),"Owned timing reader used from foreign thread");
    require(state->active&&executing_timing&&executing_owned_reader==state.get(),"Owned timing reader is outside its transaction");
    require(size<=128u*1024*1024&&std::uint64_t(address)+size<=0x100000000ull,"Owned timing read extent is invalid");
    const auto memory=state->memory.lock();require(bool(memory),"Owned timing read memory is unavailable");
    std::vector<std::byte>bytes(size);if(size)memory->read(address,bytes);return bytes;
}
MccSoundTimingReadLease capture_mcc_sound_timing_read_lease(
    const std::shared_ptr<const MccSoundTimingMemoryOwner>&input){
    const auto owner=input;auto scope=MccSoundTimingAccess::lock(owner);
    require(scope.memory&&scope.epoch,"Owned timing read lease is invalidated");
    auto lease=std::make_shared<MccSoundTimingOwnedReadAccess::Lease>();lease->owner=owner;lease->epoch=scope.epoch;
    return MccSoundTimingOwnedReadAccess::lease(std::move(lease));
}
MccSoundTimingReadStatus with_mcc_sound_timing_owned_read(
    const MccSoundTimingReadLease&input,
    const std::function<void(const MccSoundTimingOwnedReader&)>&callback){
    const auto retained_lease=input;
    require(bool(callback),"Owned timing read callback is absent");
    if(executing_timing)return MccSoundTimingReadStatus::ReentrantLeaseOperation;
    const auto state=MccSoundTimingOwnedReadAccess::get(retained_lease);
    if(!state||!state->owner)return MccSoundTimingReadStatus::StaleOwner;
    const auto owner=state->owner;
    if(!MccSoundTimingAccess::on_thread(owner))return MccSoundTimingReadStatus::WrongThread;
    auto scope=MccSoundTimingAccess::lock(owner);
    if(!scope.memory||!scope.epoch||scope.epoch!=state->epoch)return MccSoundTimingReadStatus::StaleOwner;
    auto reader_state=std::make_shared<MccSoundTimingOwnedReadAccess::Reader>();reader_state->memory=scope.memory;
    const auto reader=MccSoundTimingOwnedReadAccess::reader(reader_state);
    OwnedReadExecutionGuard execution(*reader_state);
    // Callable copies/destruction may run user code. Perform both behind the
    // current-epoch/thread checks and execution guard, including on throws.
    const auto retained_callback=callback;retained_callback(reader);
    return MccSoundTimingReadStatus::Executed;
}
MccSoundTimingReadStatus check_mcc_sound_timing_read_lease(const MccSoundTimingReadLease&lease){
    return with_mcc_sound_timing_owned_read(lease,[](const MccSoundTimingOwnedReader&){});
}
} // namespace halo2
