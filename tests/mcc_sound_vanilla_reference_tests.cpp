#include "halo2/mcc_sound_vanilla_reference.hpp"
#include "halo2/cache.hpp"
#include "../src/mcc_sound_vanilla_reference_profile.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;
namespace detail=halo2::mcc_sound_reference_detail;
void need(bool x,const char*why){if(!x)throw std::runtime_error(why);}
template<class T>void put(T& bytes,unsigned at,unsigned value,unsigned width=4){Bytes(bytes).require(at,width);for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8));}
struct Fixture {
    std::vector<Tag> tags;
    std::map<unsigned,std::vector<std::byte>> memory;
    std::map<unsigned,std::string> names{{1,"range"},{2,"first"},{3,"last"}};
    unsigned next=0x10000,gestalt{},sound1{},sound2{},playback{},imports{},ranges{},perms{};
    MccSoundVanillaPayload payload{};
    unsigned bank(unsigned size){auto at=next;next+=(size+15u&~15u)+16;memory[at].resize(size);return at;}
    unsigned tag(const char* group,const char*name,unsigned datum,unsigned size){const auto at=bank(size);tags.push_back({group,name,datum,at,size});return at;}
    unsigned block(unsigned root,unsigned at,unsigned count,unsigned stride){auto p=bank(count*stride);put(memory[root],at,count);put(memory[root],at+4,p);return p;}
    std::span<const std::byte> read(unsigned at,std::size_t size)const{auto i=memory.upper_bound(at);if(i==memory.begin())throw FormatError("Fixture owner absent");--i;return Bytes(i->second).slice(at-i->first,size);}
    Fixture(){
        const auto globals=tag("matg","globals\\globals",0xe1000001,0xc8),classes=tag("sncl","sound\\classes",0xe1000002,8);gestalt=tag("ugh!","sound\\gestalt",0xe1000003,120);sound1=tag("snd!","dialogue\\first",0xe1000004,28);sound2=tag("snd!","dialogue\\last",0xe1000005,28);
        const auto sg=block(globals,0xc0,1,0x2c);put(memory[sg],4,0xe1000002);put(memory[sg],12,0xe1000002);put(memory[sg],0x28,0xe1000003);block(classes,0,1,0x78);
        const auto codecs=block(gestalt,0,1,3);memory[codecs]={std::byte{3},std::byte{0},std::byte{5}};playback=block(gestalt,8,1,56);block(gestalt,0x10,1,20);imports=block(gestalt,0x18,3,4);for(unsigned n=0;n<3;++n)put(memory[imports],n*4,n+1);block(gestalt,0x20,1,10);ranges=block(gestalt,0x28,1,12);put(memory[ranges],10,2,2);perms=block(gestalt,0x30,2,16);put(memory[perms],0,1,2);put(memory[perms],16,2,2);
        for(auto at:{sound1,sound2}){put(memory[at],0,0x4106,2);memory[at][10]=std::byte{1};memory[at][12]=std::byte{0xff};memory[at][13]=std::byte{0xff};put(memory[at],14,0xffff,2);}
        payload.encoded_bytes=2;payload.codec=5;payload.sample_rate=48000;payload.channels=1;payload.samples=100;payload.sound_class=0;payload.sha256[0]=std::byte{0xa5};payload.packets={{2,200,true}};
    }
    MccSoundVanillaSource source()const{return {{tags,[&](auto a,auto n){return read(a,n);},[](TagHandle){return TagHandle{0xe1000003};},{}},[&](auto id){return names.at(id);},[&](TagHandle,unsigned,unsigned){return payload;}};}
    detail::Owner profile()const{
        auto input=source();detail::Owner p{{0xe1000003},gestalt,"sound\\gestalt",detail::metadata_digest(input,{0xe1000003}),{}};
        for(unsigned n=0;n<2;++n){detail::Sound s{{0xe1000004+n},n?sound2:sound1,n?"dialogue\\last":"dialogue\\first",{},0x1106,0x1102,{}};std::ranges::copy(read(s.root,28),s.header.begin());for(unsigned j=0;j<2;++j)s.permutations.push_back({0,j,0,j,0,std::uint16_t(1+j),1,2+j,"range",j?"last":"first",payload});p.sounds.push_back(std::move(s));}return p;
    }
};
void synthetic(){
    Fixture f;const auto before=f.memory;const auto profile=f.profile();const std::array profiles{profile};const auto good=detail::plan(f.source(),profiles);need(good.corrections.size()==2&&good.owners.size()==1&&good.owners[0].status==MccSoundVanillaOwnerStatus::Authenticated,"Exact sound owner was not admitted");need(f.memory==before,"Authored sound planner changed donor bytes");for(const auto& c:good.corrections)need(c.source_flags==0x4106&&c.native_before==0x1106&&c.native_after==0x1102,"Planner changed route/header bits beyond exact native word");
    unsigned negatives=0;
    const auto custom=[&](const auto& src,const auto& p){const std::array ps{p};const auto result=detail::plan(src,ps);need(result.corrections.empty()&&result.owners.size()==1&&result.owners[0].status==MccSoundVanillaOwnerStatus::Custom,"Modified owner received a partial/authored flag correction");++negatives;};
    const auto mutate=[&](auto fn){auto changed=f;fn(changed);custom(changed.source(),profile);};
    mutate([](auto& s){s.memory[s.playback][16]=std::byte{1};});
    mutate([](auto& s){s.memory[s.sound1][0]=std::byte{2};});
    mutate([](auto& s){s.memory[s.sound2][27]=std::byte{1};});
    mutate([](auto& s){s.tags[3].datum^=0x10000;});
    mutate([](auto& s){s.tags[3].group="sndx";});
    mutate([](auto& s){s.tags[3].name="custom";});
    mutate([](auto& s){s.names[3]="custom_last";});
    mutate([](auto& s){s.payload.sha256[31]=std::byte{1};});
    mutate([](auto& s){++s.payload.encoded_bytes;});
    mutate([](auto& s){++s.payload.codec;});
    mutate([](auto& s){++s.payload.sample_rate;});
    mutate([](auto& s){++s.payload.channels;});
    mutate([](auto& s){++s.payload.samples;});
    mutate([](auto& s){++s.payload.sound_class;});
    mutate([](auto& s){++s.payload.packets[0].pcm_bytes;});
    mutate([](auto& s){s.payload.packets[0].reset_after=false;});
    auto absent=f.source();absent.payload={};custom(absent,profile);
    auto atomic=f.source();atomic.payload=[&](TagHandle h,unsigned,unsigned){auto p=f.payload;if(h.value==0xe1000005)p.sha256[0]^=std::byte{1};return p;};custom(atomic,profile);
    const auto bad_profile=[&](auto fn){auto p=profile;fn(p);custom(f.source(),p);};
    bad_profile([](auto& p){p.sounds[0].permutations.pop_back();});
    bad_profile([](auto& p){p.sounds[0].permutations.push_back(p.sounds[0].permutations.back());});
    bad_profile([](auto& p){++p.sounds[0].permutations[0].global_permutation;});
    bad_profile([](auto& p){p.sounds[0].permutations[0].range_name="other";});
    bad_profile([](auto& p){p.sounds[0].after^=1;});
    bad_profile([](auto& p){p.sounds[0].after|=0x20;});
    bad_profile([](auto& p){p.sounds[1]=p.sounds[0];});
    const auto unmatched=detail::plan(f.source(),{});need(unmatched.corrections.empty()&&unmatched.owners[0].status==MccSoundVanillaOwnerStatus::Unmatched,"Unknown custom owner was silently converted");
    std::cout<<"Sound vanilla staged owner guards passed; corrections=2 negatives="<<negatives<<'\n';
}
void retained_globals(){
    struct Moved {
        Fixture f;Tag raw;unsigned original{},active{};
        Moved(){
            const auto old=f.tags[0];const auto root=f.memory.at(old.address);
            const auto row=f.read(Bytes(root).u32(0xc4),0x2c);
            const std::vector<std::byte> sound_row(row.begin(),row.end());
            original=f.bank(0x200);active=f.bank(0x300);
            for(const auto at:{original,active}){
                std::ranges::copy(root,f.memory[at].begin());
                std::ranges::copy(sound_row,f.memory[at].begin()+0x100);
                put(f.memory[at],0xc4,at+0x100);
            }
            raw=old;raw.address=original;raw.size=0x200;
            f.tags[0].address=active;f.tags[0].size=0x300;
        }
        auto canonical()const{const std::array retained{raw};return detail::raw_globals_namespace(f.source(),retained);}
    } moved;
    const auto source=moved.f.source();const auto memory=moved.f.memory;
    const auto tags=moved.canonical();need(tags.size()==moved.f.tags.size()&&tags[0].address==moved.raw.address&&tags[0].size==moved.raw.size,"Retained raw globals identity was not projected");
    for(unsigned n=1;n<tags.size();++n)need(tags[n].datum==moved.f.tags[n].datum&&tags[n].address==moved.f.tags[n].address&&tags[n].size==moved.f.tags[n].size,"Raw globals projection changed another tag identity");
    auto canonical=source;canonical.metadata.tags=tags;auto profile=moved.f.profile();profile.metadata=detail::metadata_digest(canonical,{0xe1000003});
    const std::array profiles{profile};need(detail::plan(canonical,profiles).corrections.size()==2,"Authentic retained globals lost sound admission");
    unsigned negatives=0;
    const auto reject=[&](auto change){auto copy=moved;change(copy);bool caught=false;try{(void)copy.canonical();}catch(const FormatError&){caught=true;}need(caught,"Invalid active/retained globals binding was admitted");++negatives;};
    reject([](auto& x){x.raw.datum^=0x10000;});
    reject([](auto& x){x.raw.group="xxxx";});
    reject([](auto& x){x.raw.name="globals\\custom";});
    reject([](auto& x){x.raw.size=0xc7;});
    reject([](auto& x){++x.raw.size;});
    reject([](auto& x){x.raw.size=0xffffffffu;});
    reject([](auto& x){x.raw.address=0xffffffffu;});
    reject([](auto& x){x.raw.address=x.active;});
    reject([](auto& x){x.f.tags[0].size=0xc7;});
    reject([](auto& x){++x.f.tags[0].size;});
    reject([](auto& x){x.f.tags[0].datum^=0x10000;});
    reject([](auto& x){x.f.tags[0].group="xxxx";});
    reject([](auto& x){x.f.tags[0].name="globals\\custom";});
    reject([](auto& x){put(x.f.memory[x.original],0xc0,0);});
    reject([](auto& x){put(x.f.memory[x.original],0xc0,2);});
    reject([](auto& x){put(x.f.memory[x.active],0xc0,0);});
    reject([](auto& x){put(x.f.memory[x.active],0xc0,2);});
    reject([](auto& x){put(x.f.memory[x.original],0xc4,x.active+0x100);});
    reject([](auto& x){put(x.f.memory[x.active],0xc4,x.original+0x100);});
    reject([](auto& x){put(x.f.memory[x.original],0xc4,x.original+x.raw.size-0x2b);});
    reject([](auto& x){put(x.f.memory[x.active],0xc4,0);});
    reject([](auto& x){x.f.memory[x.original][0x100]^=std::byte{1};});
    reject([](auto& x){x.f.memory[x.active][0x100]^=std::byte{1};});
    const std::array duplicate{moved.raw,moved.raw};bool caught=false;
    try{(void)detail::raw_globals_namespace(source,duplicate);}catch(const FormatError&){caught=true;}
    need(caught,"Duplicate retained globals owner was admitted");++negatives;
    caught=false;try{(void)detail::raw_globals_namespace(source,{});}catch(const FormatError&){caught=true;}
    need(caught,"Missing retained globals owner was admitted");++negatives;
    auto ambiguous=moved;auto extra=ambiguous.f.tags[0];extra.datum=0xe2000009;ambiguous.f.tags.push_back(extra);
    caught=false;try{(void)ambiguous.canonical();}catch(const FormatError&){caught=true;}
    need(caught,"Ambiguous active globals owner was admitted");++negatives;
    const auto reject_profile=[&](auto change){auto copy=moved;change(copy);const auto projected=copy.canonical();auto s=copy.f.source();s.metadata.tags=projected;const auto out=detail::plan(s,profiles);need(out.corrections.empty()&&out.owners[0].status==MccSoundVanillaOwnerStatus::Custom,"Modified raw globals received a retail correction");++negatives;};
    // Equal modified rows still fail the complete raw-owner fingerprint.
    reject_profile([](auto& x){x.f.memory[x.original][0x100]^=std::byte{1};x.f.memory[x.active][0x100]^=std::byte{1};});
    reject_profile([](auto& x){for(const auto at:{x.original,x.active}){const auto row=x.f.read(at+0x100,0x2c);const std::vector<std::byte> bytes(row.begin(),row.end());std::ranges::copy(bytes,x.f.memory[at].begin()+0x140);put(x.f.memory[at],0xc4,at+0x140);}});
    auto relocated=moved;const auto next=relocated.f.bank(0x400);std::ranges::copy(relocated.f.memory[relocated.active],relocated.f.memory[next].begin());put(relocated.f.memory[next],0xc4,next+0x100);relocated.f.tags[0].address=next;relocated.f.tags[0].size=0x400;
    const auto projected=relocated.canonical();auto r=relocated.f.source();r.metadata.tags=projected;need(detail::plan(r,profiles).corrections.size()==2,"A legitimate active allocation relocation changed raw sound admission");
    need(moved.f.memory==memory,"Retained globals source authentication wrote metadata");
    std::cout<<"Retained raw globals guards passed; relocation stable, negatives="<<negatives<<'\n';
}
}
void actual(const char* path){
    using namespace halo2;Cache cache(path);const auto plan=plan_mcc_sound_vanilla_flags(cache);
    const std::map<std::string,unsigned> expected{{"03a_oldmombasa",1},{"03b_newmombasa",1},{"04b_floodlab",2},
        {"06a_sentinelwalls",2},{"07a_highcharity",28},{"07b_forerunnership",26},{"08a_deltacliffs",2},
        {"08b_deltacontrol",3},{"turf",1}};
    const auto i=expected.find(cache.name());need(plan.corrections.size()==(i==expected.end()?0:i->second),"Actual finite sound correction census differs");
    unsigned authenticated=0;for(const auto& o:plan.owners){need(o.status!=MccSoundVanillaOwnerStatus::Custom,"Actual retail owner failed a complete authentication guard");authenticated+=o.status==MccSoundVanillaOwnerStatus::Authenticated;}
    need(authenticated==(i==expected.end()?0u:1u),"Actual authenticated sound owner census differs");
    for(const auto& c:plan.corrections){const auto* t=cache.find_tag(c.sound,tag_groups::sound);need(t&&t->address==c.source_root&&cache.mcc_sound_gestalt_owner(c.sound).handle()==c.gestalt,"Actual correction lost its full-salt owner");const auto raw=cache.read_tag_bytes(c.source_root,28);need(Bytes(raw).u16(0)==c.source_flags&&c.native_before==((c.source_flags&0x3ffu)|((c.source_flags&0xf000u)>>2)),"Actual planner changed raw route flags");}
    if(cache.name()=="backwash")for(const auto& t:cache.tags())if(t.address&&t.group=="snd!"&&
        (t.name=="sound\\vehicles\\monitor\\monitor_amb\\track1\\loop"||t.name=="sound\\vehicles\\monitor\\monitor_move\\track1\\loop"))
        need(Bytes(cache.read_tag_bytes(t.address,28)).u16(0)==0x700b&&plan.corrections.empty(),"Already-equivalent Backwash tuple adopted another map's flag correction");
    std::cout<<"Actual authenticated sound flag corrections="<<plan.corrections.size()<<" owners="<<authenticated<<'\n';
}
int main(int argc,char**argv)try{synthetic();retained_globals();if(argc>1)actual(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
