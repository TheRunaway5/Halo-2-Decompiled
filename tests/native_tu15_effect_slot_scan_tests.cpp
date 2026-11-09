#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "gameplay/weapons/native_tu15_effect_slot_scan.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
#include "birth_color_bytes_reference.inc"
namespace copies {
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified contact sign-extension width");
}
#include "effect_slot_scan_bodies.inc"
}
namespace {
constexpr std::array bases{0x61000000u,0x62000000u};
constexpr std::array sizes{16384u,8192u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,2>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::uint32_t*write_pc{};
    std::vector<SourcePhysicsContactRecordStore>writes;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<2;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Record search escaped declared source bank");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto&self=*const_cast<Memory*>(this);reads.emplace_back(at,out.size());std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(write_pc){auto before=view(at,in.size());writes.push_back({*write_pc,at,{before.begin(),before.end()},{in.begin(),in.end()}});}
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));}
    std::uint32_t raw(std::uint32_t at){auto bytes=view(at,4);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated effect slot scan input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void birth_collectors() {
    constexpr std::array sites{0x17b0a3u,0x17e484u,0x17e4d2u,0x17e58eu,0x17b0a9u,0x17e48du,0x17e4dbu,0x17e594u,0x16ea67u};
    for(auto pc:sites)for(unsigned size:{0u,1u,3u,4u,5u})for(bool throwing:{false,true}) {
        struct Event {SourceWeaponEffectPresentation presentation;};Event event;SourceWeaponEffectPresentation current;
        unsigned old_calls{},new_calls{};std::uint32_t old_seen{},new_seen{};
        SourcePhysicsContactRecordObserver o=[&](const auto&){++old_calls;old_seen=event.presentation.colors.a;if(throwing)throw std::runtime_error("outer");};
        SourcePhysicsContactRecordObserver outer=[&](const auto&){++new_calls;new_seen=current.colors.a;if(throwing)throw std::runtime_error("outer");};
#include "birth_color_original_collector.inc"
        NativeWeaponEffectBirthColorCollector collector(current,outer);
        auto lease=collector.borrow();
        if(NativeWeaponEffectBirthColorCollector::qualified(collector.observer()))throw std::runtime_error("Unknown outer observer qualified");
        SourcePhysicsContactRecordStore store{pc,0,{},std::vector<std::byte>(size,std::byte(0x89))};
        bool a{},b{};try{observed(store);}catch(const std::runtime_error&){a=true;}
        try{collector.observer()(store);}catch(const std::runtime_error&){b=true;}
        if(a!=b||old_calls!=new_calls||old_seen!=new_seen||event.presentation.colors.a!=current.colors.a||
            event.presentation.colors.b!=current.colors.b||event.presentation.colors_observed!=current.colors_observed)
            throw std::runtime_error("Actual birth-color collector order/results differ");
    }
    SourcePhysicsContactRecordObserver empty;SourceWeaponEffectPresentation parent,child;
    NativeWeaponEffectBirthColorCollector outer(parent,empty);
    if(NativeWeaponEffectBirthColorCollector::qualified(outer.observer()))throw std::runtime_error("Collector escaped call borrow");
    {
        auto lease=outer.borrow();if(!NativeWeaponEffectBirthColorCollector::qualified(outer.observer()))throw std::runtime_error("Owned collector did not qualify");
        auto copy=outer.observer();if(NativeWeaponEffectBirthColorCollector::qualified(copy))throw std::runtime_error("Copied observer gained lease");
        NativeWeaponEffectBirthColorCollector inner(child,outer.observer());auto nested=inner.borrow();
        if(!NativeWeaponEffectBirthColorCollector::qualified(inner.observer()))throw std::runtime_error("Known outer collector did not compose");
        inner.observer()({0x17b0a3u,0,{},std::vector<std::byte>(4,std::byte(0x42))});
        if(parent.colors.a!=0x42424242u||child.colors.a!=parent.colors.a)throw std::runtime_error("Nested host-color observations differ");
        bool other_thread{};std::thread worker([&]{auto foreign=outer.borrow();other_thread=NativeWeaponEffectBirthColorCollector::qualified(outer.observer());});worker.join();
        if(other_thread)throw std::runtime_error("Collector lease crossed its executor");
    }
    if(NativeWeaponEffectBirthColorCollector::qualified(outer.observer()))throw std::runtime_error("Collector remained qualified after return");
    SourcePhysicsContactRecordObserver changing;SourceWeaponEffectPresentation one,two;
    NativeWeaponEffectBirthColorCollector first(one,changing);
    auto first_borrow=first.borrow();NativeWeaponEffectBirthColorCollector second(two,first.observer());auto second_borrow=second.borrow();
    if(!NativeWeaponEffectBirthColorCollector::qualified(first.observer())||!NativeWeaponEffectBirthColorCollector::qualified(second.observer()))
        throw std::runtime_error("Initially empty live outer did not qualify");
    constexpr std::uint32_t stack=0x62001000,pool=0x61000100;
    Memory memory;memory.put(stack,0x63000000);memory.put(pool+0x38,0);
    unsigned outer_calls{},callbacks{};std::uint32_t pc{};SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto at){++callbacks;pc=at;};constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&pc,nullptr,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,false};
    changing=[&](const auto&){++outer_calls;memory.put(pool+0x60,0xdecafbad);};
    if(NativeWeaponEffectBirthColorCollector::qualified(first.observer())||NativeWeaponEffectBirthColorCollector::qualified(second.observer())||
        native_effect_slot_scan_detail::admitted(services,second.observer()))throw std::runtime_error("Rebound outer retained collector admission");
    std::array<std::uint32_t,8>gp{0,pool,0,0,stack,0,0,0};std::uint32_t flags=0x202;
    native_weapon_effect_integer_tu15_run(memory,0x16ea60,gp,flags,services,second.observer());
    if(!callbacks||!outer_calls||memory.raw(pool+0x60)!=0xdecafbad||gp[0]!=0xffffffffu||gp[4]!=stack+4u)
        throw std::runtime_error("Rebound observer did not retain actual callbacks/backing mutation");
    changing={};
    if(!NativeWeaponEffectBirthColorCollector::qualified(first.observer())||!NativeWeaponEffectBirthColorCollector::qualified(second.observer()))
        throw std::runtime_error("Live empty outer did not regain valid scoped admission");
}
void supplemental() {
    birth_collectors();
    constexpr std::uint32_t stack=0x62001000,pool=0x61000100,bitmap=0x61002000;
    for(unsigned mode=0;mode<12;++mode) {
        Memory seed;seed.put(pool+0x38,36);seed.put(pool+0x48,bitmap);seed.put(bitmap,0);seed.put(bitmap+4,2);seed.put(stack,0x63000000);
        if(mode<6)seed.after_store=[mode,done=false](auto&m,const auto&e)mutable {
            if(done)return;
            if(mode==0&&e.instruction==0x16ea67){m.put(pool+0x38,0);done=true;}
            if(mode==1&&e.instruction==0x16ea6f){m.put(pool+0x48,bitmap+4);done=true;}
            if(mode==2&&e.instruction==0x16ea73){m.put(pool+0x48,0);done=true;}
            if(mode==3&&e.instruction==0x16ea73){m.put(pool+0x38,0);done=true;}
            if(mode==4&&e.instruction==0x16ea73){m.put(stack-4,0xdeadbeef);done=true;}
            if(mode==5&&e.instruction==0x16ea73){m.put(stack-8,0xdecafbad);done=true;}
        };
        if(mode>=6)seed.after_read=[mode,count=0u](auto&m,std::uint32_t at,std::size_t)mutable {
            if(mode==6&&at==bitmap&&++count==1)m.put(bitmap,2);
            if(mode==7&&at==bitmap&&++count==1)m.put(pool+0x48,0);
            if(mode==8&&at==bitmap&&++count==1)m.put(pool+0x38,0);
            if(mode==9&&at==bitmap&&++count==1)m.put(bitmap+4,0x80000000);
            if(mode==10&&at==pool+0x38)m.put(pool+0x48,bitmap+4);
            if(mode==11&&at==bitmap&&++count==1)m.put(stack-12,0x12345678);
        };
        Memory a=seed,b=seed;std::vector<SourcePhysicsContactRecordStore>left,right;
        const auto run=[&](Memory&m,bool native,auto&stores){
            SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(e);if(m.after_store)m.after_store(m,e);};
            SourcePhysicsWorldRuntimeServices services;WorldState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,pool,0,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde};abi.eflags=0xad7;state.load(abi);
            if(native){auto flags=abi.eflags;native_effect_slot_scan_detail::run(m,state.gp,flags,observer);state.other_flags=flags&~0x8d5u;state.flags.load(flags);}
            else copies::reference::run_16ea60(state);
            return state.save();
        };
        const auto x=run(a,false,left),y=run(b,true,right);
        if(x.gp!=y.gp||x.eflags!=y.eflags||x.xmm!=y.xmm||x.x87_status!=y.x87_status||a.banks!=b.banks||a.reads!=b.reads||left!=right)
            throw std::runtime_error("Supplemental slot scan read/store/alias mismatch mode="+std::to_string(mode));
    }
    // Unknown callbacks, mutable observers, active traces, replaced callback
    // types, altered service lists and another executor retain the full body.
    Memory memory;memory.put(pool+0x38,0);memory.put(stack,0x63000000);
    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};std::uint32_t pc{};bool trace{};
    const std::array sites{0x38846eu,0x21f590u,0x33b595u};
    const auto refused=[&]{auto gp=std::array<std::uint32_t,8>{0,pool,0,0,stack,0,0,0};auto flags=0x202u;
        if(native_effect_slot_scan_detail::try_native(memory,gp,flags,services,{}))throw std::runtime_error("Unsafe slot scan admission");
        callbacks=0;native_weapon_effect_integer_tu15_run(memory,0x16ea60,gp,flags,services,{});
        if(!callbacks)throw std::runtime_error("Refused slot scan hid the original callbacks");};
    services.instruction=[&](auto at){++callbacks;pc=at;};refused();
    auto&cursor=services.instruction_cursor;cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    trace=true;refused();trace=false;
    const auto old=services.instruction;services.instruction=[old](auto at){old(at);};refused();services.instruction=old;
    cursor.executor=std::thread::id{};refused();cursor.executor=std::this_thread::get_id();
    const std::array changed{0x38846eu,0x21f590u,0x16ea85u};cursor.service_sites=changed;refused();cursor.service_sites=sites;
    cursor.literal_service_sites=false;refused();cursor.literal_service_sites=true;
    cursor.observation_only_stores=false;
    if(native_effect_slot_scan_detail::admitted(services,[](const auto&){}))throw std::runtime_error("Mutable slot scan observer admitted");
    // The raw full-ABI facade must preserve an opaque active MMX file too.
    SourcePhysicsWorldRuntimeAbi opaque{};opaque.gp={0,pool,0xffffffff,0,stack,0,0,0};opaque.eflags=0x202;
    opaque.active_mmx=SourcePhysicsMmxAlias{};opaque.active_mmx->known_mask=0x5a;
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<10;++k)opaque.active_mmx->physical[n][k]=std::byte(n*10+k);
    const auto before=opaque.active_mmx;SourcePhysicsWorldRuntimeServices empty;
    native_weapon_effect_dispatch_tu15_run(memory,0x16ea60,opaque,empty,{});
    if(!opaque.active_mmx||opaque.active_mmx->known_mask!=before->known_mask||opaque.active_mmx->physical!=before->physical)
        throw std::runtime_error("Integer slot scan changed opaque MMX transport");
}
}
int main()try {
    if(std::getenv("HALO2_EFFECT_SLOT_REFUSAL_TEST")) {
        constexpr std::uint32_t stack=0x62001000,pool=0x61000100;
        Memory memory;memory.put(pool+0x38,0);memory.put(stack,0x63000000);
        SourcePhysicsWorldRuntimeServices services;std::array<std::uint32_t,8>gp{0,pool,0,0,stack,0,0,0};std::uint32_t flags=0x202;
        if(native_effect_slot_scan_detail::try_native(memory,gp,flags,services,{}))throw std::runtime_error("Forced reference did not refuse slot scan");
        if(gp[4]!=stack)throw std::runtime_error("Forced slot refusal changed input");
        unsigned callbacks{};std::uint32_t pc{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto at){++callbacks;pc=at;};
        services.instruction_cursor={&pc,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        native_weapon_effect_integer_tu15_run(memory,0x16ea60,gp,flags,services,{});
        if(!callbacks||pc!=0x16ea98u||gp[0]!=0xffffffffu||gp[4]!=stack+4u)
            throw std::runtime_error("Forced slot reference did not deliver actual callbacks/return");
        std::cout<<"{\"reference_admission_refused\":true,\"literal_callbacks\":"<<callbacks
            <<",\"last_literal_pc\":"<<pc<<",\"returned_eax\":"<<gp[0]<<"}\n";return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>8)throw std::runtime_error("Invalid live object-scan FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&lane:abi.xmm)for(auto&v:lane)v=read_word();Memory memory;
        for(auto&bank:memory.banks)std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());
        Memory default_memory=memory;
        Memory reference_memory=memory;std::vector<SourcePhysicsContactRecordStore>reference_stores;
        SourcePhysicsContactRecordObserver reference_observe=[&](const auto&e){reference_stores.push_back(e);};
        SourcePhysicsWorldRuntimeServices reference_services;std::uint32_t reference_pc{};
        reference_services.instruction=[&](auto at){reference_pc=at;};
        WorldState reference{reference_memory,reference_observe,reference_services,SourcePhysicsContactImage::TitleUpdate15};
        reference.load(abi);copies::reference::run_16ea60(reference);const auto expected=reference.save();
        std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(e);};
        unsigned instructions{};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};
        services.instruction=[&](auto){++instructions;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        memory.write_pc=&pc;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        switch(copy){
        case 0:copies::frame::run_16ea60(state);break;
        case 1:copies::combat::run_16ea60(state);break;
        case 2:copies::light::run_16ea60(state);break;
        case 3:copies::particle::run_16ea60(state);break;
        case 4:{auto flags=abi.eflags;native_weapon_effect_integer_tu15_run(memory,0x16ea60,state.gp,flags,services,observe);
            state.other_flags=flags&~0x8d5u;state.flags.load(flags);break;}
        case 5:{auto raw=abi;native_weapon_effect_dispatch_tu15_run(memory,0x16ea60,raw,services,observe);state.load(raw);break;}
        case 6:{
            cursor.observation_only_stores=false;SourcePhysicsContactRecordObserver empty;
            SourceWeaponEffectPresentation presentation;NativeWeaponEffectBirthColorCollector collector(presentation,empty);
            if(native_effect_slot_scan_detail::admitted(services,collector.observer()))throw std::runtime_error("Unborrowed birth observer admitted");
            auto borrow=collector.borrow();if(!native_effect_slot_scan_detail::admitted(services,collector.observer()))throw std::runtime_error("Actual birth collector did not qualify");
            auto flags=abi.eflags;native_weapon_effect_integer_tu15_run(memory,0x16ea60,state.gp,flags,services,collector.observer());
            state.other_flags=flags&~0x8d5u;state.flags.load(flags);stores=memory.writes;break;}
        default:throw std::runtime_error("Invalid effect slot production route");}
        if(instructions)throw std::runtime_error("Native effect slot scan entered translated body");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Effect slot scan changed live FP depth");
        if(result.gp!=expected.gp||result.eflags!=expected.eflags||result.xmm!=expected.xmm||
            result.x87_status!=expected.x87_status||memory.banks!=reference_memory.banks||
            stores!=reference_stores||memory.reads!=reference_memory.reads||pc!=reference_pc)
            throw std::runtime_error("Canonical scan ABI/read/store/cursor mismatch");
        auto plain=abi;native_weapon_effect_integer_tu15_run(default_memory,0x16ea60,plain.gp,plain.eflags,services,{});
        if(instructions||plain.gp!=expected.gp||plain.eflags!=expected.eflags||default_memory.banks!=reference_memory.banks)
            throw std::runtime_error("Default Effect service route retained instruction work or changed DATA/ABI");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Effect slot scan changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
