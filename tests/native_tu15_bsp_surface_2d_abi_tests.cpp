#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_bsp_surface_2d.hpp"
#ifdef HALO2_BSP_2D_OWNER_DISPATCH
#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"
#endif
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
std::uint32_t sign_extend(std::uint32_t v,unsigned width){return std::uint32_t(width==1?std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))):std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
namespace copies {
#include "bsp_surface_2d_bodies.inc"
}
namespace {
constexpr std::array bases{0x61000000u,0x61080000u,0x62000000u};
constexpr std::array sizes{65536u,8192u,8192u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,3>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1]),std::vector<std::byte>(sizes[2])};
    std::uint32_t* pc{};mutable std::vector<std::array<std::uint32_t,3>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t width){
        for(unsigned n=0;n<3;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+width<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],width);
        throw std::runtime_error("BSP2D escaped declared backing");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto& self=*const_cast<Memory*>(this);reads.push_back({pc?*pc:0,at,std::uint32_t(out.size())});std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4){auto bytes=view(at,width);for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8*n));}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated BSP2D input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void abi(const SourcePhysicsWorldRuntimeAbi& a){
    for(auto v:a.gp)emit(v);emit(a.eflags);emit(a.floating_point->mxcsr);emit(a.x87_status);emit(a.floating_point->x87_control_word);
    for(const auto& lane:a.xmm)for(auto v:lane)emit(v);
}
void supplemental(){
    constexpr std::uint32_t sp=0x62001000,descriptor=0x61000100,nodes=0x61001000,point=0x6100f000;
    for(unsigned mode=0;mode<10;++mode){
        Memory seed;seed.put(sp,0x63000000);seed.put(descriptor+4,nodes);seed.put(point,0x3f800000);seed.put(point+4,0);
        for(unsigned n=0;n<3;++n){seed.put(nodes+n*16,0x3f800000);seed.put(nodes+n*16+4,0);seed.put(nodes+n*16+8,0);seed.put(nodes+n*16+12,n<2?n+1:0xffff,2);seed.put(nodes+n*16+14,n<2?n+1:0x8003,2);}
        if(mode<7)seed.after_read=[mode,done=false](auto& m,std::uint32_t at,std::size_t)mutable{
            if(done)return;
            if(mode==0&&at==descriptor+4){m.put(descriptor+4,0);done=true;}
            if(mode==1&&at==nodes+4){m.put(point+4,0x40000000);done=true;}
            if(mode==2&&at==point+4){m.put(nodes,0x40000000);done=true;}
            if(mode==3&&at==nodes){m.put(point,0xbf800000);done=true;}
            if(mode==4&&at==point){m.put(nodes+8,0x40000000);done=true;}
            if(mode==5&&at==nodes+8){m.put(nodes+14,0xffff,2);done=true;}
            if(mode==6&&at==nodes+14){m.put(descriptor+4,nodes+0x100);done=true;}
        };
        else seed.after_store=[mode,done=false](auto& m,const auto& e)mutable{
            if(done||e.instruction!=0x2a0237)return;
            if(mode==7)m.put(sp-4,0xdeadbeef);if(mode==8)m.put(point+4,0xbf800000);
            if(mode==9){m.put(descriptor+4,nodes+16);m.put(nodes+28,0xffff,2);m.put(nodes+30,0x8003,2);}done=true;
        };
        Memory left=seed,right=seed;std::vector<SourcePhysicsContactRecordStore>ls,rs;
        const auto run=[&](Memory& memory,bool native,auto& stores){
            std::uint32_t pc{};memory.pc=&pc;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto site){pc=site;};
            const std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
            cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
            SourcePhysicsContactRecordObserver observe=[&](const auto& e){stores.push_back(e);if(memory.after_store)memory.after_store(memory,e);};
            WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
            input.gp={0,descriptor,point,0x12345678,sp,0x23456789,0x3456789a,0x456789ab};input.eflags=0xed7;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(input);
            if(native){if(!native_bsp_surface_2d_detail::bridge(state))throw std::runtime_error("BSP2D supplemental refused");}
            else copies::reference::run_2a0230(state);
            auto out=state.save();memory.pc=nullptr;return std::pair{out,pc};
        };
        const auto a=run(left,false,ls),b=run(right,true,rs);
        if(a.first.gp!=b.first.gp||a.first.eflags!=b.first.eflags||a.first.xmm!=b.first.xmm||a.first.floating_point->mxcsr!=b.first.floating_point->mxcsr||a.second!=b.second||left.banks!=right.banks||left.reads!=right.reads||ls!=rs)
            throw std::runtime_error("BSP2D canonical lazy-read/store mismatch mode="+std::to_string(mode));
    }
    Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver empty;
    const auto refuses=[&](SourcePhysicsContactImage image,const SourcePhysicsContactRecordObserver& observe){
        WorldState state{memory,observe,services,image};
        if(native_bsp_surface_2d_detail::bridge(state))throw std::runtime_error("Unsafe BSP2D admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11,empty);services.instruction=[](auto){};refuses(SourcePhysicsContactImage::TitleUpdate15,empty);
    std::uint32_t pc{};const std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    bool trace=true;cursor.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15,empty);cursor.first_trace=nullptr;
    cursor.second_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15,empty);cursor.second_trace=nullptr;
    SourcePhysicsContactRecordObserver unknown=[](const auto&){};cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,unknown);cursor.observation_only_stores=true;
    cursor.executor={};refuses(SourcePhysicsContactImage::TitleUpdate15,empty);cursor.executor=std::this_thread::get_id();
    cursor.callback_type=&typeid(void);refuses(SourcePhysicsContactImage::TitleUpdate15,empty);cursor.callback_type=&services.instruction.target_type();
    WorldState state{memory,empty,services,SourcePhysicsContactImage::TitleUpdate15};
    const std::array active_site{0x2a0248u};cursor.service_sites=active_site;
    if(native_bsp_surface_2d_detail::bridge(state))throw std::runtime_error("BSP2D replaced a live source instruction service");cursor.service_sites=sites;
    state.active_mmx=SourcePhysicsMmxAlias{};
    if(native_bsp_surface_2d_detail::bridge(state))throw std::runtime_error("BSP2D entered unqualified active MMX domain");
}
}
int main()try{
    if(std::getenv("HALO2_BSP_SURFACE_2D_REFERENCE_TEST")){
        Memory memory;constexpr std::uint32_t sp=0x62001000;memory.put(sp,0x63000000);SourcePhysicsContactRecordObserver observe;
        SourcePhysicsWorldRuntimeServices services;unsigned calls{};std::uint32_t pc{};services.instruction=[&](auto site){++calls;pc=site;};
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.gp[0]=0xffffffff;input.gp[4]=sp;state.load(input);
        if(native_bsp_surface_2d_detail::bridge(state))throw std::runtime_error("Forced BSP2D reference admitted native");
#ifdef HALO2_BSP_2D_OWNER_DISPATCH
        if(native_allocator_owner_detail::dispatch(state,0x2a0230u,[](auto&,auto,auto,bool){throw std::runtime_error("BSP2D requested a child");})||!native_allocator_owner_detail::activity().bsp_entries.empty())throw std::runtime_error("Forced BSP2D reference admitted actual owner");
#endif
        copies::ai::run_2a0230(state);
        if(calls!=6||pc!=0x2a028d||state.gp[0]!=0xffffffff)throw std::runtime_error("BSP2D forced reference did not execute");
        std::cout<<"{\"callbacks\":"<<calls<<",\"last_pc\":"<<pc<<",\"returned_eax\":"<<state.gp[0]<<",\"native_refused\":true}\n";return 0;
    }
    supplemental();const bool reads_only=std::getenv("HALO2_BSP_SURFACE_2D_READ_TEST");
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi input{};for(auto& v:input.gp)v=read_word();input.eflags=read_word();
        const auto mxcsr=read_word(),cw=read_word();input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};input.x87_status=read_word();input.x87_depth=read_word();
        if(input.x87_depth>8)throw std::runtime_error("Invalid BSP2D live depth");for(unsigned n=0;n<input.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&input.x87[n]),10);
        for(auto& lane:input.xmm)for(auto& v:lane)v=read_word();Memory memory;for(auto& bank:memory.banks)std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());
        std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe;if(copy<3||copy==6)observe=[&](const auto& e){stores.push_back(e);};
        std::uint32_t pc{};memory.pc=&pc;unsigned instructions{};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++instructions;};
        const std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
        std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>phases;
        if(copy==2){if(!native_bsp_surface_2d_detail::bridge(state,[&](auto& s,auto site){phases.emplace_back(site,s.save());}))throw std::runtime_error("BSP2D raw entry refused");}
        else if(copy==0||copy==3)copies::ai::run_2a0230(state);else if(copy==1||copy==4)copies::death::run_2a0230(state);
        else if(copy==6)copies::literal::run_2a0230(state);
#ifdef HALO2_BSP_2D_OWNER_DISPATCH
        else if(copy==5){
            auto& activity=native_allocator_owner_detail::activity();const auto prior=activity.bsp_entries[0x2a0230u];
            if(!native_allocator_owner_detail::dispatch(state,0x2a0230u,[](auto&,auto,auto,bool){throw std::runtime_error("BSP2D requested a child");}))throw std::runtime_error("BSP2D actual owner refused");
            if(activity.bsp_entries[0x2a0230u]!=prior+1||!activity.entries.empty()||!activity.tick_entries.empty()||!activity.epoch_entries.empty())throw std::runtime_error("BSP2D actual owner activity differs");
        }
#endif
        else throw std::runtime_error("Invalid BSP2D copy");
        if(instructions)throw std::runtime_error("BSP2D entered translated body");const auto result=state.save();
        if(result.x87_depth!=input.x87_depth)throw std::runtime_error("BSP2D changed caller FP depth");
        for(unsigned n=0;n<input.x87_depth;++n)if(std::memcmp(&result.x87[n],&input.x87[n],10))throw std::runtime_error("BSP2D changed live80 caller");
        if(reads_only){emit(memory.reads.size());for(const auto& event:memory.reads)for(auto value:event)emit(value);continue;}
        abi(result);for(const auto& bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto& e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
        emit(phases.size());for(const auto& [site,a]:phases){emit(site);abi(a);emit(a.x87_depth);for(unsigned n=0;n<a.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&a.x87[n]),10);}
    }
    return std::cout?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
