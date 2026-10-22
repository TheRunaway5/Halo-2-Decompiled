#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/mcc_sound_permutation_time_reference.hpp"
#include "../src/audio/mcc_sound_permutation_time_reference_profile.hpp"
#include "../src/audio/mcc_sound_vanilla_reference_profile.hpp"
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
MccSoundVanillaDigest hash_bytes(std::span<const std::byte>s){MccSoundVanillaDigest h;unsigned n=0;need(EVP_Digest(s.data(),s.size(),reinterpret_cast<unsigned char*>(h.data()),&n,EVP_sha256(),nullptr)==1&&n==32,"Test SHA failed");return h;}
struct Fixture {
    Source s;
    std::map<unsigned,std::string> names;
    std::vector<EncodedSound> resources;
    std::vector<halo2::mcc_sound_permutation_time_detail::Owner> owners;
    std::vector<std::byte> records;
    
    Fixture(){
        s.tags.insert(s.tags.begin(),Tag{"","",0xffffffffu,0,0});
        for(unsigned index:{4u,5u}){const auto at=Bytes(s.read(s.tags[index].address,8)).u32(4);std::fill(s.memory[at].begin(),s.memory[at].end(),std::byte{});if(index==4)put(s.memory[at],8,0x80,2);}
        for(unsigned j=0;j<2;++j){names[0x11112222+j]="range"+std::to_string(j);names[0x33334444+j]="last"+std::to_string(j);
            const Bytes g(s.read(s.tags[2+j].address,120));const auto perm=g.u32(0x34);const auto extra=g.u32(0x74);std::fill(s.memory[extra].begin()+0x2c,s.memory[extra].end(),std::byte{});
            for(unsigned n=0;n<2;++n){const auto info=Bytes(s.read(perm+n*16,16)).u32(12);put(s.memory[info],0,100000+n*4);EncodedSound sound;sound.name=s.tags[6+j].name;sound.sample_rate=j?44100:48000;sound.channels=j?1:2;sound.codec=j?1:5;sound.samples=(100000+n*4)/(sound.channels*2);sound.sound_class=0;sound.data.resize(36+n*20,std::byte(j*16+n));sound.packet_chunks.push_back({36+n*20,128+n*64,bool(n)});resources.push_back(sound);}
        }
        auto source=input();for(unsigned j=0;j<2;++j){const auto&t=s.tags[2+j];owners.push_back({t.handle(),t.address,t.name,mcc_sound_reference_detail::metadata_digest(source,t.handle())});}
        std::vector<std::vector<std::byte>> rows;for(unsigned j=0;j<2;++j)for(unsigned n=0;n<2;++n){std::vector<std::byte> r(68);const auto key=halo2::mcc_sound_permutation_time_detail::named_key(s.tags[6+j].name,names.at(0x11112222+j),names.at(n?0x33334444+j:0x11112222+j));const auto payload=hash_bytes(resources[j*2+n].data);std::ranges::copy(key,r.begin());std::ranges::copy(payload,r.begin()+32);put(r,64,0x4321,2);put(r,66,1000+j*100+n,2);rows.push_back(r);}std::ranges::sort(rows);for(const auto&r:rows)records.insert(records.end(),r.begin(),r.end());
    }
    MccSoundVanillaSource input()const{return {s.source(),[&](auto id){return names.at(id);},[&](TagHandle h,unsigned pitch,unsigned permutation){need(!pitch&&permutation<2,"Fixture actual getter interval");const auto&x=resources.at((h.value-0xe1000006u)*2+permutation);MccSoundVanillaPayload p{hash_bytes(x.data),x.data.size(),x.codec,x.sample_rate,x.channels,x.samples,x.sound_class,{}};for(const auto&c:x.packet_chunks)p.packets.push_back({c.bytes,c.pcm_bytes,c.reset_after});return p;}};}
    halo2::mcc_sound_permutation_time_detail::Policy policy()const{return {owners,records};}
    MccSoundRuntimePlan native_plan()const{return plan_mcc_sound_runtime_delivery(s.source(),0x80000000);}
    MccSoundPermutationTimePlan plan(const MccSoundRuntimePlan&p)const{auto source=input();return halo2::mcc_sound_permutation_time_detail::build(source,source,p,policy());}
    void refresh(){auto source=input();for(auto&o:owners)o.metadata=mcc_sound_reference_detail::metadata_digest(source,o.handle);}
};
void sort_records(std::vector<std::byte>&data){std::vector<std::vector<std::byte>>rows;for(std::size_t at=0;at<data.size();at+=68)rows.emplace_back(data.begin()+at,data.begin()+at+68);std::ranges::sort(rows);data.clear();for(const auto&r:rows)data.insert(data.end(),r.begin(),r.end());}
void add_alias(Fixture&x,bool known,bool conflict){
    const auto root=x.s.bank(28);x.s.memory[root]=x.s.memory.at(x.s.tags[6].address);
    x.s.tags.push_back({"snd!",known?"sibling":"unknown",0xe1000008u,root,28});x.s.owners[0xe1000008u]=x.s.tags[2].handle();
    for(unsigned n=0;n<2;++n){x.resources.push_back(x.resources[n]);if(known){std::vector<std::byte>row(68);const auto key=halo2::mcc_sound_permutation_time_detail::named_key("sibling",x.names.at(0x11112222),x.names.at(n?0x33334444:0x11112222));std::ranges::copy(key,row.begin());const auto sha=hash_bytes(x.resources[n].data);std::ranges::copy(sha,row.begin()+32);put(row,64,0x4321,2);put(row,66,1000+n+unsigned(conflict),2);x.records.insert(x.records.end(),row.begin(),row.end());}}
    sort_records(x.records);x.refresh();
}
void tests(bool baseline){
    using Status=MccSoundPermutationTimeStatus;using halo2::mcc_sound_permutation_time_detail::apply;
    Fixture x;const auto raw=x.s.memory;const auto base=x.native_plan();const auto plan=x.plan(base);
    need(plan.corrections().size()==4&&plan.permutations().size()==4&&plan.owners().size()==2,"Complete two-owner plan did not authenticate all four permutations");
    auto after=base;if(!baseline)apply(after,x.s.source(),plan);
    for(const auto&c:plan.corrections()){
        need(c.users.size()==1&&c.source_ms==0x4321&&c.original_ms>=1000,"Finite row/user receipt differs");
        need(c.native_index==c.source_index+(c.gestalt==x.s.tags[3].handle()?2:0),"Merged index did not retain source owner ordinal");
        need(Bytes(native(after,c.native_address,16)).u16(6)==c.original_ms,"Original authored U16 was not restored");
        need(Bytes(c.source_before).u16(6)==c.source_ms&&Bytes(c.native_before).u16(6)==c.source_ms,"Full source/native row guard differs");
        need(c.source_before!=c.native_before,"Synthetic canonical row must exercise genuine localized-field projection");
    }
    need(raw==x.s.memory&&after.patches.size()==base.patches.size(),"Pure planner changed raw ownership");
    for(unsigned n=0;n<base.bytes.size();++n)if(base.bytes[n]!=after.bytes[n])need(std::ranges::any_of(plan.corrections(),[&](const auto&c){return n==c.native_address-base.address+6||n==c.native_address-base.address+7;}),"Non-timing native byte changed");
    for(unsigned n=0;n<base.patches.size();++n)need(base.patches[n].before==after.patches[n].before&&base.patches[n].after==after.patches[n].after,"Permutation conversion changed a native root");
    const auto complete=x.plan(after);need(complete.corrections().empty()&&std::ranges::all_of(complete.permutations(),[](const auto&r){return r.status==Status::AlreadyEquivalent;}),"Complete converted plan was not idempotent");
    unsigned controls=0;
    const auto invalid=[&](auto mutate){auto p=base;mutate(p);const auto bad=x.plan(p);need(bad.corrections().empty()&&std::ranges::all_of(bad.permutations(),[](const auto&r){return r.status==Status::InvalidNativeProjection;}),"Changed/mixed native plan accepted");++controls;};
    invalid([](auto&p){p.bytes[0]^=std::byte{1};});invalid([](auto&p){++p.sounds[0].native_root;});invalid([](auto&p){++p.sounds[0].sound.value;});invalid([](auto&p){++p.sounds[0].native_flags;});invalid([](auto&p){++p.chunks[0].encoded_bytes;});invalid([](auto&p){++p.gestalts[0].first[5];});invalid([](auto&p){++p.gestalts[0].count[5];});invalid([](auto&p){++p.blocks[5].address;});invalid([](auto&p){p.patches.back().after[0]^=std::byte{1};});invalid([&](auto&p){const auto&c=plan.corrections()[0];std::ranges::copy(c.after,p.bytes.begin()+c.native_address-p.address+6);});
    const auto bad_resource=[&](auto mutate){Fixture f=x;mutate(f);const auto p=f.plan(f.native_plan());need(p.corrections().size()==2&&p.owners()[0].status==MccSoundVanillaOwnerStatus::Custom&&p.owners()[1].status==MccSoundVanillaOwnerStatus::Authenticated&&std::ranges::all_of(p.corrections(),[&](const auto&c){return c.gestalt==f.s.tags[3].handle();}),"Failed encoded owner partially admitted/withheld unrelated owner");++controls;};
    bad_resource([](auto&f){f.resources[0].data[0]^=std::byte{1};});bad_resource([](auto&f){++f.resources[1].samples;});bad_resource([](auto&f){++f.resources[0].codec;});bad_resource([](auto&f){++f.resources[0].sample_rate;});bad_resource([](auto&f){++f.resources[0].channels;});bad_resource([](auto&f){++f.resources[0].sound_class;});bad_resource([](auto&f){++f.resources[0].packet_chunks[0].bytes;});bad_resource([](auto&f){f.resources[0].packet_chunks[0].reset_after=true;});
    Fixture missing=x;missing.records.erase(missing.records.begin(),missing.records.begin()+68);need(missing.plan(base).corrections().size()==2,"Incomplete named sound used current/PCM fallback");++controls;
    Fixture unknown=x;unknown.owners.clear();const auto no_owner=unknown.plan(base);need(no_owner.corrections().empty()&&std::ranges::all_of(no_owner.permutations(),[](const auto&r){return r.status==Status::UnknownOwner;}),"Known key without raw owner authority admitted");++controls;
    Fixture custom=x;put(custom.s.memory[custom.s.tags[6].address],16,123);const auto changed=custom.plan(custom.native_plan());need(changed.corrections().size()==2&&changed.owners()[0].status==MccSoundVanillaOwnerStatus::Custom,"Changed metadata matched original owner");++controls;
    Fixture reordered=x;const auto permutation_bank=Bytes(reordered.s.read(reordered.s.tags[2].address,120)).u32(0x34);auto&rows=reordered.s.memory[permutation_bank];for(unsigned i=0;i<16;++i)std::swap(rows[i],rows[16+i]);std::swap(reordered.resources[0],reordered.resources[1]);reordered.refresh();const auto by_name=reordered.plan(reordered.native_plan());need(by_name.corrections().size()==4,"Valid named permutation reorder was rejected");for(const auto&c:by_name.corrections())if(c.gestalt==reordered.s.tags[2].handle())need(c.original_ms==1001-c.source_index,"Original authored time was paired by ordinal instead of name");++controls;
    Fixture duplicated_name=x;duplicated_name.names[0x33334444]="range0";duplicated_name.refresh();need(duplicated_name.plan(duplicated_name.native_plan()).corrections().size()==2,"Duplicate resolved named identity was admitted");++controls;
    Fixture unused=x;const auto unused_bank=Bytes(unused.s.read(unused.s.tags[2].address,120)).u32(0x34);const auto spare=std::vector<std::byte>(unused.s.memory[unused_bank].begin(),unused.s.memory[unused_bank].begin()+16);unused.s.memory[unused_bank].insert(unused.s.memory[unused_bank].end(),spare.begin(),spare.end());put(unused.s.memory[unused.s.tags[2].address],0x30,3);unused.refresh();const auto unused_base=unused.native_plan();auto unused_after=unused_base;const auto only_users=unused.plan(unused_base);apply(unused_after,unused.s.source(),only_users);need(only_users.corrections().size()==4&&only_users.permutations().size()==4&&Bytes(native(unused_after,unused_after.blocks[5].address+32,16)).u16(6)==0x4321,"Unreferenced bank row acquired Original-time authority");++controls;
    for(unsigned mode=0;mode<3;++mode){Fixture f=x;add_alias(f,mode!=0,mode==2);const auto p=f.plan(f.native_plan());if(mode==1){need(p.corrections().size()==4&&std::ranges::count(p.corrections(),std::size_t(2),[](const auto&c){return c.users.size();})==2,"Same-value known aliases did not coalesce exact physical targets");}else need(p.corrections().size()==2&&p.owners()[0].status==MccSoundVanillaOwnerStatus::Custom,"Unknown/conflicting incoming user was ignored");++controls;}
    for(unsigned failed=0;failed<2;++failed){Fixture f=x;add_alias(f,true,false);f.resources[failed?4:0].data[0]^=std::byte{1};const auto p=f.plan(f.native_plan());need(p.corrections().size()==2&&p.owners()[0].status==MccSoundVanillaOwnerStatus::Custom,"Earlier/later encoded failure left same-owner sibling admitted");++controls;}
    Fixture duplicate=x;const auto record=std::vector<std::byte>(duplicate.records.begin(),duplicate.records.begin()+68);duplicate.records.insert(duplicate.records.end(),record.begin(),record.end());rejected([&]{duplicate.plan(base);});++controls;
    const auto first=x.s.tags[6].address,second=x.s.tags[7].address,legacy=x.s.tags[4].address;
    const auto reject_alias=[&](auto relocation){auto source=x.input();source.metadata.relocate=relocation;const auto p=plan_mcc_sound_runtime_delivery(source.metadata,base.address);const auto bad=halo2::mcc_sound_permutation_time_detail::build(source,source,p,x.policy());need(bad.corrections().empty()&&std::ranges::all_of(bad.permutations(),[](const auto&r){return r.status==Status::InvalidNativeProjection;}),"Native root/bank alias or wrong extent admitted");++controls;};
    reject_alias([&](unsigned a,std::size_t n){(void)x.s.read(a,n);return a+0x20000000u+(a==first&&n==20?4u:0u);});
    reject_alias([&](unsigned a,std::size_t n){(void)x.s.read(a,n);return a==second?first+0x20000004u:a+0x20000000u;});
    reject_alias([&](unsigned a,std::size_t n){(void)x.s.read(a,n);return a==first?base.address:a+0x20000000u;});
    reject_alias([&](unsigned a,std::size_t n){(void)x.s.read(a,n);return a==legacy?first+0x20000000u:a+0x20000000u;});
    const auto rejected_apply=[&](auto mutate){auto p=base;auto source=x.s.source();mutate(p,source);const auto bytes=p.bytes;rejected([&]{apply(p,source,plan);});need(p.bytes==bytes,"Failed publication partially changed timing bytes");++controls;};
    rejected_apply([](auto&p,auto&){p.bytes.back()^=std::byte{1};});rejected_apply([](auto&p,auto&){++p.gestalts[0].first[5];});
    rejected_apply([&](auto&,auto&s){s.gestalt_owner=[&](TagHandle){return x.s.tags[3].handle();};});
    rejected_apply([&](auto&,auto&s){s.relocate=[&](unsigned a,std::size_t n){(void)x.s.read(a,n);return a+0x20000004u;};});
    for(unsigned mode=0;mode<3;++mode){auto p=base;auto source=x.s.source();auto tags=x.s.tags;if(mode==0)tags[6].datum^=0x10000;if(mode==1)tags[6].group="weap";if(mode==2)tags[6].address+=4;source.tags=tags;rejected([&]{apply(p,source,plan);});need(p.bytes==base.bytes,"Foreign source directory partially changed native timing");++controls;}
    for(unsigned mode=0;mode<4;++mode){auto p=base;auto source=x.s.source();auto read=source.read;bool called=false;source.read=[&](unsigned a,std::size_t n){const auto b=read(a,n);if(!called&&n==28){called=true;if(mode==0)p.bytes.reserve(p.bytes.capacity()+512);if(mode==1)p.sounds.reserve(p.sounds.capacity()+512);if(mode==2)p.bytes[0]^=std::byte{1};if(mode==3)++p.sounds[0].native_root;}return b;};if(mode<2){apply(p,source,plan);need(p.bytes==after.bytes,"Benign callback reserve invalidated owned staging");}else{rejected([&]{apply(p,source,plan);});for(const auto&c:plan.corrections())need(Bytes(native(p,c.native_address,16)).u16(6)==c.source_ms,"Late provider mutation partially published timing");}need(called,"Late provider mutation control not reached");++controls;}
    for(unsigned mode=0;mode<2;++mode){auto p=base;auto current=x.plan(base);const auto replacement=x.plan(after);auto source=x.s.source();const auto read=source.read;bool called=false;
        source.read=[&](unsigned a,std::size_t n){const auto b=read(a,n);if(!called){called=true;current=replacement;if(mode)source={};}return b;};
        apply(p,source,current);need(called&&p.bytes==after.bytes&&current.corrections().empty(),"Callback wrapper replacement lost the retained proof/callable owner");++controls;
    }
    need(raw==x.s.memory,"Pure tests changed source");std::cout<<"Permutation-time pure planner: four exact2B receipts/two complete owners, idempotence and "<<controls<<" adversarial controls PASS\n";
}
} // namespace
int main(int argc,char**argv)try{tests(argc>1&&std::string_view(argv[1])=="--baseline");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
