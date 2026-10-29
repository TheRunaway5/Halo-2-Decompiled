#include "halo2/source_effect_particle_frame_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
#include "helpers/particle_model_child_parent_tu15_original.hpp"
class Memory final:public SourcePhysicsContactMemory {
    std::array<std::byte,65536> bytes{};
    unsigned tag_table{};
public:
    bool stack_writes{};
    void put(unsigned address,unsigned value){if(address==0x4f0414u){tag_table=value;return;}std::memcpy(bytes.data()+address-0x1000000u,&value,4);}
    void read(unsigned address,std::span<std::byte> output)const override {
        if(address==0x4f0414u && output.size()==4){std::memcpy(output.data(),&tag_table,4);return;}
        if(address<0x1000000u || address-0x1000000u>bytes.size() || output.size()>bytes.size()-(address-0x1000000u))
            throw std::runtime_error("Model child-row method escaped its actual fixture storage");
        std::memcpy(output.data(),bytes.data()+address-0x1000000u,output.size());
    }
    void write(unsigned address,std::span<const std::byte> input)override {
        if(!stack_writes || address!=0x100effcu || input.size()!=4)
            throw std::runtime_error("Child-row method wrote outside its caller stack");
        std::memcpy(bytes.data()+address-0x1000000u,input.data(),input.size());
    }
};
}
int main()try {
    if(!source_effect_particle_frame_tu15_owns_entry(0x22b710u))
        throw std::runtime_error("Original model child-row method has no native owner");
    for(const auto& row:model_child_original_cases) {
        Memory memory;memory.put(0x1000408u,0x1000800u);memory.put(0x100087cu,row.base);
        memory.put(0x100f000u,0x70000000u);memory.put(0x100f004u,row.index);
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={0x11111111u,0x1000400u,0x33333333u,0x44444444u,0x100f000u,0x66666666u,0x77777777u,0x88888888u};
        abi.eflags=0x202;SourcePhysicsWorldRuntimeServices services;std::vector<unsigned> sites;
        services.instruction=[&](unsigned pc){sites.push_back(pc);};
        source_run_effect_particle_frame_tu15(memory,0x22b710u,abi,{0x27f,0x1f80},services);
        constexpr std::array original{0x22b710u,0x22b714u,0x22b717u,0x22b71au,0x22b71du};
        if(abi.gp!=row.gp || abi.eflags!=row.eflags || !std::ranges::equal(sites,original))
            throw std::runtime_error("Native model child row differs from complete original instructions or caller ABI");
    }
    for(const auto& row:surface_child_original_cases) {
        Memory memory;memory.stack_writes=true;memory.put(0x4f0414u,0x1002000u);
        memory.put(0x1002010u,0x62736474u);memory.put(0x1002014u,0x10001u);memory.put(0x1002018u,0x1003000u);
        memory.put(0x1003018u,0x23450000u);memory.put(0x100100cu,row.index<<16);
        memory.put(0x1001010u,0x10001u);memory.put(0x1001014u,0xffffffffu);memory.put(0x100f000u,0x70000000u);
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={0x11111111u,0x1001000u,0x33333333u,0x44444444u,0x100f000u,0x66666666u,0x77777777u,0x88888888u};
        abi.eflags=0x202;SourcePhysicsWorldRuntimeServices services;std::vector<unsigned> sites;
        services.instruction=[&](unsigned pc){sites.push_back(pc);};
        source_run_effect_particle_frame_tu15(memory,0x178070u,abi,{0x27f,0x1f80},services);
        constexpr std::array original{0x178070u,0x178076u,0x178077u,0x178079u,0x17807cu,0x17807fu,
            0x178082u,0x178085u,0x17808bu,0x17808du,0x17808fu,0x178095u,0x178097u,0x17809du,
            0x17809fu,0x1780a4u,0x1780a7u,0x1780abu,0x1780afu,0x1780b2u,0x1780b5u,0x1780b7u,0x1780b8u};
        if(abi.gp!=row.gp || abi.eflags!=row.eflags || !std::ranges::equal(sites,original))
            throw std::runtime_error("Native breakable-surface child row differs from complete original branch/ABI");
    }
    std::cout<<"PASS six model and four breakable-surface original child-row cases, caller ABI and complete PCs\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
