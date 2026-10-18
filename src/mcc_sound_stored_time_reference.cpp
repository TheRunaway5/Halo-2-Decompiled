#include "halo2/mcc_sound_stored_time_reference.hpp"
#include "mcc_sound_stored_time_reference_profile.hpp"
#include "mcc_sound_permutation_time_reference_profile.hpp"
#include "mcc_sound_vanilla_reference_profile.hpp"
#include "mcc_sound_timing_reference_profile.hpp"
#include "mcc_sound_class_qualified.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <openssl/evp.h>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
using Digest=MccSoundVanillaDigest;
using Status=MccSoundStoredTimeStatus;
using mcc_sound_stored_time_detail::Owner;
void require(bool value,const char*why){if(!value)throw FormatError(why);}
class Hash {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>ctx{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
    Hash(){require(ctx&&EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,"Stored-time SHA initialization failed");}
    void bytes(std::span<const std::byte>s){require(s.empty()||EVP_DigestUpdate(ctx.get(),s.data(),s.size())==1,"Stored-time SHA update failed");}
    void word(std::uint32_t x){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(x>>(i*8));bytes(b);}
    void wide(std::uint64_t x){word(std::uint32_t(x));word(std::uint32_t(x>>32));}
    void text(std::string_view s){require(!s.empty()&&s.size()<=0x100000,"Stored-time identity is absent/excessive");word(std::uint32_t(s.size()));bytes(std::as_bytes(std::span(s.data(),s.size())));}
    Digest finish(){Digest out;unsigned n=0;require(EVP_DigestFinal_ex(ctx.get(),reinterpret_cast<unsigned char*>(out.data()),&n)==1&&n==32,"Stored-time SHA finalization failed");return out;}
};
Digest digest(std::span<const std::byte>s){Hash h;h.bytes(s);return h.finish();}
std::uint32_t add(std::uint32_t a,std::uint64_t b){require(b<=0xffffffffull-a,"Stored-time address wraps");return a+std::uint32_t(b);}
int signed16(unsigned x){return std::bit_cast<std::int16_t>(std::uint16_t(x));}
void word(std::span<std::byte>b,unsigned at,std::uint32_t x,unsigned n=4){Bytes(b).require(at,n);for(unsigned i=0;i<n;++i)b[at+i]=std::byte(x>>(8*i));}
struct Reader {
    const MccSoundVanillaSource&source;
    std::map<std::uint32_t,const Tag*>tags;
    explicit Reader(const MccSoundVanillaSource&s):source(s){
        require(s.metadata.read&&s.metadata.relocate&&s.metadata.gestalt_owner&&s.string_id&&s.payload,"Stored-time needs real typed metadata/resource owners");
        require(s.metadata.tags.size()<=65536,"Stored-time namespace exceeds source indices");
        for(std::size_t i=0;i<s.metadata.tags.size();++i){const auto&t=s.metadata.tags[i];if(t.handle().is_null())continue;require(t.handle().index()==i&&tags.emplace(t.datum,&t).second,"Stored-time namespace contains a foreign salt/index");}
    }
    std::span<const std::byte>read(std::uint32_t at,std::size_t n)const{require(at&&at!=0xffffffffu&&n<=128u*1024*1024&&std::uint64_t(at)+n<=0x100000000ull,"Stored-time source extent is invalid");const auto b=source.metadata.read(at,n);require(b.size()==n,"Stored-time source extent is partial");return b;}
    const Tag&tag(TagHandle h,std::string_view group)const{const auto i=tags.find(h.value);require(!h.is_null()&&i!=tags.end()&&i->second->address&&i->second->group==group,"Stored-time lost a full-salt typed owner");return *i->second;}
    TagBlock block(std::span<const std::byte>b,unsigned at,unsigned stride)const{const Bytes x(b);const auto n=x.u32(at),p=x.u32(at+4);require(n<=0x100000&&std::uint64_t(n)*stride<=128u*1024*1024,"Stored-time block exceeds its domain");if(n)(void)read(p,std::size_t(n)*stride);return {std::int32_t(n),p};}
    std::span<const std::byte>row(const TagBlock&b,unsigned n,unsigned stride)const{require(n<std::uint32_t(b.count),"Stored-time ordinal escapes its bank");return read(add(b.address,std::uint64_t(n)*stride),stride);}
    std::string name(const TagBlock&names,unsigned n)const{require(signed16(n)>=0,"Stored-time import-name index is negative");const auto value=source.string_id(Bytes(row(names,n,4)).u32(0));require(!value.empty(),"Stored-time import name is absent");return value;}
};
struct Value {Digest key;std::int32_t ms;};
struct Payload {Digest key,sha;};
std::vector<Value>values(std::span<const std::byte>b){require(!b.empty()&&b.size()%36==0&&b.size()/36<=100000,"Stored-time value policy has invalid extent");std::vector<Value>out;for(std::size_t at=0;at<b.size();at+=36){Value v;std::ranges::copy(b.subspan(at,32),v.key.begin());v.ms=std::bit_cast<std::int32_t>(Bytes(b).u32(at+32));require(v.ms>=0&&(out.empty()||out.back().key<v.key),"Stored-time value policy is negative/duplicated/unsorted");out.push_back(v);}return out;}
std::vector<Payload>payloads(std::span<const std::byte>b){require(!b.empty()&&b.size()%64==0&&b.size()/64<=100000,"Stored-time payload policy has invalid extent");std::vector<Payload>out;for(std::size_t at=0;at<b.size();at+=64){Payload p;std::ranges::copy(b.subspan(at,32),p.key.begin());std::ranges::copy(b.subspan(at+32,32),p.sha.begin());require(out.empty()||out.back().key<p.key,"Stored-time payload policy is duplicated/unsorted");out.push_back(p);}return out;}
MccSoundVanillaPayload payload_facts(const Reader&r,std::span<const std::byte>sound,std::span<const std::byte>g,const Bytes&perm,const Digest&sha){
    const auto codecs=r.block(g,0,3);const auto definition=project_mcc_sound_definition(sound,r.read(codecs.address,std::size_t(codecs.count)*3));
    const auto localized=r.block(perm.slice(0,16),8,8);require(localized.count>0,"Stored-time permutation lacks English info");const Bytes info(r.row(localized,0,8));
    const auto languages=r.block(g,0x60,8);require(languages.count>0,"Stored-time gestalt lacks English chunks");const auto chunks=r.block(r.row(languages,0,8),0,16);const auto first=info.u16(4),count=info.u16(6);require(signed16(first)>=0&&signed16(count)>0&&std::uint64_t(first)+count<=std::uint32_t(chunks.count),"Stored-time chunk interval escapes English owner");
    const std::array rates{22050u,44100u,32000u,48000u},channels{1u,2u,2u,4u};const auto codec=definition.codec;require(codec.sample_rate<rates.size()&&codec.encoding<channels.size(),"Stored-time actual codec is unsupported");
    MccSoundVanillaPayload out{sha,0,codec.encoding==2?4u:codec.compression,rates[codec.sample_rate],channels[codec.encoding],0,std::to_integer<unsigned>(sound[2]),{}};
    const auto pcm=info.u32(0);require(out.codec<=5&&out.sound_class<54&&pcm&&(out.codec==4||pcm%(out.channels*2)==0),"Stored-time actual PCM/container format differs");out.samples=out.codec==4?0:pcm/(out.channels*2);require(out.samples<=out.sample_rate*600ull,"Stored-time resource exceeds bounded source extent");
    for(unsigned i=0;i<count;++i){const Bytes chunk(r.row(chunks,first+i,16));const auto n=chunk.u32(4)&0x1fffffffu;require(n&&n<=128u*1024*1024-out.encoded_bytes,"Stored-time encoded extent is invalid");out.encoded_bytes+=n;out.packets.push_back({n,chunk.u32(8),bool(chunk.u32(4)&0x20000000u)});}return out;
}
// Identical complete functional encoding to the verified timing V4 plan
// digest. This new module never changes the existing implementation/body.
Digest plan_digest(const MccSoundRuntimePlan&p){
    Hash h;h.word(p.address);h.wide(p.bytes.size());h.bytes(p.bytes);for(const auto&b:p.blocks){h.word(std::uint32_t(b.count));h.word(b.address);}
    h.wide(p.patches.size());for(const auto&x:p.patches){h.word(x.owner.value);h.word(x.source_address);h.wide(x.before.size());h.bytes(x.before);h.wide(x.after.size());h.bytes(x.after);}
    h.wide(p.gestalts.size());for(const auto&g:p.gestalts){h.word(g.owner.value);for(auto x:g.first)h.word(x);for(auto x:g.count)h.word(x);}
    h.wide(p.chunks.size());for(const auto&c:p.chunks){h.word(c.gestalt.value);h.word(c.source_address);h.word(c.native_address);h.word(c.source_index);h.word(c.file_offset);h.word(c.encoded_bytes);h.word(c.decoded_bytes);h.word(c.reset_after);}
    h.wide(p.sounds.size());for(const auto&s:p.sounds){h.word(s.sound.value);h.word(s.gestalt.value);h.word(s.codec.sample_rate);h.word(s.codec.encoding);h.word(s.codec.compression);h.word(unsigned(s.file));h.word(s.remastered_index);h.bytes(s.mcc_extension);h.word(s.source_root);h.word(s.native_root);h.word(s.source_flags);h.word(s.native_flags);}
    h.wide(p.unused_flag_indices.size());for(const auto&u:p.unused_flag_indices){h.word(u.gestalt.value);h.word(u.range_index);h.word(u.encoded_index);h.wide(u.users.size());for(const auto&x:u.users){h.word(x.sound.value);h.word(x.mcc_class.value);h.word(x.native_class.value);h.word(x.class_index);h.word(x.mcc_flags);h.word(x.native_flags);}}
    h.wide(p.native_pcm_duration.size());for(auto x:p.native_pcm_duration)h.word(x.value);
    h.word(bool(p.authored_flags));if(p.authored_flags){h.wide(p.authored_flags->corrections.size());for(const auto&c:p.authored_flags->corrections){h.word(c.sound.value);h.word(c.gestalt.value);h.word(c.source_root);h.word(c.source_flags);h.word(c.native_before);h.word(c.native_after);}h.wide(p.authored_flags->owners.size());for(const auto&o:p.authored_flags->owners){h.word(o.gestalt.value);h.word(unsigned(o.status));}}
    return h.finish();
}
// Fingerprints here use the same retained-raw-globals domain as the timing
// owner policy. Reuse its already verified finite flag profile variant too.
#include "mcc_sound_vanilla_reference_profiles.inc"
MccSoundRuntimePlan base_projection(const MccSoundVanillaSource&source,const MccSoundVanillaSource&s,const MccSoundRuntimePlan&actual,bool classes){
    auto out=plan_mcc_sound_runtime_delivery(s.metadata,actual.address);const auto flags=mcc_sound_reference_detail::plan(s,raw_globals_sound_flag_profiles());
    require(actual.authored_flags||flags.corrections.empty(),"Stored-time native plan omitted vanilla flags");
    if(actual.authored_flags){out.authored_flags=std::make_shared<const MccSoundVanillaFlagPlan>(flags);for(const auto&c:flags.corrections){auto d=std::ranges::find(out.sounds,c.sound,&MccSoundRuntimeDefinition::sound);require(d!=out.sounds.end()&&d->native_flags==c.native_before,"Stored-time vanilla flags lost definition");d->native_flags=c.native_after;auto p=std::ranges::find(out.patches,c.source_root,&MccSoundRuntimePatch::source_address);require(p!=out.patches.end()&&p->owner==c.sound&&Bytes(p->after).u16(0)==c.native_before,"Stored-time vanilla flags lost root preimage");word(p->after,0,c.native_after,2);}}
    if(classes)out=mcc_sound_class_detail::stage_qualified(source.metadata,out);
    // The generic raw-bank contract remains supported. A supplied finite
    // permutation-time bank must reconstruct that COMPLETE authenticated
    // stage before stored root milliseconds are considered.
    if(actual.bytes!=out.bytes){
        const auto permutation_times=classes?mcc_sound_permutation_time_detail::plan_class_qualified(source,s,out):
            plan_mcc_sound_permutation_times(s,out);
        mcc_sound_permutation_time_detail::apply(out,classes?source.metadata:s.metadata,permutation_times);
    }
    return out;
}
void native_root_owners(const MccSoundVanillaSource&source,const MccSoundRuntimePlan&native){
    require(native.address&&native.address!=0xffffffffu&&native.address%4==0&&std::uint64_t(native.address)+native.bytes.size()<=0x100000000ull,"Stored-time merged bank extent is invalid");
    std::vector<std::pair<std::uint32_t,unsigned>>prefixes;
    for(const auto&t:source.metadata.tags)if(t.address&&(t.group=="snd!"||t.group=="ugh!"||t.group=="matg"||t.group=="sncl")){
        const auto root=source.metadata.relocate(t.address,t.size);
        const unsigned size=t.group=="snd!"?20u:t.group=="ugh!"?88u:t.group=="matg"?0xc8u:8u;
        require(root&&root!=0xffffffffu&&std::uint64_t(root)+t.size<=0x100000000ull&&std::uint64_t(root)+size<=0x100000000ull,"Stored-time typed native root extent is invalid");
        require(root==source.metadata.relocate(t.address,size),"Stored-time complete tag/prefix relocations disagree");
        require(std::uint64_t(root)+size<=native.address||std::uint64_t(native.address)+native.bytes.size()<=root,"Stored-time native root aliases the merged bank");
        prefixes.emplace_back(root,size);
    }
    std::ranges::sort(prefixes);
    for(std::size_t i=1;i<prefixes.size();++i)require(std::uint64_t(prefixes[i-1].first)+prefixes[i-1].second<=prefixes[i].first,"Stored-time separately owned native root prefixes overlap");
}
}
struct MccSoundStoredTimePlan::Impl {
    std::vector<MccSoundStoredTimeCorrection>corrections;
    std::vector<MccSoundStoredTimeResult>sounds;
    std::vector<MccSoundVanillaOwnerResult>owners;
};
struct MccSoundStoredTimeAccess {
    using Plan=MccSoundStoredTimePlan::Impl;
    static MccSoundStoredTimePlan plan(std::shared_ptr<const Plan>p){return MccSoundStoredTimePlan(std::move(p));}
};
MccSoundStoredTimePlan::MccSoundStoredTimePlan(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
std::span<const MccSoundStoredTimeCorrection>MccSoundStoredTimePlan::corrections()const noexcept{return impl_->corrections;}
std::span<const MccSoundStoredTimeResult>MccSoundStoredTimePlan::sounds()const noexcept{return impl_->sounds;}
std::span<const MccSoundVanillaOwnerResult>MccSoundStoredTimePlan::owners()const noexcept{return impl_->owners;}
namespace mcc_sound_stored_time_detail {
Digest path_key(std::string_view path){Hash h;constexpr char domain[]="H2_TU15_SOUND_STORED_TIME_KEY_V1";h.bytes(std::as_bytes(std::span(domain,sizeof(domain))));h.text(path);return h.finish();}
MccSoundStoredTimePlan build_impl(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,const MccSoundRuntimePlan&native,const Policy&policy,bool classes){
    Reader reader(source);const auto reference_values=values(policy.values);const auto reference_payloads=payloads(policy.payloads);auto out=std::make_shared<MccSoundStoredTimeAccess::Plan>();std::set<std::tuple<unsigned,unsigned,std::string,Digest>>unique;
    for(const auto&p:policy.owners)require(unique.emplace(p.handle.value,p.root,p.path,p.metadata).second,"Stored-time policy owner is duplicated");
    std::map<unsigned,unsigned>owner_indices;
    for(const auto&[_,t]:reader.tags)if(t->address&&t->group=="ugh!"){
        bool known=false,authenticated=false;Digest actual{};std::string reason;
        try{for(const auto&p:policy.owners)if(p.handle==t->handle()){known=true;if(p.root==t->address&&p.path==t->name){if(actual==Digest{})actual=mcc_sound_reference_detail::metadata_digest(fingerprint,t->handle());if(actual==p.metadata){require(!authenticated,"Stored-time owner has ambiguous profiles");authenticated=true;}}}}catch(const std::exception&e){reason=e.what();authenticated=false;}
        owner_indices.emplace(t->datum,out->owners.size());out->owners.push_back({t->handle(),authenticated?MccSoundVanillaOwnerStatus::Authenticated:known?MccSoundVanillaOwnerStatus::Custom:MccSoundVanillaOwnerStatus::Unmatched,authenticated?"Complete raw metadata owner matches":reason.empty()?"No exact raw metadata owner profile":reason});
    }
    struct Candidate {const Tag*tag;TagHandle gestalt;std::int32_t original,source;unsigned result;};std::vector<Candidate>candidates;std::set<unsigned>failed_resources;
    for(const auto&[_,t]:reader.tags)if(t->address&&t->group=="snd!"){
        const auto gestalt=source.metadata.gestalt_owner(t->handle());const auto owner=owner_indices.find(gestalt.value);const unsigned result=out->sounds.size();out->sounds.push_back({t->handle(),gestalt,Status::NoOriginalReference,"No finite original stored-time path"});
        if(t->name.empty())continue;
        const auto key=path_key(t->name);const auto value=std::ranges::lower_bound(reference_values,key,{},&Value::key);if(value==reference_values.end()||value->key!=key)continue;
        if(owner==owner_indices.end()||out->owners[owner->second].status!=MccSoundVanillaOwnerStatus::Authenticated){out->sounds.back().status=owner==owner_indices.end()||out->owners[owner->second].status==MccSoundVanillaOwnerStatus::Unmatched?Status::UnknownOwner:Status::CustomOwner;out->sounds.back().reason="Original stored time lacks exact complete source owner";continue;}
        try{
            const auto&g=reader.tag(gestalt,"ugh!");const auto raw=reader.read(t->address,28),root=reader.read(g.address,120);const Bytes snd(raw);const auto ranges=reader.block(root,40,12),permutations=reader.block(root,48,16),names=reader.block(root,24,4);const auto first=snd.u16(8);const auto count=std::to_integer<unsigned>(raw[10]);require(signed16(first)>=0&&count<128&&std::uint64_t(first)+count<=std::uint32_t(ranges.count),"Stored-time sound pitch interval escapes owner");
            std::set<Digest>seen;
            for(unsigned pitch=0;pitch<count;++pitch){const Bytes range(reader.row(ranges,first+pitch,12));const auto range_name=reader.name(names,range.u16(0));const auto pi=range.u16(8),pn=range.u16(10);require(signed16(pi)>=0&&signed16(pn)>0&&std::uint64_t(pi)+pn<=std::uint32_t(permutations.count),"Stored-time range permutation interval escapes owner");
                for(unsigned perm=0;perm<pn;++perm){const Bytes p(reader.row(permutations,pi+perm,16));const auto perm_name=reader.name(names,p.u16(0));const auto k=mcc_sound_timing_detail::named_key(t->name,range_name,perm_name);require(seen.insert(k).second,"Stored-time named permutation is ambiguous");const auto expected=std::ranges::lower_bound(reference_payloads,k,{},&Payload::key);require(expected!=reference_payloads.end()&&expected->key==k,"Stored-time owned permutation lacks finite payload authority");const auto actual=source.payload(t->handle(),pitch,perm);require(actual==payload_facts(reader,raw,root,p,expected->sha),"Stored-time actual encoded resource differs");}
            }
            candidates.push_back({t,gestalt,value->ms,std::bit_cast<std::int32_t>(snd.u32(16)),result});out->sounds.back().status=Status::Authenticated;out->sounds.back().reason="Complete source owner and every owned encoded permutation match";
        }catch(const std::exception&e){failed_resources.insert(gestalt.value);out->sounds.back().status=Status::EncodedResourceMismatch;out->sounds.back().reason=e.what();}
    }
    for(const auto h:failed_resources){auto&i=out->owners[owner_indices.at(h)];i.status=MccSoundVanillaOwnerStatus::Custom;i.reason="At least one known original sound lacks its complete encoded owner";}
    std::erase_if(candidates,[&](const auto&c){if(!failed_resources.contains(c.gestalt.value))return false;auto&r=out->sounds[c.result];r.status=Status::EncodedResourceMismatch;r.reason="Atomic source owner withheld after encoded mismatch";return true;});
    try{
        native_root_owners(source,native);
        auto base=base_projection(source,fingerprint,native,classes);auto restored=base;
        for(const auto&c:candidates){const auto definition=std::ranges::find(base.sounds,c.tag->handle(),&MccSoundRuntimeDefinition::sound);require(definition!=base.sounds.end()&&definition->gestalt==c.gestalt&&definition->source_root==c.tag->address&&definition->native_root==source.metadata.relocate(c.tag->address,20),"Stored-time native full-salt root differs");auto patch=std::ranges::find(restored.patches,c.tag->address,&MccSoundRuntimePatch::source_address);require(patch!=restored.patches.end()&&patch->owner==c.tag->handle()&&patch->after.size()==20&&Bytes(patch->after).u32(16)==std::bit_cast<unsigned>(c.source),"Stored-time native header preimage differs");word(patch->after,16,std::bit_cast<unsigned>(c.original));}
        const auto actual_digest=plan_digest(native);const bool before=actual_digest==plan_digest(base),after=actual_digest==plan_digest(restored);require(before||after,"Stored-time complete native projection is mixed or changed");
        for(const auto&c:candidates){auto&r=out->sounds[c.result];if(after||c.source==c.original){r.status=Status::AlreadyEquivalent;r.reason="Authenticated native stored milliseconds already equal TU1.5";continue;}
            const auto definition=std::ranges::find(base.sounds,c.tag->handle(),&MccSoundRuntimeDefinition::sound);const auto patch=std::ranges::find(base.patches,c.tag->address,&MccSoundRuntimePatch::source_address);MccSoundStoredTimeCorrection correction{c.tag->handle(),c.gestalt,c.tag->address,definition->native_root,c.source,c.original,{},{},{}};std::ranges::copy(patch->after,correction.native_header_before.begin());word(correction.before,0,std::bit_cast<unsigned>(c.source));word(correction.after,0,std::bit_cast<unsigned>(c.original));out->corrections.push_back(correction);
        }
    }catch(const std::exception&e){out->corrections.clear();for(auto&r:out->sounds)if(r.status==Status::Authenticated||r.status==Status::AlreadyEquivalent){r.status=Status::InvalidNativeProjection;r.reason=e.what();}}
    return MccSoundStoredTimeAccess::plan(out);
}
MccSoundStoredTimePlan build(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,
    const MccSoundRuntimePlan&native,const Policy&policy){return build_impl(source,fingerprint,native,policy,false);}
MccSoundStoredTimePlan build_class_qualified(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,
    const MccSoundRuntimePlan&native,const Policy&policy){return build_impl(source,fingerprint,native,policy,true);}
} // namespace mcc_sound_stored_time_detail
namespace {
#include "mcc_sound_stored_time_reference_profiles.inc"
mcc_sound_stored_time_detail::Policy canonical_policy(){return {stored_time_owners,std::as_bytes(std::span(stored_time_values,sizeof(stored_time_values)-1)),std::as_bytes(std::span(stored_time_payloads,sizeof(stored_time_payloads)-1))};}
}
MccSoundStoredTimePlan mcc_sound_stored_time_detail::plan_class_qualified(const MccSoundVanillaSource&source,
    const MccSoundVanillaSource&fingerprint,const MccSoundRuntimePlan&native){
    return build_class_qualified(source,fingerprint,native,canonical_policy());
}
MccSoundStoredTimePlan plan_mcc_sound_stored_times(const MccSoundVanillaSource&s,const MccSoundRuntimePlan&p){return mcc_sound_stored_time_detail::build(s,s,p,canonical_policy());}
MccSoundStoredTimePlan plan_mcc_sound_stored_times(const Cache&cache,const MccSoundRuntimePlan&p,const std::function<std::uint32_t(std::uint32_t,std::size_t)>&relocate){
    require(cache.is_mcc(),"Stored-time references require MCC ownership");const MccSoundVanillaSource source{{cache.tags(),[&](auto a,auto n){return cache.read_tag_bytes(a,n);},[&](TagHandle h){return cache.mcc_sound_gestalt_owner(h).handle();},relocate},[&](auto id){return cache.string_id(id);},[&](TagHandle h,unsigned pitch,unsigned permutation){const auto sound=cache.load_sound(h.value,pitch,permutation);MccSoundVanillaPayload out{digest(sound.data),sound.data.size(),sound.codec,sound.sample_rate,sound.channels,sound.samples,sound.sound_class,{}};for(const auto&chunk:sound.packet_chunks)out.packets.push_back({chunk.bytes,chunk.pcm_bytes,chunk.reset_after});return out;}};
    bool moved=false;for(const auto&t:cache.retained_tag_allocations())for(const auto&a:cache.tags())if(a.address&&a.group=="matg"&&(t.group=="matg"||t.datum==a.datum))moved=true;
    if(!moved)return mcc_sound_stored_time_detail::plan_class_qualified(source,source,p);
    try{const auto tags=mcc_sound_reference_detail::raw_globals_namespace(source,cache.retained_tag_allocations());auto fingerprint=source;fingerprint.metadata.tags=tags;return mcc_sound_stored_time_detail::plan_class_qualified(source,fingerprint,p);}
    catch(const FormatError&e){auto out=std::make_shared<MccSoundStoredTimeAccess::Plan>();for(const auto&t:cache.tags())if(t.address&&t.group=="ugh!")out->owners.push_back({t.handle(),MccSoundVanillaOwnerStatus::Custom,e.what()});for(const auto&t:cache.tags())if(t.address&&t.group=="snd!")out->sounds.push_back({t.handle(),{},Status::CustomOwner,e.what()});return MccSoundStoredTimeAccess::plan(out);}
}
} // namespace halo2
