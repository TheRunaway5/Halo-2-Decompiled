#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/physics_contact_record.hpp"
#include "../src/audio/mcc_sound_timing_reference_profile.hpp"
#include "../src/audio/mcc_sound_vanilla_reference_profile.hpp"
#include "../src/audio/mcc_sound_permutation_time_reference_profile.hpp"
#include <atomic>
#include <future>
#include <thread>
#include <openssl/evp.h>
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <set>
#include <ranges>

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
struct Memory final:SourcePhysicsContactMemory {
    std::map<unsigned,std::vector<std::byte>> banks;mutable std::atomic<unsigned> reads{};mutable std::atomic<unsigned>writes{};
    void read(unsigned a,std::span<std::byte>out)const override{++reads;auto i=banks.upper_bound(a);if(i==banks.begin())throw FormatError("Unowned timing fixture memory");--i;const auto b=Bytes(i->second).slice(a-i->first,out.size());std::ranges::copy(b,out.begin());}
    void write(unsigned,std::span<const std::byte>)override{++writes;throw FormatError("Timing lookup attempted a native write");}
    void scalar(unsigned a,unsigned x){banks[a].resize(4);put(banks[a],0,x);}
};
std::shared_ptr<Memory> memory(const Fixture&f,const MccSoundRuntimePlan&p){
    auto m=std::make_shared<Memory>();for(const auto&[a,b]:f.s.memory)m->banks[a+0x20000000]=b;m->banks[p.address]=p.bytes;
    for(const auto&x:p.patches){const auto at=x.source_address+0x20000000;need(m->banks.at(at).size()>=x.after.size(),"Fixture native patch owner");std::ranges::copy(x.after,m->banks.at(at).begin());}
    const auto matg=f.s.tags[1].address;const auto child=Bytes(f.s.read(matg,0xc8)).u32(0xc4);put(m->banks.at(matg+0x20000000),0xc4,child+0x20000000);const auto row=project_mcc_sound_globals(f.s.read(child,44));std::ranges::copy(row,m->banks.at(child+0x20000000).begin());
    const unsigned index=0x700000,table=0x710000;m->banks[index].resize(32);put(m->banks[index],8,table);put(m->banks[index],16,f.s.tags[1].datum);put(m->banks[index],24,f.s.tags.size());put(m->banks[index],28,0x74616773);m->banks[table].resize(f.s.tags.size()*16);
    for(unsigned i=0;i<f.s.tags.size();++i){const auto&t=f.s.tags[i];put(m->banks[table],i*16,t.handle().is_null()?0xffffffffu:t.type().value);put(m->banks[table],i*16+4,t.datum);put(m->banks[table],i*16+8,t.address?t.address+0x20000000:0);put(m->banks[table],i*16+12,t.size);}
    m->scalar(0x556d90,index);m->scalar(0x4f0414,table);m->scalar(0x52d664,f.s.tags[3].address+0x20000000);m->banks[0x720000].resize(12);return m;
}
MccSoundTimingEntry entry(Memory&m,const MccSoundTimingBinding&b,const MccSoundRuntimePlan&p){MccSoundTimingEntry e{0x21f590,0x21f6f8};e.eax=Bytes(native(p,b.native_permutation,16)).u32(8);e.ebx=Bytes(m.banks.at(0x52d664)).u32(0);e.ecx=b.native_permutation-p.blocks[5].address;e.edx=p.blocks[5].address;e.esi=b.native_root;e.edi=b.native_range;e.esp=0x720000;e.stack={e.return_address,e.edi,e.eax};for(unsigned i=0;i<3;++i)put(m.banks[e.esp],i*4,e.stack[i]);return e;}
MccSoundTimingLookup checked_lookup(const MccSoundTimingBoundPlan&b,const std::shared_ptr<const MccSoundTimingMemoryOwner>&o,const MccSoundTimingEntry&e,const MccSoundTimingResourceProof&r){const auto result=lookup_mcc_sound_original_timing(b,o,e,r);need((result.status==MccSoundTimingLookupStatus::Reference)==bool(result.original),"Timing result admission/operand invariant differs");return result;}
void synthetic_timing(){
    const MccSoundTimingLookup empty;need(empty.status==MccSoundTimingLookupStatus::NoReference&&!empty.original,"Default timing lookup was admitted");
    Fixture f;const auto before=f.s.memory;const auto p=f.native_plan();const auto plan=f.timing(p);need(plan.bindings().size()==4&&plan.gaps().empty()&&plan.owners().size()==2,"Exact two-gestalt timing ownership was not admitted");for(const auto&o:plan.owners())need(o.status==MccSoundVanillaOwnerStatus::Authenticated,"Fixture owner not authenticated");need(f.s.memory==before,"Pure timing builder wrote raw bytes");
    unsigned negatives=0;const auto bad_plan=[&](auto mutate){auto changed=p;mutate(changed);const auto result=f.timing(changed);need(result.bindings().empty()&&!result.gaps().empty(),"Changed native projection retained timing bindings");++negatives;};
    bad_plan([](auto&x){x.bytes[0]^=std::byte{1};});bad_plan([](auto&x){++x.sounds[0].native_root;});bad_plan([](auto&x){++x.gestalts[1].first[5];});bad_plan([](auto&x){++x.blocks[5].address;});bad_plan([](auto&x){x.patches[0].after[0]^=std::byte{1};});bad_plan([](auto&x){++x.chunks[0].encoded_bytes;});bad_plan([](auto&x){x.native_pcm_duration.clear();});
    const auto owner_atomic=[&](auto mutate){auto changed=f;mutate(changed);const auto result=changed.timing(changed.native_plan());need(result.bindings().size()==2&&result.owners()[0].status==MccSoundVanillaOwnerStatus::Custom&&result.owners()[1].status==MccSoundVanillaOwnerStatus::Authenticated,"Modified owner was partially admitted or changed its independent sibling");++negatives;};
    owner_atomic([](auto&x){x.resources[1].data[0]^=std::byte{1};});owner_atomic([](auto&x){++x.resources[0].samples;});owner_atomic([](auto&x){++x.resources[0].packet_chunks[0].pcm_bytes;});owner_atomic([](auto&x){++x.resources[0].codec;});owner_atomic([](auto&x){++x.resources[0].sample_rate;});owner_atomic([](auto&x){++x.resources[0].channels;});owner_atomic([](auto&x){++x.resources[0].sound_class;});owner_atomic([](auto&x){x.resources[0].packet_chunks[0].reset_after=!x.resources[0].packet_chunks[0].reset_after;});owner_atomic([](auto&x){x.names[0x11112222]="foreign-range";});owner_atomic([](auto&x){put(x.s.memory[x.s.tags[6].address],16,7);});
    auto malformed=f;put(malformed.s.memory[malformed.s.tags[2].address],0x18,0x100001);const auto closed=malformed.timing(p);need(closed.bindings().empty()&&!closed.gaps().empty(),"Malformed owner did not remain an explicit closed timing gap");++negatives;
    const auto reject_projection_owner=[&](auto relocate,unsigned bank){auto source=f.input();source.metadata.relocate=relocate;const auto changed=plan_mcc_sound_runtime_delivery(source.metadata,bank);rejected([&]{(void)build(source,source,changed,f.policy());});++negatives;};
    // A complete projected bank does not authorize aliases among its separately
    // owned tag roots, or a relocation whose root changes with the requested size.
    const auto first_sound=f.s.tags[6].address,first_gestalt=f.s.tags[2].address;
    reject_projection_owner([&](unsigned a,std::size_t n){(void)f.s.read(a,n);return (a==first_gestalt?first_sound:a)+0x20000000u;},0x80000000);
    reject_projection_owner([&](unsigned a,std::size_t n){(void)f.s.read(a,n);return a+0x20000000u;},f.s.tags[1].address+0x20000000u);
    reject_projection_owner([&](unsigned a,std::size_t n){(void)f.s.read(a,n);return a+0x20000000u+(a==first_sound&&n==20?4u:0u);},0x80000000);
    auto excluded=f;excluded.unavailable.push_back(plan.bindings()[0].named_key);const auto partial=excluded.timing(p);need(partial.bindings().size()==3&&partial.gaps().size()==1&&partial.gaps()[0].kind==MccSoundTimingGapKind::NaturalPitchNotRestored,"Explicit natural pitch gap received a binding");
    auto unknown=f;unknown.owners.clear();const auto unmatched=unknown.timing(p);need(unmatched.bindings().empty()&&unmatched.owners()[0].status==MccSoundVanillaOwnerStatus::Unmatched,"Unknown owner received a name-only timing binding");
    auto m=memory(f,p);const auto memory_before=m->banks;auto owner=std::make_shared<MccSoundTimingMemoryOwner>(m);const auto bound=bind_mcc_sound_timing_memory(plan,owner);const auto&b=plan.bindings()[0];const unsigned ri=(b.sound.value-0xe1000006u)*2+b.relative_permutation;auto encoded=std::make_shared<EncodedSound>(f.resources.at(ri));const auto proof=authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);auto e=entry(*m,b,p);const auto got=checked_lookup(bound,owner,e,proof);need(got.status==MccSoundTimingLookupStatus::Reference&&got.original==b.original&&got.original->encoded_bytes!=e.eax,"Lookup did not return only the independent original tuple");
    const auto reject_entry=[&](auto mutate){auto changed=e;mutate(changed);need(checked_lookup(bound,owner,changed,proof).status!=MccSoundTimingLookupStatus::Reference,"Wrong caller/register identity received timing reference");++negatives;};
    reject_entry([](auto&x){--x.pc;});reject_entry([](auto&x){--x.return_address;});reject_entry([](auto&x){++x.stack[0];});reject_entry([](auto&x){++x.esi;});reject_entry([](auto&x){x.ecx+=1;});reject_entry([](auto&x){++x.edx;});reject_entry([](auto&x){++x.ebx;});reject_entry([](auto&x){++x.eax;++x.stack[2];});reject_entry([](auto&x){x.esp+=4;});
    auto data_before=encoded->data;encoded->data[0]^=std::byte{1};need(checked_lookup(bound,owner,e,proof).status==MccSoundTimingLookupStatus::StaleOrForeignResource,"Mutated retained encoded bytes were trusted");encoded->data=data_before;++negatives;
    const auto metadata_mutation=[&](unsigned address,unsigned offset){auto&bank=m->banks.at(address);bank[offset]^=std::byte{1};need(checked_lookup(bound,owner,e,proof).status!=MccSoundTimingLookupStatus::Reference,"Changed native metadata retained timing authority");bank[offset]^=std::byte{1};++negatives;};
    metadata_mutation(0x710000,b.sound.index()*16+4);metadata_mutation(b.native_root,0);metadata_mutation(p.address,b.native_range-p.address);metadata_mutation(p.address,b.native_permutation-p.address+8);metadata_mutation(0x52d664,0);
    // Original last/forced local selection and resource-cache handles are mutable.
    const auto ro=b.native_range-p.address;const auto old4=m->banks[p.address][ro+4],old5=m->banks[p.address][ro+5];m->banks[p.address][ro+4]=std::byte{0x7f};m->banks[p.address][ro+5]=std::byte{0};need(checked_lookup(bound,owner,e,proof).status==MccSoundTimingLookupStatus::Reference,"Mutable local selectors were misclassified as duration operands");m->banks[p.address][ro+4]=old4;m->banks[p.address][ro+5]=old5;
    const auto reads=m->reads.load();std::atomic<unsigned> wrong_thread{};std::thread worker([&]{if(checked_lookup(bound,owner,e,proof).status==MccSoundTimingLookupStatus::WrongThread)++wrong_thread;try{(void)bind_mcc_sound_timing_memory(plan,owner);}catch(const FormatError&){++wrong_thread;}try{(void)authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);}catch(const FormatError&){++wrong_thread;}try{owner->rebind(m);}catch(const FormatError&){++wrong_thread;}});worker.join();need(wrong_thread==4&&m->reads==reads,"Wrong-thread lease operation read native memory");
    auto foreign=std::make_shared<MccSoundTimingMemoryOwner>(m);need(checked_lookup(bound,foreign,e,proof).status==MccSoundTimingLookupStatus::StaleOrForeignOwner,"Foreign lease with the same numeric addresses was admitted");++negatives;
    owner->rebind(m);need(checked_lookup(bound,owner,e,proof).status==MccSoundTimingLookupStatus::StaleOrForeignOwner,"Identical-byte rebind retained an old epoch");const auto fresh=bind_mcc_sound_timing_memory(plan,owner);const auto new_proof=authenticate_mcc_sound_timing_resource(fresh,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);need(checked_lookup(fresh,owner,e,new_proof).status==MccSoundTimingLookupStatus::Reference,"Fresh epoch failed to resolve current canonical data");need(checked_lookup(fresh,owner,e,proof).status==MccSoundTimingLookupStatus::StaleOrForeignResource,"Old scope resource proof survived rebinding");
    const auto count=m->reads.load();std::thread teardown([&]{owner->invalidate();});teardown.join();need(checked_lookup(fresh,owner,e,new_proof).status==MccSoundTimingLookupStatus::StaleOrForeignOwner&&m->reads==count,"Invalidated owner read stale memory");need(m->writes==0&&f.s.memory==before,"Pure timing operations wrote source/native state");
    std::cout<<"Pure timing bindings/owner epochs passed; two gestalts, four references, wrong-thread reads=0, negatives="<<negatives<<'\n';
}
void transaction_timing(){
    Fixture f;const auto p=f.native_plan();const auto plan=f.timing(p);auto m=memory(f,p);
    auto owner=std::make_shared<MccSoundTimingMemoryOwner>(m);const auto bound=bind_mcc_sound_timing_memory(plan,owner);const auto&b=plan.bindings()[0];
    const auto index=(b.sound.value-0xe1000006u)*2+b.relative_permutation;auto encoded=std::make_shared<EncodedSound>(f.resources.at(index));
    const auto proof=authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);const auto e=entry(*m,b,p);const auto before=m->banks;
    unsigned called=0,controls=0;const auto execute=[&](const auto&args){need(args==b.original,"Transaction did not retain exact original operands");++called;};
    need(with_mcc_sound_original_timing(bound,owner,e,proof,execute)==MccSoundTimingLookupStatus::Reference&&called==1,"Admitted transaction did not execute exactly once");
    auto wrong=e;--wrong.pc;need(with_mcc_sound_original_timing(bound,owner,wrong,proof,execute)==MccSoundTimingLookupStatus::WrongCaller&&called==1,"Wrong entry executed callback");++controls;
    encoded->data[0]^=std::byte{1};need(with_mcc_sound_original_timing(bound,owner,e,proof,execute)==MccSoundTimingLookupStatus::StaleOrForeignResource&&called==1,"Changed resource executed callback");encoded->data[0]^=std::byte{1};++controls;
    const auto root_byte=m->banks.at(b.native_root)[0];m->banks.at(b.native_root)[0]^=std::byte{1};need(with_mcc_sound_original_timing(bound,owner,e,proof,execute)!=MccSoundTimingLookupStatus::Reference&&called==1,"Changed metadata executed callback");m->banks.at(b.native_root)[0]=root_byte;++controls;
    const auto count=m->reads.load();rejected([&]{(void)with_mcc_sound_original_timing(bound,owner,e,proof,{});});need(m->reads==count,"Absent callback read native memory");++controls;
    auto thread_status=MccSoundTimingLookupStatus::NoReference;std::thread wrong_thread([&]{thread_status=with_mcc_sound_original_timing(bound,owner,e,proof,execute);});wrong_thread.join();need(thread_status==MccSoundTimingLookupStatus::WrongThread&&m->reads==count&&called==1,"Wrong-thread transaction read or executed");++controls;
    bool propagated=false;try{(void)with_mcc_sound_original_timing(bound,owner,e,proof,[&](const auto&){throw std::runtime_error("original callback exception");});}catch(const std::runtime_error&x){propagated=std::string(x.what())=="original callback exception";}
    need(propagated&&checked_lookup(bound,owner,e,proof).status==MccSoundTimingLookupStatus::Reference,"Callback throw retained a locked/reentrant lease");++controls;
    need(with_mcc_sound_original_timing(bound,owner,e,proof,[&](const auto&){const auto reads=m->reads.load();need(checked_lookup(bound,owner,e,proof).status==MccSoundTimingLookupStatus::ReentrantLeaseOperation,"Nested lookup was admitted");need(with_mcc_sound_original_timing(bound,owner,e,proof,execute)==MccSoundTimingLookupStatus::ReentrantLeaseOperation,"Nested execution was admitted");rejected([&]{(void)bind_mcc_sound_timing_memory(plan,owner);});rejected([&]{(void)authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);});rejected([&]{owner->rebind(m);});need(m->reads==reads&&called==1,"Reentrant lease operation read or executed");})==MccSoundTimingLookupStatus::Reference,"Outer transaction did not finish after rejected nested operations");controls+=5;
    std::promise<void> started,finished;auto entered=started.get_future(),completed=finished.get_future();std::thread teardown;bool invalidation_waited=false;
    const auto transaction_status=with_mcc_sound_original_timing(bound,owner,e,proof,[&](const auto&args){need(args==b.original,"Locked callback operands changed");teardown=std::thread([&]{started.set_value();owner->invalidate();finished.set_value();});entered.wait();invalidation_waited=completed.wait_for(std::chrono::milliseconds(25))==std::future_status::timeout;});teardown.join();completed.get();need(transaction_status==MccSoundTimingLookupStatus::Reference&&invalidation_waited,"Teardown invalidation interleaved with active execution");++controls;
    const auto invalid_reads=m->reads.load();need(with_mcc_sound_original_timing(bound,owner,e,proof,execute)==MccSoundTimingLookupStatus::StaleOrForeignOwner&&m->reads==invalid_reads&&called==1,"Invalidated execution read or called back");++controls;
    need(m->writes==0&&m->banks==before,"Pure transactions changed actual metadata");std::cout<<"Timing execution transactions passed; teardown serialized, throw unlocks, controls="<<controls<<'\n';
}
void transaction_lifetime(){
    Fixture f;const auto p=f.native_plan();const auto plan=f.timing(p);auto m=memory(f,p);
    // The exact const shared_ptr type avoids a converting temporary that would
    // otherwise accidentally keep the mutex owner alive during the callback.
    std::shared_ptr<const MccSoundTimingMemoryOwner> owner=std::make_shared<MccSoundTimingMemoryOwner>(m);const auto bound=bind_mcc_sound_timing_memory(plan,owner);const auto&b=plan.bindings()[0];
    const auto index=(b.sound.value-0xe1000006u)*2+b.relative_permutation;auto encoded=std::make_shared<const EncodedSound>(f.resources.at(index));std::weak_ptr<const EncodedSound> weak_resource=encoded;
    std::optional<MccSoundTimingResourceProof> proof=authenticate_mcc_sound_timing_resource(bound,owner,b.sound,b.relative_pitch,b.relative_permutation,encoded);encoded.reset();std::weak_ptr<const MccSoundTimingMemoryOwner> weak_owner=owner;const auto e=entry(*m,b,p);
    need(with_mcc_sound_original_timing(bound,owner,e,*proof,[&](const auto&args){need(args==b.original,"Lifetime transaction changed operands");need(weak_owner.use_count()>=2,"Execution did not retain the mutex owner");owner.reset();proof.reset();need(!weak_owner.expired()&&!weak_resource.expired(),"Execution released its actual owner/resource before callback completed");})==MccSoundTimingLookupStatus::Reference,"Owner-reset transaction failed");
    need(!owner&&!proof&&weak_owner.expired()&&weak_resource.expired()&&m->writes==0,"Transaction did not release retained owner/resource after unlocking");
    std::cout<<"Execution retained exact const owner and actual resource across caller reset; released after unlock\n";
}

void held_timing_read(){
    // Timing-only software operands, not a World/native publication proof.
    auto m=std::make_shared<Memory>();m->scalar(0x556d90u,0x12345678u);
    auto owner=std::make_shared<MccSoundTimingMemoryOwner>(m);
    const auto lease=capture_mcc_sound_timing_read_lease(owner);
    const auto alias=capture_mcc_sound_timing_read_lease(owner);
    auto foreign_owner=std::make_shared<MccSoundTimingMemoryOwner>(m);
    const auto foreign=capture_mcc_sound_timing_read_lease(foreign_owner);
    unsigned callbacks{};const auto reads=m->reads.load();
    {
        const auto held=hold_mcc_sound_timing_read_lease(lease);
        need(held.matches(lease)&&held.matches(alias)&&!held.matches(foreign),
            "Held timing witness lost its exact owner/epoch identity");
        need(check_mcc_sound_timing_read_lease(lease)==MccSoundTimingReadStatus::ReentrantLeaseOperation&&
            with_mcc_sound_timing_owned_read(lease,[&](const auto&){++callbacks;})==
                MccSoundTimingReadStatus::ReentrantLeaseOperation,
            "Ordinary timing read reentered the held mutex");
        rejected([&]{(void)hold_mcc_sound_timing_read_lease(lease);});
        rejected([&]{(void)capture_mcc_sound_timing_read_lease(owner);});
        rejected([&]{owner->rebind(m);});
        bool foreign_matches=true,foreign_acquired=true;MccSoundTimingReadStatus status{};
        std::thread other([&]{
            foreign_matches=held.matches(lease);
            try{(void)hold_mcc_sound_timing_read_lease(lease);}catch(const std::exception&){foreign_acquired=false;}
            status=check_mcc_sound_timing_read_lease(lease);
        });other.join();
        need(!foreign_matches&&!foreign_acquired&&status==MccSoundTimingReadStatus::WrongThread,
            "Foreign producer accessed a held timing owner");
        need(held.matches(lease)&&m->reads==reads&&m->writes==0&&callbacks==0,
            "Held direct witnesses or rejected operations read/write metadata or dispatched a callback");
    }
    need(with_mcc_sound_timing_owned_read(lease,[&](const auto& reader){
        need(Bytes(reader.read(0x556d90u,4)).u32(0)==0x12345678u,"Released timing scope changed memory");++callbacks;
    })==MccSoundTimingReadStatus::Executed&&callbacks==1,
        "Held timing scope failed to unlock or clear its reentry guard");
    owner->rebind(m);rejected([&]{(void)hold_mcc_sound_timing_read_lease(lease);});
    const auto current=capture_mcc_sound_timing_read_lease(owner);
    std::promise<void> started,finished;auto entered=started.get_future(),done=finished.get_future();
    std::thread teardown;bool waited=false;
    {
        const auto held=hold_mcc_sound_timing_read_lease(current);
        teardown=std::thread([&]{started.set_value();owner->invalidate();finished.set_value();});
        entered.wait();waited=done.wait_for(std::chrono::milliseconds(25))==std::future_status::timeout;
    }
    teardown.join();done.get();need(waited,"Invalidation interleaved with the held timing mutex");
    rejected([&]{(void)hold_mcc_sound_timing_read_lease(current);});
    need(check_mcc_sound_timing_read_lease(current)==MccSoundTimingReadStatus::StaleOwner,
        "Invalidated timing owner retained eligibility");
    std::weak_ptr<MccSoundTimingMemoryOwner> weak;
    {
        auto transient=std::make_shared<MccSoundTimingMemoryOwner>(m);weak=transient;
        std::optional<MccSoundTimingReadLease> pending=capture_mcc_sound_timing_read_lease(transient);
        {
            const auto held=hold_mcc_sound_timing_read_lease(*pending);
            transient.reset();pending.reset();need(!weak.expired(),"Held timing scope lost its real mutex owner");
        }
    }
    need(weak.expired(),"Released timing scope retained its owner after unlocking");
    std::cout<<"Held timing-only scope passed exact owner/epoch, no-read direct witnesses, reentry/foreign/stale refusal, serialized invalidation and retained-owner lifetime\n";
}

void permutation_catalog_timing(){
    Fixture f;auto source=f.input();
    std::vector<mcc_sound_permutation_time_detail::Owner> owners;
    for(const auto&o:f.owners)owners.push_back({o.handle,o.root,o.path,o.metadata});
    std::vector<std::vector<std::byte>> rows;
    const std::array<unsigned,4> original{0,0x4321,1367,0x4321};
    for(unsigned j=0;j<2;++j)for(unsigned n=0;n<2;++n){
        std::vector<std::byte> row(68);
        const auto key=named_key(f.s.tags[6+j].name,f.names.at(0x11112222+j),f.names.at(n?0x33334444+j:0x11112222+j));
        const auto sha=hash_bytes(f.resources[j*2+n].data);
        std::ranges::copy(key,row.begin());std::ranges::copy(sha,row.begin()+32);
        put(row,64,0x4321,2);put(row,66,original[j*2+n],2);rows.push_back(row);
    }
    std::ranges::sort(rows);std::vector<std::byte> values;for(const auto&r:rows)values.insert(values.end(),r.begin(),r.end());
    const mcc_sound_permutation_time_detail::Policy policy{owners,values};
    const auto raw=f.native_plan();const auto proof=mcc_sound_permutation_time_detail::build(source,source,raw,policy);
    auto restored=raw;mcc_sound_permutation_time_detail::apply(restored,source.metadata,proof);
    const auto timing=build_with_permutation_times(source,source,restored,f.policy(),policy);
    need(timing.permutation_references().size()==4&&timing.bindings().size()==4,"Finite original permutation catalog not retained");
    need(timing.stored_references().empty(),"Permutation catalog invented stored+10 authority");
    unsigned controls=0;
    for(const auto&r:timing.permutation_references()){
        const auto j=r.sound.value-0xe1000006u,n=r.relative_permutation;
        need(j<2&&!r.relative_pitch&&n<2&&r.gestalt==f.s.tags[2+j].handle()&&
            r.source_root==f.s.tags[6+j].address&&r.native_root==r.source_root+0x20000000u&&
            r.source_range==0&&r.source_permutation==n&&r.original_ms==original[j*2+n],"Permutation catalog tuple/value differs");
        need(std::ranges::equal(r.expected_native_header,patch(restored,r.sound).after)&&
            std::ranges::equal(r.expected_native_range,native(restored,r.native_range,12))&&
            std::ranges::equal(r.expected_native_permutation,native(restored,r.native_permutation,16))&&
            Bytes(r.expected_native_permutation).u16(6)==r.original_ms,"Permutation catalog lost final20/12/16-byte guards");
        need(r.encoded_payload==hash_bytes(f.resources[j*2+n].data)&&r.named_key==named_key(f.s.tags[6+j].name,
            f.names.at(0x11112222+j),f.names.at(n?0x33334444+j:0x11112222+j)),"Permutation catalog named/payload authority differs");++controls;
    }
    const auto zero=std::ranges::find(timing.permutation_references(),0,&MccSoundTimingPermutationReference::original_ms);
    need(zero!=timing.permutation_references().end(),"Valid original zero was treated as unavailable");++controls;
    const auto base=build_with_permutation_times(source,source,raw,f.policy(),policy);
    need(base.permutation_references().size()==2&&std::ranges::all_of(base.permutation_references(),[](const auto&r){return r.original_ms==0x4321;}),
        "Generic raw plan authorized unrestored U16 values");++controls;
    unsigned getters=0;auto counting=source;const auto get=source.payload;
    counting.payload=[&](auto h,auto p,auto n){++getters;return get(h,p,n);};
    const auto counted=build_with_permutation_times(counting,counting,restored,f.policy(),policy);
    const auto before=getters;const auto retained=counted;
    for(unsigned i=0;i<1000;++i)need(retained.permutation_references().data()==counted.permutation_references().data()&&retained.permutation_references().size()==4,
        "Immutable permutation accessor rebuilt its catalog");need(getters==before,"Permutation accessor re-read encoded resources");++controls;
    for(unsigned at:{0u,6u,8u,15u}){auto broken=restored;const auto&row=timing.permutation_references()[0];broken.bytes[row.native_permutation-broken.address+at]^=std::byte{1};
        const auto rejected=build_with_permutation_times(source,source,broken,f.policy(),policy);
        need(rejected.bindings().empty()&&rejected.permutation_references().empty(),"Changed native16-byte row retained timing admission");++controls;}
    auto mixed=restored;const auto&row=timing.permutation_references()[0];put(mixed.bytes,row.native_permutation-mixed.address+6,0x4321,2);
    const auto invalid=build_with_permutation_times(source,source,mixed,f.policy(),policy);
    need(invalid.permutation_references().empty(),"Mixed native authored set retained catalog");++controls;
    auto absent=policy;absent.owners={};const auto unknown=build_with_permutation_times(source,source,raw,f.policy(),absent);
    need(unknown.permutation_references().empty()&&unknown.bindings().size()==4,"Unknown permutation policy borrowed encoded timing authority");++controls;
    auto late=source;unsigned calls=0;late.payload=[&](auto h,auto p,auto n){auto value=get(h,p,n);if(++calls>4&&h==f.s.tags[6].handle()&&n==1)value.sha256[0]^=std::byte{1};return value;};
    // Baseline proof first authenticates all four payloads. The later timing
    // qualification must remove the ENTIRE failed owner, including equal rows.
    const auto withdrawn=build_with_permutation_times(late,late,raw,f.policy(),policy);
    need(calls>=6&&withdrawn.permutation_references().size()==1&&withdrawn.permutation_references()[0].gestalt==f.s.tags[3].handle()&&withdrawn.bindings().size()==2&&
        withdrawn.owners()[0].status==MccSoundVanillaOwnerStatus::Custom&&withdrawn.owners()[1].status==MccSoundVanillaOwnerStatus::Authenticated,
        "Late resource mismatch retained an earlier admitted permutation owner");++controls;
    auto excluded=f.policy();excluded.unavailable_pitch=std::span<const MccSoundVanillaDigest>(&zero->named_key,1);
    const auto independent=build_with_permutation_times(source,source,restored,excluded,policy);
    need(independent.bindings().size()==3&&independent.permutation_references().size()==4,
        "Encoded natural-pitch exclusion contaminated independent authored U16 authority");++controls;
    auto reordered=f;const auto permutation_bank=Bytes(reordered.s.read(reordered.s.tags[2].address,120)).u32(52);
    auto&permutation_rows=reordered.s.memory.at(permutation_bank);
    for(unsigned i=0;i<16;++i)std::swap(permutation_rows[i],permutation_rows[16+i]);
    std::swap(reordered.resources[0],reordered.resources[1]);auto reordered_source=reordered.input();
    std::vector<mcc_sound_permutation_time_detail::Owner> reordered_owners;
    for(auto&o:reordered.owners){o.metadata=mcc_sound_reference_detail::metadata_digest(reordered_source,o.handle);reordered_owners.push_back({o.handle,o.root,o.path,o.metadata});}
    const mcc_sound_permutation_time_detail::Policy reordered_policy{reordered_owners,values};
    const auto reordered_base=reordered.native_plan();const auto reordered_proof=mcc_sound_permutation_time_detail::build(reordered_source,reordered_source,reordered_base,reordered_policy);
    auto reordered_native=reordered_base;mcc_sound_permutation_time_detail::apply(reordered_native,reordered_source.metadata,reordered_proof);
    const auto reordered_timing=build_with_permutation_times(reordered_source,reordered_source,reordered_native,reordered.policy(),reordered_policy);
    need(reordered_timing.permutation_references().size()==4,"Named reorder lost complete admitted catalog");
    for(const auto&r:reordered_timing.permutation_references())if(r.sound==reordered.s.tags[6].handle())need(r.original_ms==(r.relative_permutation?0:0x4321),"Catalog restored original time by ordinal instead of named identity");++controls;
    auto wrong_records=f.records;const auto key=zero->named_key;const auto at=std::ranges::find_if(std::views::iota(std::size_t(0),wrong_records.size()/81),[&](auto n){return std::ranges::equal(std::span(wrong_records).subspan(n*81,32),key);});
    need(at!=std::ranges::end(std::views::iota(std::size_t(0),wrong_records.size()/81)),"Fixture exclusion key absent");
    wrong_records[*at*81+32]^=std::byte{1};auto wrong_timing=excluded;wrong_timing.records=wrong_records;
    const auto inconsistent=build_with_permutation_times(source,source,restored,wrong_timing,policy);
    need(inconsistent.permutation_references().empty()&&inconsistent.bindings().empty(),"Inconsistent finite payload policies borrowed excluded timing authority");++controls;
    need(sizeof(MccSoundTimingPlan)==16&&sizeof(MccSoundRuntimePlan)==sizeof(decltype(raw)),"Immutable public plan layout changed");
    std::cout<<"Distinct permutation catalog passed: four named tuples, original zero, raw equal-only subset, final20/12/16B, no query payload reads; controls="<<controls<<'\n';
}
} // namespace
int main()try{synthetic_timing();transaction_timing();transaction_lifetime();held_timing_read();permutation_catalog_timing();}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
