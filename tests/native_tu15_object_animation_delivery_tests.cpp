#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_animation_delivery_abi.hpp"
#include "native_tu15_object_node_admission_checks.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
struct Boundary {std::uint32_t entry;};
constexpr std::array bases{0x46a000u,0x474000u,0x4ec000u,0x4f0000u,0x51d000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,4096u,4096u,4096u,4096u,65536u,16384u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,bases.size()> banks;
    mutable std::vector<std::pair<std::uint32_t,unsigned>> reads;
    Memory(){for(unsigned n=0;n<banks.size();++n)banks[n].resize(sizes[n]);}
    std::span<std::byte> view(std::uint32_t at,unsigned size){
        for(unsigned n=0;n<banks.size();++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+size<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],size);
        throw std::runtime_error("Animation test escaped declared operand "+std::to_string(at));
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override{reads.emplace_back(at,out.size());std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());}
    void write(std::uint32_t at,std::span<const std::byte> in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4){auto bytes=view(at,width);for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));}
};
struct Entry {std::uint32_t entry;SourcePhysicsWorldRuntimeAbi abi;std::vector<std::byte> arguments;};
struct Result {SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{},boundary{};std::vector<SourcePhysicsContactRecordStore> stores;std::vector<Entry> entries;std::set<std::uint32_t> family_sites;};
thread_local Result*current;
thread_local std::uint32_t declared_stop;
void note(WorldState&s,std::uint32_t entry){
    const unsigned count=entry==0x1e3150u||entry==0x1e5c80u?1u:entry==0x1d5840u?7u:entry==0xbf9a0u?6u:entry==0x10e100u?3u:entry==0x1e1720u?2u:0u;
    auto&memory=static_cast<Memory&>(s.memory);const auto raw=memory.view(s.gp[4]+4u,count*4u);
    current->entries.push_back({entry,s.save(),{raw.begin(),raw.end()}});
    if(entry==declared_stop)throw Boundary{entry};
}
}
namespace reference {
#include "object_animation_delivery_reference.inc"
}
namespace {
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&abi,std::uint32_t owner,bool native,std::uint32_t stop=0){
    Result result;current=&result;declared_stop=stop;
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){result.stores.push_back(e);};SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){result.pc=pc;if((pc>=0xbfb90u&&pc<=0xbfc64u)||(pc>=0xb8780u&&pc<=0xb87ccu))result.family_sites.insert(pc);};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&result.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto call=[](auto&s,std::uint32_t entry,std::uint32_t site){reference::dispatch(s,entry,site,false);};
    try{
        if(!native){if(owner==0xbfb90u)reference::run_bfb90(state);else if(owner==0xb8780u)reference::run_b8780(state);else reference::run_b87b0(state);}
        else if(!native_object_animation_detail::bridge(state,owner,call))throw std::runtime_error("Qualified animation candidate refused default fixture");
    }catch(const Boundary&stop){result.boundary=stop.entry;}
    result.abi=state.save();return result;
}
SourcePhysicsWorldRuntimeAbi initial_abi(unsigned mode,bool df,std::uint32_t sp,std::uint32_t manager,std::uint32_t owner){
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp={4u,0u,0x12340000u,0x456789abu,sp,0x56789abcu,0x6789abcdu,0x789abcdeu};abi.eflags=0x2d7u|(df?1024u:0u);
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t((0x7fu+(mode%3u)*0x100u)|((mode%4u)<<10u)),0x1f80u|((mode%4u)<<13u)|(mode&0x3fu)|(mode&4u?0x8040u:0u)};
    abi.x87_status=0x4100u|((mode%8u)<<11u);abi.x87_depth=2;abi.x87[0]=1.234567890123456789L;abi.x87[1]=-9.876543210987654321L;
    for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x7f800001u+n*0x11111111u+k;
    if(owner==0xb8780u)abi.gp[1]=manager;
    if(owner==0xb87b0u)abi.gp[0]=manager;
    if(owner==0xbfb90u){
        if(mode==0||mode==16||mode==17)abi.gp[0]=0;
        if(mode==8)abi.gp[0]=0xffffffffu;
        if(mode==7)abi.gp[0]=32;
        if(mode==13||mode==14)abi.gp[0]=33;
        if(mode==11||mode==15)abi.gp[0]=1;
        if(mode==4||mode==5||mode==6||mode==7||mode==13||mode==14||mode==15||mode==16||mode==17)abi.gp[1]=0x61008000u;
    }
    return abi;
}
Memory seed(unsigned mode,std::uint32_t owner,std::uint32_t sp,std::uint32_t manager){
    constexpr std::uint32_t tags=0x61000800u,graph=0x61004000u,body=0x61003000u,type=0x61002000u;
    Memory memory;memory.put(0x4f0414u,tags);memory.put(tags+4u*16u+8u,graph);memory.put(graph+0x30u,0x61006000u);memory.put(graph+0x44u,0);
    memory.put(0x46a0d0u,0x3f800000u);memory.put(0x4ecbd0u,0x61000000u);memory.put(0x61000044u,0x61000400u);memory.put(0x61000408u,body);
    memory.put(body+0xaau,1u,1u);memory.put(body+0x10eu,0xff80u,2u);memory.put(0x474b94u,type);memory.put(type+0x84u,0);
    memory.put(manager,owner==0xbfb90u?0xffffffffu:4u);memory.put(manager+4u,0xffffu);memory.put(manager+0x68u,4u);
    if(owner==0xbfb90u){
        if(mode==2||mode==11){memory.put(manager,4u);memory.put(manager+4u,0xffffffffu);}
        if(mode==3)memory.put(manager,4u);
        if(mode==5)memory.put(0x61008000u,1u);
        if(mode==6)memory.put(0x61008000u,8u);
        if(mode==7)memory.put(0x61008000u,0x80000000u);
        if(mode==13)memory.put(0x61008000u,1u);
        if(mode==14)memory.put(0x61008004u,1u);
        if(mode==15)memory.put(0x61008000u,4u);
        if(mode==9||mode==10){memory.put(manager+0x65u,1u,1u);memory.put(manager+0x67u,mode==9?2u:0u,1u);}
        if(mode==17)memory.put(0x61007ffcu,0x80000000u);
    }else if(owner==0xb8780u){if(mode==0)memory.put(manager+6u,0xffffu,2u);}
    else{if(mode==0)memory.put(manager+0x68u,0xffffffffu);if(mode==1)memory.put(manager,0xffffffffu);if(mode==2)memory.put(manager+6u,0xffffu,2u);}
    memory.put(sp,0x63000000u);memory.put(sp+4u,0x61009000u);memory.put(sp+8u,manager);memory.put(sp+12u,0x6100a000u);
    return memory;
}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
std::uint32_t word(){std::uint32_t value;std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated animation fixture");return value;}
void emit_abi(const auto&a){for(auto v:a.gp)emit(v);emit(a.eflags);emit(a.floating_point->mxcsr);emit(a.x87_status);emit(a.floating_point->x87_control_word);for(const auto&x:a.xmm)for(auto v:x)emit(v);}
void emit_live(const auto&a){emit(a.x87_depth);for(unsigned n=0;n<a.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&a.x87[n]),10);}
void emit_result(const Result&got,const Memory&memory){
    emit_abi(got.abi);for(const auto&b:memory.banks)std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());emit(got.stores.size());
    for(const auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}
    emit(got.pc);emit(got.boundary);emit(got.entries.size());for(const auto&e:got.entries){emit(e.entry);emit_abi(e.abi);emit_live(e.abi);emit(e.arguments.size());std::cout.write(reinterpret_cast<const char*>(e.arguments.data()),e.arguments.size());}
}
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string_view(argv[1])=="--live-admission"){
        unsigned cases{};
        for(auto owner:{0xbfb90u,0xb8780u,0xb87b0u})cases+=object_node_test_detail::check_live_admission_fallbacks(
            [=](auto&s){return native_object_animation_detail::bridge(s,owner,[](auto&,auto,auto){throw std::runtime_error("Rejected animation called child");});},
            [=](auto&s){if(owner==0xbfb90u)reference::run_bfb90(s);else if(owner==0xb8780u)reference::run_b8780(s);else reference::run_b87b0(s);});
        std::cout<<"live_protocol_fallbacks="<<cases<<'\n';return 0;
    }
    if(argc==2&&(std::string_view(argv[1])=="--admission"||std::string_view(argv[1])=="--reference")){
        const auto cases=object_node_test_detail::check_admission_refusals<Memory>([](auto&s){return native_object_animation_detail::bridge(s,0xbfb90u,[](auto&,auto,auto){throw std::runtime_error("Rejected animation called child");});},0xbfb90u,0xbfc64u,std::string_view(argv[1])=="--reference");
        std::cout<<"admission_refusals="<<cases<<'\n';return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--emit-fixtures"){
        emit(72u);
        for(auto owner:{0xbfb90u,0xb8780u,0xb87b0u})for(unsigned mode=0;mode<(owner==0xbfb90u?18u:9u);++mode)for(bool df:{false,true}){
            const auto sp=0x62002000u+mode%16u,manager=(mode==8u&&owner!=0xbfb90u)?sp-8u:mode==12u?sp+16u:0x61001000u;
            auto memory=seed(mode,owner,sp,manager);const auto abi=initial_abi(mode,df,sp,manager,owner);const auto stop=(owner==0xb8780u&&mode==8u)?0x1e3150u:0u;
            auto trial=memory;const auto result=run(trial,abi,owner,false,stop);
            emit(owner);emit(mode);emit(df);emit(stop);emit(result.boundary);emit(result.pc);emit_abi(abi);emit_live(abi);
            for(const auto&b:memory.banks)std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());
        }
        return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--physical"){
        while(std::cin.peek()!=std::char_traits<char>::eof()){
            const auto owner=word(),stop=word();SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=word();abi.eflags=word();const auto mxcsr=word(),status=word(),cw=word();
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=status;for(auto&x:abi.xmm)for(auto&v:x)v=word();abi.x87_depth=word();
            for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);Memory memory;for(auto&b:memory.banks)std::cin.read(reinterpret_cast<char*>(b.data()),b.size());
            const auto got=run(memory,abi,owner,true,stop);if(!got.family_sites.empty())throw std::runtime_error("Native animation entered literal family body");emit_result(got,memory);
        }
        return 0;
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
