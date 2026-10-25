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
    // Untouched TU1.5 1765B0/1765BF/176754 load this exact readonly DOUBLE.
    // A large Scorpion/Wraith light bound reaches the latter comparison.
    constexpr std::uint32_t portal_tolerance=0x46a598u;
    constexpr std::array portal_bits{std::byte{0},std::byte{0},std::byte{0},std::byte{0},
        std::byte{0},std::byte{0},std::byte{0x90},std::byte{0x3f}};
    check(empty.is_read_only(portal_tolerance,8) &&
        std::ranges::equal(empty.view(portal_tolerance,8),portal_bits),
        "Original light portal plane tolerance is absent or changed");
    const auto portal_owner=std::ranges::find(installed.regions,portal_tolerance,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(portal_owner!=installed.regions.end() && portal_owner->bytes.size()==8,
        "Light portal operand owner borrowed adjacent source bytes");
    for(bool writable:{false,true}) {
        SourceRuntimeMemory conflict;auto bytes=portal_bits;
        if(!writable)bytes.front()^=std::byte{1};
        conflict.mount(portal_tolerance,bytes,writable?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"conflicting light portal constant owner");
        rejects_without_publication(conflict);
    }
    // Original DCB75 selects the complete five-way damage response table.
    // Warthog chassis impacts reach selector4; the owner is immutable data.
    constexpr std::array<std::uint32_t,5> damage_response{
        0xdcbeau,0xdcbd1u,0xdcba5u,0xdcd81u,0xdcbbbu};
    check(empty.is_read_only(0xdcd98u,20),"Original vehicle damage dispatch table is absent");
    for(unsigned i=0;i<damage_response.size();++i)for(unsigned byte=0;byte<4;++byte)
        check(empty.view(0xdcd98u+i*4+byte,1).front()==std::byte(damage_response[i]>>(byte*8)),
            "Original damage response target differs from pinned TU1.5");
    {
        SourceRuntimeMemory conflict;
        conflict.mount(0xdcda8u,std::array{std::byte{0xff}},SourceRuntimeMemory::Access::ReadOnly,
            "conflicting original damage response selector4");
        rejects_without_publication(conflict);
    }
    // Original31F728's distinct interface, bounded by the next constructor.
    // Actual conveyor world calls326491/320201 require its final two slots.
    constexpr std::array conveyor_interface{
        0x31f9f0u,0x2e1270u,0x2e12a0u,0xa3850u,0x2e4d60u,
        0xa3850u,0xa3850u,0x325d80u,0xaabb0u,0x31f770u,0x31f740u};
    check(empty.is_read_only(0x41ed90u,44),"Conveyor interface lacks its readonly source owner");
    for(unsigned i=0;i<conveyor_interface.size();++i)for(unsigned byte=0;byte<4;++byte)
        check(empty.view(0x41ed90u+i*4+byte,1).front()==std::byte(conveyor_interface[i]>>(byte*8)),
            "Conveyor interface differs from authenticated TU1.5 methods");
    check(empty.is_read_only(0x16d27cu,12) && empty.is_read_only(0x16d288u,12) &&
        empty.is_read_only(0x45c7ccu,2) && empty.is_read_only(0x11e260u,16),
        "Original teleporter query/notice/Relic control immutable owners are missing");
    constexpr char16_t cause_player[]=u"#cause_player";
    const auto token=std::as_bytes(std::span(cause_player));
    check(empty.is_read_only(0x46098cu,token.size()) &&
        std::ranges::equal(empty.view(0x46098cu,token.size()),token),
        "Original localized event formatter lost its exact readonly token/terminator");
    for(const auto address:{0x16d288u,0x45c7ccu,0x11e260u,0x46098cu}) {
        SourceRuntimeMemory conflict;
        conflict.mount(address,std::array{std::byte{0xff}},SourceRuntimeMemory::Access::ReadOnly,
            "conflicting teleporter literal owner");
        rejects_without_publication(conflict);
    }
    // English TU1.5 SHA21b207e7...: original cloth11A215 reads exactly
    // this .rdata FLOAT. Its file bytes are0BD7233E; no adjacent gap/state is
    // part of the immutable operand owner reached by actual CTF flag birth.
    constexpr std::uint32_t cloth_threshold=0x46a8b8u;
    constexpr std::array cloth_bits{std::byte{0x0b},std::byte{0xd7},std::byte{0x23},std::byte{0x3e}};
    check(empty.is_read_only(cloth_threshold,4) &&
        std::ranges::equal(empty.view(cloth_threshold,4),cloth_bits),
        "Original cloth displacement threshold is absent or changed");
    const auto cloth_owner=std::ranges::find(installed.regions,cloth_threshold,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(cloth_owner!=installed.regions.end() && cloth_owner->bytes.size()==4,
        "Cloth operand owner borrowed adjacent source bytes");
    // Whole original2728C0 reads these three exact DOUBLEs, including the
    // reached272914 operand. They come from audited fixed readonly AI reads.
    constexpr std::array ai_operands{
        std::pair{0x46a6b8u,0x4024000000000000ull},
        std::pair{0x46a6c0u,0x4004000000000000ull},
        std::pair{0x46a6c8u,0x4018000000000000ull}};
    for(const auto [address,bits]:ai_operands) {
        std::array<std::byte,8> expected;
        for(unsigned i=0;i<8;++i)expected[i]=std::byte(bits>>(i*8));
        check(empty.is_read_only(address,8) && std::ranges::equal(empty.view(address,8),expected),
            "Original fixed AI x87 operand is absent or changed");
    }
    bool supplied_clock=false;
    try {(void)empty.view(0x41b774u,0);supplied_clock=true;}
    catch(const std::out_of_range&) {}
    check(!supplied_clock,"Physics literals fabricated a clock import without its runtime owner");
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
    for(bool writable:{false,true}) {
        SourceRuntimeMemory conflict;
        std::array<std::byte,8> bits{};bits[6]=std::byte{4};bits[7]=std::byte{0x40};
        if(!writable)bits[0]=std::byte{1};
        conflict.mount(0x46a6c0u,bits,writable?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"existing AI operand owner");
        rejects_without_publication(conflict);
    }
    SourceRuntimeMemory reserved;
    reserved.reserve(selector_byte,1,1,"uncommitted existing selector owner");
    rejects_without_publication(reserved);
    reserved.commit(selector_byte,std::array{std::byte{1}});
    rejects_without_publication(reserved);
    for(bool writable:{false,true}) {
        SourceRuntimeMemory conflict;
        auto bytes=cloth_bits;
        if(!writable)bytes.back()^=std::byte{1};
        conflict.mount(cloth_threshold,bytes,writable?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"existing cloth threshold owner");
        rejects_without_publication(conflict);
    }
    std::cout<<"Physics branch ownership: installation/idempotence, retained partial owner, "
        "readonly mismatch, writable overlap, reserved hole and committed writable overlap pass\n";
    return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
