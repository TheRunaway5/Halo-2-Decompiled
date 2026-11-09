#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_resource_record_search.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace copies {
std::uint32_t sign_extend(std::uint32_t x,unsigned n) {
    if(n==1)return std::uint32_t(std::int32_t(std::int8_t(x)));
    if(n==2)return std::uint32_t(std::int32_t(std::int16_t(x)));
    return x;
}
#include "resource_record_search_bodies.inc"
}
namespace {
constexpr std::array bases{0x61000000u,0x62000000u,0x4f0000u,0x62fff000u};
constexpr std::array sizes{65536u,8192u,8192u,4096u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,4>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1]),std::vector<std::byte>(sizes[2]),std::vector<std::byte>(sizes[3])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<4;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Resource record search escaped declared source bank");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto&self=*const_cast<Memory*>(this);reads.emplace_back(at,out.size());std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));}
    std::uint32_t raw(std::uint32_t at){auto bytes=view(at,4);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated resource record search input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void supplemental() {
    constexpr std::uint32_t stack=0x62001000,entry=0x61000100,pool=0x61000200,bitmap=0x61000400,records=0x61001000;
    for(unsigned mode=0;mode<12;++mode) {
        Memory seed;seed.put(entry+12,0xffffffff);seed.put(0x4f041c,pool);seed.put(pool+0x38,5);
        seed.put(pool+0x48,bitmap);seed.put(pool+0x44,records);seed.put(pool+0x24,12);seed.put(bitmap,0x15);
        seed.put(records,0x1111);seed.put(records+8,entry+1);seed.put(records+24,0x9234);seed.put(records+32,entry);
        seed.put(records+48,0x4567);seed.put(records+56,entry);seed.put(stack,0x63000000);
        if(mode<3)seed.after_store=[mode](auto&m,const auto&e){
            if(e.instruction==0x12619c) {
                if(mode==0)m.put(entry+12,0x1234);
                if(mode==1)m.put(0x4f041c,pool+0x100);
                if(mode==2)m.put(stack-4,0xdeadbeef);
            }
            if(mode==1&&e.instruction==0x12619c){m.put(pool+0x138,0);}
        };
        else seed.after_read=[mode,done=false](auto&m,std::uint32_t at,std::size_t)mutable {
            if(done)return;
            if(mode==3&&at==entry+12){m.put(at,0x1234);done=true;}
            if(mode==4&&at==0x4f041c){m.put(at,pool+0x100);done=true;}
            if(mode==5&&at==bitmap){m.put(pool+0x38,1);done=true;}
            if(mode==6&&at==records+8){m.put(records+8,entry);m.put(records,0xaabb);done=true;}
            if(mode==7&&at==records){m.put(pool+0x48,bitmap+16);m.put(bitmap+16,0x10);done=true;}
            if(mode==8&&at==records+32){m.put(records+24,0xffff);m.put(at,entry+1);done=true;}
            if(mode==9&&at==records+24){m.put(at,0);done=true;}
            if(mode==10&&at==records+24){m.put(stack-12,0xabcdef01);m.put(stack-8,0xdeadbeef);done=true;}
            if(mode==11&&at==bitmap&&m.raw(pool+0x38)==5){m.put(pool+0x48,bitmap+16);m.put(bitmap+16,0);done=true;}
        };
        Memory a=seed,b=seed;std::vector<SourcePhysicsContactRecordStore>left,right;
        const auto run=[&](Memory&m,bool native,auto&stores){
            SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(e);if(m.after_store)m.after_store(m,e);};
            SourcePhysicsWorldRuntimeServices services;WorldState s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11111111,0,0x33333333,entry,stack,0x55555555,0x66666666,0x77777777};abi.eflags=0xad7;s.load(abi);
            if(native){if(!native_resource_record_search_detail::bridge(s))throw std::runtime_error("Supplemental native search refused");}
            else copies::reference::run_126190(s);
            return s.save();
        };
        const auto x=run(a,false,left),y=run(b,true,right);
        if(x.gp!=y.gp||x.eflags!=y.eflags||x.xmm!=y.xmm||x.x87_depth!=y.x87_depth||x.x87_status!=y.x87_status||
            a.banks!=b.banks||a.reads!=b.reads||left!=right)throw std::runtime_error("Supplemental resource search read/store/alias mismatch mode="+std::to_string(mode));
    }
    Memory memory;unsigned callbacks{};SourcePhysicsWorldRuntimeServices services;
    const auto refuses=[&](SourcePhysicsContactImage image,SourcePhysicsContactRecordObserver observer={}){
        WorldState s{memory,observer,services,image};if(native_resource_record_search_detail::bridge(s))throw std::runtime_error("Unsafe native search admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11);
    services.instruction=[&](auto){++callbacks;};refuses(SourcePhysicsContactImage::TitleUpdate15);
    std::uint32_t pc{};const std::array site{0x1261c1u};auto&cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.service_sites=site;cursor.callback_type=&services.instruction.target_type();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.service_sites={};bool trace=true;cursor.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.first_trace=nullptr;cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    if(callbacks)throw std::runtime_error("Refusal checks executed/replaced real instruction services");
}
}
int main()try {
    if(std::getenv("HALO2_RESOURCE_RECORD_SEARCH_REFUSAL_TEST")) {
        Memory memory;SourcePhysicsWorldRuntimeServices services;
        WorldState state{memory,{},services,SourcePhysicsContactImage::TitleUpdate15};
        if(native_resource_record_search_detail::bridge(state))throw std::runtime_error("Reference admission did not refuse native search");
        return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>4)throw std::runtime_error("Invalid live resource-record-search FP depth");
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
        if(copy==0)copies::primary::run_126190(state);
        else if(copy==1)copies::frame::run_126190(state);
        else throw std::runtime_error("Invalid resource-search production copy");
        if(pc!=0x1261f7u&&pc!=0x1261d1u)throw std::runtime_error("Native search final cursor differs");
        if(instructions)throw std::runtime_error("Native resource record search entered translated body");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Resource record search changed live FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Resource record search changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);emit(pc);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
