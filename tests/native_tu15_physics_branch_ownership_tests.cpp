#include "halo2/native_tu15_physics_branch_data.hpp"
#include <array>
#include <iostream>
#include <ranges>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
constexpr std::uint32_t selector_byte=0x115e1au;
void rejects_without_publication(SourceRuntimeMemory& memory) {
    const auto before=memory.checkpoint();
    bool rejected=false;
    try {mount_native_tu15_physics_branch_data(memory);}
    catch(const std::exception&) {rejected=true;}
    check(rejected,"Conflicting or uncommitted physics owner was accepted");
    check(memory.checkpoint().regions==before.regions,
        "Physics tables were partially published before rejecting an existing owner");
}
}
int main()try {
    SourceRuntimeMemory empty;
    mount_native_tu15_physics_branch_data(empty);
    const auto installed=empty.checkpoint();
    check(!installed.regions.empty(),"Physics table installation published no owners");
    mount_native_tu15_physics_branch_data(empty);
    check(empty.checkpoint().regions==installed.regions,
        "Repeated physics installation replaced or duplicated owners");

    SourceRuntimeMemory clock;
    const std::array tick_binding{std::byte{0x00},std::byte{0x20},std::byte{0x21},std::byte{0x60}};
    clock.mount(0x41b774u,tick_binding,SourceRuntimeMemory::Access::ReadOnly,
        "retained runtime tick-count data import");
    mount_native_tu15_physics_branch_data(clock);
    check(std::ranges::equal(clock.view(0x41b774u,4),tick_binding),
        "Physics literals replaced the actual relocated tick-count import with its file ordinal");
    const auto clock_regions=clock.checkpoint().regions;
    const auto clock_owner=std::ranges::find(clock_regions,0x41b774u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(clock_owner!=clock_regions.end()&&clock_owner->owner=="retained runtime tick-count data import",
        "Physics literals changed the tick-count import owner");

    SourceRuntimeMemory partial;
    partial.mount(selector_byte,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadOnly,
        "retained partial original movement selector");
    mount_native_tu15_physics_branch_data(partial);
    const auto saved=partial.checkpoint();
    const auto retained=std::ranges::find(saved.regions,selector_byte,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(retained!=saved.regions.end() && retained->bytes.size()==1 &&
        retained->owner=="retained partial original movement selector" &&
        retained->access==SourceRuntimeMemory::Access::ReadOnly &&
        partial.view(selector_byte,1).front()==std::byte{1},
        "Compatible partial physics ownership was changed");

    for(bool writable:{false,true}) {
        SourceRuntimeMemory conflict;
        conflict.mount(selector_byte,std::array{writable?std::byte{1}:std::byte{0}},
            writable?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,
            "conflicting retained selector owner");
        rejects_without_publication(conflict);
    }
    SourceRuntimeMemory reserved;
    reserved.reserve(selector_byte,1,1,"uncommitted existing selector owner");
    rejects_without_publication(reserved);
    reserved.commit(selector_byte,std::array{std::byte{1}});
    rejects_without_publication(reserved);
    std::cout<<"Physics branch ownership: installation/idempotence, retained partial owner, "
        "readonly mismatch, writable overlap, reserved hole and committed writable overlap pass\n";
    return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
