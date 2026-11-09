#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_record_search.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace copies {
#include "record_search_bodies.inc"
}
namespace {
constexpr std::array bases{0x61000000u,0x62000000u};
constexpr std::array sizes{16384u,8192u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,2>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<2;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
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
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated record search input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void supplemental() {
    constexpr std::uint32_t stack=0x62001000,table=0x61000100,records=0x61002000;
    for(unsigned mode=0;mode<8;++mode) {
        Memory seed;seed.put(table,mode==0?0:3);seed.put(table+4,records);
        for(unsigned n=0;n<5;++n)seed.put(records+n*4,n+1);
        seed.put(stack,0x63000000);seed.put(stack+4,table);seed.put(stack+8,mode==3?900:mode==4||mode==5?3:2);seed.put(stack+12,4);
        if(mode==0||mode==1||mode==6||mode==7)seed.after_store=[mode,done=false](auto&m,const auto&e)mutable {
            if(done)return;
            if(mode==0&&e.instruction==0x1e581e){m.put(stack+4,0xdeadbeef);done=true;}
            if(mode==1&&e.instruction==0x1e5848){m.put(stack+4,0);done=true;}
            if(mode==6&&e.instruction==0x1e581e){m.put(stack-4,0xdecafbad);done=true;}
            if(mode==7&&e.instruction==0x1e581e){m.put(table,0);done=true;}
        };
        if(mode==2||mode==3||mode==4||mode==5)seed.after_read=[mode,done=false](auto&m,std::uint32_t at,std::size_t)mutable {
            if(done)return;
            if(mode==2&&at==stack+4&&m.raw(at)==records+4){m.put(at,0xdeadbeef);done=true;}
            if(mode==3&&at==stack+8){m.put(at,2);done=true;}
            if(mode==4&&at==records+4){m.put(table+4,records+0x100);m.put(records+0x108,3);done=true;}
            if(mode==5&&at==stack+8){m.put(stack+12,8);m.put(records+16,3);done=true;}
        };
        Memory a=seed,b=seed;std::vector<SourcePhysicsContactRecordStore>left,right;
        const auto run=[&](Memory&m,bool native,auto&stores){
            SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(e);if(m.after_store)m.after_store(m,e);};
            SourcePhysicsWorldRuntimeServices services;WorldState s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde};abi.eflags=0xad7;s.load(abi);
            if(native){if(!native_record_search_detail::bridge(s))throw std::runtime_error("Supplemental native search refused");}
            else copies::reference::run_1e5810(s);
            return s.save();
        };
        const auto x=run(a,false,left),y=run(b,true,right);
        if(x.gp!=y.gp||x.eflags!=y.eflags||x.xmm!=y.xmm||x.x87_depth!=y.x87_depth||x.x87_status!=y.x87_status||
            a.banks!=b.banks||a.reads!=b.reads||left!=right)throw std::runtime_error("Supplemental search read/store/alias mismatch mode="+std::to_string(mode));
    }
    Memory memory;unsigned callbacks{};SourcePhysicsWorldRuntimeServices services;
    const auto refuses=[&](SourcePhysicsContactImage image,SourcePhysicsContactRecordObserver observer={}){
        WorldState s{memory,observer,services,image};if(native_record_search_detail::bridge(s))throw std::runtime_error("Unsafe native search admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11);
    services.instruction=[&](auto){++callbacks;};refuses(SourcePhysicsContactImage::TitleUpdate15);
    std::uint32_t pc{};const std::array site{0x1e5848u};auto&cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.service_sites=site;cursor.callback_type=&services.instruction.target_type();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.service_sites={};bool trace=true;cursor.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.first_trace=nullptr;cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    if(callbacks)throw std::runtime_error("Refusal checks executed/replaced real instruction services");
}
}
int main()try {
    if(std::getenv("HALO2_RECORD_SEARCH_REFUSAL_TEST")) {
        Memory memory;SourcePhysicsWorldRuntimeServices services;
        WorldState state{memory,{},services,SourcePhysicsContactImage::TitleUpdate15};
        if(native_record_search_detail::bridge(state))throw std::runtime_error("Reference admission did not refuse native search");
        return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>4)throw std::runtime_error("Invalid live record-search FP depth");
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
        switch(copy){case 0:copies::primary::run_1e5810(state);break;case 1:copies::actor::run_1e5810(state);break;
        case 2:copies::frame::run_1e5810(state);break;default:throw std::runtime_error("Invalid record search production copy");}
        if(instructions)throw std::runtime_error("Native record search entered translated body");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Record search changed live FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Record search changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
