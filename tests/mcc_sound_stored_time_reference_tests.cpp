#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/physics_contact_record.hpp"
#include "../src/audio/mcc_sound_timing_reference_profile.hpp"
#include "halo2/mcc_sound_stored_time_reference.hpp"
#include "../src/audio/mcc_sound_stored_time_reference_profile.hpp"
#include "../src/audio/mcc_sound_vanilla_reference_profile.hpp"
#include "../src/audio/mcc_sound_stored_time_apply.hpp"
#include <atomic>
#include <future>
#include <thread>
#include <openssl/evp.h>
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class T>void put(T& data,std::size_t at,std::uint32_t v,unsigned width=4) {
    Bytes(data).require(at,width);for(unsigned n=0;n<width;++n)data[at+n]=std::byte(v>>(n*8));
}
template<class F>void rejected(F&& f) {
    bool bad=false;try{f();}catch(const std::exception&){bad=true;}need(bad,"Malformed sound owner admitted");
}
std::int32_t i16(std::uint16_t x){return std::bit_cast<std::int16_t>(x);}
std::span<const std::byte> native(const MccSoundRuntimePlan& p,std::uint32_t at,std::size_t n) {
    need(at>=p.address,"Owned sound address precedes bank");return Bytes(p.bytes).slice(at-p.address,n);
}
const MccSoundRuntimePatch& patch(const MccSoundRuntimePlan& p,TagHandle tag) {
    const auto i=std::ranges::find(p.patches,tag,&MccSoundRuntimePatch::owner);
    need(i!=p.patches.end(),"Required sound patch missing");return *i;
}
struct Source {
    std::vector<Tag> tags;
    std::map<std::uint32_t,std::vector<std::byte>> memory;
    std::map<std::uint32_t,TagHandle> owners;
    std::uint32_t next=0x10000;
    std::uint32_t bank(std::size_t n) {
        const auto at=next;next+=std::uint32_t((n+15)&~15u)+16;memory[at].resize(n);return at;
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const {
        auto i=memory.upper_bound(at);if(i==memory.begin())throw FormatError("Unowned synthetic source");--i;
        return Bytes(i->second).slice(at-i->first,n);
    }
    void desc(std::uint32_t root,unsigned at,std::uint32_t count,std::uint32_t pointer) {
        put(memory.at(root),at,count);put(memory.at(root),at+4,pointer);
    }
    std::uint32_t table(std::uint32_t root,unsigned at,unsigned count,unsigned stride) {
        const auto b=bank(count*stride);desc(root,at,count,b);return b;
    }
    MccSoundRuntimeSource source()const {
        return {tags,[&](auto a,auto n){return read(a,n);},
            [&](TagHandle h){return owners.at(h.value);},
            [&](std::uint32_t at,std::size_t n){(void)read(at,n);return at+0x20000000u;}};
    }
    Source() {
        const auto add=[&](const char* group,unsigned index,unsigned size) {
            const auto at=bank(size);tags.push_back({group,std::string(group)+std::to_string(index),0xe1000000u+index,at,size});return at;
        };
        const auto matg=add("matg",1,0xc8),g1=add("ugh!",2,0x78),g2=add("ugh!",3,0x78),
            legacy=add("sncl",4,8),remastered=add("sncl",5,8),s1=add("snd!",6,0x1c),s2=add("snd!",7,0x1c);
        const auto globals=table(matg,0xc0,1,0x2c);
        put(memory[globals],4,tags[4].datum);put(memory[globals],12,tags[3].datum);put(memory[globals],0x28,tags[2].datum);
        const auto lc=table(legacy,0,1,0x78),rc=table(remastered,0,1,0x78);put(memory[lc],8,0x80,2);
        for(unsigned n=0;n<0x78;++n)if(n!=8&&n!=9){memory[lc][n]=std::byte(n*3);memory[rc][n]=std::byte(n*5);}
        for(unsigned j=0;j<2;++j) {
            const auto g=j?g2:g1,s=j?s2:s1;const auto codec=table(g,0,1,3);
            memory[codec]={std::byte(j?1:3),std::byte(j?0:1),std::byte(j?1:5)};
            table(g,8,1,0x38);table(g,0x10,1,0x14);
            const auto names=table(g,0x18,2,4);put(memory[names],0,0x11112222+j);put(memory[names],4,0x33334444+j);
            table(g,0x20,1,10);const auto range=table(g,0x28,1,12);
            memory[range][4]=std::byte{1};memory[range][5]=std::byte{0xff};put(memory[range],10,2,2);
            const auto perm=table(g,0x30,2,16);const auto custom=table(g,0x38,1,0x34);
            const auto mix=table(custom,0,1,8);put(memory[mix],0,7);put(memory[mix],4,0x3f123456);
            table(g,0x58,1,1);const auto languages=table(g,0x60,1,8),chunks=table(languages,0,2,16);
            for(unsigned n=0;n<2;++n) {
                const auto info=bank(8);desc(perm,n*16+8,1,info);put(memory[info],0,100000+n);
                put(memory[info],4,n,2);put(memory[info],6,1,2);
                put(memory[perm],n*16,n,2);put(memory[perm],n*16+2,0x4567,2);
                memory[perm][n*16+4]=std::byte{0xf5};memory[perm][n*16+5]=std::byte(n);
                put(memory[perm],n*16+6,0x4321,2);
                put(memory[chunks],n*16,0x1200+j*0x100+n*0x30);put(memory[chunks],n*16+4,0x80000000u|(n?0x20000000u:0u)|(36+n*20));
                put(memory[chunks],n*16+8,128+n*64);put(memory[chunks],n*16+12,0xffffffffu);
            }
            const auto promo=table(g,0x68,1,0x1c);const auto rules=table(promo,0,1,16);table(promo,8,1,4);
            put(memory[rules],0,0,2);put(memory[rules],2,2,2);put(memory[promo],0x10,0xffffffffu);
            const auto extra=table(g,0x70,1,0x14c);const auto section=table(extra,0,1,16),resources=table(extra,0x18,1,16);
            table(section,0,3,1);table(section,8,2,16);put(memory[resources],4,0x12345678);
            put(memory[extra],8,0x12300);put(memory[extra],12,0x800);put(memory[extra],16,32);
            put(memory[extra],0x20,j?tags[2].datum:tags[1].datum);put(memory[extra],0x24,0xfff8,2);
            for(unsigned n=0x2c;n<0x14c;++n)memory[extra][n]=std::byte(n);
            put(memory[s],0,j?0x800:0x400,2);put(memory[s],4,0x1234,2);memory[s][10]=std::byte{1};
            put(memory[s],16,0x76543210);for(unsigned n=20;n<28;++n)memory[s][n]=std::byte(n*9);
            owners[0xe1000006u+j]={0xe1000002u+j};
        }
    }
};
using namespace halo2::mcc_sound_timing_detail;
MccSoundVanillaDigest hash_bytes(std::span<const std::byte>s){MccSoundVanillaDigest h;unsigned n=0;need(EVP_Digest(s.data(),s.size(),reinterpret_cast<unsigned char*>(h.data()),&n,EVP_sha256(),nullptr)==1&&n==32,"Test SHA failed");return h;}
struct Fixture {
    Source s;
    std::map<unsigned,std::string> names;
    std::vector<EncodedSound> resources;
    std::vector<TimingOwner> owners;
    std::vector<std::byte> records;
    std::vector<MccSoundVanillaDigest> unavailable;
    Fixture(){
        s.tags.insert(s.tags.begin(),Tag{"","",0xffffffffu,0,0});
        for(unsigned index:{4u,5u}){const auto at=Bytes(s.read(s.tags[index].address,8)).u32(4);std::fill(s.memory[at].begin(),s.memory[at].end(),std::byte{});if(index==4)put(s.memory[at],8,0x80,2);}
        for(unsigned j=0;j<2;++j){names[0x11112222+j]="range"+std::to_string(j);names[0x33334444+j]="last"+std::to_string(j);
            const Bytes g(s.read(s.tags[2+j].address,120));const auto perm=g.u32(0x34);const auto extra=g.u32(0x74);std::fill(s.memory[extra].begin()+0x2c,s.memory[extra].end(),std::byte{});
            for(unsigned n=0;n<2;++n){const auto info=Bytes(s.read(perm+n*16,16)).u32(12);put(s.memory[info],0,100000+n*4);EncodedSound sound;sound.name=s.tags[6+j].name;sound.sample_rate=j?44100:48000;sound.channels=j?1:2;sound.codec=j?1:5;sound.samples=(100000+n*4)/(sound.channels*2);sound.sound_class=0;sound.data.resize(36+n*20,std::byte(j*16+n));sound.packet_chunks.push_back({36+n*20,128+n*64,bool(n)});resources.push_back(sound);}
        }
        auto source=input();for(unsigned j=0;j<2;++j){const auto&t=s.tags[2+j];owners.push_back({t.handle(),t.address,t.name,mcc_sound_reference_detail::metadata_digest(source,t.handle())});}
        std::vector<std::vector<std::byte>> rows;for(unsigned j=0;j<2;++j)for(unsigned n=0;n<2;++n){std::vector<std::byte> r(81);const auto key=named_key(s.tags[6+j].name,names.at(0x11112222+j),names.at(n?0x33334444+j:0x11112222+j));const auto payload=hash_bytes(resources[j*2+n].data);std::ranges::copy(key,r.begin());std::ranges::copy(payload,r.begin()+32);put(r,64,32000+j*100+n);r[68]=std::byte{1};r[69]=std::byte{0};r[70]=std::byte{1};rows.push_back(r);}std::ranges::sort(rows);for(const auto&r:rows)records.insert(records.end(),r.begin(),r.end());
    }
    MccSoundVanillaSource input()const{return {s.source(),[&](auto id){return names.at(id);},[&](TagHandle h,unsigned pitch,unsigned permutation){need(!pitch&&permutation<2,"Fixture actual getter interval");const auto&x=resources.at((h.value-0xe1000006u)*2+permutation);MccSoundVanillaPayload p{hash_bytes(x.data),x.data.size(),x.codec,x.sample_rate,x.channels,x.samples,x.sound_class,{}};for(const auto&c:x.packet_chunks)p.packets.push_back({c.bytes,c.pcm_bytes,c.reset_after});return p;}};}
    Policy policy()const{return {owners,records,unavailable};}
    MccSoundRuntimePlan native_plan()const{return plan_mcc_sound_runtime_delivery(s.source(),0x80000000);}
    MccSoundTimingPlan timing(const MccSoundRuntimePlan&p)const{auto source=input();return build(source,source,p,policy());}
};

struct StoredFixture {
    Fixture f;std::vector<halo2::mcc_sound_stored_time_detail::Owner>owners;
    std::vector<std::byte>value_records,payload_records;
    StoredFixture(){
        for(const auto&o:f.owners)owners.push_back({o.handle,o.root,o.path,o.metadata});
        std::vector<std::vector<std::byte>>values;
        for(unsigned i=0;i<2;++i){std::vector<std::byte>v(36);const auto key=halo2::mcc_sound_stored_time_detail::path_key(f.s.tags[6+i].name);std::ranges::copy(key,v.begin());put(v,32,1001+i*1001);values.push_back(v);}std::ranges::sort(values);for(const auto&v:values)value_records.insert(value_records.end(),v.begin(),v.end());
        for(std::size_t i=0;i<f.records.size();i+=81)payload_records.insert(payload_records.end(),f.records.begin()+i,f.records.begin()+i+64);
    }
    halo2::mcc_sound_stored_time_detail::Policy policy()const{return {owners,value_records,payload_records};}
    MccSoundStoredTimePlan plan(const MccSoundRuntimePlan&p)const{auto source=f.input();return halo2::mcc_sound_stored_time_detail::build(source,source,p,policy());}
};
void stored_time(){
    using StoredStatus=MccSoundStoredTimeStatus;StoredFixture x;const auto before=x.f.s.memory;const auto base=x.f.native_plan();const auto plan=x.plan(base);
    need(plan.corrections().size()==2&&plan.sounds().size()==2&&plan.owners().size()==2,"Known whole owners did not stage both stored-time words");
    for(const auto&r:plan.sounds())need(r.status==StoredStatus::Authenticated,"Known stored time not authenticated");
    auto after=base;unsigned changed_words=0;
    for(const auto&c:plan.corrections()){
        const auto i=std::ranges::find(after.patches,c.sound,&MccSoundRuntimePatch::owner);need(i!=after.patches.end()&&i->after.size()==20&&std::ranges::equal(i->after,c.native_header_before),"Full native header guard differs");need(c.native_root==c.source_root+0x20000000&&c.gestalt==x.f.s.owners.at(c.sound.value),"Stored correction lost exact root/gestalt");need(Bytes(c.before).u32(0)==Bytes(i->after).u32(16)&&Bytes(c.after).u32(0)==std::uint32_t(c.original_ms),"Stored correction word receipt differs");std::ranges::copy(c.after,i->after.begin()+16);++changed_words;
    }
    need(changed_words==2&&after.bytes==base.bytes&&x.f.s.memory==before,"Pure time plan changed source/bank outside roots");
    for(std::size_t i=0;i<base.patches.size();++i){const auto&a=base.patches[i],&b=after.patches[i];need(a.before==b.before,"Raw root preimage changed");for(unsigned n=0;n<a.after.size();++n)if(a.after[n]!=b.after[n])need(n>=16&&n<20&&a.owner==b.owner,"Stored restore changed a non-time field");}
    const auto ready=x.plan(after);need(ready.corrections().empty()&&std::ranges::all_of(ready.sounds(),[](const auto&r){return r.status==StoredStatus::AlreadyEquivalent;}),"Complete native restoration was not idempotent");
    unsigned controls=0;const auto invalid=[&](auto mutate){auto p=base;mutate(p);const auto bad=x.plan(p);need(bad.corrections().empty()&&std::ranges::all_of(bad.sounds(),[](const auto&r){return r.status==StoredStatus::InvalidNativeProjection;}),"Changed/mixed native plan accepted stored writes");++controls;};
    invalid([](auto&p){p.bytes[0]^=std::byte{1};});invalid([](auto&p){++p.sounds[0].native_root;});invalid([](auto&p){++p.sounds[0].sound.value;});invalid([](auto&p){++p.sounds[0].native_flags;});invalid([](auto&p){++p.chunks[0].encoded_bytes;});invalid([](auto&p){++p.gestalts[0].first[5];});invalid([](auto&p){p.native_pcm_duration.clear();});invalid([&](auto&p){const auto&c=plan.corrections()[0];auto q=std::ranges::find(p.patches,c.sound,&MccSoundRuntimePatch::owner);std::ranges::copy(c.after,q->after.begin()+16);});
    const auto bad_resource=[&](auto mutate){StoredFixture changed=x;mutate(changed);const auto bad=changed.plan(changed.f.native_plan());need(bad.corrections().size()==1&&bad.owners()[0].status==MccSoundVanillaOwnerStatus::Custom&&bad.owners()[1].status==MccSoundVanillaOwnerStatus::Authenticated,"Incomplete known sound was partially admitted or withheld independent owner");need(bad.corrections()[0].gestalt==changed.f.s.tags[3].handle(),"Wrong resource owner retained correction");++controls;};
    bad_resource([](auto&f){f.f.resources[0].data[0]^=std::byte{1};});bad_resource([](auto&f){++f.f.resources[1].samples;});bad_resource([](auto&f){++f.f.resources[0].codec;});bad_resource([](auto&f){++f.f.resources[0].sample_rate;});bad_resource([](auto&f){++f.f.resources[0].channels;});bad_resource([](auto&f){++f.f.resources[0].sound_class;});bad_resource([](auto&f){++f.f.resources[0].packet_chunks[0].bytes;});bad_resource([](auto&f){f.f.resources[0].packet_chunks[0].reset_after=!f.f.resources[0].packet_chunks[0].reset_after;});
    // Two distinct full-salt sound roots share one real gestalt/range. A
    // failure both before and after a successful sibling must withhold the
    // entire known-original domain of that owner, preserving its neighbour.
    for(unsigned failure=0;failure<2;++failure){
        StoredFixture siblings=x;const auto source_root=siblings.f.s.tags[6].address;
        const auto root=siblings.f.s.bank(28);siblings.f.s.memory[root]=siblings.f.s.memory.at(source_root);
        siblings.f.s.tags.push_back({"snd!","sibling",0xe1000008u,root,28});siblings.f.s.owners[0xe1000008u]=siblings.f.s.tags[2].handle();
        for(unsigned i=0;i<2;++i){auto resource=siblings.f.resources[i];resource.name="sibling";siblings.f.resources.push_back(std::move(resource));}
        std::vector<std::byte>value(36);const auto key=halo2::mcc_sound_stored_time_detail::path_key("sibling");std::ranges::copy(key,value.begin());put(value,32,3003);siblings.value_records.insert(siblings.value_records.end(),value.begin(),value.end());
        for(unsigned i=0;i<2;++i){std::vector<std::byte>row(64);const auto k=halo2::mcc_sound_timing_detail::named_key("sibling",siblings.f.names.at(0x11112222),siblings.f.names.at(i?0x33334444:0x11112222));std::ranges::copy(k,row.begin());const auto hash=hash_bytes(siblings.f.resources[4+i].data);std::ranges::copy(hash,row.begin()+32);siblings.payload_records.insert(siblings.payload_records.end(),row.begin(),row.end());}
        const auto sort_records=[](std::vector<std::byte>&bytes,unsigned stride){std::vector<std::vector<std::byte>>rows;for(std::size_t at=0;at<bytes.size();at+=stride)rows.emplace_back(bytes.begin()+at,bytes.begin()+at+stride);std::ranges::sort(rows);bytes.clear();for(const auto&r:rows)bytes.insert(bytes.end(),r.begin(),r.end());};sort_records(siblings.value_records,36);sort_records(siblings.payload_records,64);
        const auto input=siblings.f.input();for(auto&o:siblings.owners)o.metadata=halo2::mcc_sound_reference_detail::metadata_digest(input,o.handle);
        const auto valid=siblings.plan(siblings.f.native_plan());need(valid.corrections().size()==3,"Same-owner sibling positive fixture did not qualify all roots");
        siblings.f.resources[failure?4:0].data[0]^=std::byte{1};const auto rejected_owner=siblings.plan(siblings.f.native_plan());need(rejected_owner.corrections().size()==1&&rejected_owner.corrections()[0].gestalt==siblings.f.s.tags[3].handle()&&std::ranges::count(rejected_owner.sounds(),StoredStatus::EncodedResourceMismatch,&MccSoundStoredTimeResult::status)==2,"Encoded failure did not withhold earlier/later same-owner sibling");++controls;
    }
    StoredFixture metadata=x;put(metadata.f.s.memory[metadata.f.s.tags[6].address],16,123);const auto custom=metadata.plan(metadata.f.native_plan());need(custom.corrections().size()==1&&custom.owners()[0].status==MccSoundVanillaOwnerStatus::Custom,"Modified raw stored-ms field matched retail profile");++controls;
    StoredFixture missing=x;missing.value_records.erase(missing.value_records.begin(),missing.value_records.begin()+36);const auto omitted=missing.plan(base);need(omitted.corrections().size()==1&&std::ranges::count(omitted.sounds(),StoredStatus::NoOriginalReference,&MccSoundStoredTimeResult::status)==1,"Missing original value used PCM/current timing fallback");++controls;
    StoredFixture payload_missing=x;payload_missing.payload_records.erase(payload_missing.payload_records.begin(),payload_missing.payload_records.begin()+64);const auto partial=payload_missing.plan(base);need(partial.corrections().size()==1,"Missing named owned permutation was accepted");++controls;
    StoredFixture unknown=x;unknown.owners.clear();const auto unowned=unknown.plan(base);need(unowned.corrections().empty()&&std::ranges::all_of(unowned.sounds(),[](const auto&r){return r.status==StoredStatus::UnknownOwner;}),"Known name without whole-owner authority accepted");++controls;
    StoredFixture negative=x;put(negative.value_records,32,0xffffffff);rejected([&]{negative.plan(base);});++controls;
    StoredFixture duplicate=x;const std::vector<std::byte>duplicate_value(duplicate.value_records.begin(),duplicate.value_records.begin()+36);duplicate.value_records.insert(duplicate.value_records.end(),duplicate_value.begin(),duplicate_value.end());rejected([&]{duplicate.plan(base);});++controls;
    StoredFixture blank=x;blank.f.s.tags[6].name.clear();const auto unnamed=blank.plan(blank.f.native_plan());need(std::ranges::count(unnamed.sounds(),StoredStatus::NoOriginalReference,&MccSoundStoredTimeResult::status)==1,"Unknown empty name did not remain explicit unsupported");++controls;
    const auto reject_alias=[&](auto relocation,unsigned bank){auto source=x.f.input();source.metadata.relocate=relocation;const auto native=plan_mcc_sound_runtime_delivery(source.metadata,bank);const auto rejected=halo2::mcc_sound_stored_time_detail::build(source,source,native,x.policy());need(rejected.corrections().empty()&&std::ranges::all_of(rejected.sounds(),[](const auto&r){return r.status==StoredStatus::InvalidNativeProjection;}),"Aliased native root or inconsistent owner extent accepted stored writes");++controls;};
    const auto first=x.f.s.tags[6].address,second=x.f.s.tags[7].address,legacy=x.f.s.tags[4].address;
    reject_alias([&](unsigned a,std::size_t n){(void)x.f.s.read(a,n);return a+0x20000000u+(a==first&&n==20?4u:0u);},base.address);
    reject_alias([&](unsigned a,std::size_t n){(void)x.f.s.read(a,n);return a==second?first+0x20000004u:a+0x20000000u;},base.address);
    reject_alias([&](unsigned a,std::size_t n){(void)x.f.s.read(a,n);return a==first?base.address:a+0x20000000u;},base.address);
    reject_alias([&](unsigned a,std::size_t n){(void)x.f.s.read(a,n);return a==legacy?first+0x20000000u:a+0x20000000u;},base.address);
    need(x.f.s.memory==before&&x.f.native_plan().bytes==base.bytes,"Stored planner changed raw metadata/resources");std::cout<<"Stored-time native word plan passed: 2 full owners, exact4-byte-only restores, complete after idempotence, controls="<<controls<<'\n';
}
void atomic_publication(){
    StoredFixture x;const auto baseline=x.f.native_plan();const auto stored=x.plan(baseline);
    auto complete=baseline;
    halo2::mcc_sound_stored_time_detail::apply(complete,x.f.s.source(),stored);
    need(x.plan(complete).corrections().empty(),"Atomic publication did not produce full restored projection");
    unsigned controls=0;
    const auto reject=[&](auto change){
        auto target=baseline;auto source=x.f.s.source();change(target,source);
        const auto before=target;
        rejected([&]{halo2::mcc_sound_stored_time_detail::apply(target,source,stored);});
        need(target.bytes==before.bytes&&target.patches.size()==before.patches.size(),"Failed atomic publication changed its banks");
        for(unsigned i=0;i<target.patches.size();++i)need(target.patches[i].before==before.patches[i].before&&target.patches[i].after==before.patches[i].after,"Failed later guard partially applied earlier stored word");
        ++controls;
    };
    const auto last=stored.corrections().back();
    reject([&](auto&p,auto&){auto i=std::ranges::find(p.patches,last.sound,&MccSoundRuntimePatch::owner);i->after[0]^=std::byte{1};});
    reject([&](auto&p,auto&){auto i=std::ranges::find(p.patches,last.sound,&MccSoundRuntimePatch::owner);i->after[16]^=std::byte{1};});
    reject([&](auto&p,auto&){auto i=std::ranges::find(p.patches,last.sound,&MccSoundRuntimePatch::owner);i->before[16]^=std::byte{1};});
    reject([&](auto&p,auto&){auto i=std::ranges::find(p.patches,last.sound,&MccSoundRuntimePatch::owner);++i->owner.value;});
    reject([&](auto&p,auto&){auto i=std::ranges::find(p.sounds,last.sound,&MccSoundRuntimeDefinition::sound);++i->native_root;});
    reject([&](auto&p,auto&){auto i=std::ranges::find(p.sounds,last.sound,&MccSoundRuntimeDefinition::sound);++i->gestalt.value;});
    reject([&](auto&p,auto&){p.patches.push_back(*std::ranges::find(p.patches,last.sound,&MccSoundRuntimePatch::owner));});
    reject([&](auto&p,auto&){p.sounds.push_back(*std::ranges::find(p.sounds,last.sound,&MccSoundRuntimeDefinition::sound));});
    reject([&](auto&,auto&s){s.relocate=[&](unsigned a,std::size_t n){(void)x.f.s.read(a,n);return a+0x20000004u;};});
    reject([&](auto&,auto&s){s.gestalt_owner=[&](TagHandle){return x.f.s.tags[2].handle();};});
    for(unsigned mutation=0;mutation<3;++mutation){
        auto target=baseline;auto source=x.f.s.source();const auto read=source.read;bool changed=false;
        source.read=[&](unsigned at,std::size_t n){
            const auto bytes=read(at,n);
            if(at==last.source_root&&n==20&&!changed){
                changed=true;
                if(mutation==0)++std::ranges::find(target.patches,last.sound,&MccSoundRuntimePatch::owner)->owner.value;
                else if(mutation==1)++std::ranges::find(target.sounds,last.sound,&MccSoundRuntimeDefinition::sound)->native_root;
                else std::ranges::find(target.patches,stored.corrections().front().sound,&MccSoundRuntimePatch::owner)->after[0]^=std::byte{1};
            }
            return bytes;
        };
        rejected([&]{halo2::mcc_sound_stored_time_detail::apply(target,source,stored);});
        need(changed,"Late provider mutation control was not reached");
        for(unsigned i=0;i<target.patches.size();++i)if(target.patches[i].after.size()==20)
            need(std::ranges::equal(std::span(target.patches[i].after).subspan(16,4),std::span(baseline.patches[i].after).subspan(16,4)),"Late owner/preimage mutation partially published an earlier stored word");
        ++controls;
    }
    for(unsigned reallocation=0;reallocation<2;++reallocation){
        auto target=baseline;auto source=x.f.s.source();const auto read=source.read;bool moved=false;
        source.read=[&](unsigned at,std::size_t n){
            const auto bytes=read(at,n);
            if(at==last.source_root&&n==20&&!moved){moved=true;
                if(reallocation==0)target.patches.reserve(target.patches.capacity()+512);
                else target.sounds.reserve(target.sounds.capacity()+512);
            }
            return bytes;
        };
        halo2::mcc_sound_stored_time_detail::apply(target,source,stored);
        need(moved&&x.plan(target).corrections().empty(),"Harmless provider reallocation invalidated publication or changed its result");
        ++controls;
    }
    std::cout<<"Stored-time atomic native publication: full20B guards and "<<controls<<" mutation/owner/partial controls PASS\n";
}
} // namespace
int main()try{stored_time();atomic_publication();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
