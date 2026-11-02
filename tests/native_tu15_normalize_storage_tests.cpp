#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
constexpr std::uint32_t vector=0x1000100,stack=0x62000000,sp=stack+0x800;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class T>void put(SourceRuntimeMemory &m,std::uint32_t address,const T &value){m.write(address,std::as_bytes(std::span(&value,1)));}
void mount(SourceRuntimeMemory &m,bool bounded,std::uint32_t v=vector,std::uint32_t e=sp,bool small_vector=false,bool small_stack=false){
    using Access=SourceRuntimeMemory::Access;
    const auto vector_base=v==0x48c558u?0x48c530u:0x1000000u;
    if(small_vector)m.mount(v==0x48c558u?0x48c530u:v,std::vector<std::byte>(v==0x48c558u?0x88u:12u),Access::ReadWrite,"bounded vector owner");
    else m.mount(vector_base,std::vector<std::byte>(8192),Access::ReadWrite,"complete vector region control");
    m.mount(small_stack?e-4u:stack,std::vector<std::byte>(small_stack?8u:8192u),Access::ReadWrite,"actual caller stack owner");
    const std::array<float,3> input{3,4,5};m.write(v,std::as_bytes(std::span(input)));put(m,e,std::uint32_t(0x60300000));
    if(!bounded)m.mount(0x46a000,std::vector<std::byte>(4096),Access::ReadWrite,"complete constant page control");
    for(const auto [address,value]:{std::pair{0x46a0d0u,0x3f800000u},std::pair{0x46a0ecu,0x38d1b717u}}){
        if(bounded)m.mount(address,std::as_bytes(std::span(&value,1)),Access::ReadOnly,"exact authored scalar owner");
        else put(m,address,value);
    }
}
SourcePhysicsContactRuntimeAbi entry(std::uint32_t v=vector,std::uint32_t e=sp){
    SourcePhysicsContactRuntimeAbi a{};a.gp={v,0xaabbccdd,0x12345678,0x89abcdef,e,0x76543210,0xbabababa,0xcdcdcdcd};
    a.eflags=0x646;a.fs_base=0x7ffdf000;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    for(unsigned i=0;i<a.xmm.size();++i)for(unsigned n=0;n<a.xmm[i].size();++n)a.xmm[i][n]=0x7f012340u+i*16+n;
    return a;
}
SourcePhysicsContactRuntimeAbi run(SourceRuntimeMemory &m,bool guarded,NativeTu15NormalizeStats &stats,
    bool gate=true,std::uint32_t v=vector,std::uint32_t e=sp){
    SourcePhysicsContactRuntimeServices services{};services.normalize_vector_30bd0_stats=&stats;
    if(guarded)services.normalize_vector_30bd0_guarded=[&](auto &memory,auto v,auto e,const auto &observe,auto constants)
        ->std::optional<NativeTu15NormalizeOutcome>{
        need(&memory==&m,"Guarded storage owner changed");
        const std::array<SourceRuntimeNativeWriteRange,2> writes{{{v,12},{e-4u,4}}};
        SourceRuntimeNativeWriteLease lease(m,writes);if(!lease)return std::nullopt;
        auto result=native_tu15_normalize_vector_30bd0_resident(m,v,observe,constants);put(m,e-4u,result.magnitude_bits);return result;
    };
    else services.normalize_vector_30bd0=native_tu15_normalize_vector_30bd0_resident;
    const auto input=entry(v,e);physics_step_detail::FpScope fp(*input.floating_point);
    const SourcePhysicsContactRecordObserver observe{};
    physics_world_runtime_detail::WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(input);
    if(gate){const bool admitted=native_tu15_normalize_vector_try(state,NativeTu15NormalizeProfile::Tu15Literal);
        need(admitted==(stats.admitted!=0),"Storage gate result and counter differ");}
    else native_tu15_normalize_vector_state(state,native_tu15_normalize_vector_30bd0_resident,NativeTu15NormalizeProfile::Tu15Literal);
    return state.save();
}
void same(const SourcePhysicsContactRuntimeAbi &expected,const SourcePhysicsContactRuntimeAbi &actual){
    need(expected.gp==actual.gp&&expected.xmm==actual.xmm&&expected.eflags==actual.eflags&&expected.fs_base==actual.fs_base,
        "Real bounded storage changed complete integer/SIMD ABI");
    need(expected.x87_depth==actual.x87_depth&&expected.x87_status==actual.x87_status&&
        expected.floating_point->x87_control_word==actual.floating_point->x87_control_word&&
        expected.floating_point->mxcsr==actual.floating_point->mxcsr,"Real bounded storage changed FP state");
}
}
int main()try{
    SourceRuntimeMemory full,bounded;mount(full,false);mount(bounded,true);
    for(auto address:{0x46a0d0u,0x46a0ecu}){
        need(!bounded.direct_page(address,4),"Bounded scalar borrowed a whole page");
        need(bounded.try_read_view(address,4).has_value()&&bounded.is_read_only(address,4),"Exact immutable scalar capability missing");
        need(!bounded.try_read_view(address-1,1)&&!bounded.try_read_view(address+4,1),"Scalar capability borrowed adjacent unowned bytes");
    }
    NativeTu15NormalizeStats control{},candidate{};const auto expected=run(full,false,control),actual=run(bounded,true,candidate);
    if(control.admitted!=1||candidate.admitted!=1){
        std::cerr<<"storage admission control="<<control.admitted<<" candidate="<<candidate.admitted<<" control refusals=";
        for(auto n:control.refused)std::cerr<<n<<',';
        std::cerr<<" candidate refusals=";for(auto n:candidate.refused)std::cerr<<n<<',';std::cerr<<'\n';
    }
    need(control.admitted==1&&candidate.admitted==1,"Real bounded storage did not reach the kernel");
    same(expected,actual);
    need(std::ranges::equal(full.view(vector,12),bounded.view(vector,12))&&std::ranges::equal(full.view(stack,8192),bounded.view(stack,8192)),
        "Real bounded storage changed vector or complete caller stack");
    for(auto address:{0x46a0d0u,0x46a0ecu})need(std::ranges::equal(full.view(address,4),bounded.view(address,4)),"Authored constant changed");
    const auto before=bounded.checkpoint();int observer_owner;unsigned calls{};
    source_runtime_register_write_observer(bounded,&observer_owner,[&](auto,auto){++calls;});
    NativeTu15NormalizeStats refused{};const auto after=run(bounded,true,refused);
    source_runtime_unregister_write_observer(bounded,&observer_owner);
    need(!refused.admitted&&refused.refused[std::size_t(NativeTu15NormalizeRefusal::Policies)]==1&&!calls,
        "Real bank bypassed an unknown observer");
    need(before==bounded.checkpoint()&&after.gp==entry().gp&&after.xmm==entry().xmm&&after.eflags==entry().eflags,
        "Real-bank policy refusal changed source state");
    struct Case{std::uint32_t v,e;bool small_vector,small_stack;};
    for(const auto &c:{Case{vector,sp,true,false},Case{0x48c558u,sp,true,false},
        Case{0x1000ff8u,sp,false,false},Case{vector,stack+4096,false,false},Case{vector,sp,true,true}}){
        SourceRuntimeMemory reference,source;mount(reference,false,c.v,c.e);mount(source,true,c.v,c.e,c.small_vector,c.small_stack);
        need(source.try_write_extent(c.v,12).has_value()&&source.try_write_extent(c.e-4,8).has_value(),"Real ordinary write extent missing");
        NativeTu15NormalizeStats r{},n{};const auto gold=run(reference,false,r,false,c.v,c.e),native=run(source,true,n,true,c.v,c.e);
        need(n.admitted==1&&!n.refused[std::size_t(NativeTu15NormalizeRefusal::Pages)],"Real bounded or crossing write extent refused");
        same(gold,native);
        need(std::ranges::equal(reference.view(c.v,12),source.view(c.v,12))&&std::ranges::equal(reference.view(c.e-4,8),source.view(c.e-4,8)),
            "Real write-extent kernel changed vector/stack bytes");
        if(c.small_vector&&c.v==vector)need(!source.try_read_view(c.v-1,1)&&!source.try_read_view(c.v+12,1),"Vector extent borrowed its gaps");
        if(c.small_stack)need(!source.try_read_view(c.e-5,1)&&!source.try_read_view(c.e+4,1),"Stack extent borrowed its gaps");
    }
    std::cout<<"PASS real constant/vector/stack extents, crossing pages, complete ABI, unowned gaps and observer refusal\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
