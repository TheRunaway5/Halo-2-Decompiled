#include "physics/runtime/native_tu15_liquid_widget.hpp"
#include "physics/runtime/native_tu15_datum_delete_abi.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace halo2;

void check(bool condition,const char *message) {
    if(!condition)throw std::runtime_error(message);
}

struct Memory final : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    mutable std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        reads.emplace_back(address,output.size());
        for(std::size_t i=0;i<output.size();++i)output[i]=bytes.at(address+std::uint32_t(i));
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        for(std::size_t i=0;i<input.size();++i)bytes.at(address+std::uint32_t(i))=input[i];
    }
    void put(std::uint32_t address,std::uint32_t value,unsigned width=4) {
        for(unsigned i=0;i<width;++i)bytes[address+i]=std::byte(value>>(i*8u));
    }
    std::uint32_t get(std::uint32_t address,unsigned width=4)const {
        std::uint32_t result{};
        for(unsigned i=0;i<width;++i)result|=std::to_integer<std::uint32_t>(bytes.at(address+i))<<(i*8u);
        return result;
    }
};

constexpr std::uint32_t pool_root=0x4ecbecu,pool=0x82000000u,rows=pool+0x100u,bitmap=pool+0x500u;
constexpr std::uint32_t frame=0x600effbcu,datum=0xe96e0002u,stride=0x110u;
constexpr std::uint32_t return_to_widgets=0xd6ffau,return_from_delete=0xc5d0fu;
constexpr SourcePhysicsFactoryFloatingPoint fp{0x027fu,0x1f80u};

SourcePhysicsWorldRuntimeAbi caller() {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp={0x82d4b5c4u,datum,0x70u,0x82e0ed50u,frame,0xe5fe0052u,0xe97c0002u,0xffffffffu};
    abi.eflags=0x0ad7u;
    abi.fs_base=0x60200000u;
    abi.floating_point=fp;
    abi.xmm[0]={0x11223344u,0x55667788u,0x99aabbccu,0xddeeff00u};
    abi.x87[0]=1.25L;
    abi.x87_depth=1;
    abi.x87_status=0x1800u;
    return abi;
}

Memory input(std::uint32_t liquid_pool,unsigned active,std::uint32_t payload) {
    Memory memory;
    memory.put(pool_root,liquid_pool);
    if(liquid_pool)memory.put(liquid_pool+0x29u,active,1);
    memory.put(frame,return_to_widgets);
    memory.put(frame+4u,payload);
    return memory;
}

void check_preserved(const SourcePhysicsWorldRuntimeAbi& before,const SourcePhysicsWorldRuntimeAbi& after) {
    for(auto index:{3u,5u,6u,7u})
        check(after.gp[index]==before.gp[index],"Widget retirement clobbered a callee-saved register");
    check(after.fs_base==before.fs_base && after.xmm==before.xmm && after.x87[0]==before.x87[0] &&
          after.x87_depth==before.x87_depth && after.x87_status==before.x87_status && after.floating_point &&
          after.floating_point->x87_control_word==fp.x87_control_word && after.floating_point->mxcsr==fp.mxcsr,
          "Widget retirement changed the caller's FP or TLS state");
    check(after.gp[4]==frame+8u,"Widget retirement did not return with RET 4");
}

void check_inactive_paths() {
    struct Gate {
        std::uint32_t liquid_pool,active,payload,eax;
        std::vector<std::uint32_t> instructions;
        std::vector<std::pair<std::uint32_t,std::size_t>> reads;
    };
    const auto initial=caller();
    const std::array gates{
        Gate{0,0,datum,initial.gp[0],{0xc5cf0u,0xc5cf6u,0xc5cf8u,return_from_delete},
            {{pool_root,4},{frame,4}}},
        Gate{pool,0,datum,initial.gp[0]&0xffffff00u,
            {0xc5cf0u,0xc5cf6u,0xc5cf8u,0xc5cfau,0xc5cfdu,0xc5cffu,return_from_delete},
            {{pool_root,4},{pool+0x29u,1},{frame,4}}},
        Gate{pool,1,0xffffffffu,0xffffffffu,
            {0xc5cf0u,0xc5cf6u,0xc5cf8u,0xc5cfau,0xc5cfdu,0xc5cffu,0xc5d01u,0xc5d05u,0xc5d08u,return_from_delete},
            {{pool_root,4},{pool+0x29u,1},{frame+4,4},{frame,4}}}
    };
    for(const auto& gate:gates) {
        auto memory=input(gate.liquid_pool,gate.active,gate.payload);
        const auto before=memory.bytes;
        auto abi=initial;
        std::vector<std::uint32_t> instructions;
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](std::uint32_t pc){instructions.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&) {
            throw std::runtime_error("Widget gate called the delete owner without a live datum");
        };
        native_tu15_liquid_widget::retire(memory,abi,fp,services);
        check(instructions==gate.instructions && memory.reads==gate.reads,
              "Widget gate changed the original instruction/read sequence");
        check(memory.bytes==before && abi.gp[0]==gate.eax && abi.gp[1]==initial.gp[1] &&
              abi.gp[2]==gate.liquid_pool && abi.eflags==0x246u,
              "Widget gate changed the original return data or flags");
        check_preserved(initial,abi);
    }
}

void check_active_pool() {
    // TU1.5 admits any nonzero live-pool flag. Use an upper bit so an
    // accidental boolean equality check rejects this test's live row.
    auto memory=input(pool,0x80u,datum);
    for(auto offset:{4u,8u,12u,16u})memory.put(frame-offset,0xababababu);
    memory.put(pool+0x24u,stride);
    memory.put(pool+0x2au,0,1);
    memory.put(pool+0x34u,3);
    memory.put(pool+0x38u,3);
    memory.put(pool+0x3cu,3);
    memory.put(pool+0x44u,rows);
    memory.put(pool+0x48u,bitmap);
    memory.put(rows,0x8a01u,2);
    memory.put(rows+stride,0x8a02u,2);
    memory.put(rows+2u*stride,datum>>16u,2);
    memory.put(rows+2u*stride+2u,0x5aa5u,2);
    memory.put(bitmap,7);

    const auto initial=caller();
    auto abi=initial;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::vector<std::uint32_t> instructions;
    unsigned calls{};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){instructions.push_back(pc);};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer) {
        ++calls;
        check(&bank==&memory && call.instruction==0xc5d0au && call.target==0x16e8a0u && !call.tail,
              "Widget retirement lost the original datum-delete call");
        check(state.gp[0]==datum && state.gp[2]==pool && state.gp[4]==frame-4u &&
              memory.get(frame-4u)==return_from_delete && state.eflags==0x297u,
              "Widget retirement delivered a different datum-delete ABI");
        const SourcePhysicsWorldRuntimeServices child_services;
        physics_world_runtime_detail::WorldState child{bank,observer,child_services,SourcePhysicsContactImage::TitleUpdate15};
        child.load(state);
        check(native_datum_delete_detail::run(child),"The real datum-delete owner declined the fixture");
        state=child.save();
    };
    native_tu15_liquid_widget::retire(memory,abi,fp,services,
        [&](const auto& store){stores.push_back(store);});
    check(calls==1 && abi.gp[0]==2u && instructions.back()==return_from_delete,
          "Widget retirement did not reach its continuation after deleting one entry");
    check(!stores.empty() && stores.front().instruction==0xc5d0au && stores.front().address==frame-4u,
          "Widget retirement omitted the original return-address store");
    check(memory.get(pool+0x34u)==2u && memory.get(pool+0x38u)==2u && memory.get(pool+0x3cu)==2u &&
          memory.get(bitmap)==3u && memory.get(rows+2u*stride,2)==0u,
          "Widget retirement did not retire the original liquid pool datum");
    check(memory.get(rows,2)==0x8a01u && memory.get(rows+stride,2)==0x8a02u &&
          memory.get(rows+2u*stride+2u,2)==0x5aa5u &&
          memory.get(frame)==return_to_widgets && memory.get(frame+4u)==datum,
          "Widget retirement changed a neighboring row, payload or caller stack");
    check_preserved(initial,abi);
}
} // namespace

int main()try {
    check_inactive_paths();
    check_active_pool();
    std::cout<<"TU1.5 liquid widget retirement passed all gates and deleted the live datum\n";
    return EXIT_SUCCESS;
}catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';
    return EXIT_FAILURE;
}
