#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_prediction_copy.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "native_tu15_object_prediction_copy_reference.inc"
}
namespace {
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,2> banks{std::vector<std::byte>(4096),std::vector<std::byte>(4096)};
    auto view(std::uint32_t address,std::size_t size){
        const unsigned bank=address>=0x62000000u;const auto base=bank?0x62000000u:0x61000000u;
        if(address<base||std::uint64_t(address-base)+size>banks[bank].size())throw std::runtime_error("Live admission bank escape");
        return std::span(banks[bank]).subspan(address-base,size);
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override{auto v=const_cast<Memory*>(this)->view(address,out.size());std::copy(v.begin(),v.end(),out.begin());}
    void write(std::uint32_t address,std::span<const std::byte> in)override{auto v=view(address,in.size());std::copy(in.begin(),in.end(),v.begin());}
    void put(std::uint32_t address,std::uint32_t value){auto v=view(address,4);for(unsigned n=0;n<4;++n)v[n]=std::byte(value>>(8*n));}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::vector<std::byte>,2> banks;
    bool admitted{},native{};std::uint32_t position=0xabcdef01u;unsigned callbacks{};
};
Result run(unsigned policy,bool use_native){
    Result result;Memory memory;constexpr std::uint32_t sp=0x62000800u,source=0x61000100u;
    memory.put(sp,0x63000000u);memory.put(sp+4u,0x12345678u);memory.put(sp+8u,source);memory.put(sp+12u,0x61000400u);
    for(unsigned n=0;n<10;++n)memory.put(source+0x68u+4*n,0x10203040u+n);
    SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
    bool first{},second{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction=[&](auto pc){result.position=pc;++result.callbacks;};
    auto&cursor=services.instruction_cursor;
    cursor={&result.position,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi incoming{};incoming.gp={0x12345678u,2u,3u,4u,sp,6u,7u,8u};incoming.eflags=0x2d7u;
    incoming.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};state.load(incoming);
    // Mutate the actual live protocol only AFTER State caches its original
    // cursor and literal policy. Constructor-only refusals miss this case.
    if(policy==1)services.instruction={};
    if(policy==2)services.instruction=[&,different_type=7u](auto pc){result.position=pc;++result.callbacks;(void)different_type;};
    result.admitted=native_object_node_admission_detail::admitted(state,{{0xadcd0u,0xadce8u}});
    if(use_native)result.native=native_object_prediction_copy_detail::bridge(state);
    if(!result.native)reference::run_adcd0(state);
    result.abi=state.save();result.banks=memory.banks;return result;
}
bool same(const Result&a,const Result&b){
    return a.abi.gp==b.abi.gp&&a.abi.eflags==b.abi.eflags&&a.abi.xmm==b.abi.xmm&&a.abi.x87_depth==b.abi.x87_depth&&
        a.abi.x87_status==b.abi.x87_status&&a.abi.floating_point->x87_control_word==b.abi.floating_point->x87_control_word&&
        a.abi.floating_point->mxcsr==b.abi.floating_point->mxcsr&&a.banks==b.banks&&a.position==b.position;
}
void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
}
int main(int argc,char**argv)try{
    const bool expect_fixed=argc==2&&std::string_view(argv[1])=="--expect-fixed";
    auto original=run(0,false),native=run(0,true);
    require(native.admitted&&native.native&&same(original,native)&&native.callbacks==0&&original.callbacks>0,"Qualified original protocol changed");
    for(unsigned policy=1;policy<3;++policy){
        original=run(policy,false);native=run(policy,true);
        if(expect_fixed)require(!native.admitted&&!native.native&&same(original,native)&&native.callbacks==original.callbacks,"Live protocol mutation did not reach genuine retained fallback");
        else if(policy==1)require(native.admitted&&native.native&&!same(original,native)&&original.position==0xabcdef01u&&native.position==0xadce8u,"Cached-clear cursor defect did not reproduce");
        else require(native.admitted&&native.native&&same(original,native)&&native.callbacks==0&&original.callbacks>0,"Rebound callable delivery defect did not reproduce");
        std::cout<<"{\"policy\":\""<<(policy==1?"cleared":"rebound")<<"\",\"admitted\":"<<native.admitted<<",\"native\":"<<native.native<<",\"reference_callbacks\":"<<original.callbacks<<",\"default_callbacks\":"<<native.callbacks<<",\"reference_cursor\":"<<original.position<<",\"default_cursor\":"<<native.position<<"}\n";
    }
    std::cout<<(expect_fixed?"PASS live object-node fallback guards":"PASS reproduced both authentic stale object-node admission defects")<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
