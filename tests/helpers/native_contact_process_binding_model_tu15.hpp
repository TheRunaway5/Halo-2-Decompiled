#pragma once
#include "halo2/native_tu15_contact_process_binding.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
inline void native_contact_process_binding_model(){
    using namespace halo2;
    struct Memory:SourcePhysicsContactMemory{
        static constexpr std::uint32_t base(){return 0x1000000u;}
        std::array<std::byte,0x10000>bytes{};
        void valid(std::uint32_t p,std::size_t n)const{
            if(p<base()||std::uint64_t(p-base())+n>bytes.size())
                throw std::runtime_error("Native process binding model access outside DATA");
        }
        void read(std::uint32_t p,std::span<std::byte>b)const override{
            valid(p,b.size());std::copy_n(bytes.begin()+p-base(),b.size(),b.begin());
        }
        void write(std::uint32_t p,std::span<const std::byte>b)override{
            valid(p,b.size());std::copy(b.begin(),b.end(),bytes.begin()+p-base());
        }
        std::uint32_t word(std::uint32_t p)const{
            std::array<std::byte,4>b{};read(p,b);std::uint32_t v{};
            for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;
        }
        void word(std::uint32_t p,std::uint32_t v){
            std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8u));write(p,b);
        }
    }m;
    const auto require=[](bool b){if(!b)throw std::runtime_error("Native recursive process binding model differs");};
    constexpr auto b=0x1000000u,wrapper=b+0x100,compound=b+0x200,
        first=b+0x300,second=b+0x400,configuration=b+0x500,output=b+0x600,
        shape=b+0x1000,child_shape=b+0x1100,extra=b+0x2000,
        agents=b+0x3000,children=b+0x3100,next_children=b+0x3200,
        table=b+0x4000,child0=b+0x4100,child1=b+0x4200,data=b+0x9000;
    m.word(wrapper+12u,compound);m.word(compound,table);m.word(table+16u,0x2f7a50u);
    m.word(compound+12u,agents);m.word(compound+16u,2);
    m.word(agents,child0);m.word(agents+4u,child1);
    m.word(child0,table+0x40u);m.word(child1,table+0x40u);m.word(table+0x50u,0x304350u);
    m.word(first,shape);m.word(first+4u,0x98765432u);m.word(first+8u,extra);
    m.word(shape+12u,child_shape);m.word(child_shape+12u,children);
    m.word(children,b+0x5000);m.word(children+8u,b+0x5100);m.word(next_children+8u,b+0x5200);
    for(unsigned n=0;n<8;++n)m.word(extra+n*4u,0x7fa00000u+n);
    for(unsigned axis=0;axis<4;++axis)for(unsigned lane=0;lane<4;++lane){
        m.word(shape+16u+axis*16u+lane*4u,axis==lane?0x3f800000u:0u);
        m.word(extra+32u+axis*16u+lane*4u,axis==lane?0x3f800000u:0u);
    }
    m.word(shape+64u,0x40000000u);m.word(shape+68u,0x40400000u);m.word(shape+72u,0x40800000u);
    m.word(extra+80u,0x41200000u);m.word(extra+84u,0x41a00000u);m.word(extra+88u,0x41f00000u);
    unsigned calls{};NativeTu15CompoundProcessServices remaining;
    remaining.process_child=[&](const NativeTu15CompoundChildProcessCall&call,auto&){
        require(call.owner.identity==0x304350u&&call.receiver.identity==(calls?child1:child0));
        require(call.second.identity==second&&call.configuration.identity==configuration&&call.output.identity==output);
        require(call.first.identity==data+128u&&m.word(call.first.identity+4u)==calls);
        require(m.word(call.first.identity)==(calls?b+0x5200u:b+0x5000u));
        require(m.word(call.first.identity+12u)==data&&m.word(call.first.identity+8u)==data+16u);
        require(m.word(data+12u)==first&&m.word(data+4u)==0x98765432u&&m.word(data)==child_shape);
        std::array<std::byte,112>before{},after{};m.read(data,before);
        // A genuine child can use private frames below the DATA arena while
        // the nested parent and its geometry remain live above those frames.
        std::array<std::byte,0x200>frame;frame.fill(std::byte{0xa5});m.write(data-0x220u,frame);
        m.read(data,after);require(before==after);
        m.word(compound+12u,b+0x3300u);m.word(compound+16u,0);
        m.word(child_shape+12u,next_children);++calls;
    };
    NativeTu15PhysicsFrameFp fp{0x27f,0x2800,0x1f80};NativeTu15ContactProcessBindingStats stats;
    NativeTu15ContactProcessBinding binding(m,{data,0x600u},remaining,{},&stats);
    binding.process({{wrapper},{0x2f6ca0u},{first},{second},{configuration},{output}},fp);
    require(calls==2&&stats.attempts==2&&stats.completed==2&&stats.refusals==0&&stats.remaining_child_calls==2);
    require(stats.wrapped.calls==1&&stats.wrapped.geometry_calls==1&&stats.wrapped.child_returns==1);
    require(stats.compound.calls==1&&stats.compound.child_returns==2);
    for(unsigned n=0;n<8;++n)require(m.word(data+16u+n*4u)==0x7fa00000u+n);
    require(m.word(data+96u)==0x41400000u&&m.word(data+100u)==0x41b80000u&&m.word(data+104u)==0x42080000u);
    require(fp.control_word==0x27f&&fp.status==0x2800&&fp.mxcsr==0x1f80);
}
