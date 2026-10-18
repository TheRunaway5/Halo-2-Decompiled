#include "halo2/mcc_sound_permutation_time_reference.hpp"
#include "mcc_sound_permutation_time_reference_profile.hpp"
#include "mcc_sound_vanilla_reference_profile.hpp"
#include "mcc_sound_class_qualified.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <optional>
#include <openssl/evp.h>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
using Digest=MccSoundVanillaDigest;
using Status=MccSoundPermutationTimeStatus;
using mcc_sound_permutation_time_detail::Owner;
void require(bool value,const char*why){if(!value)throw FormatError(why);}
class Hash {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>ctx{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
    Hash(){require(ctx&&EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,"Permutation-time SHA initialization failed");}
    void bytes(std::span<const std::byte>s){require(s.empty()||EVP_DigestUpdate(ctx.get(),s.data(),s.size())==1,"Permutation-time SHA update failed");}
    void word(std::uint32_t x){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(x>>(i*8));bytes(b);}
    void wide(std::uint64_t x){word(std::uint32_t(x));word(std::uint32_t(x>>32));}
    void text(std::string_view s){require(!s.empty()&&s.size()<=0x100000,"Permutation-time identity is absent/excessive");word(std::uint32_t(s.size()));bytes(std::as_bytes(std::span(s.data(),s.size())));}
    Digest finish(){Digest out;unsigned n=0;require(EVP_DigestFinal_ex(ctx.get(),reinterpret_cast<unsigned char*>(out.data()),&n)==1&&n==32,"Permutation-time SHA finalization failed");return out;}
};
Digest digest(std::span<const std::byte>s){Hash h;h.bytes(s);return h.finish();}
std::uint32_t add(std::uint32_t a,std::uint64_t b){require(b<=0xffffffffull-a,"Permutation-time address wraps");return a+std::uint32_t(b);}
int signed16(unsigned x){return std::bit_cast<std::int16_t>(std::uint16_t(x));}
void word(std::span<std::byte>b,unsigned at,std::uint32_t x,unsigned n=4){Bytes(b).require(at,n);for(unsigned i=0;i<n;++i)b[at+i]=std::byte(x>>(8*i));}
struct Reader {
    const MccSoundVanillaSource&source;
    std::map<std::uint32_t,Tag>tags;
    std::vector<Tag> snapshot;
    explicit Reader(const MccSoundVanillaSource&s):source(s),snapshot(s.metadata.tags.begin(),s.metadata.tags.end()){
        require(s.metadata.read&&s.metadata.relocate&&s.metadata.gestalt_owner&&s.string_id&&s.payload,"Permutation-time needs real typed metadata/resource owners");
        require(snapshot.size()<=65536,"Permutation-time namespace exceeds source indices");
        for(std::size_t i=0;i<snapshot.size();++i){const auto&t=snapshot[i];if(t.handle().is_null())continue;require(t.handle().index()==i&&tags.emplace(t.datum,t).second,"Permutation-time namespace contains a foreign salt/index");}
    }
    std::vector<std::byte>read(std::uint32_t at,std::size_t n)const{require(at&&at!=0xffffffffu&&n<=128u*1024*1024&&std::uint64_t(at)+n<=0x100000000ull,"Permutation-time source extent is invalid");const auto b=source.metadata.read(at,n);require(b.size()==n,"Permutation-time source extent is partial");return {b.begin(),b.end()};}
    const Tag&tag(TagHandle h,std::string_view group)const{const auto i=tags.find(h.value);require(!h.is_null()&&i!=tags.end()&&i->second.address&&i->second.group==group,"Permutation-time lost a full-salt typed owner");return i->second;}
    TagBlock block(std::span<const std::byte>b,unsigned at,unsigned stride)const{const Bytes x(b);const auto n=x.u32(at),p=x.u32(at+4);require(n<=0x100000&&std::uint64_t(n)*stride<=128u*1024*1024,"Permutation-time block exceeds its domain");if(n){const auto extent=std::uint64_t(n)*stride;require(p&&p!=0xffffffffu&&std::uint64_t(p)+extent<=0x100000000ull,"Permutation-time block pointer is invalid");require(source.metadata.read(p,std::size_t(extent)).size()==extent,"Permutation-time block is partial");}return {std::int32_t(n),p};}
    std::vector<std::byte>row(const TagBlock&b,unsigned n,unsigned stride)const{require(n<std::uint32_t(b.count),"Permutation-time ordinal escapes its bank");return read(add(b.address,std::uint64_t(n)*stride),stride);}
    std::string name(const TagBlock&names,unsigned n)const{require(signed16(n)>=0,"Permutation-time import-name index is negative");const auto bytes=row(names,n,4);const auto value=source.string_id(Bytes(bytes).u32(0));require(!value.empty(),"Permutation-time import name is absent");return value;}
};
struct Value {Digest key,sha;std::uint16_t source,original;};
std::vector<Value> values(std::span<const std::byte>b){
    require(!b.empty()&&b.size()%68==0&&b.size()/68<=100000,"Permutation-time policy has invalid extent");
    std::vector<Value> out;for(std::size_t at=0;at<b.size();at+=68){Value v;
        std::ranges::copy(b.subspan(at,32),v.key.begin());std::ranges::copy(b.subspan(at+32,32),v.sha.begin());
        v.source=Bytes(b).u16(at+64);v.original=Bytes(b).u16(at+66);
        require(out.empty()||out.back().key<v.key,"Permutation-time policy is duplicated/unsorted");out.push_back(v);
    }return out;
}
MccSoundVanillaPayload payload_facts(const Reader&r,std::span<const std::byte>sound,std::span<const std::byte>g,const Bytes&perm,const Digest&sha){
    const auto codecs=r.block(g,0,3);const auto definition=project_mcc_sound_definition(sound,r.read(codecs.address,std::size_t(codecs.count)*3));
    const auto localized=r.block(perm.slice(0,16),8,8);require(localized.count>0,"Permutation-time permutation lacks English info");const auto info_bytes=r.row(localized,0,8);const Bytes info(info_bytes);
    const auto languages=r.block(g,0x60,8);require(languages.count>0,"Permutation-time gestalt lacks English chunks");const auto chunks=r.block(r.row(languages,0,8),0,16);const auto first=info.u16(4),count=info.u16(6);require(signed16(first)>=0&&signed16(count)>0&&std::uint64_t(first)+count<=std::uint32_t(chunks.count),"Permutation-time chunk interval escapes English owner");
    const std::array rates{22050u,44100u,32000u,48000u},channels{1u,2u,2u,4u};const auto codec=definition.codec;require(codec.sample_rate<rates.size()&&codec.encoding<channels.size(),"Permutation-time actual codec is unsupported");
    MccSoundVanillaPayload out{sha,0,codec.encoding==2?4u:codec.compression,rates[codec.sample_rate],channels[codec.encoding],0,std::to_integer<unsigned>(sound[2]),{}};
    const auto pcm=info.u32(0);require(out.codec<=5&&out.sound_class<54&&pcm&&(out.codec==4||pcm%(out.channels*2)==0),"Permutation-time actual PCM/container format differs");out.samples=out.codec==4?0:pcm/(out.channels*2);require(out.samples<=out.sample_rate*600ull,"Permutation-time resource exceeds bounded source extent");
    for(unsigned i=0;i<count;++i){const auto chunk_bytes=r.row(chunks,first+i,16);const Bytes chunk(chunk_bytes);const auto n=chunk.u32(4)&0x1fffffffu;require(n&&n<=128u*1024*1024-out.encoded_bytes,"Permutation-time encoded extent is invalid");out.encoded_bytes+=n;out.packets.push_back({n,chunk.u32(8),bool(chunk.u32(4)&0x20000000u)});}return out;
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
    require(actual.authored_flags||flags.corrections.empty(),"Permutation-time native plan omitted vanilla flags");
    if(actual.authored_flags){out.authored_flags=std::make_shared<const MccSoundVanillaFlagPlan>(flags);for(const auto&c:flags.corrections){auto d=std::ranges::find(out.sounds,c.sound,&MccSoundRuntimeDefinition::sound);require(d!=out.sounds.end()&&d->native_flags==c.native_before,"Permutation-time vanilla flags lost definition");d->native_flags=c.native_after;auto p=std::ranges::find(out.patches,c.source_root,&MccSoundRuntimePatch::source_address);require(p!=out.patches.end()&&p->owner==c.sound&&Bytes(p->after).u16(0)==c.native_before,"Permutation-time vanilla flags lost root preimage");word(p->after,0,c.native_after,2);}}
    if(classes)out=mcc_sound_class_detail::stage_qualified(source.metadata,out);
    return out;
}
void native_root_owners(const MccSoundVanillaSource&source,const MccSoundRuntimePlan&native){
    require(native.address&&native.address!=0xffffffffu&&native.address%4==0&&std::uint64_t(native.address)+native.bytes.size()<=0x100000000ull,"Permutation-time merged bank extent is invalid");
    std::vector<std::pair<std::uint32_t,unsigned>>prefixes;
    for(const auto&t:source.metadata.tags)if(t.address&&(t.group=="snd!"||t.group=="ugh!"||t.group=="matg"||t.group=="sncl")){
        const auto root=source.metadata.relocate(t.address,t.size);
        const unsigned size=t.group=="snd!"?20u:t.group=="ugh!"?88u:t.group=="matg"?0xc8u:8u;
        require(root&&root!=0xffffffffu&&std::uint64_t(root)+t.size<=0x100000000ull&&std::uint64_t(root)+size<=0x100000000ull,"Permutation-time typed native root extent is invalid");
        require(root==source.metadata.relocate(t.address,size),"Permutation-time complete tag/prefix relocations disagree");
        require(std::uint64_t(root)+size<=native.address||std::uint64_t(native.address)+native.bytes.size()<=root,"Permutation-time native root aliases the merged bank");
        prefixes.emplace_back(root,size);
    }
    std::ranges::sort(prefixes);
    for(std::size_t i=1;i<prefixes.size();++i)require(std::uint64_t(prefixes[i-1].first)+prefixes[i-1].second<=prefixes[i].first,"Permutation-time separately owned native root prefixes overlap");
}
}
struct MccSoundPermutationTimePlan::Impl {
    std::vector<MccSoundPermutationTimeCorrection> corrections;
    std::vector<MccSoundPermutationTimeResult> permutations;
    std::vector<MccSoundVanillaOwnerResult> owners;
    std::vector<Tag> source_tags;
    Digest before_digest{};
};
struct MccSoundPermutationTimeAccess {
    using Plan=MccSoundPermutationTimePlan::Impl;
    static MccSoundPermutationTimePlan plan(std::shared_ptr<const Plan>p){return MccSoundPermutationTimePlan(std::move(p));}
    static const Plan& get(const MccSoundPermutationTimePlan&p){return *p.impl_;}
};
MccSoundPermutationTimePlan::MccSoundPermutationTimePlan(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
std::span<const MccSoundPermutationTimeCorrection>MccSoundPermutationTimePlan::corrections()const noexcept{return impl_->corrections;}
std::span<const MccSoundPermutationTimeResult>MccSoundPermutationTimePlan::permutations()const noexcept{return impl_->permutations;}
std::span<const MccSoundVanillaOwnerResult>MccSoundPermutationTimePlan::owners()const noexcept{return impl_->owners;}
namespace mcc_sound_permutation_time_detail {
Digest named_key(std::string_view path,std::string_view range,std::string_view permutation){
    Hash h;constexpr char domain[]="H2_TU15_SOUND_TIMING_KEY_V1";
    h.bytes(std::as_bytes(std::span(domain,sizeof(domain))));h.text(path);h.text(range);h.text(permutation);return h.finish();
}
MccSoundPermutationTimePlan build_impl(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,
    const MccSoundRuntimePlan&native,const Policy&policy,bool classes){
    Reader reader(source);const auto reference=values(policy.records);auto out=std::make_shared<MccSoundPermutationTimeAccess::Plan>();out->source_tags=reader.snapshot;
    std::set<std::tuple<unsigned,unsigned,std::string,Digest>> unique;
    for(const auto&p:policy.owners)require(unique.emplace(p.handle.value,p.root,p.path,p.metadata).second,"Permutation-time policy owner is duplicated");
    std::map<unsigned,unsigned> owner_indices;
    for(const auto&[_,t]:reader.tags)if(t.address&&t.group=="ugh!"){
        bool known=false,authenticated=false;Digest actual{};std::string reason;
        try{for(const auto&p:policy.owners)if(p.handle==t.handle()){known=true;if(p.root==t.address&&p.path==t.name){
            if(actual==Digest{})actual=mcc_sound_reference_detail::metadata_digest(fingerprint,t.handle());
            if(actual==p.metadata){require(!authenticated,"Permutation-time owner has ambiguous profiles");authenticated=true;}
        }}}catch(const std::exception&e){reason=e.what();authenticated=false;}
        owner_indices.emplace(t.datum,out->owners.size());out->owners.push_back({t.handle(),authenticated?MccSoundVanillaOwnerStatus::Authenticated:known?MccSoundVanillaOwnerStatus::Custom:MccSoundVanillaOwnerStatus::Unmatched,
            authenticated?"Complete raw metadata owner matches":reason.empty()?"No exact raw metadata owner profile":reason});
    }
    struct Incoming {
        MccSoundPermutationTimeUser user;
        unsigned result{};
        std::array<std::byte,16> source;
        std::optional<Value> value;
    };
    using Target=std::pair<unsigned,unsigned>;
    std::map<Target,std::vector<Incoming>> incoming;
    std::set<unsigned> failed;
    // Enumerate ALL incoming sound/range references before selecting any
    // write, including unknown names and unmatched/custom owners.
    try{for(const auto&[_,t]:reader.tags)if(t.address&&t.group=="snd!"){
        const auto gestalt=source.metadata.gestalt_owner(t.handle());const auto&g=reader.tag(gestalt,"ugh!");
        const auto raw=reader.read(t.address,28),root=reader.read(g.address,120);const Bytes snd(raw);
        const auto ranges=reader.block(root,40,12),perms=reader.block(root,48,16),names=reader.block(root,24,4);
        const auto first=signed16(snd.u16(8)),count=int(std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[10])));
        require(count>=0&&(!count||(first>=0&&std::uint64_t(first)+count<=std::uint32_t(ranges.count))),"Permutation-time pitch interval escapes owner");
        std::set<Digest> seen;std::vector<std::pair<Target,unsigned>> sound_entries;bool has_reference=false,complete=true;
        for(int pitch=0;pitch<count;++pitch){
            const auto range_bytes=reader.row(ranges,first+pitch,12);const Bytes range(range_bytes);
            const auto pi=signed16(range.u16(8)),pn=signed16(range.u16(10));
            require(pn>=0&&(!pn||(pi>=0&&std::uint64_t(pi)+pn<=std::uint32_t(perms.count))),"Permutation-time permutation interval escapes owner");
            std::string range_name;try{range_name=reader.name(names,range.u16(0));}catch(const FormatError&){}
            for(int perm=0;perm<pn;++perm){
                Incoming entry;entry.result=out->permutations.size();entry.user.sound=t.handle();entry.user.source_root=t.address;
                entry.user.range=pitch;entry.user.permutation=perm;entry.user.source_range=first+pitch;entry.user.source_permutation=pi+perm;
                std::ranges::copy(raw,entry.user.source_header.begin());const auto bytes=reader.row(perms,pi+perm,16);std::ranges::copy(bytes,entry.source.begin());
                out->permutations.push_back({t.handle(),gestalt,unsigned(pitch),unsigned(perm),unsigned(pi+perm),Status::NoOriginalReference,"No finite original named permutation"});
                std::string permutation_name;try{permutation_name=reader.name(names,Bytes(bytes).u16(0));}catch(const FormatError&){}
                if(!t.name.empty()&&!range_name.empty()&&!permutation_name.empty()){
                    const auto key=named_key(t.name,range_name,permutation_name);const auto ref=std::ranges::lower_bound(reference,key,{},&Value::key);
                    if(ref!=reference.end()&&ref->key==key){entry.value=*ref;has_reference=true;if(!seen.insert(key).second)complete=false;}
                }
                if(!entry.value)complete=false;
                const Target target{gestalt.value,unsigned(pi+perm)};auto&users=incoming[target];sound_entries.emplace_back(target,users.size());users.push_back(std::move(entry));
            }
        }
        if(!has_reference)continue;
        const auto owner=owner_indices.find(gestalt.value);const bool authenticated=owner!=owner_indices.end()&&out->owners[owner->second].status==MccSoundVanillaOwnerStatus::Authenticated;
        if(!authenticated){for(const auto&[key,n]:sound_entries){const auto&e=incoming.at(key)[n];if(e.value){auto&r=out->permutations[e.result];r.status=owner==owner_indices.end()||out->owners[owner->second].status==MccSoundVanillaOwnerStatus::Unmatched?Status::UnknownOwner:Status::CustomOwner;r.reason="No exact complete source metadata owner";}}continue;}
        try{
            require(complete,"Known sound lacks complete unique named permutation authority");
            for(const auto&[key,n]:sound_entries){auto&e=incoming.at(key)[n];require(e.value&&Bytes(e.source).u16(6)==e.value->source,"Authored MCC permutation U16 differs from finite preimage");
                const auto actual=source.payload(t.handle(),e.user.range,e.user.permutation);
                require(actual==payload_facts(reader,raw,root,Bytes(e.source),e.value->sha),"Actual encoded permutation resource differs");
                auto&r=out->permutations[e.result];r.status=Status::Authenticated;r.reason="Complete source owner and owned encoded resources match";
            }
        }catch(const std::exception&e){failed.insert(gestalt.value);for(const auto&[key,n]:sound_entries){auto&r=out->permutations[incoming.at(key)[n].result];if(incoming.at(key)[n].value){r.status=Status::EncodedResourceMismatch;r.reason=e.what();}}}
    }}catch(const std::exception&e){
        for(auto&r:out->permutations){r.status=Status::InvalidNativeProjection;r.reason=e.what();}return MccSoundPermutationTimeAccess::plan(out);
    }
    // A physical row is writable only if EVERY incoming user qualifies the
    // same exact original value. No unknown/unnamed alias is ignored.
    std::set<unsigned> unsafe;
    for(const auto&[target,users]:incoming){
        const auto admitted=std::ranges::find_if(users,[&](const auto&e){return out->permutations[e.result].status==Status::Authenticated;});
        if(admitted==users.end())continue;
        for(const auto&e:users)if(!e.value||out->permutations[e.result].status!=Status::Authenticated||e.value->original!=admitted->value->original)unsafe.insert(target.first);
    }
    for(const auto h:failed){auto&i=out->owners[owner_indices.at(h)];i.status=MccSoundVanillaOwnerStatus::Custom;i.reason="Atomic encoded owner withheld after resource mismatch";}
    for(const auto h:unsafe){auto&i=out->owners[owner_indices.at(h)];i.status=MccSoundVanillaOwnerStatus::Custom;i.reason="Unknown or conflicting incoming permutation user";}
    for(auto&r:out->permutations)if(r.status==Status::Authenticated){if(failed.contains(r.gestalt.value)){r.status=Status::EncodedResourceMismatch;r.reason="Atomic source owner withheld";}else if(unsafe.contains(r.gestalt.value)){r.status=Status::UnsafeIncomingReference;r.reason="Atomic incoming owner withheld";}}
    try{
        // Provider callbacks may not change the proof's source namespace or
        // complete metadata after resource authentication.
        require(reader.snapshot.size()==source.metadata.tags.size(),"Permutation-time namespace changed during authentication");
        for(std::size_t i=0;i<reader.snapshot.size();++i){const auto&a=reader.snapshot[i],&b=source.metadata.tags[i];require(a.group==b.group&&a.name==b.name&&a.datum==b.datum&&a.address==b.address&&a.size==b.size,"Permutation-time namespace changed during authentication");}
        for(const auto&o:out->owners)if(o.status==MccSoundVanillaOwnerStatus::Authenticated){const auto actual=mcc_sound_reference_detail::metadata_digest(fingerprint,o.gestalt);require(std::ranges::any_of(policy.owners,[&](const auto&p){return p.handle==o.gestalt&&p.metadata==actual;}),"Permutation-time owner changed after resource authentication");}
        native_root_owners(source,native);auto base=base_projection(source,fingerprint,native,classes),restored=base;
        struct Ready {Target target;MccSoundPermutationTimeCorrection correction;};std::vector<Ready> ready;
        for(const auto&[target,users]:incoming){if(users.empty()||out->permutations[users.front().result].status!=Status::Authenticated)continue;
            const auto&first=users.front();const TagHandle gestalt{target.first};
            const auto g=std::ranges::find(base.gestalts,gestalt,&MccSoundRuntimeGestalt::owner);require(g!=base.gestalts.end()&&target.second<g->count[5],"Permutation-time native merged permutation owner differs");
            const auto index=add(g->first[5],target.second),address=add(base.blocks[5].address,std::uint64_t(index)*16);
            require(index<std::uint32_t(base.blocks[5].count)&&address>=base.address,"Permutation-time target escapes native bank");
            const auto native_row=Bytes(base.bytes).slice(address-base.address,16);MccSoundPermutationTimeCorrection c;
            c.gestalt=gestalt;c.source_index=target.second;c.native_index=index;c.native_address=address;
            const auto&tag=reader.tag(gestalt,"ugh!");const auto root=reader.read(tag.address,120);const auto perms=reader.block(root,48,16);
            c.source_address=add(perms.address,std::uint64_t(target.second)*16);c.source_ms=first.value->source;c.original_ms=first.value->original;
            c.source_before=first.source;std::ranges::copy(native_row,c.native_before.begin());word(c.before,0,c.source_ms,2);word(c.after,0,c.original_ms,2);
            require(Bytes(native_row).u16(6)==c.source_ms,"Permutation-time native U16 preimage differs");
            for(const auto&e:users){require(e.source==c.source_before,"Physical permutation has different source preimages");auto u=e.user;
                const auto d=std::ranges::find(base.sounds,u.sound,&MccSoundRuntimeDefinition::sound);
                require(d!=base.sounds.end()&&d->gestalt==gestalt&&d->source_root==u.source_root&&d->native_root==source.metadata.relocate(u.source_root,20),"Permutation-time source/native sound association differs");u.native_root=d->native_root;
                const auto p=std::ranges::find(base.patches,u.sound,&MccSoundRuntimePatch::owner);require(p!=base.patches.end()&&p->source_address==u.source_root&&p->after.size()==20,"Permutation-time native sound root missing");std::ranges::copy(p->after,u.native_header.begin());
                const auto native_range=add(g->first[4],u.source_range);require(native_range<std::uint32_t(base.blocks[4].count),"Permutation-time native range escapes merged owner");
                const auto range=Bytes(base.bytes).slice(base.blocks[4].address-base.address+std::uint64_t(native_range)*12,12);
                require(signed16(Bytes(u.native_header).u16(8))+u.range==native_range&&signed16(Bytes(range).u16(8))+u.permutation==index&&u.permutation<Bytes(range).u16(10),"Permutation-time merged range/permutation ordinals differ");c.users.push_back(std::move(u));
            }
            word(restored.bytes,address-base.address+6,c.original_ms,2);ready.push_back({target,std::move(c)});
        }
        const auto actual=plan_digest(native);const bool before=actual==plan_digest(base),after=actual==plan_digest(restored);
        require(before||after,"Permutation-time native projection is mixed or changed");out->before_digest=plan_digest(base);
        for(auto&r:ready){const bool equal=after||r.correction.source_ms==r.correction.original_ms;
            for(const auto&e:incoming.at(r.target)){auto&p=out->permutations[e.result];p.status=equal?Status::AlreadyEquivalent:Status::Authenticated;p.reason=equal?"Authenticated native authored time already equals TU1.5":"Exact native U16 correction staged";}
            if(!equal)out->corrections.push_back(std::move(r.correction));
        }
    }catch(const std::exception&e){out->corrections.clear();for(auto&r:out->permutations)if(r.status==Status::Authenticated||r.status==Status::AlreadyEquivalent){r.status=Status::InvalidNativeProjection;r.reason=e.what();}}
    return MccSoundPermutationTimeAccess::plan(out);
}
MccSoundPermutationTimePlan build(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,
    const MccSoundRuntimePlan&native,const Policy&policy){return build_impl(source,fingerprint,native,policy,false);}
MccSoundPermutationTimePlan build_class_qualified(const MccSoundVanillaSource&source,const MccSoundVanillaSource&fingerprint,
    const MccSoundRuntimePlan&native,const Policy&policy){return build_impl(source,fingerprint,native,policy,true);}
void apply(MccSoundRuntimePlan&native,const MccSoundRuntimeSource&source,const MccSoundPermutationTimePlan&plan){
    // A provider can reassign its caller's wrappers. Retain both the immutable
    // proof and the callable owners through the entire checked transaction.
    const auto retained=plan;const auto retained_source=source;
    const auto&proof=MccSoundPermutationTimeAccess::get(retained);if(proof.corrections.empty())return;
    require(retained_source.read&&retained_source.relocate&&retained_source.gestalt_owner,"Permutation-time publication lacks source owners");
    const std::vector<Tag> tags(retained_source.tags.begin(),retained_source.tags.end());
    require(tags.size()==proof.source_tags.size(),"Permutation-time publication source namespace differs");
    for(std::size_t i=0;i<tags.size();++i){const auto&a=tags[i],&b=proof.source_tags[i];require(a.group==b.group&&a.name==b.name&&a.datum==b.datum&&a.address==b.address&&a.size==b.size,"Permutation-time publication source identity differs");}
    // All external calls precede native iterators/indices and the final
    // complete-plan check. Harmless vector reserve by a provider is safe.
    for(const auto&c:proof.corrections){
        const auto row=retained_source.read(c.source_address,16);require(std::ranges::equal(row,c.source_before),"Permutation-time source row changed before publication");
        for(const auto&u:c.users){
            require(retained_source.gestalt_owner(u.sound)==c.gestalt&&retained_source.relocate(u.source_root,20)==u.native_root,"Permutation-time publication lost source owner");
            const auto raw=retained_source.read(u.source_root,28);require(std::ranges::equal(raw,u.source_header),"Permutation-time source header changed before publication");
        }
    }
    require(plan_digest(native)==proof.before_digest,"Permutation-time complete native preimage changed before publication");
    std::set<unsigned> targets;
    for(const auto&c:proof.corrections){
        require(targets.insert(c.native_address).second&&c.native_address==add(native.blocks[5].address,std::uint64_t(c.native_index)*16)&&c.native_index<std::uint32_t(native.blocks[5].count),"Permutation-time publication target is duplicated/foreign");
        require(std::ranges::equal(Bytes(native.bytes).slice(c.native_address-native.address,16),c.native_before),"Permutation-time native row changed before publication");
    }
    for(const auto&c:proof.corrections)std::ranges::copy(c.after,native.bytes.begin()+(c.native_address-native.address)+6);
}
} // namespace mcc_sound_permutation_time_detail
namespace {
#include "mcc_sound_permutation_time_reference_profiles.inc"
mcc_sound_permutation_time_detail::Policy canonical_policy(){return {permutation_time_owners,std::as_bytes(std::span(permutation_time_records,sizeof(permutation_time_records)-1))};}
}
MccSoundPermutationTimePlan mcc_sound_permutation_time_detail::plan_class_qualified(const MccSoundVanillaSource&source,
    const MccSoundVanillaSource&fingerprint,const MccSoundRuntimePlan&native){
    return build_class_qualified(source,fingerprint,native,canonical_policy());
}
MccSoundPermutationTimePlan plan_mcc_sound_permutation_times(const MccSoundVanillaSource&s,const MccSoundRuntimePlan&p){return mcc_sound_permutation_time_detail::build(s,s,p,canonical_policy());}
MccSoundPermutationTimePlan plan_mcc_sound_permutation_times(const Cache&cache,const MccSoundRuntimePlan&p,const std::function<std::uint32_t(std::uint32_t,std::size_t)>&relocate){
    require(cache.is_mcc(),"Permutation-time references require MCC ownership");
    const MccSoundVanillaSource source{{cache.tags(),[&](auto a,auto n){return cache.read_tag_bytes(a,n);},[&](TagHandle h){return cache.mcc_sound_gestalt_owner(h).handle();},relocate},[&](auto id){return cache.string_id(id);},[&](TagHandle h,unsigned pitch,unsigned permutation){const auto sound=cache.load_sound(h.value,pitch,permutation);MccSoundVanillaPayload out{digest(sound.data),sound.data.size(),sound.codec,sound.sample_rate,sound.channels,sound.samples,sound.sound_class,{}};for(const auto&chunk:sound.packet_chunks)out.packets.push_back({chunk.bytes,chunk.pcm_bytes,chunk.reset_after});return out;}};
    bool moved=false;for(const auto&t:cache.retained_tag_allocations())for(const auto&a:cache.tags())if(a.address&&a.group=="matg"&&(t.group=="matg"||t.datum==a.datum))moved=true;
    if(!moved)return mcc_sound_permutation_time_detail::plan_class_qualified(source,source,p);
    try{const auto tags=mcc_sound_reference_detail::raw_globals_namespace(source,cache.retained_tag_allocations());auto fingerprint=source;fingerprint.metadata.tags=tags;return mcc_sound_permutation_time_detail::plan_class_qualified(source,fingerprint,p);}
    catch(const FormatError&e){auto out=std::make_shared<MccSoundPermutationTimeAccess::Plan>();for(const auto&t:cache.tags())if(t.address&&t.group=="ugh!")out->owners.push_back({t.handle(),MccSoundVanillaOwnerStatus::Custom,e.what()});return MccSoundPermutationTimeAccess::plan(out);}
}
} // namespace halo2
