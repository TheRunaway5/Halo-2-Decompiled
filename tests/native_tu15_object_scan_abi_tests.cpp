#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_scan.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace copies {
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified contact sign-extension width");
}
#include "object_scan_bodies.inc"
}
namespace {
constexpr std::array bases{0x4ec000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,16384u,8192u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,3>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1]),std::vector<std::byte>(sizes[2])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<3;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Record search escaped declared source bank");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto&self=*const_cast<Memory*>(this);reads.emplace_back(at,out.size());std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));}
    std::uint32_t raw(std::uint32_t at){auto bytes=view(at,4);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated object scan input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void supplemental() {
    constexpr std::uint32_t stack=0x62001000,pool=0x61000100,rows=0x61002000,scan=0x61000300;
    struct DomainMemory:Memory,NativeTu15RetainedStoreObservationDomain {
        bool observes_retained_store(std::uint32_t,std::uint32_t,std::size_t)const override{return false;}
    };
    for(unsigned mode=0;mode<12;++mode) {
        DomainMemory seed;seed.put(0x4ecbd0,pool);seed.put(pool+0x38,3);seed.put(pool+0x44,rows);
        seed.put(scan,0x80);seed.put(scan+4,0);seed.put(scan+8,0xffffffff);
        for(unsigned n=0;n<3;++n){seed.put(rows+n*12,0x07000001+n);seed.put(rows+n*12+8,0x90000000+n);}
        seed.put(stack,0x63000000);seed.put(stack+4,scan);seed.put(stack+8,0);
        if(mode==0||mode==1||mode==2||mode==3||mode==4||mode==5||mode==10||mode==11)
            seed.after_store=[mode,done=false](auto&m,const auto&e)mutable {
                if(done)return;
                if(mode==0&&e.instruction==0xbcee6){m.put(0x4ecbd0,0);done=true;}
                if(mode==1&&e.instruction==0xbcee7){m.put(stack+4,scan+0x20);m.put(scan+0x20,0x80);m.put(scan+0x24,0);done=true;}
                if(mode==2&&e.instruction==0xbcef0){m.put(pool+0x44,0);done=true;}
                if(mode==3&&e.instruction==0xbcf4a){m.put(rows+8,0xdeadbeef);done=true;}
                if(mode==4&&e.instruction==0xbcf4a){m.put(stack-4,0xdecafbad);done=true;}
                if(mode==5&&e.instruction==0xbcf50){m.put(scan+8,0x12345678);done=true;}
                if(mode==10&&e.instruction==0xbceec){m.put(pool+0x44,rows+12);done=true;}
                if(mode==11&&e.instruction==0xbcef0){m.put(pool+0x38,0);done=true;}
            };
        if(mode>=6&&mode<=9)seed.after_read=[mode,done=false](auto&m,std::uint32_t at,std::size_t)mutable {
            if(done)return;
            if(mode==6&&at==scan+4){m.put(rows,0x07010001);done=true;}
            if(mode==7&&at==rows){m.put(scan,0);done=true;}
            if(mode==8&&at==scan){m.put(rows+8,0x12345678);done=true;}
            if(mode==9&&at==scan+6){m.put(pool+0x38,0);done=true;}
        };
        DomainMemory a=seed,b=seed;std::vector<SourcePhysicsContactRecordStore>left,right;
        const auto run=[&](auto&m,bool native,auto&stores){
            SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(e);if(m.after_store)m.after_store(m,e);};
            SourcePhysicsWorldRuntimeServices services;WorldState s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde};abi.eflags=0xad7;s.load(abi);
            if(native){if(!native_object_scan_detail::bridge(s))throw std::runtime_error("Supplemental native scan refused");}
            else copies::reference::run_bcee0(s);
            return s.save();
        };
        const auto x=run(a,false,left),y=run(b,true,right);
        if(x.gp!=y.gp||x.eflags!=y.eflags||x.xmm!=y.xmm||x.x87_depth!=y.x87_depth||x.x87_status!=y.x87_status||
            a.banks!=b.banks||a.reads!=b.reads||left!=right)throw std::runtime_error("Supplemental scan read/store/alias mismatch mode="+std::to_string(mode));
    }
    Memory memory;unsigned callbacks{};SourcePhysicsWorldRuntimeServices services;
    const auto refuses=[&](SourcePhysicsContactImage image,SourcePhysicsContactRecordObserver observer={}){
        WorldState s{memory,observer,services,image};if(native_object_scan_detail::bridge(s))throw std::runtime_error("Unsafe native scan admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11);
    services.instruction=[&](auto){++callbacks;};refuses(SourcePhysicsContactImage::TitleUpdate15);
    std::uint32_t pc{};const std::array site{0xbcf4au};auto&cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.service_sites=site;cursor.callback_type=&services.instruction.target_type();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.service_sites={};bool trace=true;cursor.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.first_trace=nullptr;cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    if(callbacks)throw std::runtime_error("Refusal checks executed/replaced real instruction services");
}
}
int main()try {
    if(std::getenv("HALO2_OBJECT_SCAN_REFUSAL_TEST")) {
        constexpr std::uint32_t stack=0x62001000,pool=0x61000100,scan=0x61000300;
        Memory memory;memory.put(0x4ecbd0,pool);memory.put(pool+0x38,0);memory.put(pool+0x44,0x61002000);
        memory.put(scan+4,0);memory.put(stack,0x63000000);memory.put(stack+4,scan);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=0x202;
        SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        if(native_object_scan_detail::bridge(state))throw std::runtime_error("Reference admission did not refuse native search");
        if(state.gp!=abi.gp)throw std::runtime_error("Reference refusal changed source registers");
        unsigned callbacks{};std::uint32_t pc{};
        services.instruction=[&](auto at){++callbacks;pc=at;};
        WorldState literal{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};literal.load(abi);
        copies::scene::run_bcee0(literal);
        if(!callbacks||pc!=0xbcf65u||literal.gp[0]||literal.gp[4]!=stack+8u)
            throw std::runtime_error("Forced reference did not execute the literal source route");
        std::cout<<"{\"reference_admission_refused\":true,\"literal_callbacks\":"<<callbacks
            <<",\"last_literal_pc\":"<<pc<<",\"returned_eax\":"<<literal.gp[0]<<"}\n";
        return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>4)throw std::runtime_error("Invalid live object-scan FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&lane:abi.xmm)for(auto&v:lane)v=read_word();Memory memory;
        for(auto&bank:memory.banks)std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());
        Memory reference_memory=memory;std::vector<SourcePhysicsContactRecordStore>reference_stores;
        SourcePhysicsContactRecordObserver reference_observe=[&](const auto&e){reference_stores.push_back(e);};
        SourcePhysicsWorldRuntimeServices reference_services;std::uint32_t reference_pc{};
        reference_services.instruction=[&](auto at){reference_pc=at;};
        WorldState reference{reference_memory,reference_observe,reference_services,SourcePhysicsContactImage::TitleUpdate15};
        reference.load(abi);copies::reference::run_bcee0(reference);const auto expected=reference.save();
        std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(e);};
        unsigned instructions{};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};
        services.instruction=[&](auto){++instructions;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        switch(copy){case 0:copies::scene::run_bcee0(state);break;case 1:copies::vehicle::run_bcee0(state);break;
        case 2:copies::frame::run_bcee0(state);break;default:throw std::runtime_error("Invalid object scan production copy");}
        if(instructions)throw std::runtime_error("Native object scan entered translated body");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Object scan changed live FP depth");
        if(result.gp!=expected.gp||result.eflags!=expected.eflags||result.xmm!=expected.xmm||
            result.x87_status!=expected.x87_status||memory.banks!=reference_memory.banks||
            stores!=reference_stores||memory.reads!=reference_memory.reads||pc!=reference_pc)
            throw std::runtime_error("Canonical scan ABI/read/store/cursor mismatch");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Object scan changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
