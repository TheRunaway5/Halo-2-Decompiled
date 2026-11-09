#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_model_default_pose.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace copies {
#include "model_default_pose_bodies.inc"
}
namespace {
constexpr std::array bases{0x46a000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,65536u,8192u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,3>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1]),std::vector<std::byte>(sizes[2])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable std::vector<std::tuple<bool,std::uint32_t,std::size_t>>accesses;
    std::optional<std::size_t>fail_access;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<3;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Default pose escaped declared source bank");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto&self=*const_cast<Memory*>(this);reads.emplace_back(at,out.size());access(false,at,out.size());std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void access(bool write,std::uint32_t at,std::size_t size)const {
        accesses.emplace_back(write,at,size);
        if(fail_access&&accesses.size()-1==*fail_access)throw std::runtime_error("Declared retained memory fault");
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{access(true,at,in.size());std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated default pose input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void supplemental() {
    constexpr std::uint32_t stack=0x62001000,model=0x61000100,records=0x61001000,output=0x61002000;
    for(unsigned mode=0;mode<10;++mode) {
        Memory seed;seed.put(model+0x48,3);seed.put(model+0x4c,records);seed.put(0x46a0d0,0x3f800000);
        seed.put(model+0x148,1);seed.put(model+0x14c,records+0x100);
        for(unsigned n=0;n<128;++n)seed.put(records+n*4,n*1707+mode);
        seed.put(stack,0x63000000);seed.put(stack+4,model);seed.put(stack+8,output);
        if(mode<6)seed.after_store=[mode,done=false](auto&m,const auto&e)mutable {
            if(done)return;
            if(mode==0&&e.instruction==0x1707cf){m.put(model+0x48,1);done=true;}
            if(mode==1&&e.instruction==0x1707d4){m.put(records+0x20,0xdeadbeef);done=true;}
            if(mode==2&&e.instruction==0x1707f6){m.put(stack+4,model+0x100);done=true;}
            if(mode==3&&e.instruction==0x1707fd){m.put(model+0x4c,records+0x100);done=true;}
            if(mode==4&&e.instruction==0x1707eb){m.put(stack-4,0xdeadbeef);done=true;}
            if(mode==5&&e.instruction==0x1707cf){m.put(stack+8,0);done=true;}
        };
        else seed.after_read=[mode,done=false](auto&m,std::uint32_t at,std::size_t)mutable {
            if(done)return;
            if(mode==6&&at==records+0x18){m.put(records+0x1c,0xdeadbeef);done=true;}
            if(mode==7&&at==model+0x4c){m.put(model+0x4c,records+0x100);done=true;}
            if(mode==8&&at==stack+4){m.put(stack+4,model+0x100);done=true;}
            if(mode==9&&at==0x46a0d0){m.put(0x46a0d0,0);done=true;}
        };
        Memory a=seed,b=seed;std::vector<SourcePhysicsContactRecordStore>left,right;
        const auto run=[&](Memory&m,bool native,auto&stores){
            SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(e);if(m.after_store)m.after_store(m,e);};
            SourcePhysicsWorldRuntimeServices services;WorldState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde};abi.eflags=0xad7;state.load(abi);
            if(native){if(!native_model_default_pose_detail::bridge(state))throw std::runtime_error("Supplemental native pose refused");}
            else copies::reference::run_1707a0(state);
            return state.save();
        };
        const auto x=run(a,false,left),y=run(b,true,right);
        if(x.gp!=y.gp||x.eflags!=y.eflags||x.xmm!=y.xmm||x.x87_depth!=y.x87_depth||x.x87_status!=y.x87_status||
            a.banks!=b.banks||a.reads!=b.reads||left!=right)throw std::runtime_error("Supplemental pose read/store/alias mismatch mode="+std::to_string(mode));
    }
    Memory memory;unsigned callbacks{};SourcePhysicsWorldRuntimeServices services;
    const auto refuses=[&](SourcePhysicsContactImage image,SourcePhysicsContactRecordObserver observe={}){
        WorldState state{memory,observe,services,image};
        if(native_model_default_pose_detail::bridge(state))throw std::runtime_error("Unsafe native pose admission");
    };
    refuses(SourcePhysicsContactImage::MapPack11);
    services.instruction=[&](auto){++callbacks;};refuses(SourcePhysicsContactImage::TitleUpdate15);
    std::uint32_t pc{};auto&cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    const std::array site{0x1707cfu};cursor.service_sites=site;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.service_sites={};bool trace=true;cursor.first_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.first_trace=nullptr;cursor.second_trace=&trace;refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.second_trace=nullptr;cursor.observation_only_stores=false;refuses(SourcePhysicsContactImage::TitleUpdate15,[](const auto&){});
    cursor.observation_only_stores=true;cursor.executor={};refuses(SourcePhysicsContactImage::TitleUpdate15);
    cursor.executor=std::this_thread::get_id();cursor.callback_type=&typeid(void);refuses(SourcePhysicsContactImage::TitleUpdate15);
    if(callbacks)throw std::runtime_error("Pose refusal checks replaced real instruction services");
}
void faults() {
    constexpr std::uint32_t stack=0x62001000,model=0x61000100,records=0x61001000,output=0x61008000;
    unsigned prefixes{},observer_faults{};
    for(unsigned mode=0;mode<6;++mode)for(bool observed:{false,true}) {
        Memory seed;seed.put(model+0x48,mode==0?0:mode==1?0xffffffff:mode==2?3:1);seed.put(model+0x4c,records);
        for(unsigned n=0;n<128;++n)seed.put(records+n*4,0x12345678+n);
        seed.put(0x46a0d0,0x7f800001);seed.put(stack,0x63000000);seed.put(stack+4,model);seed.put(stack+8,output);
        if(mode==3)seed.put(stack+8,records+0x1c);
        if(mode==4){seed.put(stack+8,stack-16);seed.put(records+0x0c,0x63000000);seed.put(records+0x10,model);}
        if(mode==5){seed.put(stack+8,model+0x48);seed.put(records+0x18,1);}
        struct Result {SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool threw{};};
        const auto run=[&](Memory& memory,bool native,std::vector<SourcePhysicsContactRecordStore>& stores,std::optional<std::size_t> fail_observer={}) {
            std::uint32_t pc{};SourcePhysicsContactRecordObserver observe;
            if(observed)observe=[&](const auto& store){stores.push_back(store);if(fail_observer&&stores.size()-1==*fail_observer)throw std::runtime_error("Declared observer fault");};
            SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto site){pc=site;};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
            cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
            cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
            WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde};abi.eflags=0xed7;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};abi.x87_status=0x4500;abi.x87_depth=2;
            abi.x87[0]=1.234567890123456789L;abi.x87[1]=-9.876543210987654321L;
            for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=n*0x1234567u+k;
            state.load(abi);bool threw{};
            try {if(native){if(!native_model_default_pose_detail::bridge(state))throw std::runtime_error("Native pose fault fixture refused");}
                else copies::reference::run_1707a0(state);}
            catch(const std::runtime_error&){threw=true;}
            return Result{state.save(),pc,threw};
        };
        const auto equal=[&](const Result& a,const Result& b,const Memory& left,const Memory& right,const auto& ls,const auto& rs){
            const auto& x=a.abi;const auto& y=b.abi;
            bool same=x.gp==y.gp&&x.eflags==y.eflags&&x.xmm==y.xmm&&x.x87_depth==y.x87_depth&&x.x87_status==y.x87_status&&
                x.floating_point->mxcsr==y.floating_point->mxcsr&&x.floating_point->x87_control_word==y.floating_point->x87_control_word&&
                a.pc==b.pc&&a.threw==b.threw&&left.banks==right.banks&&left.reads==right.reads&&left.accesses==right.accesses&&ls==rs;
            for(unsigned n=0;n<x.x87_depth;++n)same=same&&!std::memcmp(&x.x87[n],&y.x87[n],10);
            if(!same)throw std::runtime_error("Pose retained fault ABI mismatch mode="+std::to_string(mode)+",observed="+std::to_string(observed)+",access="+(left.fail_access?std::to_string(*left.fail_access):"observer")+",pc="+std::to_string(a.pc)+"/"+std::to_string(b.pc));
        };
        Memory baseline=seed;std::vector<SourcePhysicsContactRecordStore>events;const auto initial=run(baseline,false,events);
        if(initial.threw)throw std::runtime_error("Invalid pose retained fault baseline");
        for(std::size_t failure=0;failure<=baseline.accesses.size();++failure) {
            Memory left=seed,right=seed;left.fail_access=failure;right.fail_access=failure;
            std::vector<SourcePhysicsContactRecordStore>ls,rs;
            const auto a=run(left,false,ls),b=run(right,true,rs);equal(a,b,left,right,ls,rs);++prefixes;
        }
        if(observed)for(std::size_t failure=0;failure<events.size();++failure) {
            Memory left=seed,right=seed;std::vector<SourcePhysicsContactRecordStore>ls,rs;
            const auto a=run(left,false,ls,failure),b=run(right,true,rs,failure);equal(a,b,left,right,ls,rs);++observer_faults;
        }
    }
    std::cout<<"{\"retained_memory_prefixes\":"<<prefixes<<",\"observer_faults\":"<<observer_faults<<",\"physical_page_fault_atomicity_qualified\":false}\n";
}
}
int main()try {
    if(std::getenv("HALO2_MODEL_DEFAULT_POSE_FAULT_TEST")){faults();return 0;}
    if(std::getenv("HALO2_MODEL_DEFAULT_POSE_REFUSAL_TEST")) {
        Memory memory;SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};std::uint32_t last{};
        constexpr std::uint32_t stack=0x62001000,model=0x61000100;
        memory.put(stack,0x63000000);memory.put(stack+4,model);memory.put(model+0x48,0);
        services.instruction=[&](auto pc){++callbacks;last=pc;};
        SourcePhysicsContactRecordObserver observe;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        if(native_model_default_pose_detail::bridge(state))throw std::runtime_error("Reference admission did not refuse native pose");
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;state.load(abi);copies::primary::run_1707a0(state);
        if(callbacks!=8||last!=0x170813||state.gp[0]!=model)throw std::runtime_error("Forced pose reference did not execute original body: callbacks="+std::to_string(callbacks)+",last="+std::to_string(last)+",eax="+std::to_string(state.gp[0]));
        std::cout<<"{\"native_refused\":true,\"literal_callbacks\":"<<callbacks<<",\"last_pc\":"<<last<<",\"returned_eax\":"<<state.gp[0]<<"}\n";
        return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>4)throw std::runtime_error("Invalid live pose FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&lane:abi.xmm)for(auto&v:lane)v=read_word();Memory memory;
        for(auto&bank:memory.banks)std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());
        std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe;
        if(copy<2)observe=[&](const auto&e){stores.push_back(e);};
        unsigned instructions{};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};
        services.instruction=[&](auto){++instructions;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        switch(copy%2){case 0:copies::primary::run_1707a0(state);break;case 1:copies::frame::run_1707a0(state);break;
        default:throw std::runtime_error("Invalid default pose production copy");}
        if(instructions)throw std::runtime_error("Native default pose entered translated body");
        if(pc!=0x170813)throw std::runtime_error("Native default pose final cursor changed");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Default pose changed live FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Default pose changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
