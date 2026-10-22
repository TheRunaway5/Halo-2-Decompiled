#include "halo2/native_tu15_static_owners.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <array>

namespace halo2 {
namespace {
template<std::size_t N>std::array<std::byte,N*4> encode(const std::array<std::uint32_t,N>& values) {
    std::array<std::byte,N*4> bytes{};
    for(std::size_t i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(values[i]>>(j*8));return bytes;
}
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t p) {
    std::array<std::byte,4> bytes{};memory.read(p,bytes);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return value;
}
void put(SourcePhysicsContactMemory& memory,std::uint32_t p,std::uint32_t value) {
    memory.write(p,encode(std::array{value}));
}
constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
}
void mount_native_tu15_static_data(SourceRuntimeMemory& memory) {
    // 3233E0 constructs the BSS deactivator's vtable and refcount. The size
    // flags remain zero, so its actual B4150 destructor releases no heap block.
    memory.mount(0x48d3d8u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold static rigid-body deactivator");
    // The scratch owner starts with its authored vtable/control word, empty
    // buffer/chunk bindings and original -1 sentinel. 2E1040
    // publishes the borrowed static buffer and its 0x4000-byte extent.
    memory.mount(0x486080u,encode(std::array<std::uint32_t,6>{0x45f4ccu,1,0,0,0,0xffffffffu}),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 initial static Havok scratch owner");
    memory.mount(0x4860a0u,std::vector<std::byte>(0x4000),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold borrowed Havok scratch buffer");
    // Six authored class bindings consumed by 3825D0. They are source object
    // identities, never narrowed host pointers or preconstructed table slots.
    memory.mount(0x4822a0u,encode(std::array<std::uint32_t,4>{0x48c498u,0x48c49cu,0x48c4a0u,0x48c4a4u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored engine class bindings");
    memory.mount(0x47d174u,encode(std::array{0x48bfa0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored shared engine class binding");
    memory.mount(0x47a2f8u,encode(std::array{0x48a138u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored additional engine class binding");
    memory.mount(0x56dfa0u,std::vector<std::byte>(11*4),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold engine class registration table");
}
NativeTu15StaticOwners::NativeTu15StaticOwners(SourceRuntimeMemory& memory,
    const SourcePhysicsContactRecordObserver& observer):memory_(memory),observe_(observer) {
    mount_native_tu15_static_data(memory_);
    for(const auto entry:{0x3835f0u,0x381370u,0x3825d0u})run(entry);
    scratch_=memory_.native_address(word(memory_,0x486088u),word(memory_,0x48608cu));
    deactivator_=memory_.native_address(0x48d3d8u,8);
    engine_table_=memory_.native_address(0x56dfa0u,11*4);
}
NativeTu15StaticOwners::~NativeTu15StaticOwners(){close();}
void NativeTu15StaticOwners::close() {
    while(!exits_.empty()) {
        const auto entry=exits_.back();run(entry);exits_.pop_back();
    }
}
SourcePhysicsWorldRuntimeServices NativeTu15StaticOwners::services() {
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[this](auto& memory,const auto& call,auto& abi,const auto& observer) {
        if(&memory!=&memory_)throw std::invalid_argument("Native static constructor bank owner differs");
        const auto sp=abi.gp[4];
        if(!call.tail && call.target==0x32b0b4u &&
            (call.instruction==0x3835ffu || call.instruction==0x381389u)) {
            const auto callback=call.instruction==0x3835ffu?0x383c90u:0x383970u;
            if(word(memory,sp)!=call.instruction+5 || word(memory,sp+4)!=callback)
                throw std::invalid_argument("Native static exit registration frame differs");
            exits_.push_back(callback);abi.gp[0]=0;abi.gp[4]+=4;return;
        }
        if(call.tail && call.instruction==0x3233d0u && call.target==0xb4150u && abi.gp[1]==0x48d3d8u) {
            source_run_physics_world_runtime(memory,0xdd970u,abi,
                SourcePhysicsContactImage::TitleUpdate15,fp,this->services(),observer);return;
        }
        throw std::runtime_error("Native static startup reached an undeclared source owner");
    };
    return services;
}
void NativeTu15StaticOwners::run(std::uint32_t entry) {
    constexpr std::uint32_t sp=0x600f0000u;
    put(memory_,sp,0x60300000u);
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=sp;abi.eflags=2;abi.fs_base=0x60200000u;abi.floating_point=fp;
    source_run_physics_live_support_tu15(memory_,entry,abi,fp,services(),observe_);
    if(abi.gp[4]!=sp+4)throw std::runtime_error("Native static source owner did not retire its caller frame");
}
}
