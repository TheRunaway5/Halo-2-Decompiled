#include "../src/audio/mcc_sound_class_qualified.hpp"
#include "../src/audio/mcc_sound_geometry_apply.hpp"
#include "../src/audio/mcc_sound_permutation_time_reference_profile.hpp"
#include "../src/audio/mcc_sound_stored_time_apply.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include <nlohmann/json.hpp>
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/cache.hpp"
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
void synthetic() {
    Source s;const auto before=s.memory;const auto p=plan_mcc_sound_runtime_delivery(s.source(),0x80000000u);
    need(s.memory==before,"Planning changed raw MCC bytes");
    need(!p.authored_flags,"Generic Source path unexpectedly selected a retail authored reference");
    need(p.gestalts.size()==2&&p.sounds.size()==2&&p.chunks.size()==4&&p.patches.size()==6,"Merged plan owner census differs");
    need(p.native_pcm_duration==std::vector<TagHandle>{{0xe1000006}},"Extended PCM codec was hidden or coerced");
    need(p.sounds[0].file==MccSoundDataFile::English&&p.sounds[1].file==MccSoundDataFile::Remastered,"Real DAT class routing differs");
    need(p.sounds[0].codec.compression==5&&p.sounds[0].codec.sample_rate==3,"Truthful MCC codec was changed");
    for(unsigned n=0;n<2;++n)need(p.sounds[n].source_root==s.tags[5+n].address&&
        p.sounds[n].native_root==s.tags[5+n].address+0x20000000u,"Typed full sound source/native root binding differs");
    for(unsigned j=0;j<2;++j) {
        const auto& o=p.gestalts[j];const Bytes def(patch(p,{0xe1000006u+j}).after);
        need(p.sounds[j].source_flags==(j?0x800u:0x400u)&&p.sounds[j].native_flags==0&&def.u16(0)==0,
            "MCC DAT routes became original distance defaults or disappeared from sidecar");
        for(const auto [offset,table]:{std::pair(6u,0u),{8u,4u},{14u,10u}})need(def.u16(offset)==o.first[table],"Sound SHORT table ordinal not rebased");
        for(const auto [offset,table]:{std::pair(11u,1u),{12u,9u},{13u,6u}})need(def.slice(offset,1)[0]==std::byte(o.first[table]),"Sound BYTE table ordinal not rebased");
        need(def.u32(16)==0x76543210,"Authored max play time changed");
        const Bytes range(native(p,p.blocks[4].address+j*12,12));
        need(range.u16(0)==j*2&&range.u16(2)==j&&range.u16(4)==0xff01&&range.u16(6)==j*8&&range.u16(8)==j*2,"Pitch/global flag rebasing or local runtime state differs");
        for(unsigned n=0;n<2;++n) {
            const Bytes perm(native(p,p.blocks[5].address+(j*2+n)*16,16));
            need(perm.u16(0)==j*2+n&&perm.slice(5,1)[0]==std::byte(n)&&perm.u16(6)==0x4321,"Permutation global name or local dialogue index differs");
            need(perm.u32(8)==36+n*20&&perm.u16(12)==j*2+n&&perm.u16(14)==1,"Encoded extent/chunk index differs");
            const auto& c=p.chunks[j*2+n];const Bytes chunk(native(p,c.native_address,12));
            need(chunk.u32(0)==c.file_offset&&chunk.u32(4)==(0x80000000u|c.encoded_bytes)&&chunk.u32(8)==0xffffffffu,"NONE resource became fake-ready or DAT bytes changed");
            need(c.decoded_bytes==128+n*64&&c.reset_after==bool(n),"PCM/reset metadata lost from route");
        }
        const auto& root=patch(p,{0xe1000002u+j});
        for(unsigned n=0;n<11;++n)need(Bytes(root.after).u32(n*8)==std::uint32_t(p.blocks[n].count)&&Bytes(root.after).u32(n*8+4)==p.blocks[n].address,"Gestalt roots do not use one complete merged owner");
        const Bytes extra(native(p,p.blocks[10].address+j*0x2c,0x2c));
        need(extra.u32(0x20)==0xe1000002u+j&&extra.u16(0x24)==0xfff8,"Extra-info full owner or signed local displacement rebased");
        const auto source_extra=Bytes(s.read(s.tags[1+j].address,0x78)).u32(0x74);
        need(extra.u32(4)==Bytes(s.read(source_extra,0x2c)).u32(4)+0x20000000u&&extra.u32(0x1c)==Bytes(s.read(source_extra,0x2c)).u32(0x1c)+0x20000000u,"English resource pointers not relocated");
    }
    unsigned negatives=0;
    const auto mutate=[&](auto fn){auto changed=s;fn(changed);rejected([&]{plan_mcc_sound_runtime_delivery(changed.source(),0x80000000u);});++negatives;};
    mutate([](Source& c){c.owners[0xe1000006]={0xf1000002};});
    mutate([](Source& c){c.tags[1].group="snd!";});
    mutate([](Source& c){put(c.memory[c.tags[1].address],0x28,0x100001);});
    mutate([](Source& c){auto g=Bytes(c.read(c.tags[1].address,0x78));const auto l=g.u32(0x64);const auto chunks=Bytes(c.read(l,8)).u32(4);put(c.memory[chunks],12,7);});
    mutate([](Source& c){auto g=Bytes(c.read(c.tags[1].address,0x78));const auto perm=g.u32(0x34);const auto info=Bytes(c.read(perm,16)).u32(12);put(c.memory[info],4,0x7fff,2);});
    mutate([](Source& c){c.table(c.tags[1].address,0x10,128,0x14);});
    mutate([](Source& c){c.table(c.tags[1].address,0x58,8192,1);});
    mutate([](Source& c){c.memory[c.tags[5].address][2]=std::byte{0x7f};});
    mutate([](Source& c){c.memory[c.tags[5].address][3]=std::byte{0x7f};});
    mutate([](Source& c){put(c.memory[c.tags[5].address],8,0x7fff,2);});
    mutate([](Source& c){c.tags[6].address=c.tags[5].address;});
    auto missing=s.source();missing.relocate=[](auto,auto){return 0u;};rejected([&]{plan_mcc_sound_runtime_delivery(missing,0x80000000);});++negatives;
    auto alias_roots=s.source();alias_roots.relocate=[&](std::uint32_t at,std::size_t n) {
        return n==20&&(at==s.tags[5].address||at==s.tags[6].address)?0x22220000u:at+0x20000000u;
    };
    rejected([&]{plan_mcc_sound_runtime_delivery(alias_roots,0x80000000);});++negatives;
    for(auto a:{0u,0xffffffffu,0x80000001u,0xfffffffcu}){rejected([&]{plan_mcc_sound_runtime_delivery(s.source(),a);});++negatives;}
    Source dead=s;
    for(unsigned t:{3u,4u}) {
        const auto data=Bytes(dead.read(dead.tags[t].address,8)).u32(4);put(dead.memory[data],10,0x20,2);
    }
    const auto range=Bytes(dead.read(dead.tags[2].address,0x78)).u32(0x2c);put(dead.memory[range],6,128,2);
    const auto dead_before=dead.memory;const auto admitted=plan_mcc_sound_runtime_delivery(dead.source(),0x80000000);
    need(dead_before==dead.memory&&admitted.unused_flag_indices.size()==1,"Exact nonpersistent no-access ownership proof missing");
    const auto& proof=admitted.unused_flag_indices[0];
    need(proof.gestalt==TagHandle{0xe1000003}&&proof.range_index==0&&proof.encoded_index==128&&proof.users.size()==1&&proof.users[0].sound==TagHandle{0xe1000007},"Nonpersistent proof lost full salted users");
    need(Bytes(native(admitted,admitted.blocks[4].address+12,12)).u16(6)==128,"Dead flag index received fake backing/rebase");
    for(unsigned t:{3u,4u}) {
        auto changed=dead;const auto data=Bytes(changed.read(changed.tags[t].address,8)).u32(4);put(changed.memory[data],10,0,2);
        rejected([&]{plan_mcc_sound_runtime_delivery(changed.source(),0x80000000);});++negatives;
    }
    auto unreferenced=dead;unreferenced.memory[unreferenced.tags[6].address][10]=std::byte{0};
    rejected([&]{plan_mcc_sound_runtime_delivery(unreferenced.source(),0x80000000);});++negatives;
    auto alias=dead;
    for(unsigned t:{3u,4u}) {const auto data=alias.table(alias.tags[t].address,0,2,0x78);put(alias.memory[data],10,0x20,2);}
    const auto clone=alias.bank(28);const auto raw=alias.read(alias.tags[6].address,28);alias.memory[clone].assign(raw.begin(),raw.end());alias.memory[clone][2]=std::byte{1};
    alias.tags.push_back({"snd!","persistent shared range alias",0xe1000008,clone,28});alias.owners[0xe1000008]={0xe1000003};
    rejected([&]{plan_mcc_sound_runtime_delivery(alias.source(),0x80000000);});++negatives;
    std::cout<<"Synthetic merged sound ownership/indices/encoded extents passed; negative_guards="<<negatives<<'\n';
}

namespace {
using J=nlohmann::json;
std::string hex(std::span<const std::byte>b){std::string s;for(auto x:b){const auto n=std::to_integer<unsigned>(x);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
void put(std::vector<std::byte>&b,std::size_t p,unsigned x){for(unsigned n=0;n<4;++n)b.at(p+n)=std::byte(x>>(8*n));}
J encode(const MccSoundRuntimePlan&p){J j={{"address",p.address},{"bytes",hex(p.bytes)}};for(const auto&b:p.blocks)j["blocks"].push_back({b.count,b.address});for(const auto&x:p.patches)j["patches"].push_back({x.owner.value,x.source_address,hex(x.before),hex(x.after)});for(const auto&g:p.gestalts)j["gestalts"].push_back({g.owner.value,g.first,g.count});for(const auto&c:p.chunks)j["chunks"].push_back({c.gestalt.value,c.source_address,c.native_address,c.source_index,c.file_offset,c.encoded_bytes,c.decoded_bytes,c.reset_after});for(const auto&s:p.sounds)j["sounds"].push_back({s.sound.value,s.gestalt.value,s.codec.sample_rate,s.codec.encoding,s.codec.compression,unsigned(s.file),s.remastered_index,hex(s.mcc_extension),s.source_root,s.native_root,s.source_flags,s.native_flags});for(const auto&u:p.unused_flag_indices){J users=J::array();for(const auto&x:u.users)users.push_back({x.sound.value,x.mcc_class.value,x.native_class.value,x.class_index,x.mcc_flags,x.native_flags});j["unused"].push_back({u.gestalt.value,u.range_index,u.encoded_index,users});}for(const auto h:p.native_pcm_duration)j["pcm"].push_back(h.value);j["authored_flags"]=bool(p.authored_flags);if(p.authored_flags){for(const auto&c:p.authored_flags->corrections)j["flags"].push_back({c.sound.value,c.gestalt.value,c.source_root,c.source_flags,c.native_before,c.native_after});for(const auto&o:p.authored_flags->owners)j["flag_owners"].push_back({o.gestalt.value,unsigned(o.status),o.reason});}return j;}

}
void actual(const char* path) {
    Cache cache(path);const auto relocated=[](auto a,auto){return a;};
    const auto converted=plan_mcc_sound_runtime_delivery(cache,0xe0000000u,relocated);
    const MccSoundRuntimeSource generic_source{cache.tags(),[&](auto a,auto n){return cache.read_tag_bytes(a,n);},
        [&](TagHandle h){return cache.mcc_sound_gestalt_owner(h).handle();},relocated};
    const auto generic=plan_mcc_sound_runtime_delivery(generic_source,0xe0000000u);
    // Keep the original metadata-only checks below in their actual generic
    // domain. Independently stage every finite C/P/S correction and the
    // authenticated English facial-resource tokens before requiring exact
    // complete converted-plan equality.
    auto p=generic;p.authored_flags=converted.authored_flags;
    need(bool(p.authored_flags),"Cache-aware native plan lost finite flag authority");
    for(const auto&c:p.authored_flags->corrections){
        const auto d=std::ranges::find(p.sounds,c.sound,&MccSoundRuntimeDefinition::sound);
        const auto patch=std::ranges::find(p.patches,c.source_root,&MccSoundRuntimePatch::source_address);
        need(d!=p.sounds.end()&&patch!=p.patches.end()&&patch->owner==c.sound&&d->gestalt==c.gestalt&&
            d->source_root==c.source_root&&d->native_flags==c.native_before&&Bytes(patch->after).u16(0)==c.native_before,
            "Finite flag target lost complete generic preimage");
        d->native_flags=c.native_after;patch->after[0]=std::byte(c.native_after);patch->after[1]=std::byte(c.native_after>>8);
    }
    auto reconstructed=mcc_sound_class_detail::stage_qualified(generic_source,p);
    const auto permutation=plan_mcc_sound_permutation_times(cache,reconstructed,relocated);
    mcc_sound_permutation_time_detail::apply(reconstructed,generic_source,permutation);
    const auto stored=plan_mcc_sound_stored_times(cache,reconstructed,relocated);
    mcc_sound_stored_time_detail::apply(reconstructed,generic_source,stored);
    mcc_sound_geometry_detail::apply(reconstructed,plan_mcc_sound_geometry_resources(cache));
    need(encode(reconstructed)==encode(converted),"Cache output escaped exact typed C/P/S and geometry receipts");
    const auto timing=build_mcc_sound_timing_reference(cache,converted,relocated);
    need(std::ranges::all_of(timing.owners(),[](const auto&o){return o.status==MccSoundVanillaOwnerStatus::Authenticated;}),
        "Converted sound native plan lost complete authenticated owner qualification");
    const std::map<std::string,std::array<unsigned,3>> expected{{"lockout",{4506,1359,4503}},
        {"01b_spacestation",{16210,4178,16210}},{"03a_oldmombasa",{13824,3389,13821}}};
    if(expected.contains(cache.name())){const auto n=expected.at(cache.name());
        need(timing.permutation_references().size()==n[0]&&timing.stored_references().size()==n[1]&&timing.bindings().size()==n[2],
            "Converted native plan omitted mandatory finite Original timing authority");}
    need(!generic.authored_flags&&p.bytes==generic.bytes&&p.native_pcm_duration==generic.native_pcm_duration&&
        p.patches.size()==generic.patches.size(),"Authored flag correction changed complete V3 child banks/codecs");
    for(unsigned n=0;n<11;++n)need(p.blocks[n].count==generic.blocks[n].count&&p.blocks[n].address==generic.blocks[n].address,
        "Authored flag correction changed an original sound root descriptor");
    for(std::size_t n=0;n<p.patches.size();++n) {
        const auto& a=p.patches[n];const auto& old=generic.patches[n];
        need(a.owner==old.owner&&a.source_address==old.source_address&&a.before==old.before&&a.after.size()==old.after.size(),
            "Authored flag correction changed a V3 owner/preimage/extent");
        auto after=a.after;
        for(const auto& c:p.authored_flags->corrections)if(c.sound==a.owner) {
            need(Bytes(old.after).u16(0)==c.native_before&&Bytes(a.after).u16(0)==c.native_after,
                "Authored two-byte native correction differs from its admitted receipt");
            after[0]=old.after[0];after[1]=old.after[1];
        }
        need(after==old.after,"Authored flag correction changed bytes outside the exact native flag word");
    }
    std::map<std::uint32_t,const MccSoundRuntimeGestalt*> owners;
    for(const auto& o:p.gestalts)owners.emplace(o.owner.value,&o);
    unsigned permutations=0,ranges=0,extra_info=0,classes=0;
    for(const auto& o:p.gestalts) {
        const auto* tag=cache.find_tag(o.owner,tag_groups::sound_cache_file_gestalt);need(tag,"Actual gestalt full salt disappeared");
        const Bytes g(cache.read_tag_bytes(tag->address,0x78));
        const auto rawperm=cache.read_tag_bytes(g.u32(0x34),std::size_t(g.u32(0x30))*16);
        const auto lang=cache.read_tag_bytes(g.u32(0x64),8);const Bytes l(lang);
        const auto chunks=cache.read_tag_bytes(l.u32(4),std::size_t(l.u32(0))*16);
        for(unsigned n=0;n<g.u32(0x30);++n) {
            const auto raw=rawperm.subspan(n*16,16);const Bytes r(raw);
            const Bytes info(cache.read_tag_bytes(r.u32(12),8));
            const auto projected=native(p,p.blocks[5].address+(o.first[5]+n)*16,16);const Bytes b(projected);
            for(unsigned a=2;a<8;++a)need(projected[a]==raw[a],"Actual permutation local authored bits changed");
            std::uint64_t encoded=0;for(int c=0;c<i16(info.u16(6));++c)encoded+=Bytes(chunks).u32((info.u16(4)+c)*16+4)&0x1fffffffu;
            need(b.u32(8)==encoded&&b.u16(14)==info.u16(6),"Actual encoded bytes/chunk count differ");
            const auto first=i16(info.u16(4));need(b.u16(12)==(first==-1?0xffffu:o.first[8]+first),"Actual selected chunks not rebased");++permutations;
        }
        const auto rawranges=cache.read_tag_bytes(g.u32(0x2c),std::size_t(g.u32(0x28))*12);
        for(unsigned n=0;n<g.u32(0x28);++n) {
            const auto raw=rawranges.subspan(n*12,12);const Bytes r(raw),b(native(p,p.blocks[4].address+(o.first[4]+n)*12,12));
            need(b.u16(4)==r.u16(4)&&b.u16(10)==r.u16(10),"Actual local permutation state changed");
            const bool dead=r.u16(6)!=0xffffu&&std::uint64_t(r.u16(6)>>3)+(std::uint32_t(r.u16(10))+7)/8>o.count[7];
            need(b.u16(6)==(r.u16(6)==0xffffu||dead?r.u16(6):r.u16(6)+o.first[7]*8),"Actual flag byte ownership differs");
            if(dead) {
                const auto proof=std::ranges::find_if(p.unused_flag_indices,[&](const auto& proof){return proof.gestalt==o.owner&&proof.range_index==n&&proof.encoded_index==r.u16(6);});
                need(proof!=p.unused_flag_indices.end()&&!proof->users.empty(),"Actual out-of-range flags lack exact user proof");
                for(const auto& u:proof->users) {
                    need((u.mcc_flags&0x20)&&(u.native_flags&0x20),"Effective native/MCC class permits dead flag access");
                    for(const auto h:{u.sound,u.mcc_class,u.native_class})need(cache.find_tag(h),"Dead flag proof lost full namespace identity");
                }
            }
            ++ranges;
        }
        const auto rawextra=g.u32(0x70)?cache.read_tag_bytes(g.u32(0x74),std::size_t(g.u32(0x70))*0x14c):std::span<const std::byte>{};
        for(unsigned n=0;n<g.u32(0x70);++n) {
            const auto raw=rawextra.subspan(n*0x14c,0x2c);const auto converted=native(p,p.blocks[10].address+(o.first[10]+n)*0x2c,0x2c);
            need(std::ranges::equal(raw,converted),"Identity-relocated English prefix changed authored fields");++extra_info;
        }
    }
    for(const auto& sound:p.sounds) {
        const auto& o=*owners.at(sound.gestalt.value);const auto* tag=cache.find_tag(sound.sound,tag_groups::sound);need(tag,"Actual sound full salt disappeared");
        need(cache.mcc_sound_gestalt_owner(sound.sound).handle()==sound.gestalt,"Actual sound took another gestalt's codec/banks");
        need(sound.source_root==tag->address&&sound.native_root==tag->address,"Actual full-salt native root identity differs from typed relocation");
        const auto raw=cache.read_tag_bytes(tag->address,0x1c);const auto& changed=patch(p,sound.sound);const Bytes r(raw),b(changed.after);
        unsigned expected_flags=(r.u16(0)&0x03ffu)|((r.u16(0)&0xf000u)>>2);
        need(bool(p.authored_flags),"Cache-aware sound plan lost authored owner status");
        for(const auto& correction:p.authored_flags->corrections)if(correction.sound==sound.sound) {
            need(correction.gestalt==sound.gestalt&&correction.source_root==tag->address&&
                correction.source_flags==r.u16(0)&&correction.native_before==expected_flags,
                "Authored sound receipt lost its complete source/native preimage");
            expected_flags=correction.native_after;
        }
        need(sound.source_flags==r.u16(0)&&sound.native_flags==b.u16(0)&&b.u16(0)==expected_flags,
            "Actual MCC route flags lost or native compiled/authored predicates differ");
        need(std::ranges::equal(changed.before,raw.first(20)),"Actual Cache sound changed while planning");
        for(const auto [at,table]:{std::pair(6u,0u),{8u,4u},{14u,10u}})need(b.u16(at)==(r.u16(at)==0xffffu?0xffffu:r.u16(at)+o.first[table]),"Actual sound SHORT ordinal rebasing differs");
        need(changed.after[3]==std::byte(sound.codec.sample_rate)&&changed.after[4]==std::byte(sound.codec.encoding)&&changed.after[5]==std::byte(sound.codec.compression),"Actual sound codec coerced");
    }
    for(const auto& c:p.chunks) {
        const Bytes raw(cache.read_tag_bytes(c.source_address,16)),b(native(p,c.native_address,12));
        need(raw.u32(12)==0xffffffffu&&b.u32(8)==0xffffffffu,"Actual unloaded chunk became ready");
        need(raw.u32(0)==b.u32(0)&&b.u32(4)==(raw.u32(4)&~0x20000000u)&&c.encoded_bytes==(raw.u32(4)&0x1fffffffu)&&c.decoded_bytes==raw.u32(8),"Actual chunk DAT/codec extent lost");
    }
    for(const auto& t:cache.tags())if(t.address&&t.group=="sncl") {
        const Bytes r(cache.read_tag_bytes(t.address,8));const Bytes changed(patch(p,t.handle()).after);
        const auto raw=cache.read_tag_bytes(r.u32(4),std::size_t(r.u32(0))*0x78);const auto converted=native(p,changed.u32(4),std::size_t(r.u32(0))*0x5c);
        for(unsigned n=0;n<r.u32(0);++n)need(std::ranges::equal(raw.subspan(n*0x78,0x5c),converted.subspan(n*0x5c,0x5c)),"Actual class scalar fields changed");classes+=r.u32(0);
    }
    std::cout<<"Actual MCC sound plan passed: sounds="<<p.sounds.size()<<" gestalts="<<p.gestalts.size()<<" permutations="<<permutations<<" ranges="<<ranges<<" chunks="<<p.chunks.size()<<" English_extra_info="<<extra_info<<" classes="<<classes<<" unused_flag_proofs="<<p.unused_flag_indices.size()<<" extended_PCM_duration="<<p.native_pcm_duration.size()<<" bytes="<<p.bytes.size()<<'\n';
}
}
int main(int argc,char**argv)try{synthetic();if(argc>1)actual(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
