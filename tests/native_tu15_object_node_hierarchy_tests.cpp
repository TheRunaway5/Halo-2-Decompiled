#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_node_hierarchy_abi.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "object_node_hierarchy_reference.inc"
}
namespace {
constexpr std::array bases{0x45f000u,0x46a000u,0x56e000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,4096u,4096u,65536u,8192u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,5> banks;
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)> feedback;
    Memory(){for(unsigned i=0;i<banks.size();++i)banks[i].resize(sizes[i]);}
    std::span<std::byte> view(std::uint32_t at,std::size_t count){
        for(unsigned n=0;n<banks.size();++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Object hierarchy escaped declared memory bank at "+std::to_string(at));
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override{reads.emplace_back(at,out.size());auto bytes=const_cast<Memory*>(this)->view(at,out.size());std::ranges::copy(bytes,out.begin());}
    void write(std::uint32_t at,std::span<const std::byte> in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t v){auto b=view(at,4);for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8*n));}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>> entries;
    std::uint64_t callbacks{},native_children{},reference_children{};
};
Result run(Memory &memory,const SourcePhysicsWorldRuntimeAbi &abi,unsigned mode){
    Result result;SourcePhysicsContactRecordObserver observe=[&](const auto&event){result.stores.push_back(event);if(memory.feedback)memory.feedback(memory,event);};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){result.pc=pc;++result.callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    auto &cursor=services.instruction_cursor;cursor.position=&result.pc;cursor.service_sites=service_sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto call=[&](auto&s,std::uint32_t target){
        result.entries.emplace_back(target,s.save());
        if(mode==2){
            bool accepted{};
            if(target==0x144cf0u)accepted=native_tu15_leaves::run(s,[](auto&io,auto&m){native_tu15_leaves::kernel_144cf0(io,m);},[](auto&s){reference::run_144cf0(s);},0x144e4eu);
            else accepted=native_tu15_leaves::run(s,[](auto&io,auto&m){native_tu15_leaves::kernel_145560(io,m);},[](auto&s){reference::run_145560(s);},0x14566eu);
            if(!accepted)throw std::runtime_error("Qualified hierarchy matrix child refused");++result.native_children;
        }else{++result.reference_children;if(target==0x144cf0u)reference::run_144cf0(s);else reference::run_145560(s);}
    };
    if(mode==0)reference::run_stage(state);
    else native_object_node_hierarchy_detail::run_private_stage(state,call);
    result.abi=state.save();return result;
}
bool same_abi(const auto&a,const auto&b){
    bool same=a.gp==b.gp&&a.eflags==b.eflags&&a.xmm==b.xmm&&a.x87_depth==b.x87_depth&&a.x87_status==b.x87_status&&
        a.floating_point->mxcsr==b.floating_point->mxcsr&&a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    for(unsigned n=0;n<a.x87_depth;++n)same=same&&!std::memcmp(&a.x87[n],&b.x87[n],10);
    return same;
}
void supplemental(){
    constexpr std::uint32_t stack=0x62001000,model=0x61000100,records=0x61001000,locals=0x61004000,matrices=0x61006000;
    unsigned cases{},children{};
    for(unsigned scenario=0;scenario<10;++scenario)for(unsigned count:{0u,1u,2u,4u,33u,0xffffffffu,0x80000000u}){
        Memory seed;seed.put(model+0x48,count);seed.put(model+0x4c,records);seed.put(model+0x148,2u);seed.put(model+0x14c,records);
        seed.put(stack+0x14,model);seed.put(stack+0x18,locals);seed.put(stack+0x1c,matrices);seed.put(stack+0xf8,0x63000000u);
        seed.put(0x46a0d0,0x3f800000u);seed.put(0x56e684,1u);seed.put(0x56e680,0x34000000u);
        for(unsigned n=0;n<4;++n)seed.put(0x45fbc0+n*4,0x80000000u);
        for(unsigned n=0;n<64;++n){seed.put(records+n*96+4,n?((n-1)&0xffffu):0xffffu);
            seed.put(locals+n*32+12,0x3f800000u);seed.put(locals+n*32+28,0x3f800000u);seed.put(matrices+n*52,0x3f800000u);
            for(unsigned k:{4u,20u,36u})seed.put(matrices+n*52+k,0x3f800000u);}
        if(scenario==1)for(unsigned n=0;n<8;++n)seed.put(stack+0x70+n*4,0xffffffffu);
        if(scenario==2)seed.put(stack+0x70,0xaaaaaaaau);
        if(scenario>=3)seed.feedback=[scenario,done=false](auto&m,const auto&e)mutable{
            if(done||e.instruction<0x144cf0u||e.instruction>0x144eb0u)return;
            done=true;
            if(scenario==3)m.put(model+0x48,2u);
            if(scenario==4)m.put(stack+0x14,model+0x100u);
            if(scenario==5)m.put(stack+0x70,0xffffffffu);
            if(scenario==6)m.put(records+96+4,0u);
            if(scenario==7)m.put(stack+0x1c,matrices+52u);
            if(scenario==8)m.put(model+0x4c,records-96u);
            if(scenario==9)m.put(stack+0x14,model+0x100u);
        };
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1u,2u,3u,4u,stack,model,6u,7u};abi.eflags=0xed7u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};abi.x87_depth=2;abi.x87_status=0x4100;
        abi.x87[0]=1.234567890123456789L;abi.x87[1]=-9.876543210987654321L;
        for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=n*0x11111111u+k;
        Memory expected=seed;const auto ref=run(expected,abi,0);
        for(unsigned mode:{1u,2u}){Memory actual=seed;const auto got=run(actual,abi,mode);
            if(!same_abi(ref.abi,got.abi)||ref.pc!=got.pc||expected.banks!=actual.banks||ref.stores!=got.stores||expected.reads!=actual.reads)
                throw std::runtime_error("Object hierarchy canonical seam mismatch scenario="+std::to_string(scenario)+",count="+std::to_string(count)+",mode="+std::to_string(mode)+",pc="+std::to_string(ref.pc)+"/"+std::to_string(got.pc));
            if(mode==2&&got.callbacks)throw std::runtime_error("Native hierarchy used an instruction callback");
            children+=got.native_children;++cases;
        }
    }
    std::cerr<<"canonical_cases="<<cases<<" native_children="<<children<<"\n";
}
std::uint32_t word(){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated hierarchy fixture");return v;}
void emit(std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);}
}
int main(){try{
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        const auto mode=word();SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=word();abi.eflags=word();
        const auto mxcsr=word(),cw=word(),status=word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=status;
        abi.x87_depth=word();for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&x:abi.xmm)for(auto&v:x)v=word();Memory memory;for(auto&b:memory.banks)std::cin.read(reinterpret_cast<char*>(b.data()),b.size());
        const auto result=run(memory,abi,mode);
        for(auto v:result.abi.gp)emit(v);emit(result.abi.eflags);emit(result.abi.floating_point->mxcsr);emit(result.abi.x87_status);emit(result.abi.floating_point->x87_control_word);
        for(auto&x:result.abi.xmm)for(auto v:x)emit(v);for(auto&b:memory.banks)std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());
        emit(result.stores.size());for(const auto&e:result.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}
        emit(result.pc);emit(result.native_children);emit(result.reference_children);emit(result.callbacks);
        emit(result.entries.size());for(const auto&[entry,a]:result.entries){
            emit(entry);for(auto v:a.gp)emit(v);emit(a.eflags);emit(a.floating_point->mxcsr);emit(a.x87_status);emit(a.floating_point->x87_control_word);
            for(const auto&x:a.xmm)for(auto v:x)emit(v);emit(a.x87_depth);
            for(unsigned n=0;n<a.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&a.x87[n]),10);
        }
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
