#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using WorldState=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
J snapshot(const WorldState&s){
    const auto a=s.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n){
        std::string raw;constexpr char hex[]="0123456789abcdef";
        for(unsigned j=0;j<10;++j){const auto b=reinterpret_cast<const unsigned char*>(&a.x87[n])[j];raw+=hex[b>>4];raw+=hex[b&15];}
        live.push_back(raw);
    }
    return J{{"gp",a.gp},{"flags",a.eflags},{"xmm",a.xmm},{"fs",a.fs_base},{"cw",a.floating_point->x87_control_word},{"sw",a.x87_status},{"mxcsr",a.floating_point->mxcsr},{"live80",live}};
}
struct RequiredStop{};
struct CalleeStop{std::uint32_t entry;J abi;};
struct Memory:SourcePhysicsContactMemory{
    std::array<std::byte,64>stack{};bool backed{};unsigned reads{},writes{};
    void read(std::uint32_t address,std::span<std::byte>bytes)const override{
        if(!backed||address<0x61000000u||address-0x61000000u+bytes.size()>stack.size())throw std::runtime_error("Unexpected router read");
        ++const_cast<Memory*>(this)->reads;std::copy_n(stack.data()+address-0x61000000u,bytes.size(),bytes.data());
    }
    void write(std::uint32_t address,std::span<const std::byte>bytes)override{
        if(!backed||address<0x61000000u||address-0x61000000u+bytes.size()>stack.size())throw std::runtime_error("Unexpected router write");
        ++writes;std::copy(bytes.begin(),bytes.end(),stack.data()+address-0x61000000u);
    }
};
#include "sandbox_dispatchers.inc"
int main()try{
    unsigned packets{},entries{},direct_packets{},direct_entries{};
    for(unsigned fresh=0;fresh<3;++fresh)for(unsigned image=0;image<2;++image)
    for(unsigned precision:{0x7fu,0x27fu,0x37fu})for(unsigned rounding=0;rounding<4;++rounding)
    for(unsigned depth:{0u,1u,4u,8u})for(unsigned target:{0xcabc0u,0xdead1234u,0x10eeb0u}){
        Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        WorldState s{memory,observe,services,image?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};for(unsigned n=0;n<8;++n){input.gp[n]=0x53420000u+n*0x731u;for(unsigned j=0;j<4;++j)input.xmm[n][j]=0x4d4d0000u+n*32u+j;}
        input.eflags=0xed7;input.fs_base=0x61004000;input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(precision|(rounding<<10)),0x1f80u|(rounding<<13)};
        input.x87_depth=depth;input.x87_status=0x4500u|(((depth+rounding)%8u)<<11u);
        for(unsigned n=0;n<depth;++n)input.x87[n]=n+1; s.load(input);
        const auto expected=snapshot(s);const auto site=0x1f2500u+depth;const bool tail=rounding&1u;bool called{},stopped{},callee{};
        services.required_owner=[&](auto&m,const auto&call,auto&a,const auto&owner_observe){
            if(&m!=&memory||&owner_observe!=&observe||call.target!=target||call.instruction!=site||call.tail!=tail)throw std::runtime_error("Required owner routing changed");
            WorldState received{memory,observe,services,s.image};received.load(a);
            if(snapshot(received)!=expected)throw std::runtime_error("Actual required-owner ABI changed");
            called=true;++packets;throw RequiredStop{};
        };
        try{if(fresh==0)canonical::dispatch(s,target,site,tail);else if(fresh==1)preserved::dispatch(s,target,site,tail);else raw::dispatch(s,target,site,tail);}
        catch(const RequiredStop&){stopped=true;}
        catch(const CalleeStop&stop){if(stop.entry!=target||stop.abi!=expected)throw std::runtime_error("Declared callee entry ABI changed");callee=true;++entries;}
        const bool expected_required=target==0xdead1234u||(target==0xcabc0u&&!image&&fresh!=2);
        if(expected_required?(!called||!stopped||callee):(called||stopped||!callee))throw std::runtime_error("Sandbox changed the required-owner boundary");
        if(snapshot(s)!=expected)throw std::runtime_error("Stopped router changed caller ABI");
    }
    constexpr std::array sites{0x1088e5u,0x15e326u,0x15fdf4u,0x1600adu};
    constexpr std::array returns{0x1088eau,0x15e32bu,0x15fdf9u,0x1600b2u};
    constexpr std::array targets{0xcabc0u,0x15ddc0u,0x1a8740u,0x1a87a0u};
    using Fragment=void(*)(WorldState&);
    constexpr std::array<std::array<Fragment,4>,3>calls{{
        {direct0::call_0,direct0::call_1,direct0::call_2,direct0::call_3},
        {direct1::call_0,direct1::call_1,direct1::call_2,direct1::call_3},
        {direct2::call_0,direct2::call_1,direct2::call_2,direct2::call_3}}};
    for(unsigned variant=0;variant<3;++variant)for(unsigned image=0;image<2;++image)
    for(unsigned fragment=0;fragment<4;++fragment)for(unsigned depth:{0u,1u,4u,8u})
    for(unsigned rounding=0;rounding<4;++rounding){
        Memory memory;memory.backed=true;SourcePhysicsWorldRuntimeServices services;unsigned observed{};
        SourcePhysicsContactRecordObserver observe=[&](const auto&event){
            if(event.instruction!=sites[fragment]||event.address!=0x6100001cu||event.before!=std::vector<std::byte>(4)||event.written.size()!=4||memory.writes!=1)
                throw std::runtime_error("Direct CALL store protocol changed");
            ++observed;
        };
        WorldState s{memory,observe,services,image?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};
        for(unsigned n=0;n<8;++n){input.gp[n]=0x54320000u+n*41u;for(unsigned lane=0;lane<4;++lane)input.xmm[n][lane]=0x53420000u+n*16u+lane;}
        input.gp[4]=0x61000020u;input.eflags=0xed7;input.fs_base=0x61004000u;
        input.x87_depth=depth;input.x87_status=0x4500u|(((depth+rounding)%8u)<<11u);
        input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|(rounding<<10)),0x1f80u|(rounding<<13)};
        for(unsigned n=0;n<depth;++n)input.x87[n]=n+1;s.load(input);
        auto expected=snapshot(s);expected["gp"][4]=0x6100001cu;bool owner{},callee{};
        services.required_owner=[&](auto&m,const auto&call,auto&a,const auto&owner_observe){
            WorldState received{memory,observe,services,s.image};received.load(a);
            if(&m!=&memory||&owner_observe!=&observe||call.instruction!=sites[fragment]||call.target!=targets[fragment]||call.tail||snapshot(received)!=expected)
                throw std::runtime_error("Direct PUSH/required-owner ABI changed");
            owner=true;++direct_packets;throw RequiredStop{};
        };
        try{calls[variant][fragment](s);}
        catch(const RequiredStop&){}
        catch(const CalleeStop&stop){if(stop.entry!=targets[fragment]||stop.abi!=expected)throw std::runtime_error("Direct local entry changed");callee=true;++direct_entries;}
        const bool required=fragment!=0||(!image&&variant!=2);
        std::uint32_t return_word{};std::memcpy(&return_word,memory.stack.data()+0x1c,4);
        if(owner!=required||callee==required||snapshot(s)!=expected||observed!=1||memory.writes!=1||memory.reads!=1||return_word!=returns[fragment])
            throw std::runtime_error("Direct CALL retained owner/store boundary changed");
    }
    std::cout<<J{{"passed",true},{"actual_required_owner_packets",packets},{"declared_callee_entries",entries},{"direct_required_owner_packets",direct_packets},{"direct_local_entry_packets",direct_entries},{"callee_outcomes_supplied",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
