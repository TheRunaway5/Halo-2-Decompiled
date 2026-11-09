#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_record_lookup.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace copies {
#include "record_lookup_bodies.inc"
}
namespace {
constexpr std::array bases{0x61000000u,0x62000000u,0x4f0000u,0x62fff000u};
constexpr std::array sizes{16384u,8192u,8192u,4096u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,4>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1]),std::vector<std::byte>(sizes[2]),std::vector<std::byte>(sizes[3])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<4;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Record lookup escaped declared source bank");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto&self=*const_cast<Memory*>(this);reads.emplace_back(at,out.size());std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));}
    std::uint32_t raw(std::uint32_t at){auto bytes=view(at,4);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated record lookup input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void supplemental() {
    constexpr std::uint32_t stack=0x62001000,descriptor=0x61000100,slots=0x61002000,objects=0x61003000;
    for(unsigned mode=0;mode<8;++mode) {
        Memory seed;seed.put(descriptor+0x4c,3);seed.put(descriptor+0x50,slots);seed.put(slots+4,2);
        seed.put(0x4f0414,objects);seed.put(objects+40,0x12345678);seed.put(stack,0x63000000);
        if(mode==0||mode==1)seed.after_store=[mode](auto&m,const auto&e){
            if(e.instruction==0x1e3270)m.put(mode==0?descriptor+0x4c:stack-4,mode==0?0:0xdeadbeef);
        };
        else seed.after_read=[mode,done=false](auto&m,std::uint32_t at,std::size_t)mutable {
            if(done)return;
            if(mode==2&&at==descriptor+0x4c){m.put(descriptor+0x50,slots+32);m.put(slots+36,2);done=true;}
            if(mode==3&&at==descriptor+0x50){m.put(descriptor+0x4c,0);done=true;}
            if(mode==4&&at==slots+4){m.put(at,0xffffffff);done=true;}
            if(mode==5&&at==0x4f0414){m.put(at,objects+0x100);done=true;}
            if(mode==6&&at==objects+40){m.put(at,0xdeadbeef);done=true;}
            if(mode==7&&at==objects+40){m.put(stack-4,0xdecafbad);done=true;}
        };
        Memory a=seed,b=seed;std::vector<SourcePhysicsContactRecordStore>left,right;
        const auto run=[&](Memory&m,bool native,auto&stores){
            SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(e);if(m.after_store)m.after_store(m,e);};
            SourcePhysicsWorldRuntimeServices services;WorldState s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11111111,0,descriptor,0x44444444,stack,0x55555555,0x66666666,0x77777777};abi.eflags=0xad7;s.load(abi);
            if(native){if(!native_record_lookup_detail::bridge(s))throw std::runtime_error("Supplemental native lookup refused");}
            else copies::reference::run_1e3270(s);
            return s.save();
        };
        const auto x=run(a,false,left),y=run(b,true,right);
        // The old canonical Flags::shift forces undefined AF to zero; these
        // supplementary cases qualify read/store/GP aliasing. The separate
        // unchanged-hardware gate compares every returned EFLAGS bit.
        if(x.gp!=y.gp||x.xmm!=y.xmm||x.x87_depth!=y.x87_depth||x.x87_status!=y.x87_status||
            a.banks!=b.banks||a.reads!=b.reads||left!=right)throw std::runtime_error("Supplemental lookup read/store/alias mismatch mode="+std::to_string(mode));
    }
    Memory memory;unsigned callbacks{};SourcePhysicsWorldRuntimeServices services;
    const auto refuses=[&](SourcePhysicsContactImage image,SourcePhysicsContactRecordObserver observer={}){
        WorldState s{memory,observer,services,image};if(native_record_lookup_detail::bridge(s))throw std::runtime_error("Unsafe native search admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11);
    services.instruction=[&](auto){++callbacks;};refuses(SourcePhysicsContactImage::TitleUpdate15);
    std::uint32_t pc{};const std::array site{0x1e3282u};auto&cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.service_sites=site;cursor.callback_type=&services.instruction.target_type();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.service_sites={};bool trace=true;cursor.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.first_trace=nullptr;cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    if(callbacks)throw std::runtime_error("Refusal checks executed/replaced real instruction services");
}
}
int main()try {
    if(std::getenv("HALO2_RECORD_LOOKUP_REFUSAL_TEST")) {
        Memory memory;SourcePhysicsWorldRuntimeServices services;
        WorldState state{memory,{},services,SourcePhysicsContactImage::TitleUpdate15};
        if(native_record_lookup_detail::bridge(state))throw std::runtime_error("Reference admission did not refuse native search");
        return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>4)throw std::runtime_error("Invalid live record-lookup FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&lane:abi.xmm)for(auto&v:lane)v=read_word();Memory memory;
        for(auto&bank:memory.banks)std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());
        std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(e);};
        unsigned instructions{};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};
        services.instruction=[&](auto){++instructions;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        if(copy)throw std::runtime_error("Invalid lookup production copy");
        copies::primary::run_1e3270(state);
        if(pc!=0x1e329du)throw std::runtime_error("Native lookup final cursor differs");
        if(instructions)throw std::runtime_error("Native record lookup entered translated body");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Record lookup changed live FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Record lookup changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
