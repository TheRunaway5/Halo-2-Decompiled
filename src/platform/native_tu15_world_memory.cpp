#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/native_tu15_ctf_static_data.hpp"
#include "halo2/native_tu15_multiplayer_data.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/native_tu15_player_result_data.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
#include "halo2/source_cache_startup_tu15.hpp"
#include "halo2/native_tu15_physics_branch_data.hpp"
#include "halo2/native_tu15_contrail_draw_data.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include "halo2/native_tu15_heap_data.hpp"
#include "halo2/native_tu15_arena_data.hpp"
#include "halo2/native_tu15_static_owners.hpp"
#include "halo2/native_tu15_prt_cache_data.hpp"
#include "halo2/native_tu15_main_view_data.hpp"
#include "halo2/native_tu15_camera_packet_data.hpp"
#include "halo2/source_game_storage_startup_tu15.hpp"
#include "halo2/source_level_handles_startup_tu15.hpp"
#include "halo2/native_tu15_rng_startup.hpp"
#include "halo2/source_startup_state_tu15.hpp"
#include "halo2/source_state_context_startup_tu15.hpp"
#include "halo2/native_tu15_player_startup_data.hpp"
#include "halo2/native_tu15_view_storage_data.hpp"
#include "halo2/native_tu15_observer_data.hpp"
#include "halo2/source_static_registration_tu15.hpp"
#include "halo2/native_tu15_scalar_state_data.hpp"
#include "halo2/native_tu15_object_storage.hpp"
#include "halo2/source_aux_object_state_tu15.hpp"
#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/source_renderer_request_tu15.hpp"
#include "halo2/source_renderer_target_storage_tu15.hpp"
#include "halo2/source_renderer_cache_pools_tu15.hpp"
#include "halo2/source_renderer_resource_storage_tu15.hpp"
#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include "halo2/source_renderer_transparent_storage_tu15.hpp"
#include "halo2/source_render_object_startup_tu15.hpp"
#include "halo2/source_startup_sound_storage_tu15.hpp"
#include "halo2/source_ui_script_startup_tu15.hpp"
#include "halo2/source_final_startup_storage_tu15.hpp"
#include "halo2/source_input_startup_tu15.hpp"
#include <algorithm>
#include <set>
#include <sstream>

namespace halo2 {
namespace {
constexpr auto type_runtime_owner="native TU1.5 cold complete 1E7690 object-type runtime flags";
template<std::size_t N>auto encode(const std::array<std::uint32_t,N>& words) {
    std::array<std::byte,N*4> bytes{};
    for(std::size_t i=0;i<N;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte(words[i]>>(j*8));
    return bytes;
}
// Three exact authored object-widget descriptors, each group/reserved plus
// twelve nullable callbacks. Coalesce only the existing three constructor
// identity aliases during fresh cold composition, before any game calls.
void mount_object_widget_types(SourceRuntimeMemory& memory) {
    const auto bytes=encode(std::array<std::uint32_t,42>{
        0x616e7421u,0x00000000u,0x001183f0u,0x00118430u,0x00118450u,0x00118460u,0x00118480u,
        0x001186d0u,0x001187a0u,0x001186f0u,0x00000000u,0x00000000u,0x00000000u,0x00118760u,
        0x636c7764u,0x00000000u,0x001190f0u,0x00119130u,0x00119150u,0x00119160u,0x00119060u,
        0x001190d0u,0x00118f80u,0x0008fdb0u,0x001191e0u,0x00119740u,0x001191a0u,0x0008fdb0u,
        0x7464746cu,0x00000000u,0x000c5a70u,0x000c5ab0u,0x000c5ad0u,0x000c5ae0u,0x000c5b00u,
        0x000c5cf0u,0x000c5fc0u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u
    });
    constexpr std::array<std::uint32_t,3> aliases{0x473a00u,0x473a38u,0x473a70u};
    unsigned found{};
    // Reject every other owner overlapping the complete replacement before
    // releasing any alias, including identical bytes with foreign provenance.
    for(const auto& region:memory.checkpoint().regions) {
        const auto end=std::uint64_t(region.address)+region.bytes.size();
        if(region.address>=0x473aa0u || end<=0x4739f8u)continue;
        if(std::ranges::find(aliases,region.address)==aliases.end() ||
            region.bytes.size()!=4 || region.access!=SourceRuntimeMemory::Access::ReadOnly ||
            region.owner!="source widget child constructor identity" ||
            region.page_bytes || !region.committed.empty() ||
            !std::ranges::equal(region.bytes,std::span(bytes).subspan(region.address-0x4739f8u,4)))
            throw FormatError("Object-widget descriptor conflicts with its cold constructor provenance");
        ++found;
    }
    if(found!=aliases.size())throw FormatError("Object-widget descriptors lack their complete cold constructor aliases");
    for(const auto address:aliases)memory.release(address);
    memory.mount(0x4739f8u,bytes,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored three complete object-widget callback descriptors");
}
void share_immutable_word(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t bits,
    const char* owner) {
    const auto bytes=encode(std::array{bits});
    try {
        const auto old=memory.view(address,4);
        if(!memory.is_read_only(address,4) || !std::ranges::equal(old,bytes)) {
            std::ostringstream message;message<<"Native physics scalar conflicts with its immutable owner: address=0x"
                <<std::hex<<address<<" expected_bits=0x"<<bits<<" requested_owner="<<owner;
            throw std::invalid_argument(message.str());
        }
        return;
    }catch(const std::out_of_range&){}
    try {memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,owner);return;}
    catch(const std::invalid_argument&){}
    // A typed byte table can end inside an older word declaration (29AF5C
    // has19 opcode bytes; the old final word also names one padding byte).
    // Validate every shared byte before publishing only the uncovered pieces.
    std::array<bool,4> shared{};
    for(unsigned i=0;i<bytes.size();++i) {
        try {(void)memory.view(address+i,0);shared[i]=true;}
        catch(const std::out_of_range&){}
        if(shared[i] && (!memory.is_read_only(address+i,1) || memory.view(address+i,1)[0]!=bytes[i])) {
            std::ostringstream message;message<<"Native physics scalar conflicts with its immutable owner: address=0x"
                <<std::hex<<address<<" byte_address=0x"<<address+i<<" expected_bits=0x"<<bits
                <<" requested_owner="<<owner;
            throw std::invalid_argument(message.str());
        }
    }
    for(unsigned i=0;i<bytes.size();) {
        if(shared[i]){++i;continue;}
        const auto begin=i++;
        while(i<bytes.size() && !shared[i])++i;
        memory.mount(address+begin,std::span(bytes).subspan(begin,i-begin),
            SourceRuntimeMemory::Access::ReadOnly,owner);
    }
}
template<std::size_t N>void share_immutable_words(SourceRuntimeMemory& memory,
    std::uint32_t address,const std::array<std::uint32_t,N>& words,const char* owner) {
    const auto bytes=encode(words);
    try {
        const auto old=memory.view(address,bytes.size());
        if(!memory.is_read_only(address,bytes.size()) || !std::ranges::equal(old,bytes))
            throw std::invalid_argument("Native model-render literal conflicts with its immutable owner");
        return;
    }catch(const std::out_of_range&){}
    try {memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,owner);}
    catch(const std::invalid_argument&) {
        for(const auto& region:memory.checkpoint().regions)
            if(std::uint64_t(region.address)<std::uint64_t(address)+bytes.size() &&
               std::uint64_t(address)<std::uint64_t(region.address)+region.bytes.size()) {
                std::ostringstream message;message<<"Native model-render literal0x"<<std::hex<<address
                    <<"+0x"<<bytes.size()<<" overlaps0x"<<region.address<<"+0x"<<region.bytes.size()
                    <<" owner: "<<region.owner;
                throw std::invalid_argument(message.str());
            }
        throw;
    }
}
}
void native_tu15_validate_type_runtime_restore(const SourceRuntimeMemory& current,
    const SourceRuntimeMemory::Checkpoint& incoming) {
    bool native_owner{};
    current.visit_committed([&](auto address,auto bytes) {
        if(address==0x5020b4u && bytes.size()==15u)native_owner=true;
    });
    if(!native_owner)return;
    const auto owner=std::ranges::find(incoming.regions,0x5020b4u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    if(owner==incoming.regions.end() || owner->bytes.size()!=15u ||
        owner->access!=SourceRuntimeMemory::Access::ReadWrite ||
        owner->owner!=type_runtime_owner || owner->page_bytes || !owner->committed.empty())
        throw FormatError("TU1.5 authority lacks its complete object-type runtime owner");
}
SourceRuntimeMemory native_tu15_world_memory(const SourceRuntimeKernelTickCount& tick) {
    if(tick.address<0x60000000u || tick.address>0xfffffffcu || tick.owner.empty())
        throw std::invalid_argument("Native TU1.5 world requires named retained tick-count backing");
    auto memory=native_tu15_process_heap_memory();
    mount_native_tu15_ctf_static_data(memory);
    mount_native_tu15_multiplayer_data(memory);
    mount_native_tu15_player_result_data(memory);
    //19B860/19B7D0/19BAA0 record weapon/PvP results, CTF grabs/returns,
    //bomb plants and medals through the original7/2/32-row metadata owners.
    //No statistic or award value is supplied here.
#include "gameplay/multiplayer/native_tu15_result_award_data.inc"
    mount_native_tu15_contrail_draw_data(memory);
    mount_native_tu15_arena_data(memory);
    mount_native_tu15_static_data(memory);
    mount_native_tu15_level_handles_startup_data(memory);
    mount_native_tu15_game_startup_data(memory);
    mount_native_tu15_rng_startup_data(memory);
    mount_native_tu15_clock_startup_data(memory);
    mount_native_tu15_state_context_startup_data(memory);
    mount_native_tu15_player_startup_data(memory);
    mount_native_tu15_view_storage_data(memory);
    mount_native_tu15_observer_data(memory);
    mount_native_tu15_static_registration_data(memory);
    mount_native_tu15_scalar_state_data(memory);
    mount_native_tu15_object_storage_data(memory);
    mount_native_tu15_aux_object_state_data(memory);
    mount_native_tu15_renderer_startup_data(memory);
    mount_native_tu15_renderer_request_data(memory);
    // Reuse the existing typed target-storage prefix; resources stay cold
    // until genuine target allocation publishes them. Slot18 is required by1FCB0.
    mount_native_tu15_renderer_target_storage_data(memory);
    mount_native_tu15_renderer_cache_pools_data(memory);
    mount_native_tu15_renderer_resource_storage_data(memory);
    // Actual2B630..2B63A clears exactly sixteen DWORDs once per render
    // frame. Keep the independent512-cluster union cold until publication.
    memory.mount(0x4d1ee0u,std::array<std::byte,64>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold512-cluster render visibility union");
    mount_native_tu15_renderer_tail_storage_data(memory);
    mount_native_tu15_renderer_transparent_storage_data(memory);
    //22570 also resets the cold renderer query/debug banks. Retain the same
    // exact loader-zero4112-byte declaration used by1EFD0's optional graphics
    // constructor; no debug texture or initialized-GPU flag is manufactured.
    memory.mount(0x4c2ac0u,std::array<std::byte,4112>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1EFD0 cold debugger bitmap binding, counters and512 row slots");
    memory.mount(0x492278u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 22570 final renderer new-map scalar");
    mount_native_tu15_prt_cache_data(memory);
    mount_native_tu15_main_view_data(memory);
    mount_native_tu15_camera_packet_data(memory);
    mount_native_tu15_render_object_startup_data(memory);
    mount_native_tu15_startup_sound_storage_data(memory);
    mount_native_tu15_ui_script_startup_data(memory);
    mount_native_tu15_final_startup_storage_data(memory);
    // Authenticated TU1.5 CRT formatter method slots initially select the
    // genuine "floating point not loaded" failure owner. Actual32CBC0 still
    // replaces all six entries before the working world's printf consumers.
    memory.mount(0x46ec80u,encode(std::array<std::uint32_t,6>{
        0x3334e7u,0x3334e7u,0x3334e7u,0x3334e7u,0x3334e7u,0x3334e7u}),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 cold CRT floating-point method slots");
    // Actual CRT rows46D7A0/A4/A8 construct these three static constraint
    // prototypes. The complete326100 constructor writes through offsetF4;
    // the F8-byte destinations are loader-zeroed in the offline reference.
    for(const auto address:{0x48cf90u,0x48d0a0u,0x48d1b0u})
        memory.mount(address,std::array<std::byte,0xf8>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold static constraint prototype destination");
    // Actual380FA0 constructs the six alternating empty-bound components.
    // Preserve the two authored IEEE754 operands and cold destination;
    // source instructions perform the minimum/maximum publication.
    share_immutable_word(memory,0x45695cu,0x7f7fffffu,
        "native TU1.5 authored empty-bounds minimum operand");
    memory.mount(0x44aac8u,encode(std::array{0xff7fffffu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored empty-bounds maximum operand");
    memory.mount(0x55655cu,std::array<std::byte,24>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold portal and object empty-bounds destination");
    // Actual38780A reads391278 before384224 publishes the real SDK object.
    // Reuse the existing authenticated cold SDK provider, including the full
    // accounting destinations used by387BAA/387BB5/38A988/38A993. It supplies
    // no device object, reference count, capability or initialized result.
    mount_native_tu15_sound_sdk_prefix_data(memory);
    // Final startup owns the complete authored47524C camera-effect type
    // table. Actual1578C9 reads selector1 from that same owner.
    // Complete printf character classes and state transitions consumed by
    //32D29E/32D2AC. The 0x20..0x78 classification interval and transition
    // bank deliberately share this same authored 89-byte table.
    constexpr std::array<std::uint8_t,89> format_states{
        0x6,0x0,0x0,0x6,0x0,0x1,0x0,0x0,0x10,0x0,0x3,0x6,0x0,0x6,0x2,0x10,
        0x4,0x45,0x45,0x45,0x5,0x5,0x5,0x5,0x5,0x35,0x30,0x0,0x50,0x0,0x0,0x0,
        0x0,0x20,0x28,0x38,0x50,0x58,0x7,0x8,0x0,0x37,0x30,0x30,0x57,0x50,0x7,0x0,
        0x0,0x20,0x20,0x8,0x0,0x0,0x0,0x0,0x8,0x60,0x68,0x60,0x60,0x60,0x60,0x0,
        0x0,0x70,0x70,0x78,0x78,0x78,0x78,0x8,0x7,0x8,0x0,0x0,0x7,0x0,0x8,0x8,
        0x8,0x0,0x0,0x8,0x0,0x8,0x0,0x7,0x8};
    memory.mount(0x41f7c8u,std::as_bytes(std::span(format_states)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 printf character-class and state-transition operands");
    memory.mount(0x32d9bfu,encode(std::array<std::uint32_t,8>{
        0x32d44au,0x32d2ccu,0x32d2e9u,0x32d335u,0x32d370u,0x32d379u,0x32d3b1u,0x32d483u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 printf state branch identities");
    share_immutable_word(memory,0x4500b8u,0x42c80000u,
        "native TU1.5 authored 1CD272 allocator utilization percent operand");
    memory.mount(0x46a524u,encode(std::array{0x3a800000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1CD2A0 allocator byte-to-kilobyte operand");
    memory.mount(0x46a568u,encode(std::array{0u,0x40905470u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1CD2AA allocator memory-report double operand");
    // Complete authored decimal scaling tables selected by 333ABA. Each
    // of the 29 LD12 records is three little-endian words: seven radix-8
    // exponent choices per group, followed by the 4096 exponent choice.
    // 333B00/333B03 select twelve-byte rows; inter-table padding is excluded.
    memory.mount(0x46ef60u,encode(std::array<std::uint32_t,87>{
        0x00000000u,0x00000000u,0x4002a000u,
        0x00000000u,0x00000000u,0x4005c800u,
        0x00000000u,0x00000000u,0x4008fa00u,
        0x00000000u,0x00000000u,0x400c9c40u,
        0x00000000u,0x00000000u,0x400fc350u,
        0x00000000u,0x00000000u,0x4012f424u,
        0x00000000u,0x80000000u,0x40169896u,
        0x00000000u,0x20000000u,0x4019bebcu,
        0x00000000u,0xc9bf0400u,0x40348e1bu,
        0xa1000000u,0x1bceccedu,0x404ed3c2u,
        0xb59ef020u,0xada82b70u,0x40699dc5u,
        0x25fd5dd0u,0x4f8e1ae5u,0x4083eb19u,
        0x95d79671u,0x8d050e43u,0x409eaf29u,
        0x44a0bff9u,0x8f1281edu,0x40b98281u,
        0xa6d53cbfu,0x1f49ffcfu,0x40d3c278u,
        0x8ce0c66fu,0x47c980e9u,0x41a893bau,
        0x556b85bcu,0xf78d3927u,0x427ce070u,
        0xde8eddbcu,0xebfb9df9u,0x4351aa7eu,
        0xe376e6a1u,0x2f29f2ccu,0x44268184u,
        0xaa171028u,0xe310aef8u,0x44fac4c5u,
        0xf3d4a7ebu,0x4ae1ebf7u,0x45cf957au,
        0x91c7cc65u,0xa0aea60eu,0x46a3e319u,
        0x0c17650du,0x75868175u,0x4d48c976u,
        0xa7e44258u,0x353b3993u,0x53edb2b8u,
        0x5de5a74du,0x3b5dc53du,0x5a929e8bu,
        0xf0a65dffu,0x54c020a1u,0x61378ca5u,
        0x5a8bfdd1u,0x5d25d88bu,0x67dbf989u,
        0xf3f895aau,0xc8a2bf27u,0x6e80dd5du,
        0x979bc94cu,0x52028a20u,0x7525c460u
        }),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored CRT positive decimal LD12 scaling records");
    memory.mount(0x46f0c0u,encode(std::array<std::uint32_t,87>{
        0xcccdcccdu,0xccccccccu,0x3ffbccccu,
        0xd70a3d71u,0x0a3d70a3u,0x3ff8a3d7u,
        0xdf3b645au,0x6e978d4fu,0x3ff58312u,
        0x652cd3c3u,0x1758e219u,0x3ff1d1b7u,
        0x84230fd0u,0xac471b47u,0x3feea7c5u,
        0x69b6a640u,0xbd05af6cu,0x3feb8637u,
        0x42bc3d33u,0x94d5e57au,0x3fe7d6bfu,
        0xcefdfdc2u,0x77118461u,0x3fe4abccu,
        0xe15b4c2fu,0x94bec44du,0x3fc9e695u,
        0x3b53c492u,0x14cd4475u,0x3faf9abeu,
        0x94ba67deu,0x1ead4539u,0x3f94cfb1u,
        0xe2c62324u,0x313bbabcu,0x3f7a8b61u,
        0xc1595561u,0x7c53b17eu,0x3f5fbb12u,
        0x8d2feed7u,0x8592be06u,0x3f44fb15u,
        0xe9a53f24u,0xea27a539u,0x3f2aa87fu,
        0xe4a1ac7du,0x467c64bcu,0x3e55ddd0u,
        0xcc067b63u,0x83775423u,0x3d8191ffu,
        0x193afa91u,0x4325637au,0x3cacc031u,
        0x38d18921u,0xb8974782u,0x3bd7fd00u,
        0x085888dcu,0xe3e8b11bu,0x3b03a686u,
        0x424584c6u,0x7599b607u,0x3a2edb37u,
        0xd21c7133u,0xee32db23u,0x395a9049u,
        0xc0be87a6u,0x82a5da57u,0x32b5a2a6u,
        0x11b268e2u,0x449f52a7u,0x2c10b759u,
        0x2de44925u,0x534f3436u,0x256bceaeu,
        0xa404598fu,0x7dc2dec0u,0x1ec6e8fbu,
        0x5a88e79eu,0xbf3c9157u,0x18228350u,
        0x62654b4eu,0xaf8f83fdu,0x117d9406u,
        0x9fde2de4u,0x04c8d2ceu,0x0ad8a6ddu
        }),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored CRT negative decimal LD12 scaling records");
    // Complete 326E20 CRT memmove branch operands. Alignment selectors
    // are 1..3; byte remainders are 0..3; small dword counts are 0..7
    // (negated for the backward table). Exclude unreachable padding/code.
    memory.mount(0x32a9f4u,encode(std::array{0x32aa00u,0x32aa2cu,0x32aa50u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored CRT memmove forward alignment branch identities");
    memory.mount(0x32aa70u,encode(std::array{0x32aad3u,0x32aac0u,0x32aab8u,0x32aab0u,0x32aaa8u,0x32aaa0u,0x32aa98u,0x32aa90u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored CRT memmove forward dword count branch identities");
    // Forward byte-remainder table32AADC is already owned by native heap data.
    memory.mount(0x32ab80u,encode(std::array{0x32ab8cu,0x32abb0u,0x32abd8u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored CRT memmove backward alignment branch identities");
    memory.mount(0x32ac0cu,encode(std::array{0x32ac2cu,0x32ac34u,0x32ac3cu,0x32ac44u,0x32ac4cu,0x32ac54u,0x32ac5cu,0x32ac6fu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored CRT memmove negated backward dword count branch identities");
    memory.mount(0x32ac78u,encode(std::array{0x32ac88u,0x32ac90u,0x32aca0u,0x32acb4u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored CRT memmove backward byte remainder branch identities");
    memory.mount(0x46e8ecu,std::array{std::byte{0x2e}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored C-locale decimal-separator character");
    // Complete 11E060 branch table: unsigned selector 0..3 is guarded
    // by 11E064/11E067; padding after the fourth identity is excluded.
    memory.mount(0x11e09cu,encode(std::array{0x11e070u,0x11e074u,0x11e070u,0x11e074u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 11E069 four-case component selector identities");
    // Complete authored 1CD130 Havok statistics format strings, selected by
    // the original 1CD1F9..1CD4FD calls into the retained CRT formatter.
    memory.mount(0x4618ecu,std::as_bytes(std::span("Havok Performance Stats")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD1F9 Havok statistics format text");
    memory.mount(0x4618bcu,std::as_bytes(std::span("\t simulation islands (active/inactive):   %d/%d")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD205 Havok statistics format text");
    memory.mount(0x46188cu,std::as_bytes(std::span("\t fixed entities:                         %d")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD214 Havok statistics format text");
    memory.mount(0x46185cu,std::as_bytes(std::span("\t dynamic entities (active/inactive):     %d/%d")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD220 Havok statistics format text");
    memory.mount(0x46181cu,std::as_bytes(std::span("Havok Memory (total %.2f KB, debug %.2f KB, percent used %.3f)")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD2B3 Havok statistics format text");
    memory.mount(0x4617ecu,std::as_bytes(std::span("Startup pool: mem(%.2f KB/%.2f KB), allocs(%d)")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD2EC Havok statistics format text");
    memory.mount(0x4617b8u,std::as_bytes(std::span("Overflow pool: mem(%.2f KB/%.2f KB), allocs(%d))")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD325 Havok statistics format text");
    memory.mount(0x461780u,std::as_bytes(std::span("2nd Overflow pool: mem(%.2f KB/%.2f KB), allocs(%d))")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD35E Havok statistics format text");
    memory.mount(0x461738u,std::as_bytes(std::span("Runtime Pool(%.2f KB): usage==pages(%d/%d)*size(%.2f KB)+last(%.2f KB)")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD3B8 Havok statistics format text");
    memory.mount(0x46171cu,std::as_bytes(std::span("\t b%d(%d) (%d / %d (*%d))")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD3E6 Havok statistics format text");
    memory.mount(0x4616ecu,std::as_bytes(std::span("Havok Component Meta Information Resource Stats")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD45F Havok statistics format text");
    memory.mount(0x461690u,std::as_bytes(std::span("HEAP meta contact point memory %.2f KB (max %d), meta rigid body memory %.2f KB (max %d)")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD492 Havok statistics format text");
    memory.mount(0x461638u,std::as_bytes(std::span("HEAP meta phantom memory %.2f KB (max %d), meta constaint memory %.2f KB (max %d)")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD4C9 Havok statistics format text");
    memory.mount(0x461614u,std::as_bytes(std::span("TOAL HEAP meta memory %.2f KB")),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 1CD4FD Havok statistics format text");
    // Authenticated default C-locale ushort classification flags consumed
    // by32D457. Encode the declared ASCII categories, including the exact
    // original hexadecimal and whitespace bits; high bytes are unclassified.
    std::array<std::byte,512> character_flags{};
    for(unsigned c=0;c<128;++c) {
        std::uint16_t flags=0;
        if(c<32 || c==127)flags|=0x20;
        if(c>=9 && c<=13)flags|=0x8;
        if(c==32)flags|=0x48;
        if(c>=33 && c<=126) {
            if(c>='A' && c<='Z')flags|=1;
            else if(c>='a' && c<='z')flags|=2;
            else if(c>='0' && c<='9')flags|=4;
            else flags|=0x10;
        }
        if((c>='0' && c<='9') || (c>='A' && c<='F') || (c>='a' && c<='f'))flags|=0x80;
        character_flags[c*2]=std::byte(flags);character_flags[c*2+1]=std::byte(flags>>8);
    }
    memory.mount(0x41fa48u,character_flags,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 default C-locale character classification flags");
    memory.mount(0x46e8e0u,encode(std::array{0x41fa48u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial C-locale classification binding");
    // Actual125C90 resource-cache storage, before any map activation.
    memory.mount(0x51df20u,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold animation resource-cache initialized and active flags");
    memory.mount(0x4f0418u,std::array<std::byte,24>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold animation resource-cache pool and request bindings");
    constexpr std::array<char,24> animation_cache_name{"xbox animation cache"};
    constexpr std::array<char,28> predicted_animation_name{"xbox predicted animations"};
    constexpr std::array<char,16> animation_name{"xbox animation"};
    memory.mount(0x45f194u,std::as_bytes(std::span(animation_cache_name)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation cache pool name");
    memory.mount(0x45f1acu,std::as_bytes(std::span(predicted_animation_name)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored predicted animation pool name");
    memory.mount(0x45f1c8u,std::as_bytes(std::span(animation_name)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation resource pool name");
    memory.mount(0x474cbcu,encode(std::array{0x486038u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored resource-cache embedded-pool allocator binding");
    // Actual13475 publishes the second submitted-render input word here;
    // 2070F tests that word before admitting the current lens-query view.
    // Its neighboring inputs4918E4/4918EC remain separate, unborrowed owners.
    memory.mount(0x4918e8u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold submitted lens-query gate word");
    // Complete the five absent immutable literal operands in the admitted
    // lens-query/draw closure (v794 offline TU1.5 and live owner audit).
    // Preserve authored bits; no host recalculation or completed lens result.
    for(const auto&[address,bits]:std::array{
        std::pair{0x46a250u,0x4b7fffffu},std::pair{0x46a268u,0x3c008081u},
        std::pair{0x46a384u,0x3c800000u},std::pair{0x46a51cu,0x3fb504f3u},
        std::pair{0x46a9b4u,0x4f000000u}})
        share_immutable_word(memory,address,bits,"native TU1.5 authored lens-query/draw scalar operand");
    // Authenticated readonly operand: ctf-return-literal-authentication-v1.json.
    share_immutable_word(memory,0x46a9c0u,0x3d23d70bu,"native TU1.5 authored physics immutable dispatch data");
    // Actual2F2B3 admits selectors0..4 before2F2C5 reads this complete
    // inline .text table. Preserve original branch identities, not results.
    memory.mount(0x2f5c8u,encode(std::array{0x2f5bau,0x2f2ccu,0x2f3e2u,0x2f3feu,0x2f50au}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored five-entry lens draw dispatch table");
    // Original132B0 copies exactlyA6 DWORDs from its real submitted frame
    // into this loader-zeroed record. Camera491E38 is its74-byte subrecord,
    // independently saved/restored by3BD4C/3BE4E. Preserve one complete
    // owner for those true copies; no camera or projection is prepopulated.
    memory.mount(0x491e20u,std::array<std::byte,0x298>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold submitted frame record");
    // Separate from that A6-DWORD copy, actual12A58 publishes the submitted
    // render mode here.2068A and the real view branches read this exact word.
    // Preserve loader-zero storage; no mode or completed frame is fabricated.
    memory.mount(0x4920b8u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold submitted render-mode word");
    // Actual1FC10 reads this separate loader-zeroed double at1FC3B.
    // Original12A71/12A7A publishes the submitted frame's two real clock
    // words here. Keep its storage cold until that clock publication runs.
    memory.mount(0x4920c0u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold submitted screen-clock double");
    // Original13EAD0 writes this separate byte at13EB25 when starting
    // the actual cinematic. Preserve its loader-zeroed backing only.
    memory.mount(0x515ef5u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold original cinematic byte destination");
    // Original1D4200 copies exactly eight DWORDs from this authored
    // packed quaternion/translation/scale identity before real preparation.
    // Its32-byte record ends at the separate44AC24 matrix identity below.
    memory.mount(0x474d38u,encode(std::array{0x44ac04u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1D420D packed identity record binding");
    memory.mount(0x44ac04u,encode(std::array{
        0u,0u,0u,0x3f800000u,0u,0u,0u,0x3f800000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1D421D packed identity record");
    // Actual168BC5 copies this authored scale/basis/translation identity
    // into each of its four newly constructed first-person view records.
    memory.mount(0x474d30u,encode(std::array{0x44ac24u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored first-person identity transform binding");
    memory.mount(0x44ac24u,encode(std::array<std::uint32_t,13>{
        0x3f800000u,0x3f800000u,0,0,0,0x3f800000u,0,0,0,0x3f800000u,0,0,0}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored identity transform operands");
    memory.mount(0x46a49cu,encode(std::array{0x3c000000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored facial-weight normalization operand");
    memory.mount(0x5564dcu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold facial-weight normalization destination");
    // Four complete map-world CRT scalar publications381710..381770.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
        {0x46ab88u,0x3f9c61aau},{0x46a164u,0x3c8efa35u},
        {0x46a5f8u,0x3ff5be0bu},{0x44eaa0u,0x47435000u}}})
        memory.mount(address,encode(std::array{bits}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored map-world CRT scalar operand");
    memory.mount(0x55d6e4u,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold map-world CRT scalar destinations");
    // Actual383130 invokes30BAE0 to construct the static contact agent.
    memory.mount(0x48cdc4u,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold static contact-agent destination");
    // The authenticated _cinit table46DAD8..46DBF0 contains the exact
    // consecutive381A60+10*n AI handler-registration callback identities.
    // The existing world executes all71 owners; no handler masks are filled
    // by this memory factory.
    std::array<std::uint32_t,71> ai_registration_callbacks{};
    for(unsigned i=0;i<ai_registration_callbacks.size();++i)
        ai_registration_callbacks[i]=0x381a60u+i*0x10u;
    memory.mount(0x46dad8u,encode(ai_registration_callbacks),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored AI CRT handler-registration callback identities");
    struct AiRegistrationField {std::uint32_t operand,mask,destination;};
    constexpr std::array<AiRegistrationField,71> ai_registration_fields{{
#include "campaign/ai/native_tu15_world_ai_registration.inc"
    }};
    std::set<std::uint32_t> mounted_masks;
    for(const auto field:ai_registration_fields) {
        if(mounted_masks.insert(field.operand).second)
            memory.mount(field.operand,encode(std::array{field.mask}),SourceRuntimeMemory::Access::ReadOnly,
                "native TU1.5 authored AI handler mask operand");
        memory.mount(field.destination,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold AI handler mask registration destination");
    }
    // Authored no-scenario selection and loader-zeroed resident scenario
    // binding.212A20 reads these before any map is admitted; the normal tag
    // binder later publishes the actual selected scenario and definition.
    memory.mount(0x474c20u,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial no-scenario datum selection");
    // Actual191CBA reads the authored absent-session handle. Original551E2
    // publishes an allocated handle here;55254 restores the absent value.
    memory.mount(0x4736a0u,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored initial absent network session handle");
    memory.mount(0x4ecc20u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold resident scenario definition binding");
    for(const auto address:{0x55658cu,0x556d90u,0x4f0414u,0x4ecc1cu,0x52d664u})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold map reservation, tag-index or resident definition binding");
    // Complete1F31E0 switch for the source's0..15 serialized shape classes,
    // consumed when normal tag activation resolves real physics definitions.
    memory.mount(0x1f3284u,encode(std::array<std::uint32_t,16>{
        0x1f31f5u,0x1f3214u,0x1f3225u,0x1f3236u,0x1f3247u,0x1f3203u,0x1f3274u,0x1f3282u,
        0x1f3282u,0x1f3282u,0x1f3282u,0x1f3282u,0x1f3282u,0x1f3282u,0x1f3255u,0x1f3264u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 serialized physics shape dispatch identities");
    memory.mount(0x461fc0u,encode(std::array{0x3061f0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 serialized physics shape child-count method identity");
    memory.mount(0x46a374u,encode(std::array{0x3ae4c388u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored biped definition conversion scalar");
    share_immutable_word(memory,0x46a370u,0x42b40000u,
        "native TU1.5 authored biped definition angle operand");
    // Remaining named immutable operands of the complete165B30 biped
    // resident-definition conversion. Source code chooses every branch and
    // performs each original store; these IEEE754 words are never recomputed
    // from host decimal constants.
    //46A22C is already retained by the final-storage declaration above.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,16>{{
        {0x449fd4u,0x3fc00000u},{0x455a90u,0x42f00000u},{0x455a94u,0x42700000u},
        {0x46a0ccu,0x3f000000u},{0x46a13cu,0x3f400000u},{0x46a194u,0x3f666666u},
        {0x46a1a4u,0x42480000u},{0x46a20cu,0x41f00000u},
        {0x46a350u,0x40940000u},{0x46a354u,0x40b00000u},{0x46a358u,0x43480000u},
        {0x46a35cu,0x3fa66666u},{0x46a360u,0x43200000u},{0x46a364u,0x3fe00000u},
        {0x46a368u,0x42960000u},{0x46a36cu,0x425c0000u}}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored biped definition conversion operand");
    memory.mount(0x47c72cu,std::array<std::byte,3>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial optional cache-file comparison policy bytes");
    memory.mount(0x556d94u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold selected BSP resource base binding");
    // BSP selection writes the low halfword. Complete1CFE60 also copies
    // the original fullDWORD at1CFE75, including its authored adjacent0001.
    // Preserve these four initial bytes; the next pointer begins474C28.
    memory.mount(0x474c24u,encode(std::array{0x0001ffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial BSP selection halfword and adjacent authored halfword");
    memory.mount(0x4ecc0cu,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BSP collision, lightmap and definition bindings");
    memory.mount(0x4f9c6cu,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold player reset suppression byte");
    for(const auto address:{0x4f2d70u,0x4f2d7cu})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold model-resource activation counter");
    memory.mount(0x4f2d80u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold model-resource activation byte");
    // Original130F90 reads this authored writable .data setting during the
    // complete130890 geometry-cache frame. Initial01 is pinned in v800;
    // retain the warning policy and the independent cold activation gate.
    // The renderer-cache-pool declaration already owns this exact byte.
    // Validate that owner instead of mounting a second overlapping copy.
    if(memory.view(0x48c754u,1).front()!=std::byte{1} || memory.is_read_only(0x48c754u,1))
        throw std::invalid_argument("Native geometry-cache pressure policy differs from its renderer owner");
    // Actual2B520 clears the cached-render-state counter and exact C/1AA
    // DWORD scratch banks before marking its authored first -1 sentinel.
    memory.mount(0x4c6954u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold cached-render-state counter");
    memory.mount(0x4c6190u,std::array<std::byte,0x30>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold cached-render-state twelve-word scratch bank");
    memory.mount(0x4d1838u,std::array<std::byte,0x6a8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold cached-render-state activation scratch bank");
    memory.mount(0x515edcu,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold cached-render-state activation byte");
    // Whole3FD60 bounds eight cached special banks after scratch bank0;
    // 44AB0/44B2A selects them with stride408. Keep the complete nine-bank
    // loader-zeroed extent and the separate32-short fallback list. Original
    // cached-frame reset/collection owners still produce their live contents.
    memory.mount(0x4d2b18u,std::array<std::byte,9*0x408>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed nine cached special-unit banks");
    memory.mount(0x4d4f60u,std::array<std::byte,0x40>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed cached special fallback index bank");
    memory.mount(0x515f1cu,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed cached special count and selected-bank index");
    // Existing pre-world CRT callbacks37F110..37FCB0 register these eight
    // authored Havok class nodes into their original intrusive chain. Each
    // node retains a method identity, cold next link and class binding.
    memory.mount(0x48c994u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold Havok class-registration chain head");
    struct HavokClassNode {std::uint32_t address,method,binding;};
    constexpr std::array<HavokClassNode,8> class_nodes{{
        {0x46e344u,0x2e1400u,0x48c93cu},{0x46e358u,0x2e1fc0u,0x48c968u},
        {0x46e364u,0x2e2a10u,0x48c980u},{0x46e3dcu,0x2e4320u,0x48c9f4u},
        {0x46e440u,0x2e61c0u,0x48cac4u},{0x473338u,0x37c580u,0x491350u},
        {0x473344u,0x37c7c0u,0x491358u},{0x473350u,0x37c830u,0x49135cu}}};
    for(const auto node:class_nodes)
        memory.mount(node.address,encode(std::array{node.method,0u,node.binding}),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 initial Havok class-registration node");
    mount_object_widget_types(memory);
    // Complete188860 genuinely publishes its NONE sentinels into these
    // three fixed destinations:64DWORDs,2048DWORDs,and256DWORD pairs.
    // Loader-zeroed backing only; exclude the separate32-byte4F9330 owner.
    memory.mount(0x4f7230u,std::array<std::byte,64*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 188860 sixty-four reset DWORD destinations");
    memory.mount(0x4f7330u,std::array<std::byte,2048*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 188860 two-thousand-forty-eight reset DWORD destinations");
    memory.mount(0x4f9350u,std::array<std::byte,256*8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 188860 two-hundred-fifty-six reset DWORD-pair destinations");
    memory.mount(0x486054u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold Havok memory activation byte");
    memory.mount(0x486058u,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial Havok memory owner sentinel");
    for(const auto address:{0x486064u,0x486068u,0x48606cu})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold Havok memory activation state binding");
    memory.mount(0x46e350u,encode(std::array{0x2e2800u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored Havok base-system initializer identity");
    memory.mount(0x46e354u,encode(std::array{0x2e1f80u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1CB3F2 Havok shutdown function identity");
    for(const auto address:{0x48c938u,0x48c93cu,0x48c968u,0x48c980u,0x48c9f4u,
                            0x48cac4u,0x491350u,0x491358u,0x49135cu})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold Havok allocator or registered class-instance binding");
    memory.mount(0x48c96cu,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold Havok base-system initialized flag");
    memory.mount(0x48c970u,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold Havok2E2A10 allocation tracking fields");
    memory.mount(0x45f4dcu,encode(std::array{0x149da0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok allocator allocate-method identity");
    share_immutable_word(memory,0x45f4f0u,0x2e10a0u,
        "native TU1.5 authored canonical 30021F retained allocator growth-method identity");
    share_immutable_word(memory,0x45f4f4u,0x2e1110u,
        "native TU1.5 authored canonical 3002AC retained allocator release-method identity");
    share_immutable_word(memory,0x45f4e8u,0x2e4d60u,
        "native TU1.5 authored 1CD169 retained allocator statistics-method identity");
    memory.mount(0x46394cu,encode(std::array{0x232d00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok main-heap allocate-method identity");
    memory.mount(0x463944u,encode(std::array{0x232ad0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok main-heap block-method identity");
    //232C67's complete17-category map shares its first zero byte with the
    // high byte of232C5C's third (null) branch identity. Preserve that exact
    // packed operand owner. Invalid categories still reach the real failure
    // dependency; no allocation result is provided by this table.
    std::array<std::byte,28> allocation_dispatch{};
    const auto allocation_branches=encode(std::array<std::uint32_t,3>{0x232c1au,0x232c14u,0});
    std::copy(allocation_branches.begin(),allocation_branches.end(),allocation_dispatch.begin());
    constexpr std::array<std::uint8_t,17> allocation_categories{0,0,1,2,1,2,2,2,1,2,2,2,2,2,2,2,1};
    for(unsigned i=0;i<allocation_categories.size();++i)
        allocation_dispatch[11+i]=std::byte(allocation_categories[i]);
    memory.mount(0x232c5cu,allocation_dispatch,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 main-heap category and allocation dispatch operands");
    const auto aligned_allocation_branches=encode(std::array<std::uint32_t,3>{0x232d4eu,0x232d64u,0});
    std::copy(aligned_allocation_branches.begin(),aligned_allocation_branches.end(),allocation_dispatch.begin());
    memory.mount(0x232d70u,allocation_dispatch,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 main-heap aligned-allocation category and dispatch operands");
    memory.mount(0x48c998u,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold Havok base-system retained resource bindings");
    memory.mount(0x41be10u,encode(std::array{0x2e1770u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok retained resource initialization-method identity");
    memory.mount(0x41bdfcu,encode(std::array{0x31d400u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored Havok base-system completion method identity");
    for(const auto address:{0x48b860u,0x50ec10u})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold physics world binding or update counter");
    memory.mount(0x48b864u,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored physics world selection sentinel");
    memory.mount(0x50ec98u,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold physics world datum traversal bindings");
    memory.mount(0x52d880u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold physics world interface bindings");
    for(const auto address:{0x48b870u,0x51df58u})
        memory.mount(address,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold physics world activation policy byte");
    memory.mount(0x5020f0u,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold deferred callback policy and pending flags");
    memory.mount(0x5020d0u,std::array<std::byte,32>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold deferred scope callback and userdata slots");
    memory.mount(0x5020f2u,std::array<std::byte,0x100>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold deferred scope message and terminator buffer");
    memory.mount(0x5021f4u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold deferred scope progress counters");
    constexpr std::array<char,40> bsp_havok_name{"building havok representation for bsp"};
    memory.mount(0x461908u,std::as_bytes(std::span(bsp_havok_name)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored BSP Havok construction scope name");
    memory.mount(0x48b872u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial BSP Havok construction policy byte");
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
        {0x456914u,0x3f19999au},{0x46a0f8u,0x3f7eb852u},
        {0x46a1a0u,0x42800000u},{0x46a74cu,0x3ea7ef9eu}}})
        memory.mount(address,encode(std::array{bits}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored Havok world construction scalar operand");
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{{
        {0x48b874u,0x3d06594bu},{0x48b88cu,0xbc2137f4u}}})
        memory.mount(address,encode(std::array{bits}),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 initial Havok world construction scalar setting");
    for(const auto address:{0x48b885u,0x48b886u,0x48b887u,0x48b895u})
        memory.mount(address,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 authored Havok world construction option byte");
    // Original1ADFE8 publishes the current targeting parameter bank here
    // before32B560 invokes comparator1AF2E0 (which reads this same cell).
    // Retain the loader-zero writable owner; the actual query supplies its
    // pointer on every sort. Offline TU1.5 audit: mcc-vehicle-data-v870.
    memory.mount(0x52be40u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold targeting comparator parameter binding");
    memory.mount(0x52be48u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BSP physics representation binding");
    // Original176462 reads this separate .data alias of the same bounds
    // destination used by474D40. Preserve its writable loader initializer;
    // actual380FA0 publishes the six bounds components during native startup.
    memory.mount(0x474d3cu,encode(std::array{0x55655cu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold portal empty-bounds binding");
    memory.mount(0x474d40u,encode(std::array{0x55655cu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored BSP physics empty-bounds binding");
    memory.mount(0x52be74u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BSP physics representation count");
    memory.mount(0x52be78u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BSP physics activation binding");
    memory.mount(0x41c6a0u,encode(std::array<std::uint32_t,4>{
        0x477ffc00u,0x477ffc00u,0x477ffc00u,0x477ffc00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored Havok world quantization SIMD operand");
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{
        {0x41c680u,0x47800000u},{0x41c690u,0u},{0x461960u,0x80000000u}}})
        memory.mount(address,encode(std::array<std::uint32_t,4>{bits,bits,bits,bits}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored Havok quantization bias, zero or sign-mask SIMD operand");
    memory.mount(0x46aac8u,encode(std::array{0x41300000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored Havok quantization exponent operand");
    memory.mount(0x41f274u,encode(std::array{0x323420u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world construction callback method identity");
    memory.mount(0x41f090u,encode(std::array{0x320130u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world construction secondary callback method identity");
    memory.mount(0x45f4e0u,encode(std::array{0x149e00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok allocator free-method identity");
    memory.mount(0x463950u,encode(std::array{0x232d20u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok main-heap free-method identity");
    memory.mount(0x463948u,encode(std::array{0x232c80u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok main-heap release-block method identity");
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,2>{{
        {0x46a690u,0x4cbebc20u},{0x46a694u,0xccbebc20u}}})
        memory.mount(address,encode(std::array{bits}),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored Havok world initial bound operand");
    memory.mount(0x41d894u,encode(std::array{0x315170u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world secondary interface initialization method identity");
    memory.mount(0x41c6c4u,encode(std::array{0x2ec4b0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world quantized interface method identity");
    memory.mount(0x41d988u,encode(std::array{0x3172a0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world retained interface method identity");
    memory.mount(0x41d93cu,encode(std::array{0x31ffc0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world retained interface activation method identity");
    memory.mount(0x41d898u,encode(std::array{0x315300u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world secondary interface activation method identity");
    memory.mount(0x466c80u,encode(std::array{0x282700u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world temporary interface destructor identity");
    memory.mount(0x52be4cu,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold Havok world auxiliary resource bindings");
    memory.mount(0x556588u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold resident-cache pointer admission flag");
    memory.mount(0x5565acu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold resident-cache pointer admission extent");
    memory.mount(0x41d970u,encode(std::array{0x316540u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world filter-interface method identity");
    memory.mount(0x41d98cu,encode(std::array{0x3172f0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world retained interface filter method identity");
    memory.mount(0x46e46cu,encode(std::array{0x3038c0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initial Havok world callback method binding");
    memory.mount(0x48b868u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold physics world update counter");
    memory.mount(0x4638c8u,encode(std::array{0x233250u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok active-heap allocate-method identity");
    memory.mount(0x461570u,encode(std::array{0x2e9600u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world body-interface method identity");
    memory.mount(0x462164u,encode(std::array{0x2e9600u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 Havok world collision body-interface method identity");
    memory.mount(0x462128u,encode(std::array{0x1f7300u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 BSP collision body-interface method identity");
    memory.mount(0x461950u,encode(std::array<std::uint32_t,4>{
        0x7fffffffu,0x7fffffffu,0x7fffffffu,0x7fffffffu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored collision absolute-value SIMD mask");
    memory.mount(0x56dfccu,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold optional Havok world diagnostic policy byte");
    memory.mount(0x48b86cu,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored physics world traversal sentinel");
    // Actual130511 selects all six resource-mode branches. Original CC
    // padding starts immediately after the final authenticated branch ID.
    memory.mount(0x1305c0u,encode(std::array{
        0x13051fu,0x13051fu,0x13051fu,0x130518u,0x13051fu,0x130518u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 model resource-cache allocation dispatch identities");
    memory.mount(0x4f2d64u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold model resource-cache physical base and extent");
    memory.mount(0x4f2d30u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold texture resource-cache physical base");
    memory.mount(0x4f63c0u,std::array<std::byte,50*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold view activation fifty-word reset storage");
    memory.mount(0x50ee28u,std::array<std::byte,40*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold game-engine activation forty-word reset storage");
    share_immutable_word(memory,0x46a11cu,0x40000000u,
        "native TU1.5 authored game-engine tick scaling operand");
    memory.mount(0x474c78u,encode(std::array{0x44aafcu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored player-effect initial vector binding");
    // Actual1FEB9 selects this authored fallback when the screen vector
    // equals its original zero operand. Keep both the pointer and vector.
    memory.mount(0x474c80u,encode(std::array{0x44ab1cu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1FEB9 default screen-vector binding");
    memory.mount(0x44ab1cu,encode(std::array{0u,0x3f800000u,0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored1FEB9 default screen-vector operand");
    // Actual17C84C skips the alpha word of this complete authored RGBA
    // record before copying its three RGB components into the lens input.
    memory.mount(0x474c2cu,encode(std::array{0x44aad8u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored17C84C default lens RGBA binding");
    // RGB+4 already has its complete shared color/scale owner below.
    memory.mount(0x44aad8u,encode(std::array{0x3f800000u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored default lens RGBA alpha operand");
    // ActualD6D16 selects this authored color record, then skips its
    // alpha word and copies the same existing three-component black RGB.
    memory.mount(0x474c34u,encode(std::array{0x44aaf8u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored D6D10 initial RGB record binding");
    memory.mount(0x4751acu,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 130A1F resource request policy byte");
    memory.mount(0x44aafcu,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored player-effect initial three-element vector");
    memory.mount(0x4f54f8u,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold player-effect activation vector destination");
    memory.mount(0x51df60u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold player-effect activation policy byte");
    memory.mount(0x48c498u,encode(std::array{
        0x468be8u,0x468cb8u,0x468d88u,0x468e58u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored four static game-engine class interfaces");
    // Exact registered method-record prefixes, bounded by independent next
    // owners/padding and the final original string. Existing identical RO
    // providers are shared; callback bodies remain required native owners.
#include "gameplay/multiplayer/native_tu15_game_engine_methods.inc"
    // Constructor-backed eight-slot physical records, authenticated in v1185.
    // Actual MCC PRT scenes reach41D03C+10; class3 bodies also require41D638.
    // Next constructors/type-name/scalar owners bound these exact records.
    // Callback bodies/results remain required original execution.
#include "physics/native_tu15_physical_methods.inc"
    memory.mount(0x52d898u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold selected game-engine state binding");
    // 69220 publishes this static simulation-world owner, and complete
    // 69FF0 constructs its fields through +1220 before object admission.
    // The next original static owner starts at 554BD4. This is cold storage,
    // not an imported initialized world or a supplied constructor result.
    memory.mount(0x5539b0u,std::array<std::byte,0x1224>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold simulation-world construction destination");
    //69220 publishes this adjacent context at4DC050; its next independent
    //owner is555808. Whole692D0/86BD0/887B0/86C30 construct the context,
    //machine masks and player rows before retained-result startup reads it.
    memory.mount(0x554bd4u,std::array<std::byte,0xc34>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold simulation player and machine context");
    memory.mount(0x4dc041u,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 692D0 simulation-map policy bytes");
    memory.mount(0x4dc048u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 692D0 completed simulation-map publication");
    //Original19ACBA reads one loader-zeroed policy byte before querying
    //the retained network-session binding during result finalization.
    memory.mount(0x535f00u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold retained-result network-session publication policy");
    //Whole77950 reads its original result-channel kind/state and64-bit
    //identity. Whole77AB0 checks the same state before requesting work.
    //These are loader inputs, not an initialized/submitted result channel.
    memory.mount(0x51d014u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold retained-result channel kind and state");
    memory.mount(0x51d040u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold retained-result channel identity");
    memory.mount(0x779d0u,encode(std::array{0x77996u,0x7797eu,0x77986u,0x7798eu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 original77950 bounded result-state branches");
    memory.mount(0x779e0u,encode(std::array{0x779a8u,0x779b0u,0x779b8u,0x779c0u,0x779c8u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 original77950 bounded result-kind branches");
    //Whole23975C builds the actual completed-result display projection.
    //239789/239795 clear exactly450/210 DWORDs; later source calls own
    //the names, rankings, mode-specific counters and published row counts.
    memory.mount(0x56b990u,std::vector<std::byte>(0x1140u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold completed player result display rows");
    memory.mount(0x56cad0u,std::vector<std::byte>(0x840u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold completed team result display rows");
    memory.mount(0x52d698u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold completed player and team result row counts");
    memory.mount(0x50ed24u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold completed result display mode selector");
    //Original wide formatter32DB25 unsigned-bounds its next state by7
    //at32DB8F, then selects this exact unaligned eight-word code table.
    memory.mount(0x32e2c9u,encode(std::array{0x32dd3bu,0x32dba1u,0x32dbbeu,0x32dc08u,
        0x32dc44u,0x32dc4du,0x32dc86u,0x32dd53u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 original completed-result wide-format state branches");
    //32E242 delivers the authored one-byte multibyte-character bound to
    //whole330389 while the original formatter emits score/name text.
    memory.mount(0x46e8e8u,encode(std::array{1u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 original CRT multibyte-character maximum width");
    //Whole164660 formats time-based result scores using these three
    //exact NUL-terminated UTF16 literals, including its blank minute0.
    memory.mount(0x437544u,encode(std::array{0x20u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 original result time-format blank minute literal");
    memory.mount(0x45f71cu,encode(std::array{0x00730025u,0x0025003au,0x00000073u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 original result minute-second format literal");
    memory.mount(0x45f728u,encode(std::array{0x00250030u,0x00000064u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 original result seconds padding format literal");
    memory.mount(0x6a190u,encode(std::array{
        0x6a00cu,0x6a01eu,0x6a015u,0x6a030u,0x6a027u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 simulation-world mode branch identities");
    memory.mount(0x157fa8u,encode(std::array{
        0x157f2cu,0x157f2cu,0x157f07u,0x157f4fu,0x157f2cu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored director new-map mode branch identities");
    // 157F60 copies one initial interpolation selector from each 20-byte
    // stride. Keep only those four declared cold source fields; the actual
    // director callback writes the view-local interpolation state.
    for(unsigned n=0;n<4;++n)
        memory.mount(0x48c4b8u+n*0x20u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 cold director interpolation source selector");
    memory.mount(0x51d048u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold simulation clock override policy");
    memory.mount(0x51d04cu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold simulation clock override value");
    // Complete B9960 copies the authored forward/up defaults into its fresh
    // object-build request. These are immutable source operands, not poses.
    // Original F5F44 and the other direct consumers audited in v861 read
    // these separate file-initialized .data pointer cells. Preserve their
    // original writable access and existing shared constant-vector owners;
    // these are initial aliases, not supplied runtime body/camera results.
    for(const auto [address,target]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
        {0x474cc0u,0x44abecu},{0x474cd4u,0x44abecu},
        {0x474cd8u,0x44ac28u},{0x474cdcu,0x44ac34u}}})
        memory.mount(address,encode(std::array{target}),SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 authored direct-consumer vector default alias");
    memory.mount(0x474d08u,encode(std::array{0x44ac28u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 object-build forward default binding");
    memory.mount(0x474d10u,encode(std::array{0x44ac40u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 object-build up default binding");
    // Both vectors already belong to the authored 44AC24 identity bank used
    // by 168BC5 above; retain that shared owner instead of mounting aliases.
    memory.mount(0x48b873u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold physics 148360 reentrancy guard byte");
    // 148740 checks optional retained source bindings before consulting their
    // fields. Cells48606C/486070 already belong to earlier Havok declarations.
    memory.mount(0x486074u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 148740 optional retained owner binding");
    // Original148779..1487C8 computes and classifies the retained owner's
    // scalar using these exact IEEE operands. Share earlier declarations
    // where present, preserving their immutable bytes and allocation owner.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,5>{{
        {0x46a144u,0x3c0ad8f3u},{0x456910u,0x3f4ccccdu},
        {0x46a140u,0x3f59999au},{0x46a13cu,0x3f400000u},{0x46a138u,0x3f266666u}}}) {
        share_immutable_word(memory,address,bits,"native TU1.5 authored 148740 scalar classification operand");
    }
    memory.mount(0x46a134u,encode(std::array{0x358ad8f3u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 22ECB0 retained heap scalar normalization operand");
    share_immutable_word(memory,0x46a0ecu,0x38d1b717u,
        "native TU1.5 authored 1CE845 physical representation scalar operand");
    // Original2508DC/250905 compare their actual x87 results with this
    // single authored dword operand. Adjacent scalar owners are excluded.
    share_immutable_word(memory,0x456670u,0x38d1b717u,
        "native TU1.5 authored 2508DC scalar comparison operand");
    share_immutable_word(memory,0x46a1b4u,0x477fff00u,
        "native TU1.5 authored 250D66 x87 scalar multiplication operand");
    share_immutable_word(memory,0x46a7f4u,0x3b360b61u,
        "native TU1.5 authored 2524BD scalar multiplication operand");
    // Original169C8C/169CA6 load these separate scalar operands in the
    // same basic block. Their actual native consumer retains the tuple.
    share_immutable_word(memory,0x46aaf0u,0x42900000u,
        "native TU1.5 authored 169C8C scalar operand");
    share_immutable_word(memory,0x46aaecu,0x41d80000u,
        "native TU1.5 authored 169CA6 scalar operand");
    share_immutable_word(memory,0x46a1f8u,0x3f99999au,
        "native TU1.5 authored E891F scalar comparison operand");
    // Original1F932A adds this file-initialized data scalar to its real
    // x87 result. Preserve its original writable data storage and bytes.
    memory.mount(0x48c7d0u,encode(std::array{0x3f000000u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 1F932A additive scalar default");
    // Whole176E70's exhausted location-pool path reads this authored -1000
    // warning tick at176FE0 and updates it at176FEA. It is writable .data,
    // independent of the live allocation result and source game clock.
    memory.mount(0x48c7a8u,encode(std::array{0xfffffc18u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 176E70 effect-location exhaustion warning tick");
    // CompleteBA3E0 invokes exactly eight object deactivation callbacks
    // before retiring BSP-excluded scene placements (BA410 bounds32bytes).
    memory.mount(0x474bc4u,encode(std::array{
        0xbd8b0u,0xc36a0u,0x1d21e0u,0x155820u,
        0x1693edu,0x1611e0u,0x2118a0u,0x17e250u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored BA407 object deactivation callback identities");
    // Same four typed activity pairs already recovered for the working
    // unit-constructor tail; complete111B65 indexes this original row order.
    memory.mount(0x474be4u,encode(std::array{
        0x0a000014u,0x09000015u,0x09000015u,0x0a000014u,
        0x09000016u,0x0a000017u,0x0a000017u,0x09000016u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored unit constructor activity pairs");
    memory.mount(0x47c1b5u,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 1D5050 physical query policy byte");
    memory.mount(0x47c1b4u,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 1D50C2 physical query policy byte");
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{
        {0x46a188u,0x3f7ff972u},{0x46a184u,0x3f80068eu},{0x46a234u,0x3f0ccccdu}}})
        share_immutable_word(memory,address,bits,"native TU1.5 authored 1D5050 physical query scalar operand");
    memory.mount(0x47c1b8u,encode(std::array{
        0x0a000014u,0x0a000521u,0x09000015u,0x09000522u,
        0x09000016u,0x09000523u,0x0a000017u,0x0a000524u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1D4F80 four physical-query selector pairs");
    // Complete115E95 selects the same authored blend operands retained by
    // OriginalData::unit_constructor_tail_constants in the working build.
    // The actual constructor still decides and writes the blend state.
    share_immutable_word(memory,0x450170u,0x3e88b439u,
        "native TU1.5 authored unit constructor normal blend operand");
    share_immutable_word(memory,0x46a1e0u,0x3e08b439u,
        "native TU1.5 authored unit constructor special blend operand");
    share_immutable_word(memory,0x46a0e8u,0u,
        "native TU1.5 authored unit constructor blend comparison zero");
    // Complete1ED780 dispatches the six authored physics-representation
    // kinds through these two branch tables; branch identities are data.
    memory.mount(0x1ed84cu,encode(std::array{
        0x1ed7a4u,0x1ed7a4u,0x1ed7a4u,0x1ed798u,0x1ed798u,0x1ed7a4u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored physical representation disposal branch identities");
    memory.mount(0x1ed864u,encode(std::array{
        0x1ed7b5u,0x1ed7bfu,0x1ed7e1u,0x1ed7f5u,0x1ed7f5u,0x1ed7ebu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored physical representation construction branch identities");
    memory.mount(0x48a0b0u,std::array{std::byte{0xff},std::byte{0xff}},
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1F86A0 physical representation absent index");
    memory.mount(0x474c70u,encode(std::array{0x44aadcu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored C0270 object scale default binding");
    memory.mount(0x44aadcu,encode(std::array{0x3f800000u,0x3f800000u,0x3f800000u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored unit scale and neutral color default vector");
    // CompleteC0270 retains authored object variation arithmetic, including
    // its x87 operands. Keep the exact stored IEEE bits and operation order.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,7>{{
        {0x46a46cu,0x3f364cd7u},{0x46a470u,0x4412c849u},
        {0x46a474u,0x43d79081u},{0x46a478u,0x439df252u},
        {0x46a47cu,0x443a07f2u},{0x46a1d0u,0u},{0x46a1d4u,0x3ff00000u}}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored C0270 object variation arithmetic operand");
    memory.mount(0x46ecd0u,encode(std::array{0x2694u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored CRT math dispatch policy word");
    memory.mount(0x46ecb4u,std::array{
        std::byte{8},std::byte{4},std::byte{8},std::byte{8},
        std::byte{8},std::byte{4},std::byte{8},std::byte{8},
        std::byte{0},std::byte{4},std::byte{12},std::byte{8},
        std::byte{0},std::byte{4},std::byte{12},std::byte{8}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored CRT x87 class-to-handler byte offsets");
    memory.mount(0xc0e78u,encode(std::array{0xc0dd7u,0xc0dfdu,0xc0e1au,0xc0e35u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored C0CD0 object definition fixup branch identities");
    // Actual37DC0A supplies this fmod descriptor to the existing x87 CRT
    // dispatcher. Header/name and sixteen class-pair method identities are
    // immutable authored data; complete331ECB still selects the operation.
    memory.mount(0x48c8ccu,encode(std::array{
        0x6f6d6604u,0x00000064u,0u,0x00160200u,
        0x37dc14u,0x33200au,0x331fa3u,0x33200au,
        0x331f68u,0x33200au,0x331fa3u,0x33200au,
        0x331fa1u,0x331fa1u,0x331fcbu,0x331fa1u,
        0x33200au,0x33200au,0x331fa3u,0x33200au}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored fmod x87 classification and dispatch descriptor");
    memory.mount(0x48d4f4u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold CRT x87 error dispatch policy storage");
    share_immutable_word(memory,0x46a1b8u,0x3e2aaaabu,
        "native TU1.5 authored 1343E0 color hue normalization operand");
    share_immutable_word(memory,0x46a10cu,0x40c00000u,
        "native TU1.5 authored 134510 color hue sector operand");
    memory.mount(0x134610u,encode(std::array{
        0x1345a3u,0x1345b5u,0x1345c7u,0x1345d9u,0x1345ebu,0x1345fdu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored HSV to RGB six-sector branch identities");
    share_immutable_word(memory,0x46a0c4u,0x3b808081u,
        "native TU1.5 authored 1346C0 color byte normalization operand");
    memory.mount(0x56ebacu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BF0C0 object model lazy initialization flags");
    // BF351 copies eight words from the actual model variant and C1EB0
    // transforms that fresh scratch. No prepared variant is supplied here.
    memory.mount(0x56eb8cu,std::array<std::byte,32>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BF0C0 object model variant fallback scratch");
    // 1260C0 indexes three 12-byte resource request scheduling rows. The
    // existing resource adapter performs real cache-file admission and I/O.
    memory.mount(0x474d70u,encode(std::array{
        8u,0u,0u,5u,10u,30u,2u,20u,50u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation resource request scheduling rows");
    // Actual18BE50/18BEB0 choose these two distinct complete32-byte
    // sound request callback descriptors using the genuine18BE00 predicate.
    // Preserve authored null hooks; native source owners execute callbacks.
    memory.mount(0x44ebf4u,encode(std::array{
        0x18f1a0u,0x18f420u,0x18f490u,0u,0x18f6b0u,0u,0x18f750u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored ordinary sound request callback descriptor");
    memory.mount(0x44ec14u,encode(std::array{
        0x18f1a0u,0x18f420u,0x18f490u,0u,0x18f6b0u,0u,0x18f750u,0x18f770u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored weapon sound request callback descriptor");
    // Complete2835B0 clears twenty-four channel scratch words and fills the
    // animation decode context itself before consuming actual clip bytes.
    memory.mount(0x56e000u,std::array<std::byte,24*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold animation channel decode scratch words");
    // Original283F95..283FA7 publishes node*32 at50EF10. The actual
    // 256-node array is loader-zero (offline v888), with node255 already
    // owned by retained startup data at510EF0. Supply its missing prefix;
    // original animation codecs still produce every transform themselves.
    memory.mount(0x50ef10u,std::array<std::byte,255*32>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed animation transform node array prefix");
    // 2847D0 adds three channel counters and the current decode cursor
    // through510FA0, before the existing next storage owner at510FA4.
    memory.mount(0x510f30u,std::array<std::byte,0x74>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 2835B0 animation decode context");
    // The same real decode destination owns quaternion +0, translation
    // +10 and scale +1C (2963D0/296410/296220), ending before context510F30.
    // Retain only loader-zero bytes; original codecs write every output.
    memory.mount(0x510f10u,std::array<std::byte,32>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed animation transform decode destination");
    memory.mount(0x48c364u,encode(std::array{
        0x296180u,0x2961f0u,0x296220u,0x296180u,0x2961f0u,
        0x296220u,0x2962e0u,0x466d00u,1u,0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation codec one dispatch descriptor");
    memory.mount(0x48c3dcu,encode(std::array{
        0x296ad0u,0x296c60u,0x296db0u,0x296700u,0x296890u,
        0x2969e0u,0x2962e0u,0x466d6cu,2u,2u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation codec four dispatch descriptor");
    memory.mount(0x48c3b4u,encode(std::array{
        0x296680u,0x296410u,0x2cec60u,0x296450u,0x296540u,
        0x296620u,0x2962e0u,0x466d4cu,2u,1u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation codec three dispatch descriptor");
    // Original2839A0 computes the exact40-byte codec row stride; the
    // first ordinary forward clip selects this complete codec-six row.
    memory.mount(0x48c42cu,encode(std::array{
        0x296ad0u,0x296c60u,0x296db0u,0x296700u,0x296890u,
        0x2969e0u,0x2962e0u,0x466db4u,2u,2u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored animation codec six dispatch descriptor");
    // Original2839A6 selects codecs5/7's two three-method variants using
    // an actual Boolean input. Both stored rows contain these same six
    // authored identities; exclude preceding metadata and following words.
    for(const auto address:{0x48c404u,0x48c454u})
        memory.mount(address,encode(std::array{
            0x297270u,0x297400u,0x297550u,0x296e90u,0x297030u,0x297180u}),
            SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored Boolean-selected animation decode triples");
    // Actual2839AD publishes only this selected decode triple. Do not
    // extend it into the separately owned game-engine binding at48C498.
    memory.mount(0x48c47cu,encode(std::array{0x2963d0u,0x296410u,0x2cec60u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2839AD selected animation codec eight dispatch triple");
    memory.mount(0x474d24u,encode(std::array{0x44abecu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 114750 unit orientation default binding");
    for(unsigned n=0;n<2;++n)
        share_immutable_word(memory,0x44abecu+n*4u,0u,
            "native TU1.5 authored unit orientation default component");
    // BF20C selects this static model-pose destination; actual1707A0
    // populates 32-byte nodes from the owning model. Its255slots end at
    // the existing4EABA0 scratch owner; retain that distinct declaration.
    memory.mount(0x4e8bc0u,std::array<std::byte,255*32>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BF0C0 model pose scratch nodes");
    // Complete144CF0 lazily writes the quaternion conversion tolerance from
    // authored46A0EC, publishing its own flag only after that real store.
    memory.mount(0x56e680u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold quaternion conversion tolerance and lazy flags");
    // Complete296540 copies both selected authored translation triples
    // before interpolating into its actual model-pose output. These two
    // three-float scratch vectors are loader-zeroed, never warm pose data.
    memory.mount(0x56e688u,std::array<std::byte,24>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 296540 two translation interpolation scratch vectors");
    memory.mount(0x45f490u,encode(std::array{
        0x80000000u,0u,0u,0x80000000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored quaternion to matrix SIMD sign mask");
    memory.mount(0x4f3ce4u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 1D2F10 physics query invocation counter");
    memory.mount(0x4f3ce1u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 1D2F10 physics query active traversal flag");
    // 14D0E0/16C620 mark visited source BSP cluster IDs against the current
    // query counter. The established512cluster ABI ends at the next
    // original owner4F44E8; actual traversal supplies each visited stamp.
    memory.mount(0x4f3ce8u,std::array<std::byte,512*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold BSP cluster query visit stamps");
    share_immutable_word(memory,0x46a238u,0x3eb33333u,
        "native TU1.5 authored 1D7C00 constructor tick scaling operand");
    memory.mount(0x1d74a4u,encode(std::array{
        0x1d742au,0x1d7456u,0x1d743au,0x1d7423u,0x1d7433u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored physical object type constructor branch identities");
    memory.mount(0x1d74bcu,std::array{
        std::byte{0},std::byte{1},std::byte{5},std::byte{5},std::byte{5},
        std::byte{5},std::byte{5},std::byte{2},std::byte{5},std::byte{5},
        std::byte{5},std::byte{3},std::byte{4}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored thirteen physical object type branch selectors");
    memory.mount(0x46152cu,encode(std::array{0x31ede0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored physical shape type query method identity");
    share_immutable_word(memory,0x46a0e4u,0x40400000u,
        "native TU1.5 authored 1E1010 vector normalization refinement operand");
    memory.mount(0x41f278u,encode(std::array{0x31ede0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored retained shape type query method identity");
    memory.mount(0x4638ccu,encode(std::array{0x233420u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 148AA2 physics allocation owner method identity");
    // C9550 compares the object's authored string identity against precisely
    // five entries, ending at4739A4, without changing the live predicate.
    memory.mount(0x473990u,encode(std::array{
        0x05000049u,0x07000679u,0x0700067au,0x0700067bu,0x0700067cu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored C9550 five object string identity operands");
    share_immutable_word(memory,0x46aa4cu,0x3d88b439u,
        "native TU1.5 authored 1154A0 alternate constructor blend operand");
    memory.mount(0x461530u,encode(std::array{0x2e8120u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 314542 physical shape method identity");
    memory.mount(0x41c664u,encode(std::array{0x2e9a00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 378FCF retained physics callback method identity");
    memory.mount(0x41d9a4u,encode(std::array{0x317b70u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 378FEA retained physics callback method identity");
    memory.mount(0x46e400u,encode(std::array{
        0x3f800000u,0u,0u,0u,0u,0x3f800000u,0u,0u,
        0u,0u,0x3f800000u,0u,0u,0u,0u,0x3f800000u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored physical transform identity matrix");
    memory.mount(0x474d1cu,encode(std::array{0x44ac74u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1791C0 downward vector binding");
    memory.mount(0x44ac74u,encode(std::array{0u,0u,0xbf800000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored downward unit vector");
    memory.mount(0x51df7cu,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 178FAC optional constraint policy byte");
    memory.mount(0x51df78u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 179039 retained constraint state binding");
    memory.mount(0x4c6709u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 3EB10 optional object state policy byte");
    // Complete14CEA0 indexes six axis/sign permutations (three shorts
    // each) after choosing the dominant source plane component.
    memory.mount(0x44ac8cu,encode(std::array{
        0x00010002u,0x00010000u,0x00000002u,
        0x00020000u,0x00020001u,0x00010000u,
        0x00000001u,0x00000002u,0x00020001u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored BSP plane axis and sign permutations");
    share_immutable_word(memory,0x46a120u,0x40800000u,
        "native TU1.5 authored C31F7 object scalar clamp operand");
    memory.mount(0x4ecbd8u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold C2358 object update generation counter");
    memory.mount(0x473d48u,encode(std::array{0x473ae4u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored E8E50 construction callback selector eight");
    memory.mount(0x473ae4u,encode(std::array{
        0xeaf00u,0xeb0a0u,0xeb290u,0xeb190u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored selected E8E50 construction callback methods");
    memory.mount(0x473d28u,encode(std::array{0x473ac4u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored E8F31 construction callback selector zero");
    memory.mount(0x473ac4u,encode(std::array{
        0xe97d0u,0xe9870u,0u,0xe9910u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored selected E8F31 construction callback methods");
    memory.mount(0x473d50u,encode(std::array{0x473ac4u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored E8F31 construction callback selector ten");
    memory.mount(0x473d94u,encode(std::array{0x473b74u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored E8F31 construction callback selector twenty-seven");
    // Complete CD348 also selects the original four-slot callback26 row;
    // its fourth method is authentically absent and remains absent here.
    memory.mount(0x473d90u,encode(std::array{0x473b64u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored CD348 construction callback selector twenty-six");
    memory.mount(0x473b64u,encode(std::array{0xebdb0u,0u,0u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored selected construction callback twenty-six methods");
    memory.mount(0x473b74u,encode(std::array{
        0xec450u,0xec620u,0u,0xec750u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored selected E8F31 callback twenty-seven methods");
    memory.mount(0x473d80u,encode(std::array{0x473b24u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored E8F31 construction callback selector twenty-two");
    memory.mount(0x473b24u,encode(std::array{
        0xea500u,0xea8d0u,0u,0xea970u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored selected E8F31 callback twenty-two methods");
    // CompleteE8D80 iterates exactly60 authored action entries. Reuse
    // the existing native typed catalog and preserve identical earlier
    // pointer aliases and complete sixteen-byte callback descriptors.
    {
        const auto native_actions=OriginalData::native_tu15();
        if(!native_actions->unit_action_tables)
            throw FormatError("Native TU1.5 action callback catalog unavailable");
        const auto& actions=*native_actions->unit_action_tables;
        if(actions.action_bank!=0x473d28u)
            throw FormatError("Native TU1.5 action callback catalog identity differs");
        for(std::size_t i=0;i<actions.descriptors.size();++i) {
            share_immutable_word(memory,actions.action_bank+std::uint32_t(i*4),
                actions.descriptor_addresses[i],
                "native TU1.5 authored shared action callback selector identity");
            const auto address=actions.descriptor_addresses[i];
            const auto& bytes=actions.descriptors[i];
            try {
                const auto old=memory.view(address,bytes.size());
                if(!memory.is_read_only(address,bytes.size()) || !std::ranges::equal(old,bytes))
                    throw FormatError("Native TU1.5 action callback descriptor differs");
            }catch(const std::out_of_range&) {
                memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,
                    "native TU1.5 authored shared four-slot action callback descriptor");
            }
        }
    }
    memory.mount(0x4dc04bu,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 14EAE9 player construction notification policy");
    memory.mount(0x1dde84u,encode(std::array{
        0x1ddcb1u,0x1ddcb8u,0x1ddcbfu,0x1ddcc6u,0x1ddccdu,0x1ddcd4u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DDCAA physical representation type branch identities");
    memory.mount(0x6a984u,encode(std::array{
        0x6a91du,0x6a921u,0x6a921u,0x6a92eu,0x6a940u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored simulation player construction state branch identities");
    share_immutable_word(memory,0x468c08u,0x2e4d60u,
        "native TU1.5 authored game engine player construction method identity");
    // Complete1A7A62 admits categories0..10 before selecting the actual
    // owning globals record. Preserve both authored default branches.
    memory.mount(0x1a7bbcu,encode(std::array{
        0x1a7a69u,0x1a7a91u,0x1a7a71u,0x1a7a79u,0x1a7a81u,0x1a7aafu,
        0x1a7a89u,0x1a7aafu,0x1a7a99u,0x1a7aa1u,0x1a7aa9u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored player event globals category branch identities");
    memory.mount(0x51dfa0u,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold result recording enabled and frozen bytes");
    // Whole198E70's catalog admission operands, authenticated original .data.
    // The flags are initially0 and the adjacent operand is4; later catalog
    // producers retain their own writes. No lookup result is supplied.
    memory.mount(0x48a0c4u,encode(std::array{0u,4u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 original result variant catalog admission operands");
    // Original19A430 owns the complete retained-result header/event bank:
    // REP STOSD at19A440 clears379E DWORDs from51DFB0 through52BE28.
    // Keep the existing contiguous statistic view below as its own owner;
    // the new spans supply only its loader-zeroed surrounding storage.
    memory.mount(0x51dfa4u,std::vector<std::byte>(0xf50u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold retained result header and event clocks");
    memory.mount(0x522f34u,std::vector<std::byte>(0x8ef4u),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold retained result event ring and machine records");
    // Complete original retained-stat storage, independently of its validity
    // bytes.19A430 and the stat writers own later initialization/publication;
    // these exact loader-zeroed bytes do not enable a result recording.
    memory.mount(0x51eef4u,std::array<std::byte,0x4040>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold retained player PvP and team statistics");
    memory.mount(0x461facu,encode(std::array{0xad710u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DF4D4 retained physics type query method identity");
    memory.mount(0x15b39cu,encode(std::array{
        0x15b397u,0x15b377u,0x15b383u,0x15b38fu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored player binding scalar policy branch identities");
    // Complete1A89E8 validates the signed equipment policy plus one in
    //0..19 before its original tag selection. Retain every NONE/default.
    memory.mount(0x1a8af8u,encode(std::array{
        0x1a89efu,0x1a89f6u,0x1a8a3bu,0x1a8a45u,0x1a8a4fu,
        0x1a8a59u,0x1a8a63u,0x1a8a6du,0x1a8a77u,0x1a8a81u,
        0x1a8a8bu,0x1a8a95u,0x1a8a9fu,0x1a8aa9u,0x1a8ab3u,
        0x1a8abdu,0x1a8ac7u,0x1a8ad1u,0x1a8adbu,0x1a8ae8u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored twenty equipment policy tag selection branch identities");
    memory.mount(0x1f34b4u,encode(std::array{
        0x1f346au,0x1f3476u,0x1f3482u,0x1f348eu,0x1f349au,0x1f34a6u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored phantom world shape layout branch identities");
    memory.mount(0x10ab20u,encode(std::array{
        0x10aa0du,0x10aa14u,0x10aa1bu,0x10aa22u,0x10aa29u,0x10aa30u,
        0x10aa30u,0x10aa37u,0x10aa37u,0x10aa3eu,0x10aa45u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored weapon animation state branch identities");
    share_immutable_word(memory,0x468c28u,0x25bca0u,
        "native TU1.5 authored game engine equipment admission method identity");
    share_immutable_word(memory,0x46a560u,0x3c23d70bu,
        "native TU1.5 authored C019B object motion squared comparison operand");
    // TU1.5 21b207e7... .rdata contains06008037 at46A5A0. Complete
    //1677B0 reads it at167CAB/16821D for the large-bound light/portal query.
    share_immutable_word(memory,0x46a5a0u,0x37800006u,
        "native TU1.5 authored large-bound light portal query comparison operand");
    // Its signed counterpart is read at168155/1686F7 in the same original
    // light/shadow query family (.rdata060080B7).
    share_immutable_word(memory,0x46a340u,0xb7800006u,
        "native TU1.5 authored light shadow query negative comparison operand");
    // Complete151540/151630 read these separate .data policy parameters.
    // Preserve the exact initialized bytes, loader-zero BSS fields and source
    // widths/access. No computed event, camera state or admission is supplied.
    memory.mount(0x48c78cu,encode(std::array{0x40000000u,0x3f4ccccdu,0x3f4ccccdu,0x3f800000u}),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 initialized 151540 policy scalars");
    memory.mount(0x48c79cu,std::array{std::byte{5},std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initialized 151540 policy selector");
    for(const auto address:{0x56e20cu,0x56e218u})
        memory.mount(address,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 loader-zeroed 151540 151630 policy scalars");
    for(const auto address:{0x56e214u,0x56e220u})
        memory.mount(address,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 loader-zeroed 151540 151630 policy selector");
    memory.mount(0xcf99cu,encode(std::array{
        0xcf876u,0xcf876u,0xcf8e4u,0xcf8e4u,0xcf8e4u,0xcf8fbu,0xcf8fbu,0xcf912u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored CF86F unit weapon selection branch identities");
    memory.mount(0x1715a8u,encode(std::array{
        0x17147cu,0x1714e0u,0x171570u,0x1714eeu,0x171522u,
        0x17155au,0x171565u,0x171536u,0x171548u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 171475 nine tag policy branch identities");
    share_immutable_word(memory,0x46aa04u,0xc7000000u,
        "native TU1.5 authored B94B3 position lower bound operand");
    share_immutable_word(memory,0x46a530u,0x47000000u,
        "native TU1.5 authored B94C0 position upper bound operand");
    memory.mount(0x10ac50u,encode(std::array{
        0x10ac25u,0x10abe5u,0x10abe5u,0x10abe5u,0x10abe5u,0x10ac25u,0x10abbau}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 10ABB3 weapon clock policy branch identities");
    memory.mount(0x4f2d2cu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 12EF10 resource completion callback queue head");
    memory.mount(0x4f2d58u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 12F0C0 resource cache generation counter");
    memory.mount(0x4f2d50u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 12F110 resource cache clock deadline and frame counter");
    memory.mount(0x474da1u,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 12EDF0 resource cache adjustment policy byte");
    memory.mount(0x4f2d4au,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold resource cache adjustment active flag");
    memory.mount(0x4f2d4cu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold resource cache adjustment accumulated scalar");
    // The reached12EDF0 computes actual occupancy and retains its existing
    // thresholds/step arithmetic; these are its exact authored operands.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,5>{{
        {0x46a12cu,0x3d4ccccdu},{0x46a18cu,0x3c23d70au},
        {0x46a22cu,0x3f333333u},{0x46a2bcu,0x3f733333u},{0x46a444u,0x3ca3d70au}}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored 12EDF0 resource cache adjustment operand");
    // Actual13B800/13B850 visits the two option-change callback columns of
    // the existing68-row subsystem registry. These authenticated columns
    // contain only the two callbacks below; every other slot is NONE.
    // Preserve the separate initializer-column owner and let the existing
    // source compare options, select callbacks and perform their real work.
    for(unsigned row=0;row<68;++row) {
        memory.mount(0x44aeecu+row*0x24u,encode(std::array{row==37?0x180470u:0u}),
            SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored subsystem primary option-change callback "+std::to_string(row));
        memory.mount(0x44aef0u+row*0x24u,encode(std::array{row==27?0xb8e20u:0u}),
            SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored subsystem secondary option-change callback "+std::to_string(row));
    }
    memory.mount(0x41c6ccu,encode(std::array{0x2ec9b0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 318237 retained physics owner method identity");
    memory.mount(0x41d9a8u,encode(std::array{0x317c40u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 379014 retained physics callback method identity");
    memory.mount(0x41e3c0u,encode(std::array{0x31b610u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored retained physical body disposal method identity");
    memory.mount(0x41f260u,encode(std::array{0x324870u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored retained physical shape disposal method identity");
    memory.mount(0x461970u,encode(std::array{0x1d9c80u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored retained physical owner disposal method identity");
    memory.mount(0x461fb0u,encode(std::array{0x306170u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 314542 retained physical shape method identity");
    // The retained physics-world module's canonical3180E4 table maps to
    // TU1.5 address31BC24. Store authenticated TU identities so its existing
    // canonical_branch translation keeps handling the actual control flow.
    memory.mount(0x31bc24u,encode(std::array{
        0x31bae0u,0x31ba18u,0x31ba7cu,0x31ba4au,0x31baaeu,0x31bb85u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored physical world owner operation branch identities");
    memory.mount(0x41f30cu,encode(std::array{0x324f00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3180B5 retained physical owner operation method identity");
    memory.mount(0x41f31cu,encode(std::array{0x3238b0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3180C1 retained physical owner operation method identity");
    share_immutable_word(memory,0x41f118u,0x2edbe6ffu,
        "native TU1.5 authored 31F3C8 contact constraint denominator operand");
    share_immutable_word(memory,0x46a16cu,0x3e4ccccdu,
        "native TU1.5 authored 31F3E3 contact constraint scale operand");
    share_immutable_word(memory,0x455dc4u,0x4bbebc20u,
        "native TU1.5 authored 22C660 object query distance operand");
    share_immutable_word(memory,0x4502c8u,0xbe800000u,
        "native TU1.5 authored 1FB1ED unit presentation displacement operand");
    share_immutable_word(memory,0x46a71cu,0xbccccccdu,
        "native TU1.5 authored 1FA935 unit contact comparison operand");
    share_immutable_word(memory,0x46a510u,0x3f5db3d7u,
        "native TU1.5 authored 1FA39C unit contact direction comparison operand");
    share_immutable_word(memory,0x456690u,0x3c010204u,
        "native TU1.5 authored E7243 unit signed-byte normalization operand");
    share_immutable_word(memory,0x46a19cu,0x40100000u,
        "native TU1.5 authored E75F4 unit velocity comparison operand");
    share_immutable_word(memory,0x46a2dcu,0x42fe0000u,
        "native TU1.5 authored E76BE unit signed-byte encoding operand");
    share_immutable_word(memory,0x46a25cu,0x35800000u,
        "native TU1.5 authored 30EC81 contact interval scale operand");
    share_immutable_word(memory,0x41d018u,0xbdcccccdu,
        "native TU1.5 authored 303810 physical contact comparison operand");
    share_immutable_word(memory,0x46a550u,0x3f490fdbu,
        "native TU1.5 authored CAD52 object angle comparison operand");
    // This literal differs by one ULP from the separate41F658 solver
    // operand; preserve the original309CED comparison threshold exactly.
    share_immutable_word(memory,0x46a454u,0x358637beu,
        "native TU1.5 authored 309CED contact comparison operand");
    share_immutable_word(memory,0x46a794u,0x3f3504f3u,
        "native TU1.5 authored 1F59C7 scalar comparison operand");
    share_immutable_word(memory,0x4500b4u,0x41800000u,
        "native TU1.5 authored 2296A2 spatial comparison operand");
    // CompleteEBF20 walks exactly four12-byte melee candidate records
    // (EBF4A..EBF51). Keep their original action, channel and availability
    // identities; the live selector still owns availability and shared RNG.
    // English TU1.5 reference21b207e7..., .rdata44A8DC..44A90B.
    share_immutable_words(memory,0x44a8dcu,std::array{
        0x0e000607u,0x21u,0x0e000607u,
        0x0e000608u,0x22u,0x0e000608u,
        0x0e000609u,0x23u,0x0e000609u,
        0x0e00060au,0x24u,0x0e00060au},
        "native TU1.5 authored complete melee candidate table");
    // Complete111080 searches exactly13 authored emotion StringIds.
    // Preserve the original namespace; MCC names require semantic conversion
    // before this genuine lookup, rather than modifying the retail table.
    memory.mount(0x44a9a4u,encode(std::array{
        0x06000084u,0x070001a4u,0x050001a5u,0x0b0001a6u,0x080001a7u,
        0x0c0001a8u,0x070001a9u,0x060001aau,0x080001abu,0x070001acu,
        0x050001adu,0x070001aeu,0x040001afu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 111080 emotion-name identities");
    // The reached111700 invocation selects index3. Its consecutive
    //11174F/11175A/111765 loads consume these exact three authored fields.
    share_immutable_word(memory,0x44aa0cu,0x0c000043u,
        "native TU1.5 authored 11174F unit animation kind-three string identity");
    share_immutable_word(memory,0x44aa2cu,0x06000086u,
        "native TU1.5 authored 11175A unit animation kind-three string identity");
    share_immutable_word(memory,0x44a9ecu,0x00000002u,
        "native TU1.5 authored 111765 unit animation kind-three request operand");
    // The later reached kind5 consumes the same three arrays. Its genuine
    // canonical11174F/11175A/111765 instructions are TU1.5 primary
    //11266F/11267A/112685; preserve only this selected authored tuple.
    share_immutable_word(memory,0x44aa14u,0x0d000042u,
        "native TU1.5 authored 11266F unit animation kind-five string identity");
    share_immutable_word(memory,0x44aa34u,0x06000086u,
        "native TU1.5 authored 11267A unit animation kind-five string identity");
    share_immutable_word(memory,0x44a9f4u,0xffffffffu,
        "native TU1.5 authored 112685 unit animation kind-five request operand");
    // The complete reaction-name table owns this writable .data word.
    // Original111786 reads selector2; preserve that table's protection and
    // lifetime rather than declaring its identical bytes immutable.
    if(memory.is_read_only(0x47c1f8u,4) || Bytes(memory.view(0x47c1f8u,4)).u32(0)!=0x0600008du)
        throw FormatError("Native TU1.5 reaction table lost its writable request-two name");
    // The reached1D3EB2 fallback starts at state0 and follows0->2->1->0.
    // Retain only these three authenticated ushort entries, not adjacent
    // animation request identities or unobserved fallback-state rows.
    constexpr std::array<std::uint8_t,6> animation_fallback_zero{2,0,0,0,1,0};
    memory.mount(0x47c1d8u,std::as_bytes(std::span(animation_fallback_zero)),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored reached 1D3EB2 animation fallback zero chain");
    // The same reached1D3E00 request follows this separate four-state
    // fallback3->2->0->1->3 at1D3ED3; the next word is a float operand.
    memory.mount(0x45012cu,encode(std::array{0x00030001u,0x00020000u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1D3ED3 four-state animation fallback permutation");
    // Original102E48..102E4F decrements the signed magazine mode and
    // bounds its dispatch index to0..5. These six authored targets include
    // the chamber phases reached after actual shotgun shell consumption.
    share_immutable_words(memory,0x102f98u,std::array{
        0x102e5cu,0x102e5cu,0x102e5cu,0x102f87u,0x102f61u,0x102f77u},
        "native TU1.5 authored 102E55 six magazine transition branch identities");
    // Actual10F102 bounds the placement selector to0..3; padding follows
    // these four branch identities at10F158 and is intentionally omitted.
    memory.mount(0x10f148u,encode(std::array{0x10f10eu,0x10f12bu,0x10f133u,0x10f138u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 10F107 placement selector branch identities");
    // CompleteC0E90 retires four authored attachment classes; C0EE2/5
    // bounds the selector to0..3 before reading this original branch row.
    memory.mount(0xc0f4cu,encode(std::array{0xc0eeeu,0xc0ef8u,0xc0f05u,0xc0f0cu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored C0EE7 object attachment retirement branch identities");
    // Actual30AA70 decodes byte opcodes0..6B (30AA84), then selects one
    // of32 original branches. These are data identities, not code bytes.
    // The complete AI dispatch metadata now reaches these same physical
    // opcode tables. Verify and borrow its exact immutable regions.
    share_immutable_words(memory,0x30b738u,std::array{
        0x01010100u,0x04030201u,0x0706051fu,0x1f1f1f1fu,0x09080808u,
        0x0d0c0b0au,0x11100f0eu,0x1f1f1f12u,0x14131313u,0x15151414u,
        0x16161615u,0x1f1f1f1fu,0x17171717u,0x17171717u,0x17171717u,
        0x17171717u,0x17171717u,0x17171717u,0x17171717u,0x17171717u,
        0x1b1a1918u,0x1f1f1f1fu,0x1f1f1f1fu,0x1f1f1f1fu,0x1c1c1c1cu,
        0x1d1d1d1du,0x1e1e1e1eu},
        "native TU1.5 authored 30AAA0 physical opcode selector bytes");
    share_immutable_words(memory,0x30b6b8u,std::array{
        0x30b6afu,0x30b36au,0x30b328u,0x30b335u,0x30b34bu,0x30b437u,
        0x30b477u,0x30b4c0u,0x30ad9bu,0x30aaaeu,0x30aae3u,0x30ab22u,
        0x30ab56u,0x30ab94u,0x30abc8u,0x30ac06u,0x30ac42u,0x30ac84u,
        0x30acc6u,0x30ad0cu,0x30ad4au,0x30b1dbu,0x30b17du,0x30b5f2u,
        0x30b5a3u,0x30b5a9u,0x30b5b8u,0x30b5d0u,0x30b51bu,0x30b52bu,
        0x30b544u,0x30ae16u},
        "native TU1.5 authored 30AAA7 physical opcode branch identities");
    share_immutable_word(memory,0x46a17cu,0x3d088889u,
        "native TU1.5 authored 1CF1ED animation frame-to-time operand");
    share_immutable_word(memory,0x46aafcu,0xbfb33333u,
        "native TU1.5 authored FB3B8 animation response scalar operand");
    share_immutable_word(memory,0x41f064u,0x7f7fffffu,
        "native TU1.5 authored 320510 scalar reduction initial operand");
    share_immutable_word(memory,0x44a8a0u,0x3e19999au,
        "native TU1.5 authored 1F5BCA scalar multiplication operand");
    share_immutable_word(memory,0x46aa2cu,0xbd4ccccdu,
        "native TU1.5 authored 1F509D scalar operand");
    // Complete 21F590 uses these authored 3/3/5 sound-duration operands.
    // Its existing native caller must still admit the original index domains;
    // extended MCC formats require the separately authenticated timing owner.
    share_immutable_word(memory,0x46a540u,0x3a176b9au,
        "native TU1.5 authored 21F590 sound pitch scale operand");
    share_immutable_word(memory,0x46a1dcu,0x447a0000u,
        "native TU1.5 authored 21F590 milliseconds-per-second operand");
    memory.mount(0x455aacu,encode(std::array{0x5622u,0xac44u,0x7d00u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored three sound duration sample-rate divisors");
    memory.mount(0x455ab8u,encode(std::array{0x3f800000u,0x3f000000u,0x3f800000u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored three sound duration channel scales");
    memory.mount(0x455ac4u,encode(std::array{
        0x3f000000u,0x3fe38e39u,0x3f000000u,0x3f000000u,0x3f800000u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored five sound duration encoding scales");
    share_immutable_word(memory,0x473980u,0x44a648u,
        "native TU1.5 authored C63BC scene fatal-cleanup input binding");
    // The actual damage-effect owner copies precisely three dwords from
    // this authored absent player/unit/designator record (1782AD..1782BD).
    memory.mount(0x44a648u,encode(std::array{0xffffffffu,0xffffffffu,0x0000ffffu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored default damage attribution owner");
    // C13B0 walks five 16-byte recovery records and a null callback sentinel.
    // Each row contains reason flags, a condition byte with authored padding,
    // an optional preparation callback and the actual recovery callback.
    // The original heap thresholds, callback bodies and result state stay live.
    memory.mount(0x44a658u,encode(std::array<std::uint32_t,24>{
        0x8u,0x0u,0x0u,0xc1390u,
        0xfu,0x0u,0xc1270u,0xc12f0u,
        0x8u,0x0u,0x0u,0xc1390u,
        0x8u,0x1u,0x0u,0x1d0810u,
        0x8u,0x1u,0x1d09b0u,0x1d0b70u,
        0x0u,0x0u,0x0u,0x0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored bounded resource recovery callback records");
    // C3300 passes these five authored class tables to complete C2A80.
    // Preserve the original operands; the live light owner computes all fades.
    memory.mount(0x44a6d8u,encode(std::array<std::uint32_t,25>{
        0x41a00000u,0x41700000u,0x41200000u,0x40a00000u,0u,
        0x41700000u,0x41200000u,0x40e00000u,0x40400000u,0u,
        0x40200000u,0x40000000u,0x3fc00000u,0x3f800000u,0x3f000000u,
        0x40200000u,0x40000000u,0x3fc00000u,0x3f800000u,0x3f000000u,
        0u,0u,0u,0u,0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored five effect-light class fade tables");
    share_immutable_word(memory,0x46a520u,0x7149f2cau,
        "native TU1.5 authored 24DCF3 object update comparison operand");
    share_immutable_word(memory,0x46a1bcu,0x37800000u,
        "native TU1.5 authored 30B8DB physical update scale operand");
    share_immutable_word(memory,0x46a38cu,0x39800000u,
        "native TU1.5 authored 16CD50 physical update comparison operand");
    share_immutable_word(memory,0x46a388u,0x3d000000u,
        "native TU1.5 authored 16CE07 physical update interval operand");
    share_immutable_word(memory,0x46ab04u,0x3f350481u,
        "native TU1.5 authored FAFBE object update comparison operand");
    share_immutable_word(memory,0x46a590u,0x42124925u,
        "native TU1.5 authored 17F7AD selected response phase scale operand");
    share_immutable_word(memory,0x46a610u,0x322bcc76u,
        "native TU1.5 authored C38F4 object direction comparison operand");
    share_immutable_word(memory,0x46a860u,0x3ccaa525u,
        "native TU1.5 authored 243EAB signed-byte angle scale operand");
    share_immutable_word(memory,0x46aa3cu,0x3c4aa525u,
        "native TU1.5 authored 243EC2 signed-byte angle scale operand");
    share_immutable_word(memory,0x46a4d4u,0x3b800000u,
        "native TU1.5 authored 181109 placement coordinate offset operand");
    // Complete181DA0 uses these exact comparison, absent-coordinate and
    // RNG-to-angle operands; retain its original x87/SSE instruction path.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
        {0x46a30cu,0xb8d1b717u},{0x46a0dcu,0xbf800000u},
        {0x46a0c8u,0x4f800000u},{0x44923cu,0x40c90fdbu}}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored 181DA0 placement comparison and angle operand");
    // Actual16C852 increments the loader-zeroed update generation and
    //16C858/16CC70 bracket its active flag. Source instructions own both.
    memory.mount(0x4f44ecu,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 16C852 object update generation counter");
    memory.mount(0x4f44e8u,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 16C858 object update active flag");
    // Actual24DF34/24DF40 index uint32 generation stamps using the same
    // genuine4F44EC query generation. The next independent player-census
    // owner begins4F54F0, bounding this1024-slot loader-zeroed table.
    memory.mount(0x4f44f0u,std::array<std::byte,1024*4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold indexed collision query generation stamps");
    // Actual1E7690 bounds its index to0..14 before reading/writing this
    // complete15-byte runtime array;2B5FC9 queries the same bounded owner.
    // It is loader-zeroed .data tail storage.102A54 uses member4 at5020B8.
    memory.mount(0x5020b4u,std::array<std::byte,15>{},SourceRuntimeMemory::Access::ReadWrite,
        type_runtime_owner);
    // Original1E7F18 reads these four authored .data bytes;1E7F1E
    // publishes them into actor+418. Preserve the source operand and its
    // writable section ownership; the actor constructor still makes its store.
    memory.mount(0x47d6c0u,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 1E7F18 actor initialization operand");
    // Original1EB3DE/1EB7B3 publish the active actor datum;1EB853/1EB885
    // restore it and1EB891 clears it. Retain the authored absent cold value;
    // the actual frame instructions still produce every live context value.
    memory.mount(0x47c240u,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored active AI actor context");
    // Original1AFB75/1AFB85 and1B08BB/1B08EC load, increment and publish
    // these two behavior-evaluation generations. Keep both authored -1
    // inputs; the actual AI calls still produce their live generation values.
    memory.mount(0x47b908u,encode(std::array{0xffffffffu,0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored AI behavior evaluation generations");
    // Original270350 clears actor+328, then retains the maximum result of
    // 270E90 only for contacts with signed state+24>=1. That path returns
    // 0..12;2700A1 reads the corresponding word. Mount precisely that
    // reachable domain, preserving the authored readonly contact priorities.
    {
        constexpr std::array<std::uint16_t,13> priorities{1,1,2,3,4,6,7,8,8,9,9,9,9};
        std::array<std::byte,26> bytes{};
        for(unsigned i=0;i<priorities.size();++i)for(unsigned j=0;j<2;++j)
            bytes[i*2+j]=std::byte(priorities[i]>>(j*8));
        memory.mount(0x4568c4u,bytes,SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored bounded AI contact priority mapping");
    }
    // Original272D50/20B150 initialize target+26 to0;273290 keeps the
    // maximum target state.274FA0's four-state callback loop and its
    // 275340/275370/2753C0/275410 callbacks preserve0..3.2700D1 reads
    // these four original priorities; retain their authored .data access.
    memory.mount(0x47d6c4u,encode(std::array{0u,0x00030002u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored bounded AI target-state priorities");
    // Original271B01 divides this authored half-life exponent by the
    // selected update interval before its original x87 exponential sequence.
    share_immutable_word(memory,0x46a7dcu,0xbf317218u,
        "native TU1.5 authored 271B01 AI half-life exponent operand");
    memory.mount(0x47a2a8u,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 18150A placement enabled policy byte");
    // The authored absent selection is overwritten by actual1816A0 before
    // its placement continuation; preserve the original cold sentinel.
    memory.mount(0x47a2acu,encode(std::array{0xffffffffu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 1816A0 initial absent placement selection");
    // Actual181660/181676/18168C publish the three computed coordinates;
    // keep only their genuine loader-zeroed contiguous vector destination.
    memory.mount(0x51df80u,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 181660 placement position vector destination");
    // Actual180D30..180D3E alternates two96-byte polygon buffers;
    //180E5B passes capacity12 Vec2 vertices to complete2439E0, whose
    //243C26/243C2C instructions compute each output pair themselves.
    memory.mount(0x56e8e8u,std::array<std::byte,2*12*8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold 243C26 alternating polygon clipping destinations");
    // The reached180C10/182BB0 selector is row0.180C82/180EF7/1812AD
    // read its three float fields;182C62 reads the final enabled byte.
    // Both selectors explicitly use this sixteen-byte row stride.
    memory.mount(0x44eb30u,encode(std::array{0x42200000u,0x42dc0000u,0x3fc00000u,1u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 180C10 placement selector-zero parameters");
    // Actual180C77/180C82 selects row1 in the MCC Cyclotron world. The
    // complete sixteen-byte row is original TU1.5 .rdata, not a supplied
    // placement result (offline audit: mcc-physics-frontier-v768).
    memory.mount(0x44eb40u,encode(std::array{0x42200000u,0x42dc0000u,0x3fc00000u,1u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 180C10 placement selector-one parameters");
    // Actual180C77 selects row3 during the first Cairo outer step;
    // the same four-field stride is consumed by180C10/182BB0.
    memory.mount(0x44eb60u,encode(std::array{0x41200000u,0x41200000u,0x3fc00000u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 180C10 placement selector-three parameters");
    memory.mount(0x44eb50u,encode(std::array{0x42200000u,0x42dc0000u,0x3fc00000u,1u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 180C10 placement selector-two parameters");
    // Complete185020 bounds the incremented physical type to0..23,
    // then uses these authored selector bytes and two branch identities.
    memory.mount(0x18504cu,encode(std::array{
        0x00010100u,0x00000000u,0x01010101u,0x01010101u,0x01010101u,0x00010101u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 185020 physical type selector bytes");
    memory.mount(0x185044u,encode(std::array{0x18503cu,0x18503eu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 185020 physical type branch identities");
    // Actual313BEC/313BEF bound the constraint selector to0..4.
    memory.mount(0x3146f8u,encode(std::array{
        0x313db2u,0x313d69u,0x313d14u,0x313c89u,0x313bfcu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 313BF5 constraint selector branch identities");
    // Canonical30A751/30A75B bound the packed contact command minus9
    // to0..24. Its TU-selected25 bytes choose eleven original branches.
    constexpr std::array<std::uint8_t,25> packed_contact_command_kinds{
        0,1,2,3,10,10,10,10,4,5,6,10,10,10,10,10,7,8,10,10,10,10,10,10,9};
    memory.mount(0x30e908u,std::as_bytes(std::span(packed_contact_command_kinds)),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored canonical 30A761 contact command selector bytes");
    memory.mount(0x30e8dcu,encode(std::array{
        0x30e89bu,0x30e6bfu,0x30e3a7u,0x30e362u,0x30e7c7u,0x30e2afu,
        0x30e25fu,0x30e504u,0x30e27du,0x30e4d3u,0x30e8b5u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored canonical 30A768 contact command branch identities");
    // Canonical22C704 selects the four authored sound-distance modes;
    // the next original entry begins immediately after these four words.
    memory.mount(0x231038u,encode(std::array{
        0x23100bu,0x23100bu,0x23100bu,0x23100bu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored canonical 22C704 sound-distance mode branch identities");
    share_immutable_word(memory,0x46a338u,0x43ffc000u,
        "native TU1.5 authored 18BC27 packed vector component scale operand");
    share_immutable_word(memory,0x46a33cu,0x447fe000u,
        "native TU1.5 authored 18BC3B packed vector component scale operand");
    // The complete18B160 environment selector owns these five authored
    // return operands; retain exact bits rather than host-derived values.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,5>{{
        {0x46a110u,0x41200000u},{0x46a1a8u,0x42200000u},{0x46a108u,0x41700000u},
        {0x46a1a4u,0x42480000u},{0x46a118u,0x41a00000u}}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored 18B160 environment selector scalar operand");
    // The reached325BB0 contact solver owns these three scalar reads at
    //325D3C,325DBE and3266B7; retain each exact authored bit pattern.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{
        {0x41f64cu,0x34000000u},{0x41f654u,0xbd4cccd0u},{0x41f658u,0x358637bdu}}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored 325BB0 contact solver scalar operand");
    share_immutable_word(memory,0x41bf00u,0x49742400u,
        "native TU1.5 authored 3238BA physical owner scalar offset operand");
    memory.mount(0x41f2fcu,encode(std::array{0x324bc0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3180CD retained physical owner operation method identity");
    memory.mount(0x41f300u,encode(std::array{0x324d90u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 324BD0 retained physical scalar method identity");
    memory.mount(0x41f330u,encode(std::array{0x3239f0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3181D8 retained physical owner state method identity");
    memory.mount(0x461eb0u,encode(std::array{0x2e9600u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 306191 retained physical shape method identity");
    memory.mount(0x461ff8u,encode(std::array{0x307e30u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 302075 retained physical shape method identity");
    memory.mount(0x461ff4u,encode(std::array{0xad390u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DF4D4 retained physical shape type query method identity");
    memory.mount(0x41f334u,encode(std::array{0x323a00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 318203 retained physical owner state method identity");
    memory.mount(0x41f310u,encode(std::array{0x324f50u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DF33C retained physical owner state method identity");
    memory.mount(0x41f2f8u,encode(std::array{0x31dd70u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored retained physical owner type query method identity");
    memory.mount(0x41f2f4u,encode(std::array{0x323420u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D292 retained physical owner query method identity");
    memory.mount(0x41f2e8u,encode(std::array{0x3252d0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D069 retained physical owner update method identity");
    memory.mount(0x41f2e4u,encode(std::array{0x325170u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D071 retained physical owner integration method identity");
    memory.mount(0x41f1d0u,encode(std::array{0xffffffffu,0xffffffffu,0xffffffffu,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3216AA physical integration xyz SIMD mask");
    // Actual309379 reads all four lanes in the physical contact solver.
    memory.mount(0x41c500u,encode(std::array{0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored physical contact four-lane unit operand");
    memory.mount(0x41c3e8u,encode(std::array{0x2e5e80u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 314542 retained physical shape method identity");
    memory.mount(0x41c408u,encode(std::array{0x2e5da0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 30557A retained physical shape contact geometry method identity");
    memory.mount(0x41c400u,encode(std::array{0x2e5db0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3055BE retained physical shape contact geometry method identity");
    memory.mount(0x41c3e4u,encode(std::array{0xa9520u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D51B retained physical shape query method identity");
    memory.mount(0x46156cu,encode(std::array{0xadb50u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D524 retained physical owner type query method identity");
    memory.mount(0x466c60u,encode(std::array{0x281530u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D5A2 retained physical owner filter method identity");
    memory.mount(0x466c7cu,encode(std::array{0x281f10u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 27DA9F retained physical owner filter method identity");
    memory.mount(0x466c6cu,encode(std::array{0x281e10u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31C3E1 retained physical owner update listener method identity");
    memory.mount(0x466c4cu,encode(std::array{0x2e4d60u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31C3E1 retained physical owner update callback method identity");
    memory.mount(0x41eb6cu,encode(std::array{0x31e7f0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1D5174 retained physical shape disposal method identity");
    memory.mount(0x41c6d4u,encode(std::array{0x2ea380u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31DD49 retained physical shape collision query method identity");
    memory.mount(0x41d02cu,encode(std::array{0x304350u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31DFCE retained physical owner collision method identity");
    memory.mount(0x46217cu,encode(std::array{0x1f6d10u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2EFD5E retained physical contact query method identity");
    memory.mount(0x462144u,encode(std::array{0x1f7c10u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2F01D8 retained physical contact shape query method identity");
    memory.mount(0x461594u,encode(std::array{0x1cbe70u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2F0202 retained physical contact filter method identity");
    memory.mount(0x461550u,encode(std::array{0x2e7e10u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2E22A5 retained nested physical shape geometry method identity");
    memory.mount(0x461548u,encode(std::array{0x2e7d90u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2E22C9 retained nested physical shape geometry method identity");
    memory.mount(0x4620e4u,encode(std::array{0xab4b0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 2EF6A0 retained physical contact shape type method identity");
    memory.mount(0x462108u,encode(std::array{0x2e7e10u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 305589 retained physical contact shape geometry method identity");
    memory.mount(0x41f130u,encode(std::array{0x3230c0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31E11D retained physical contact notification method identity");
    memory.mount(0x41c35cu,encode(std::array{0x2e4d60u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31EA00 retained physical contact callback method identity");
    memory.mount(0x41d960u,encode(std::array{0x2e4d60u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31EA00 retained physical contact listener method identity");
    memory.mount(0x41f38cu,encode(std::array{0x324f00u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3180B5 retained physical owner operation method identity");
    memory.mount(0x41f39cu,encode(std::array{0x3238b0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3180C1 retained physical owner operation method identity");
    memory.mount(0x41f37cu,encode(std::array{0x324bc0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 3180CD retained physical owner operation method identity");
    memory.mount(0x41f380u,encode(std::array{0x324d90u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 324BD0 retained physical owner scalar setter method identity");
    memory.mount(0x41f078u,encode(std::array{0x320ef0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 378FEA retained physical owner notification method identity");
    memory.mount(0x41f120u,encode(std::array{0x3232e0u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 31D502 retained physical owner notification method identity");
    memory.mount(0x1864a8u,encode(std::array{
        0x185f09u,0x185fa2u,0x1860b0u,0x186187u,0x18626bu,0x186383u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 185F02 physical shape query branch identities");
    std::array<std::byte,21> shape_query_selectors;
    shape_query_selectors.fill(std::byte{6});
    for(unsigned i=0;i<5;++i)shape_query_selectors[i]=std::byte(i);
    shape_query_selectors[20]=std::byte{5};
    memory.mount(0x1864c4u,shape_query_selectors,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 185EFB physical shape kind query selectors");
    memory.mount(0x46e44cu,encode(std::array{0x3d4ccccdu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored physical shape construction scalar default");
    share_immutable_word(memory,0x46a100u,0x40a00000u,
        "native TU1.5 authored 2E2577 physical shape construction subtraction operand");
    share_immutable_word(memory,0x46a130u,0x3e800000u,
        "native TU1.5 authored 1DF686 physical shape construction scale operand");
    share_immutable_word(memory,0x46a160u,0x37800080u,
        "native TU1.5 authored 1709D7 random high-word normalization operand");
    memory.mount(0x46e468u,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 2EF634 physical world update policy");
    memory.mount(0x46e4f4u,encode(std::array{0x173fu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 32B1A6 CRT x87 wrapper control policy word");
    memory.mount(0x46e4f0u,encode(std::array{0x1b3fu}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 32B0D3 CRT x87 wrapper control policy word");
    memory.mount(0x48b880u,encode(std::array{0x3c23d70au}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored canonical 1F16F3 physical contact query tolerance");
    share_immutable_word(memory,0x468c54u,0x25bc70u,
        "native TU1.5 authored 160A7B game engine combat policy method identity");
    memory.mount(0x48b85cu,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 1C7697 physical world update delay policy");
    memory.mount(0x56dfd4u,encode(std::array{0u,0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 27F0F0 physical update object and clock context");
    memory.mount(0x52d888u,std::array{std::byte{},std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 1C776B physical update generation counter");
    memory.mount(0x56e1dcu,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 11444F unit update generation counter");
    memory.mount(0x5564d8u,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 110411 unit and model presentation policy scalar");
    // The complete282BC0 owner publishes its delivered object then the
    // genuine149B90 clock result into these two distinct cold fields.
    memory.mount(0x56dfdcu,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 282BC4 contact update object binding");
    memory.mount(0x56dfd0u,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 282BCE contact update tick scalar");
    // Original322D60 validates the authored SDK registration inputs and
    // publishes this lazy guard itself; retain its loader-zeroed state.
    memory.mount(0x48d480u,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 322EDC contact solver lazy registration guard");
    // Original322D60 compares322AD0's real hash of this authored SDK
    // owner name against450108, then publishes its own lazy guard.
    share_immutable_word(memory,0x450108u,0x880fff49u,
        "native TU1.5 authored 322D60 SDK registration owner hash operand");
    share_immutable_word(memory,0x47c1b0u,0x461930u,
        "native TU1.5 authored 322D69 SDK registration owner-name binding");
    constexpr std::array<char,7> sdk_registration_owner{"Bungie"};
    memory.mount(0x461930u,std::as_bytes(std::span(sdk_registration_owner)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored SDK registration owner name");
    constexpr std::array<char,19> sdk_registration_features{"rigidbodysoftcloth"};
    memory.mount(0x41f4f8u,std::as_bytes(std::span(sdk_registration_features)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored 322CA2 SDK registration feature names");
    // Actual1CFD6D/1CFD72 reset these two AI selections toNONE after the
    // source datum pools are cleared. Cold composition preserves zeros.
    memory.mount(0x50ef00u,encode(std::array{0u,0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed AI reset selection destinations");
    // Original_cinit callback46DC64 selects382230, which performs FLD,
    // FCOS and FSTP. Its result must be computed by that genuine owner.
    memory.mount(0x46aba0u,encode(std::array{0u,0x40468000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 382230 spatial threshold qword operand");
    memory.mount(0x56b170u,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 382230 spatial threshold destination");
    memory.mount(0x55d728u,std::array{std::byte{},std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 27DF0E contact query ushort scalar");
    // Actual18F9F4 selects this immutable default object reference when
    // a looping request uses an attached/spatial location.18F9F9 copies its
    // literal zero word; it supplies no listener or audio readiness.
    share_immutable_word(memory,0x44ebd8u,0u,
        "native TU1.5 authored spatial-loop default reference operand");
    // Actual18F900 clears ceil(active BSP clusters/32) DWORDs here before
    // listener-distance publication;18EE50 then tests this independent sound
    // visibility vector. Retain the original loader-zero64-byte prefix for
    // the source's512-cluster domain. These are backing bytes, not an active
    // listener, visible-cluster mask or successful sound-system result.
    memory.mount(0x55d730u,std::array<std::byte,64>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed sound cluster visibility backing");
    // Actual CRT row46D978 selects380E70. Its26 absolute stores bound this
    // fallback effect data;1024C0 returns its base at102545.
    // Keep loader bytes cold. The complete constructor produces the tags,
    // sentinels and scalar values before FFA60 or other consumers read them.
    memory.mount(0x5564f0u,std::array<std::byte,0x58>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 380E70 fallback effect record destinations");
    memory.mount(0x55d700u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed original 3817F0 ratio threshold destination");
    share_immutable_word(memory,0x46a4b0u,0x3d800000u,
        "native TU1.5 authored original 3817F0 ratio threshold operand");
    memory.mount(0x502088u,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 1C62FE campaign physical shape publication binding");
    memory.mount(0x48cbb8u,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 2EFD61 optional contact query policy");
    memory.mount(0x4f2d4bu,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 12F844 optional resource cache policy");
    memory.mount(0x4f2d38u,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 12F9A8 optional resource cache request policy");
    memory.mount(0x4f2d40u,encode(std::array{0u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 12F9D5 resource cache request counter");
    memory.mount(0x51daf8u,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed 111B95 unit animation update policy");
    // E349D/E366E read this separate one-byte .data virtual-tail cell.
    // Pinned TU1.5 21b207e7... establishes cold loader zero; retain writable
    // world/checkpoint ownership without importing any reference asset.
    memory.mount(0x51daecu,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed E366E object update policy byte");
    // E3440's actual E35D5 consumer reads one writable .data virtual-tail
    // byte. Pinned TU1.5 21b207e7... proves loader zero, not a supplied asset
    // or a completed update result. Keep its own exact checkpointed region.
    memory.mount(0x51daedu,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed E35D5 object update policy byte");
    // F8850 tests this original loader-zero byte before its optional vehicle
    // query;2B5F2B is its sole absolute writer. Preserve the writable policy
    // cell rather than suppressing the original caller (offline audit v871).
    memory.mount(0x51daeeu,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed F8850 optional vehicle query policy");
    // Complete47860 retains its real allocation-failure path.47907/47910
    // read/set only this one .data BSS byte, beyond its raw file extent.
    memory.mount(0x56e198u,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed receiver request allocation warning");
    //282170 lazily initializes the conveyor callback's16-byte vector and
    // its adjacent4-byte guard. All20 bytes are writable loader-zero .data
    // BSS (offline original audit v1395). The original initializer owns the
    // guard transition and vector stores; mount no computed contact result.
    memory.mount(0x56e6c0u,std::array<std::byte,20>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed conveyor callback vector and guard");
    // Actual class3 birth reaches F4D1A's lazy initialization. F4D5C sets
    // the flag and F4D78 computes/stores the adjacent scalar from the real
    // tick-rate owner. Both words are loader-zero .data BSS (v1162); retain
    // their mutable storage without supplying that computed result.
    memory.mount(0x56ecccu,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 loader-zeroed F4D1A scalar and initialization flags");
    share_immutable_word(memory,0x46a178u,0x3dcccccdu,
        "native TU1.5 authored F4D70 lazy tick scaling operand");
    // Original101877 selects45EFB4 and1018A9 reads the vector from ESI-8.
    // Exact readonly TU1.5 default direction is(0,0,-1), independent of the
    // adjacent original strings (offline actual prop frontier v916).
    memory.mount(0x45efacu,encode(std::array{0u,0u,0xbf800000u}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored object effect default direction");
    // Original16095F bounds the actual damage-response selector to0..3;
    //160961 reads this exact in-text branch table (offline audit v928).
    memory.mount(0x160a4cu,encode(std::array{0x16098au,0x160968u,0x160972u,0x16097cu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 damage-response branch identities");
    // OriginalDBF3C bounds the actual selector to0..12, then DBF45/DBF4C
    // read these13 byte selectors and six exact targets (offline v952).
    memory.mount(0xdc084u,encode(std::array{0xdbfadu,0xdbf53u,0xdbf98u,0xdbf89u,0xdbfbeu,0xdbfccu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 object damage selector branch identities");
    memory.mount(0xdc09cu,std::array{std::byte{0},std::byte{1},std::byte{2},std::byte{2},std::byte{2},
        std::byte{3},std::byte{5},std::byte{1},std::byte{5},std::byte{5},std::byte{5},std::byte{1},std::byte{4}},
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 object damage bounded byte selectors");
    // Complete SDK407AD8 reads hardware-format14's actual8-bit entry for
    // DXT3. Existing renderer startup owns formats6/7/12/15; admit only the
    // newly reached literal byte, without replacing their owners (v966).
    memory.mount(0x40db3eu,std::array{std::byte{8}},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 SDK DXT3 format information");
    // OriginalCBEEB compares its computed object-effect value with this
    // exact authored float, independently authenticated at frontier v968.
    share_immutable_word(memory,0x46ab2cu,0x400a3ae7u,
        "native TU1.5 object-effect comparison constant");
    // Full vehicle-look closure audit v981 identifies this sole missing
    // readonly scalar. Original18A724 multiplies by its exact0.08f bits.
    memory.mount(0x46a7f0u,encode(std::array{0x3da3d70au}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 vehicle-look input scalar");
    // Original1DCDD6..1DCDF7 selects18 or27 directions;1DD2DB advances
    // exactly12 bytes per row. Preserve all27 original readonly rows,
    // including their original normalized diagonal bits (v1028).
    memory.mount(0x461998u,encode(std::array{
        0x0u,0x0u,0x0u,0x3f800000u,0x0u,0x0u,0x0u,0x0u,0x3f800000u,
        0x3f3504f3u,0x0u,0x3f3504f3u,0x3f13cd3au,0x3f13cd3au,0x3f13cd3au,0x3f13cd3au,0xbf13cd3au,0x3f13cd3au,
        0x3f3504f3u,0x3f3504f3u,0x0u,0x3f3504f3u,0xbf3504f3u,0x0u,0x0u,0x3f3504f3u,0x3f3504f3u,
        0x0u,0xbf3504f3u,0x3f3504f3u,0xbf800000u,0x0u,0x0u,0x0u,0x3f800000u,0x0u,
        0x0u,0xbf800000u,0x0u,0xbf3504f3u,0xbf3504f3u,0x0u,0xbf3504f3u,0x3f3504f3u,0x0u,
        0xbf3504f3u,0x0u,0x3f3504f3u,0xbf13cd3au,0xbf13cd3au,0x3f13cd3au,0xbf13cd3au,0x3f13cd3au,0x3f13cd3au,
        0x0u,0x0u,0xbf800000u,0x3f3504f3u,0x0u,0xbf3504f3u,0xbf3504f3u,0x0u,0xbf3504f3u,
        0x0u,0x3f3504f3u,0xbf3504f3u,0x0u,0xbf3504f3u,0xbf3504f3u,0x3f13cd3au,0x3f13cd3au,0xbf13cd3au,
        0x3f13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0xbf13cd3au,0x3f13cd3au,0xbf13cd3au}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored complete physical query offset directions");
    // Original37CFDF's x87 multiplication reads this exact .data double.
    // Keep its file-initialized writable ownership, not a host math shortcut.
    memory.mount(0x473360u,encode(std::array{0u,0x3fe00000u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored CRT double half operand");
    // Actual11266F/11267A mode1 selects the original NONE sentinels;
    //112685 selects index0 through the separate44A9E0 table (v985/v1007).
    // Preserve independent existing owners while admitting the reached words
    // from the original indexed tables (v985).
    share_immutable_word(memory,0x44aa04u,0xffffffffu,
        "native TU1.5 object effect mode1 primary event");
    share_immutable_word(memory,0x44aa24u,0xffffffffu,
        "native TU1.5 object effect mode1 secondary event");
    share_immutable_word(memory,0x44a9e4u,0u,
        "native TU1.5 object effect mode1 event selector");
    // Original11704E compares its random damage-event selector against this
    // exact immutable0.6667f operand (v1040), not a host-rounded fraction.
    share_immutable_word(memory,0x44aa48u,0x3f2aacdau,
        "native TU1.5 authored damage event selection scalar");
    // That mode1 selector reads index0 at original1126A6. The literal
    // StringID0600008B is file-initialized .data, not produced event state;
    // preserve the complete reaction table's existing writable ownership.
    if(memory.is_read_only(0x47c1f0u,4) || Bytes(memory.view(0x47c1f0u,4)).u32(0)!=0x0600008bu)
        throw FormatError("Native TU1.5 reaction table lost its writable mode1 event name");
    // Actual1F1A3C indexes the original readonly short table at450210.
    // The reached index1 is exactly5 in authenticated TU1.5 (audit v1212).
    // The final startup difficulty table now owns this exact operand. Keep
    // its existing lifetime; a second overlapping mount would reject startup.
    if(!memory.is_read_only(0x450212u,2) || Bytes(memory.view(0x450212u,2)).u16(0)!=5)
        throw FormatError("Native TU1.5 difficulty table lost the authored 1F1A3C index1 operand");
    // Original13E7BD's MMX color interpolation reads this initialized .data
    // scale. Keep16384.0f's exact bits and original writable policy (v1093).
    memory.mount(0x48c770u,encode(std::array{0x46800000u}),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored packed color interpolation scale");
    // Actual151630 copies this initialized overshield player response
    // before1578A0.151540's distinct camouflage descriptor is already owned
    // above. These are exact writable .data words from authenticated TU1.5.
    memory.mount(0x48c778u,encode(std::array{
        0x40000000u,0x3eb33333u,0x3eb33333u,0x3f800000u,0x00000002u}),
        SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored overshield player response");
    memory.mount(0x474d9du,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 12F96A resource cache request policy");
    memory.mount(0x474d9cu,std::array{std::byte{}},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 12F980 resource cache request policy");
    // Complete305E40 bounds the authored contact command byte to0..107,
    // then selects one of32 branch identities. Preserve the actual TU
    // targets; the existing executor performs its canonical translation.
    memory.mount(0x309f30u,encode(std::array{
        0x00309e97u,0x00309cbdu,0x00309c7bu,0x00309c88u,
        0x00309c9eu,0x00309d59u,0x00309d7cu,0x00309da8u,
        0x00309b15u,0x0030996eu,0x0030998du,0x003099b6u,
        0x003099d4u,0x003099fcu,0x00309a1au,0x00309a3fu,
        0x00309a65u,0x00309a8eu,0x00309ab7u,0x00309b57u,
        0x00309b8eu,0x00309c03u,0x00309c2du,0x00309eefu,
        0x00309ea1u,0x00309ea7u,0x00309eb6u,0x00309eceu,
        0x00309de3u,0x00309df3u,0x00309e0cu,0x00309e60u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored contact command dispatch branch identities");
    memory.mount(0x309fb0u,encode(std::array{
        0x01010100u,0x04030201u,0x0706051fu,0x1f1f1f1fu,
        0x09080808u,0x0d0c0b0au,0x11100f0eu,0x1f1f1f12u,
        0x14131313u,0x15151414u,0x16161615u,0x1f1f1f1fu,
        0x17171717u,0x17171717u,0x17171717u,0x17171717u,
        0x17171717u,0x17171717u,0x17171717u,0x17171717u,
        0x1b1a1918u,0x1f1f1f1fu,0x1f1f1f1fu,0x1f1f1f1fu,
        0x1c1c1c1cu,0x1d1d1d1du,0x1e1e1e1eu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored108 contact command dispatch byte selectors");
    share_immutable_word(memory,0x46a588u,0xbfbf0243u,
        "native TU1.5 authored 18A108 retained presentation lower bound");
    share_immutable_word(memory,0x46a58cu,0x3fbf0243u,
        "native TU1.5 authored 18A0EC retained presentation upper bound");
    share_immutable_word(memory,0x46a180u,0x3a83126fu,
        "native TU1.5 authored B1773 vector normalization comparison tolerance");
    share_immutable_word(memory,0x46a43cu,0x447fc000u,
        "native TU1.5 authored 17F8C4 ten-bit scalar quantization scale");
    share_immutable_word(memory,0x46a148u,0x437f0000u,
        "native TU1.5 authored 10550B byte scalar quantization scale");
    share_immutable_word(memory,0x456918u,0x3e99999au,
        "native TU1.5 authored D08A4 object update scalar operand");
    share_immutable_word(memory,0x4502c4u,0xbdcccccdu,
        "native TU1.5 authored 117527 unit update comparison operand");
    share_immutable_word(memory,0x46a178u,0x3dcccccdu,
        "native TU1.5 authored 117537 unit update and 17F8F4 packing operand");
    share_immutable_word(memory,0x46a174u,0x3eaaaaabu,
        "native TU1.5 authored C8E4A object update scalar operand");
    share_immutable_word(memory,0x46a8fcu,0xbf666666u,
        "native TU1.5 authored E2A43 object direction comparison operand");
    share_immutable_word(memory,0x46a63cu,0x3f7d70a4u,
        "native TU1.5 authored E2AB2 object direction comparison operand");
    share_immutable_word(memory,0x46aa80u,0x4099999au,
        "native TU1.5 authored 1EDFDA physical unit update scalar operand");
    share_immutable_word(memory,0x4502bcu,0x3e6f9db2u,
        "native TU1.5 authored 1F9171 physical unit velocity comparison operand");
    share_immutable_word(memory,0x44a98cu,0x3d99999au,
        "native TU1.5 authored 1103FE unit update subtraction operand");
    share_immutable_word(memory,0x46a240u,0x3ecccccdu,
        "native TU1.5 authored 153C0D retained combat action addition operand");
    share_immutable_word(memory,0x46a194u,0x3f666666u,
        "native TU1.5 authored 110419 unit update scalar operand");
    share_immutable_word(memory,0x46a270u,0x43000000u,
        "native TU1.5 authored 110421 unit update scalar operand");
    share_immutable_word(memory,0x46a348u,0x461c4000u,
        "native TU1.5 authored 1D712E physical velocity squared comparison bound");
    share_immutable_word(memory,0x46a814u,0x451c4000u,
        "native TU1.5 authored 1D71A1 second physical velocity squared comparison bound");
    share_immutable_word(memory,0x46a284u,0x3e23d70au,
        "native TU1.5 authored DF680 physical unit update scalar operand");
    share_immutable_word(memory,0x46a1acu,0x3f7ae148u,
        "native TU1.5 authored 122DF6 unit pose update scalar operand");
    share_immutable_word(memory,0x46a458u,0x3e3851ecu,
        "native TU1.5 authored E65DD player command tick rate scale operand");
    share_immutable_word(memory,0x46a558u,0xbc23d70au,
        "native TU1.5 authored E0836 unit prephysics lower comparison bound");
    share_immutable_word(memory,0x41d658u,0x34000000u,
        "native TU1.5 authored canonical 30B360 physical contact comparison scale");
    share_immutable_word(memory,0x46a190u,0x3f8ccccdu,
        "native TU1.5 authored canonical 2F4E4B physical contact margin scale");
    share_immutable_word(memory,0x46a98cu,0xbc490fdbu,
        "native TU1.5 authored 18A771 retained look lower comparison bound");
    share_immutable_word(memory,0x46a988u,0x3c490fdbu,
        "native TU1.5 authored 18A77C retained look upper comparison bound");
    memory.mount(0xdf5d4u,encode(std::array{
        0xdeed5u,0xdef41u,0xdef71u,0xdf571u,0xdf571u,0xdef9au}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored DEECE six physical unit mode branch identities");
    memory.mount(0x1edde0u,encode(std::array{
        0x1eddb1u,0x1eddbfu,0x1eddcdu,0x1eddbau,0x1eddbau,0x1eddbau}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1EDDAA six physical unit query branch identities");
    memory.mount(0x1edcf4u,encode(std::array{
        0x1ed93fu,0x1ed94cu,0x1ed959u,0x1ed99eu,0x1ed99eu,0x1ed965u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1ED938 six physical unit update branch identities");
    memory.mount(0x1ede40u,encode(std::array{
        0x1ede1du,0x1ede26u,0x1ede11u,0x1ede34u,0x1ede34u,0x1ede2du}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1EDE16 six physical unit state branch identities");
    memory.mount(0xdf804u,encode(std::array{
        0xdf766u,0xdf78bu,0xdf77du,0xdf799u,0xdf799u,0xdf78bu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored DF75F six physical unit mode notification branch identities");
    memory.mount(0x1edd84u,encode(std::array{
        0x1edd3cu,0x1edd6au,0x1edd48u,0x1edd54u,0x1edd54u,0x1edd6au}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1EDD35 six physical unit result branch identities");
    memory.mount(0x153de0u,encode(std::array{
        0x153d78u,0x153d61u,0x153d33u,0x153d8fu,0x153d8fu,
        0x153d8fu,0x153d8fu,0x153d8fu,0x153d4au}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 153D2C nine combat action mode branch identities");
    share_immutable_words(memory,0x1e16ccu,std::array{0x1e1661u,0x1e1673u,0x1e1685u,0x1e1699u},
        "native TU1.5 authored 1E165A four unit response mode branch identities");
    memory.mount(0x1e5350u,encode(std::array{0x1e528bu,0x1e52a8u,0x1e52b4u,0x1e52d2u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1E5284 four unit response selection branch identities");
    memory.mount(0x13e5f4u,encode(std::array{
        0x13deabu,0x13debcu,0x13dee5u,0x13e035u,0x13e0edu,0x13e146u,
        0x13e5e2u,0x13e2a2u,0x13e5e2u,0x13df3fu,0x13e2a2u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 13DEA4 eleven retained update response branch identities");
    memory.mount(0x10464cu,encode(std::array{
        0x1037d3u,0x103d68u,0x103e32u,0x103ed4u,
        0x1040f0u,0x103f1du,0x104006u,0x103ed4u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1037CC eight unit equipment state branch identities");
    memory.mount(0x10466cu,encode(std::array{
        0x1037e7u,0x10387cu,0x103a6du,0x103d0fu,0x1038f5u,0x103994u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1037E0 six unit equipment action branch identities");
    memory.mount(0x1046a4u,encode(std::array{0x104204u,0x1042a6u,0x10434cu,0x104360u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1041FD four unit equipment action phase branch identities");
    memory.mount(0x1f77acu,encode(std::array{
        0x1f7711u,0x1f77a7u,0x1f775bu,0x1f775bu,0x1f76c1u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored canonical 1F2DCA five physical contact kind branch identities");
    memory.mount(0x30f6e0u,encode(std::array{
        0x30f253u,0x30f066u,0x30eb76u,0x30eb31u,0x30f175u,0x30ea8eu,
        0x30ea3eu,0x30ecd3u,0x30ea5cu,0x30eca2u,0x30edd0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored canonical 30AF47 physical contact command branches");
    constexpr std::array<std::uint8_t,25> contact_command_kinds{
        0,1,2,3,10,10,10,10,4,5,6,10,10,10,10,10,7,8,10,10,10,10,10,10,9};
    memory.mount(0x30f70cu,std::as_bytes(std::span(contact_command_kinds)),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored canonical 30AF40 twenty-five contact command byte selectors");
    // Canonical30389D bounds the contact result mask to0..14. Its exact
    // selector bytes choose one of these seven authenticated branch IDs.
    memory.mount(0x307988u,encode(std::array{
        0x3078ddu,0x307904u,0x30791cu,0x307925u,0x3078bfu,0x3078e6u,0x3078b0u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored contact result mask branch identities");
    constexpr std::array<std::uint8_t,15> contact_result_masks{
        0,0,1,6,2,6,6,6,3,6,6,6,6,4,5};
    memory.mount(0x3079a4u,std::as_bytes(std::span(contact_result_masks)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored contact result mask selectors");
    // Canonical322F4A bounds the physical solver command to0..16.
    memory.mount(0x3287a4u,encode(std::array{
        0x3281c0u,0x326aa0u,0x327097u,0x326ac6u,0x327a82u,0x32803eu,
        0x327d3fu,0x327632u,0x3272fbu,0x327896u,0x32798eu,0x327e80u,
        0x327b55u,0x327c21u,0x3281a6u,0x328168u,0x32818du}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored physical solver command branch identities");
    // The same complete solver owns its seven-kind phase at32468B and
    // its eight-case completion phase at324AB3. Their source comparisons
    // bound the first index to6 and the completion mask selector to16.
    memory.mount(0x3287e8u,encode(std::array{
        0x3285dau,0x3281e1u,0x3285cfu,0x328500u,0x328572u,0x328261u,0x3283e9u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored physical solver kind branch identities");
    memory.mount(0x328804u,encode(std::array{
        0x328796u,0x3285f7u,0x328667u,0x32862au,0x3286aau,0x3286edu,0x328731u,0x32878bu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored physical solver completion branch identities");
    constexpr std::array<std::uint8_t,17> solver_completion_kinds{
        0,1,2,3,4,2,2,5,6,4,4,2,4,2,7,7,7};
    memory.mount(0x328824u,std::as_bytes(std::span(solver_completion_kinds)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored physical solver completion selectors");
    // Original18B160 bounds the environment kind to0..20; its byte
    // selector chooses exactly one of the five original return branches.
    memory.mount(0x18b1a0u,encode(std::array{
        0x18b19eu,0x18b196u,0x18b17bu,0x18b184u,0x18b18du}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored environment kind branch identities");
    constexpr std::array<std::uint8_t,21> environment_kinds{
        0,0,0,0,0,0,0,0,0,0,0,1,2,3,3,4,1,1,1,1,1};
    memory.mount(0x18b1b4u,std::as_bytes(std::span(environment_kinds)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored environment kind selectors");
    // OriginalE544B bounds the locomotion choice minus1 to0..10.
    memory.mount(0xe54c0u,encode(std::array{
        0xe5457u,0xe545fu,0xe5467u,0xe546fu,0xe5477u,0xe547fu,
        0xe5487u,0xe548fu,0xe5497u,0xe549fu,0xe54a7u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored unit locomotion choice branch identities");
    // OriginalE7707 bounds the unit velocity channel kind minus2 to0..8.
    memory.mount(0xe7764u,encode(std::array{
        0xe7713u,0xe7724u,0xe775eu,0xe775eu,0xe775eu,
        0xe775eu,0xe775eu,0xe7738u,0xe774du}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored unit velocity channel branch identities");
    // Canonical309A29 masks the contact feature to7;30B09A handles7
    // before this signed classification. Nonnegative results are0..2;
    // the paired three-axis lookup rows overlap by one authored byte.
    constexpr std::array<std::uint8_t,7> contact_feature_axes{255,0,1,250,2,249,248};
    memory.mount(0x41d758u,std::as_bytes(std::span(contact_feature_axes)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored contact feature signed axis selectors");
    constexpr std::array<std::uint8_t,5> contact_axis_pairs{2,0,1,2,0};
    memory.mount(0x41d760u,std::as_bytes(std::span(contact_axis_pairs)),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored contact cyclic axis pair selectors");
    // Complete314D80 creates four0x50-byte rows with three entries per
    // row.314DD4/314DDE/314DE1 consume one authored three-offset tuple
    // per entry, bounding this table to twelve tuples.
    memory.mount(0x41d7b0u,encode(std::array{
        0x00u,0xa0u,0x20u, 0x40u,0xf0u,0x00u, 0x20u,0x50u,0x00u,
        0x00u,0x00u,0x20u, 0x20u,0xf0u,0x20u, 0x60u,0xa0u,0x00u,
        0x00u,0x50u,0x20u, 0x60u,0xf0u,0x10u, 0x40u,0x00u,0x00u,
        0x20u,0x00u,0x10u, 0x40u,0xa0u,0x10u, 0x60u,0x50u,0x10u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 314D80 contact constraint offset tuples");
    share_immutable_word(memory,0x41d7a0u,0x3727c5acu,
        "native TU1.5 authored 31483A contact-plane interval tolerance operand");
    memory.mount(0x46e480u,encode(std::array{
        0x3a83126fu,0x3a83126fu,0x3a83126fu,0x3a83126fu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 313FE4 packed constraint comparison operand");
    // Actual3143E6/3144DB pass this original zero-vector operand;
    //3123B7 consumes all four components without publishing an output.
    memory.mount(0x48ca00u,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 loader-zeroed original constraint zero-vector operand");
    // Complete3067D0 selects the authored eight box-corner vectors by
    // vertex ID. The following table starts with a distinct fourth lane1;
    // these corner-direction operands retain their original fourth lane0.
    memory.mount(0x41d2f0u,encode(std::array{
        0x3f800000u,0x3f800000u,0x3f800000u,0u,
        0xbf800000u,0x3f800000u,0x3f800000u,0u,
        0x3f800000u,0xbf800000u,0x3f800000u,0u,
        0xbf800000u,0xbf800000u,0x3f800000u,0u,
        0x3f800000u,0x3f800000u,0xbf800000u,0u,
        0xbf800000u,0x3f800000u,0xbf800000u,0u,
        0x3f800000u,0xbf800000u,0xbf800000u,0u,
        0xbf800000u,0xbf800000u,0xbf800000u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 306800 box-corner direction vectors");
    // Original112DCB bounds the animation event ID minus0A000010 to48.
    memory.mount(0x11313cu,encode(std::array{
        0x112e2eu,0x112e85u,0x112f1au,0x112f43u,0x112c1au}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored unit animation event branch identities");
    std::array<std::byte,49> animation_event_kinds;animation_event_kinds.fill(std::byte{4});
    animation_event_kinds[0]=animation_event_kinds[3]=animation_event_kinds[29]=std::byte{0};
    animation_event_kinds[13]=std::byte{1};animation_event_kinds[46]=std::byte{2};
    animation_event_kinds[48]=std::byte{3};
    memory.mount(0x113150u,animation_event_kinds,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored unit animation event selectors");
    // Original2106AE bounds the arithmetic opcode minus7 to0..5.
    memory.mount(0x210768u,encode(std::array{
        0x2106bau,0x2106c8u,0x2106d6u,0x2106e4u,0x2106f2u,0x2106fdu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored arithmetic builtin branch identities");
    memory.mount(0x1ded60u,encode(std::array{
        0x1ded17u,0x1ded04u,0x1ded3du,0x1dece2u,0u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DECDB physical owner kind branch identities");
    memory.mount(0x1ded74u,encode(std::array{0x04040100u,0x02040404u,0x03040404u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DECD4 twelve physical owner kind byte selectors");
    // The reached17F790 input selects channel2. Its original byte samples
    // repeat this exact authored256-byte quarter four times; preserve the
    // samples losslessly rather than regenerating a waveform mathematically.
    constexpr std::array<std::uint8_t,256> response_two_quarter{
        255,253,247,238,226,210,193,173,152,130,108,87,67,49,33,19,
        9,3,0,0,5,13,25,39,56,75,96,118,139,161,182,200,
        217,231,242,250,254,254,251,244,233,219,203,184,164,143,121,99,
        78,59,41,26,15,6,1,0,2,8,18,30,46,64,84,105,
        127,149,170,190,208,224,236,246,252,254,253,248,239,228,213,195,
        176,155,133,111,90,70,51,35,21,10,3,0,0,4,12,23,
        37,54,72,93,115,136,158,179,198,215,229,241,249,254,254,251,
        245,235,221,205,187,167,146,124,102,81,61,44,28,16,7,1,
        0,1,7,16,28,44,61,81,102,124,146,167,187,205,221,235,
        245,251,254,254,249,241,229,215,198,179,158,136,115,93,72,54,
        37,23,12,4,0,0,3,10,21,35,51,70,90,111,133,155,
        176,195,213,228,239,248,253,254,252,246,236,224,208,190,170,149,
        127,105,84,64,46,30,18,8,2,0,1,6,15,26,41,59,
        78,99,121,143,164,184,203,219,233,244,251,254,254,250,242,231,
        217,200,182,161,139,118,96,75,56,39,25,13,5,0,0,3,
        9,19,33,49,67,87,108,130,152,173,193,210,226,238,247,253};
    std::array<std::uint8_t,1024> response_two{};
    for(unsigned quarter=0;quarter<4;++quarter)
        std::ranges::copy(response_two_quarter,response_two.begin()+quarter*256);
    memory.mount(0x475aa8u,std::as_bytes(std::span(response_two)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 17F80B response curve two 1024 byte samples");
    // Reached17F80B selects channel10 from the authored4752A8 bank.
    // Preserve its1024 stored byte samples exactly; the actual interpolator
    // masks both adjacent sample indices to0..1023 and supplies its result.
    constexpr std::array<std::uint8_t,1024> response_ten{
        165,163,146,125,116,123,133,142,151,163,163,134,83,43,40,66,
        87,81,68,71,88,87,58,32,50,110,163,168,139,122,139,160,
        148,114,104,139,181,178,128,80,80,112,129,112,94,113,158,182,
        164,133,135,174,212,215,193,183,198,214,202,170,148,150,160,153,
        130,110,109,118,121,117,113,114,118,121,127,139,153,158,154,149,
        148,142,122,98,93,110,125,112,76,47,47,60,59,43,45,82,
        129,141,108,72,79,123,157,155,140,150,181,188,144,83,64,101,
        149,158,129,106,116,133,123,91,85,130,198,233,216,180,168,181,
        190,180,171,186,215,223,191,138,101,94,105,115,123,132,141,137,
        120,102,96,107,126,149,171,184,176,146,113,100,108,117,110,97,
        95,102,94,59,21,17,54,95,103,84,78,104,135,134,106,97,
        132,180,191,155,115,116,146,156,125,88,88,124,148,129,93,90,
        130,169,170,145,137,168,207,217,198,183,194,212,206,174,144,142,
        158,166,151,128,114,109,104,96,96,111,133,148,153,150,143,130,
        119,121,141,164,167,145,117,99,90,71,46,38,64,103,114,83,
        41,33,61,91,97,97,123,170,195,165,108,82,112,160,174,150,
        130,138,151,129,76,44,72,137,178,168,138,133,154,169,158,146,
        168,219,254,243,197,160,153,160,158,147,144,153,158,141,109,84,
        81,96,115,132,146,153,149,137,130,134,141,142,138,140,147,142,
        114,76,58,69,86,81,55,42,61,89,91,68,57,85,132,151,
        130,106,120,164,186,161,121,111,138,157,135,95,85,119,153,144,
        102,78,102,146,167,158,153,179,215,221,189,157,160,193,218,211,
        187,168,161,147,119,93,92,115,139,146,136,120,107,98,100,117,
        145,170,178,170,157,141,119,92,80,97,128,140,113,68,41,42,
        49,43,38,62,114,150,136,89,64,89,136,159,150,144,166,191,
        175,116,67,75,124,159,145,112,104,124,136,116,89,102,159,213,
        221,192,169,178,198,200,182,173,188,207,200,164,126,111,117,123,
        119,113,116,122,123,118,117,123,132,138,145,154,159,152,136,125,
        130,135,121,91,67,70,86,85,61,43,54,83,92,70,48,67,
        119,158,153,124,119,145,167,150,113,107,144,183,174,120,76,80,
        110,123,107,99,129,176,190,157,118,121,165,209,220,210,208,217,
        214,180,137,120,140,170,178,160,134,113,98,84,80,94,123,151,
        165,162,150,133,115,108,123,153,174,167,138,109,93,80,57,37,
        45,82,114,104,62,32,44,80,100,96,101,136,180,185,142,97,
        99,142,174,161,127,117,138,148,118,72,63,104,153,163,136,119,
        140,176,188,170,159,182,219,232,208,176,168,181,185,166,138,126,
        130,132,123,109,106,114,120,120,118,121,128,136,144,157,167,162,
        141,119,113,120,121,106,89,90,99,90,54,18,20,60,100,105,
        88,86,111,132,120,91,92,141,197,206,164,118,111,129,130,103,
        85,110,158,174,135,81,66,102,147,164,162,176,210,230,208,162,
        140,163,205,224,211,186,170,156,131,99,84,98,128,148,146,130,
        114,102,97,106,129,157,174,172,162,150,136,113,90,87,109,130,
        122,85,50,45,58,61,47,42,72,118,136,112,79,85,128,163,
        157,131,129,160,182,159,110,87,111,147,144,106,80,98,138,152,
        130,110,128,172,196,183,162,172,208,232,218,184,165,171,178,167,
        146,135,142,147,134,107,88,88,102,118,133,147,154,149,135,126,
        129,136,140,143,151,159,147,107,60,40,56,81,86,72,67,81,
        91,71,36,32,80,146,178,159,130,129,148,148,119,98,122,175,
        200,163,99,66,82,110,113,101,113,157,193,182,136,109,133,185,
        218,217,207,211,219,203,163,129,128,154,174,167,142,120,108,99,
        91,91,107,131,147,152,150,145,136,126,126,140,158,159,137,111,
        101,102,92,66,45,53,80,92,71,43,46,83,116,112,90,91,
        130,167,165,133,118,143,176,171,125,88,99,138,152,124,91,95,
        128,144,124,98,112,165,211,212,184,168,182,201,198,179,174,195,
        214,202,159,117,101,106,115,119,125,134,138,128,109,97,101,117,
        139,164,184,186,162,124,98,97,109,114,107,103,109,106,74,25,
        0,24,78,112,108,93,99,122,126,100,80,106,169,212,196,143,
        110,118,133,121,91,89,129,168,160,110,71,84,130,162,162,158,
        180,215,224,194,159,158,189,216,211,184,164,159,154,133,108,99,
        111,126,129,121,114,115,118,123,131,144,152,151,144,144,150,148,
        130,107,100,106,105,81,52,47,71,91,81,50,39,65,102,111,
        95,95,131,174,177,137,100,110,152,175,155,122,120,142,146,108,
        61,60,110,162,171,144,128,144,168,169,151,154,195,244,254,219,
        173,151,154,158,150,145,153,165,157,124,87,70,78,102,127,150};
    memory.mount(0x477aa8u,std::as_bytes(std::span(response_ten)),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 17F80B response curve ten 1024 byte samples");
    // Original148DE0 selects one of1026 authored float3 direction rows
    // using its genuine updated seed and copies exactly12 bytes. Reuse
    // the existing native operand catalog, preserving every float bit.
    const auto operands=OriginalData::native_tu15();
    std::array<std::uint32_t,1026*3> direction_words{};
    for(std::size_t i=0;i<operands->weapon_directions.size();++i) {
        const auto& direction=operands->weapon_directions[i];
        direction_words[i*3]=std::bit_cast<std::uint32_t>(direction.x);
        direction_words[i*3+1]=std::bit_cast<std::uint32_t>(direction.y);
        direction_words[i*3+2]=std::bit_cast<std::uint32_t>(direction.z);
    }
    memory.mount(0x44b8e8u,encode(direction_words),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 148DE0 shared float3 direction samples");
    // Complete17F8A0 selects a 1024-byte response channel and interpolates
    // samples 0..1023. Retain only the actual reached channel five. These
    // counts losslessly declare its stored byte samples 0..255 in order;
    // they do not approximate or regenerate the curve mathematically.
    constexpr std::array<std::uint8_t,256> response_five_runs{
        41,17,13,11,10,9,8,7,8,6,7,6,6,6,5,5,
        6,5,5,4,5,5,4,5,4,4,4,4,5,4,3,4,
        4,4,4,3,4,4,3,4,3,4,3,3,4,3,3,4,
        3,3,3,4,3,3,3,3,3,3,3,3,3,3,3,3,
        3,3,3,3,3,3,3,2,3,3,3,3,2,3,3,3,
        2,3,3,3,2,3,3,2,3,3,2,3,3,2,3,3,
        2,3,3,2,3,2,3,3,2,3,2,3,3,2,3,2,
        3,2,3,3,2,3,2,3,2,3,3,2,3,2,3,2,
        3,2,3,2,3,3,2,3,2,3,2,3,3,2,3,2,
        3,2,3,3,2,3,2,3,3,2,3,2,3,3,2,3,
        3,2,3,3,2,3,3,2,3,3,2,3,3,3,2,3,
        3,3,2,3,3,3,3,2,3,3,3,3,3,3,3,3,
        3,3,3,3,3,3,3,3,3,3,3,4,3,3,3,4,
        3,3,4,3,3,4,3,4,3,4,4,3,4,4,4,4,
        3,4,5,4,4,4,4,5,4,5,5,4,5,5,6,5,
        5,6,6,6,7,6,8,7,8,9,10,11,13,17,40,1};
    std::vector<std::byte> response_five;
    response_five.reserve(1024);
    for(unsigned value=0;value<response_five_runs.size();++value)
        response_five.insert(response_five.end(),response_five_runs[value],std::byte(value));
    if(response_five.size()!=1024)throw std::logic_error("Authored response curve extent differs");
    memory.mount(0x4796a8u,response_five,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 17F946 response curve five 1024 byte samples");
    // Final startup already owns the complete HS registry and descriptors,
    // including opcode9 (480C24 ->456BB0) and rumble (481634 ->459D58).
    // Preserve its writable registry and immutable metadata; mounting these
    // older individual aliases again would overlap their genuine owners.
    // These actual reached constructors publish the following typed method
    // tables. Adjacent constructors bound461518/4620D0 to16 slots;41C3D0
    // ends after17 function identities before the pi literal, and41F360
    // ends after31 before the next class name. Share already retained slots
    // only when their readonly identity agrees. Native dispatch still owns
    // every implementation and rejects any unimplemented target.
    const auto declare_dispatch=[&]<std::size_t N>(std::uint32_t address,
        const std::array<std::uint32_t,N>& methods,const char* owner) {
        for(std::size_t slot=0;slot<N;++slot)
            share_immutable_word(memory,address+std::uint32_t(slot*4),methods[slot],owner);
    };
    // Original185111 publishes this two-method query on its actual stack.
    // The next independently published interface begins45F8C8 (1848A8).
    // Keep the complete table while sharing its already retained first word.
    declare_dispatch(0x45f8c0u,std::array{0x1848a0u,0x185190u},
        "native TU1.5 authored 185111 physical stack query identities");
    // Actual1CB914 installs the five-method primary interface below;
    // the following six/two-method base interfaces have independent
    // installers2E9C2F/2E9C36 and teardown restores1CC00F/1CC000.
    // 4615D0 begins a separate class installed1CB4FD. The four derived
    // embedded interfaces at +8/+C/+10/+14 are separately bounded too.
    declare_dispatch(0x46159cu,std::array{
        0x1cbfb0u,0x2e1270u,0x2e12a0u,0x3060c0u,0x2e4d60u},
        "native TU1.5 authored 1CB914 primary object interface identity");
    declare_dispatch(0x4615b0u,std::array{
        0x32b41eu,0x1cb0b0u,0x1ce050u,0x1cdfa0u,0x1cb110u,0x32b41eu},
        "native TU1.5 authored 2E9C2F embedded base object interface identity");
    declare_dispatch(0x4615c8u,std::array{0x32b41eu,0x1cb0d0u},
        "native TU1.5 authored 2E9C36 embedded base object interface identity");
    declare_dispatch(0x41c664u,std::array{0x2e9a00u},
        "native TU1.5 authored 1CB91A embedded object interface identity");
    declare_dispatch(0x461594u,std::array{0x1cbe70u,0x1ce500u},
        "native TU1.5 authored 1CB921 embedded object interface identity");
    declare_dispatch(0x46158cu,std::array{0x2e9920u,0x1ce510u},
        "native TU1.5 authored 1CB928 embedded object interface identity");
    declare_dispatch(0x461588u,std::array{0x2e9b10u},
        "native TU1.5 authored 1CB92F embedded object interface identity");
    // The authored allocator object486080 selects this complete eleven-
    // method table. Actual14A04A installs the next distinct interface at
    // 45F4F8, excluding it. Share retained method aliases only if identical.
    declare_dispatch(0x45f4ccu,std::array{
        0x149bf0u,0x149c40u,0x149cc0u,0x149d20u,0x149da0u,0x149e00u,
        0x2e4d60u,0x2e4d60u,0x233120u,0x2e10a0u,0x2e1110u},
        "native TU1.5 authored 486080 static allocator interface identity");
    // Actual2E12FE installs this one-method interface in the genuine
    // twenty-byte constructor2E12D0; its destructor2E1371 restores it.
    // The next word41BDF8 belongs to the distinct2E1FC0 object interface.
    declare_dispatch(0x41bdf4u,std::array{0x2e1320u},
        "native TU1.5 authored 2E12FE retained object interface identity");
    // Actual2E1FDB installs this distinct two-method interface in the
    // genuine eight-byte allocation constructor2E1FC0. Neighbor41BE00
    // has its own2E1618 installer and is excluded from this owner.
    declare_dispatch(0x41bdf8u,std::array{0x2e42f0u,0x31d400u},
        "native TU1.5 authored 2E1FDB retained object interface identity");
    // These six distinct classes are genuinely allocated/registered by
    // 2E2800/2E1D20 and selected by the reached2E1E50 release loop.
    // Bound each interface at its own next scalar, text or class owner.
    declare_dispatch(0x41bee0u,std::array{0x2e29e0u},
        "native TU1.5 authored 2E2A2D retained object interface identity");
    declare_dispatch(0x41c328u,std::array{0x2e42f0u,0x2e4150u,0x2e4170u},
        "native TU1.5 authored 2E433B retained object interface identity");
    declare_dispatch(0x41c4b8u,std::array{0x2e6200u},
        "native TU1.5 authored 2E61DB retained object interface identity");
    declare_dispatch(0x4485c8u,std::array{0x2e42f0u},
        "native TU1.5 authored 37C5AF retained object interface identity");
    // Adjacent4485E8/4485EC are separate one-method classes, installed
    // by the twenty-/twenty-four-byte constructors37C7C0/37C830.
    declare_dispatch(0x4485e8u,std::array{0x37c800u},
        "native TU1.5 authored 37C7DB retained object interface identity");
    declare_dispatch(0x4485ecu,std::array{0x37c870u},
        "native TU1.5 authored 37C84D retained object interface identity");
    // Complete2E2800 additionally constructs these three distinct base-
    // system objects. Reached2E1F80 releases their actual retained bindings.
    // The fourteen-method32-byte object ends before the41BEBC path literal.
    declare_dispatch(0x41be84u,std::array{
        0x2e2560u,0x2e20b0u,0x2e2360u,0x2e23b0u,0x2e2400u,0x2e2060u,
        0x2e2620u,0x2e2620u,0x2e2620u,0x2e2720u,0x2e2720u,0x2e2720u,
        0x2e2030u,0x2e1ff0u},
        "native TU1.5 authored 2E28B4 base-system object interface identity");
    // Constructor2E15F0 installs the primary three-method interface and
    // constructs separate embedded12-byte interfaces at +8/+14 via2E3110.
    declare_dispatch(0x41be00u,std::array{0x2e16f0u,0x2e16d0u,0x2e16e0u},
        "native TU1.5 authored 2E1618 base-system object interface identity");
    declare_dispatch(0x41bf0cu,std::array{0x2e3130u},
        "native TU1.5 authored 2E3116 embedded object interface identity");
    // This eight-byte object's four-method interface stops at41BE1C,
    // independently installed by2E178B in a distinct constructor.
    declare_dispatch(0x41be0cu,std::array{0x2e42f0u,0x2e1770u,0x2e17d0u,0x2e1a40u},
        "native TU1.5 authored 2E2856 base-system object interface identity");
    // Actual2E1770's genuine eight-byte constructor installs this separate
    // seven-method object interface at2E178B. The fopen mode string "rb"
    // starts immediately after its last method, at41BE38.
    declare_dispatch(0x41be1cu,std::array{
        0x2e17a0u,0x325d80u,0x2e1c80u,0x31d400u,0x2e38c0u,0x8d140u,0x1cb030u},
        "native TU1.5 authored 2E178B retained object interface identity");
    // Original14956B/1496D9/149A94 install this eleven-method overflow
    // allocator interface. Its +20 release method is reached by1498F0.
    // Neighbor463968 is separately installed by2336C2/235FC4; exclude it.
    // Retain earlier allocate/free aliases only with identical RO bytes.
    declare_dispatch(0x46393cu,std::array{
        0x232aa0u,0x232ac0u,0x232ad0u,0x232c80u,0x232d00u,0x232d20u,
        0x2e4d60u,0x2e4d60u,0x233120u,0x2e10a0u,0x2e1110u},
        "native TU1.5 authored 1496D9 overflow allocator interface identity");
    // Genuine232DE0 installs this eleven-method primary pool interface
    // at232DFA. Reached149615 consumes its +20 release method. A zero
    // padding word and authored Statistics text follow at4638E4/4638E8.
    declare_dispatch(0x4638b8u,std::array{
        0x233160u,0x2331a0u,0x2331d0u,0x233220u,0x233250u,0x233420u,
        0x233140u,0x233490u,0x233120u,0x2e10a0u,0x2e1110u},
        "native TU1.5 authored 232DFA primary pool interface identity");
    // Complete17AEB0 dispatches four original selector cases through
    // this bounded table. The next original function begins at17AEF0.
    memory.mount(0x17aee0u,encode(std::array{
        0x17aebdu,0x17aec1u,0x17aed0u,0x17aed9u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 17AEB0 selector branch identities");
    // Complete13A6B0 selects these two authored static tag-accessor
    // instances by the original group codes70727433/5052544D. Their
    // final method22B790 publishes the actual tag and definition at+4/+8.
    memory.mount(0x48603cu,encode(std::array{0x45f330u,0u,0u}),
        SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 70727433 static tag-accessor instance");
    memory.mount(0x486048u,encode(std::array{0x45f2d8u,0u,0u}),
        SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 authored 5052544D static tag-accessor instance");
    // Each distinct interface has21 original methods. Exclude the
    // padding at45F32C and the next format string beginning45F384.
    declare_dispatch(0x45f330u,std::array{
        0x22ac50u,0x22ac60u,0x22ac70u,0x22ac80u,0x1cb030u,0x1cb030u,
        0x22ac90u,0x22aca0u,0x22acc0u,0x22acd0u,0x22ace0u,0x22acf0u,
        0x22ad20u,0x22ad30u,0x22b750u,0x22b760u,0x22b770u,
        0x22ad40u,0x22ad50u,0x22ad60u,0x22b790u},
        "native TU1.5 authored 70727433 static tag-accessor interface identity");
    declare_dispatch(0x45f2d8u,std::array{
        0x22b6e0u,0x22b6f0u,0x22b700u,0x22ac50u,0x1cb030u,0x1cb030u,
        0x22ac80u,0x22b710u,0x22ac60u,0x22ac70u,0x22b720u,0x22b740u,
        0x22ad20u,0x22ad30u,0x22b750u,0x22b760u,0x22b770u,
        0x22ad40u,0x22ad50u,0x22b780u,0x22b790u},
        "native TU1.5 authored 5052544D static tag-accessor interface identity");
    // Complete17BB10 dispatches the authored signed WORD selector at
    // record+E through these four cases;17BD38 padding is excluded.
    memory.mount(0x17bd28u,encode(std::array{
        0x17bcc2u,0x17bbcbu,0x17bc47u,0x17bcfcu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 17BB10 selector branch identities");
    // Actual17D62C reads its signed authored type WORD, then17D630
    // selects one of these four native code identities without a local
    // enum clamp. Preserve only the original row, excluding its padding.
    memory.mount(0x17d718u,encode(std::array{
        0x17d674u,0x17d637u,0x17d654u,0x17d6dcu}),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 17D630 four-case effect selector identities");
    // Original252106/25210C bound this authored selector to0..16.
    // Complete252100 dispatches17 branch words, excluding252474 padding.
    memory.mount(0x252430u,encode(std::array{
        0x252119u,0x25212au,0x25213bu,0x25213bu,0x252175u,0x252186u,
        0x252186u,0x252198u,0x2521a9u,0x2521bcu,0x2521e2u,0x25220fu,
        0x25223cu,0x2522f6u,0x252158u,0x252158u,0x252415u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 252100 selector branch identities");
    // Complete252480 immediately dispatches its authored record+78
    // selector after2524BD. These ten original cases end before252DAC
    // padding and the separate252DB0 function; no clamp is introduced.
    memory.mount(0x252d84u,encode(std::array{
        0x2524d4u,0x252597u,0x252628u,0x252672u,0x2526eeu,
        0x2527acu,0x2528b0u,0x252a12u,0x252b19u,0x252bd8u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 252480 selector branch identities");
    // Complete16F150 and16F6A0 bound the signed selector+1 to0..3.
    // Preserve their distinct authored table order, excluding nearby code.
    // Original15CEF7 selects all seven branches through this authored table.
    //15D4F0 reaches it while admitting the MCC Coagulation world; no branch
    // result is supplied by the native data owner (offline CFG audit v780).
    memory.mount(0x15cfa8u,encode(std::array{
        0x15cefeu,0x15cf16u,0x15cf2eu,0x15cf46u,0x15cf5eu,0x15cf76u,0x15cf8eu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 15CEF7 seven-way configuration branch identities");
    // Original1DFC2E selects six paths within complete1DFA80. The reached
    // Cyclotron branch reads index4; retain the exact complete source table.
    memory.mount(0x1e0184u,encode(std::array{
        0x1dfc35u,0x1dfc9cu,0x1dfd9eu,0x1dff3eu,0x1dffb4u,0x1e001cu}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 1DFC2E six-way physical branch identities");
    memory.mount(0x16f33cu,encode(std::array{
        0x16f247u,0x16f21bu,0x16f29cu,0x16f265u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored16F150 four camera dispatch identities");
    memory.mount(0x16fd30u,encode(std::array{
        0x16f6f4u,0x16fa29u,0x16fbf2u,0x16fc79u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored16F6A0 four camera dispatch identities");
    // Original16FD46..16FD55 sign-extends the authored selector, subtracts1
    // and bounds the result to0..3 before these four retained case targets.
    memory.mount(0x16fdf0u,encode(std::array{
        0x16fd62u,0x16fd67u,0x16fd9au,0x16fdb3u}),
        SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored 16FD40 camera selector branch identities");
    // Actual2EFFE5 installs this one-method interface;2F00A0 restores
    // it in teardown. Both adjacent owners are authored diagnostic/enum
    // text, with HK_SHAPE_USER7 beginning immediately at41C7C8.
    declare_dispatch(0x41c7c4u,std::array{0x2f0160u},
        "native TU1.5 authored 2EFFE5 retained object interface identity");
    // Actual2EF00E reinstalls this sixteen-method object interface
    // during teardown. The next word at41C700 begins authored diagnostic
    // text (Agent handling types), so it is excluded from this table.
    declare_dispatch(0x41c6c0u,std::array{
        0x2ef960u,0x2ec4b0u,0x2ef0d0u,0x2ec9b0u,0x2ef590u,0x2ea380u,
        0x2edb00u,0x2eb050u,0x2eb0c0u,0x2eb0e0u,0x2eb670u,0x2ea150u,
        0x2edd40u,0x2ea030u,0x2ed370u,0x2ee660u},
        "native TU1.5 authored 2EF00E retained object interface identity");
    // Actual2E4DCD/2E4DD3/2E4DD9 install three distinct object interfaces
    // at +0/+8/+C, with one/three/two methods respectively. Teardown
    // 2E4B07/2E4B0D/2E4B13 reinstalls these same identities. The next
    // word at41C364 is the authored34000000 float operand, not a method.
    declare_dispatch(0x41c360u,std::array{0x2e4e10u},
        "native TU1.5 authored 2E4DCD primary object interface identity");
    declare_dispatch(0x41c354u,std::array{0x2e4b60u,0x2e4d60u,0x2e4d60u},
        "native TU1.5 authored 2E4DD3 embedded object interface identity");
    declare_dispatch(0x41c34cu,std::array{0x2e4d70u,0x2e4a70u},
        "native TU1.5 authored 2E4DD9 embedded object interface identity");
    // Actual316879/31687F construct the primary two-method interface
    // and its embedded +8 five-method interface in registration312C60.
    // Neighboring41D938/41D958/41D964/41D970 have distinct installers.
    declare_dispatch(0x41d968u,std::array{0x3167a0u,0x316790u},
        "native TU1.5 authored 316879 primary object interface identity");
    declare_dispatch(0x41d944u,std::array{0x322b70u,0x316410u,0x2e4d60u,0x8eb50u,0x31d400u},
        "native TU1.5 authored 31687F embedded object interface identity");
    // Canonical3169EE installs this three-method listener.312CFF installs
    // the next distinct embedded interface at41D944, bounding this class.
    declare_dispatch(0x41d938u,std::array{0x316680u,0x31ffc0u,0x320040u},
        "native TU1.5 authored canonical 3169EE physical listener typed dispatch identity");
    // Canonical311A43 installs this15-method shape-phantom interface.
    // A zero alignment word and its class-name text follow the last method.
    declare_dispatch(0x41d87cu,std::array{
        0x315620u,0x2e1270u,0x2e12a0u,0x31c340u,0x2e4d60u,0xa3850u,
        0x315170u,0x315300u,0x315100u,0x31d400u,0x3156a0u,0x315710u,
        0x31c440u,0x315650u,0x315660u},
        "native TU1.5 authored canonical 311A43 shape phantom typed dispatch identity");
    // Actual3164FF installs the three-method interface at object+8;
    // actual3164F9 separately installs the one-method primary interface.
    // Keep the same authored words with their precise object boundaries.
    declare_dispatch(0x41d958u,std::array{0x316510u,0x2e4d60u,0x2e4d60u},
        "native TU1.5 authored 3164FF embedded object interface identity");
    declare_dispatch(0x41d964u,std::array{0x316540u},
        "native TU1.5 authored 3164F9 primary object interface identity");
    // Canonical27DAF0 installs this three-method listener at object+4.
    // Its primary interface, installed by27DAEA, begins at466C70.
    declare_dispatch(0x466c64u,std::array{0x281710u,0x281d40u,0x281e10u},
        "native TU1.5 authored canonical 27DAF0 contact listener typed dispatch identity");
    // Canonical27DAEA installs this thirteen-method primary interface;
    // its distinct base interface at466CA4 is installed first by27DAE3.
    declare_dispatch(0x466c70u,std::array{
        0x2815a0u,0x2815f0u,0x2816a0u,0x281f10u,0x282700u,0x282780u,
        0x282840u,0x2827f0u,0x2826c0u,0x281510u,0x32b41eu,0x32b41eu,0x32b41eu},
        "native TU1.5 authored canonical 27DAEA physical state listener typed dispatch identity");
    // Canonical1C6037 installs this three-method world listener;27DB60
    // separately installs the next interface at466C50.
    declare_dispatch(0x466c44u,std::array{0x282080u,0x2e4d60u,0x2e4d60u},
        "native TU1.5 authored canonical 1C6037 world contact listener typed dispatch identity");
    // Canonical1C6C0B installs this three-method physical listener; the
    // following bytes begin authored codec-name text, not another method.
    declare_dispatch(0x466cbcu,std::array{0x282850u,0x282870u,0x282ab0u},
        "native TU1.5 authored canonical 1C6C0B physical state listener typed dispatch identity");
    // Actual3233E8 installs this eight-method activation interface. The
    // following sixteen bytes are the separate authored xyz SIMD mask.
    declare_dispatch(0x41f1b0u,std::array{
        0x3233f0u,0x2e1270u,0x2e12a0u,0x3265a0u,
        0x2e4d60u,0x3233c0u,0x3233c0u,0xad740u},
        "native TU1.5 authored 3233E8 physical activation typed dispatch identity");
    // Actual31A292 installs this five-method interface and31AB30
    // reinstalls it during teardown. 41DA9C is padding; the following
    // SolverType reflection enum records are separate typed data.
    declare_dispatch(0x41da88u,std::array{0x31b170u,0x2e1270u,0x2e12a0u,0x31b260u,0x31aa50u},
        "native TU1.5 authored 31A292 retained object interface identity");
    // Actual1F7010 constructs this complete fifteen-method root on a
    // 48-byte object. Geometry branches change bounds data only;
    // no embedded vptrs. The next46214C belongs to1CC6ED construction.
    declare_dispatch(0x462110u,std::array{
        0x1f72a0u,0x2e1270u,0x2e12a0u,0x305ba0u,0x2e4d60u,0x31d520u,
        0x1f7300u,0x305e30u,0x305be0u,0x305f00u,0x1cb560u,0x1cb560u,
        0x1f7990u,0x1f7c10u,0x212330u},
        "native TU1.5 authored 1F7010 primary object interface identity");
    // Actual1CC6ED constructs this thirteen-method root on a 24-byte
    // object. Its +14 field retains a pointee, not an embedded interface.
    // Neighboring462110/462180 have independent1F7010/1F7D6A installers.
    declare_dispatch(0x46214cu,std::array{
        0x1f67f0u,0x2e1270u,0x2e12a0u,0x308580u,0x2e4d60u,0xa9be0u,
        0x2e9600u,0x2e9400u,0x2e9710u,0x2e9750u,0x1f6ac0u,0x1f6ae0u,0x1f6d10u},
        "native TU1.5 authored 1CC6ED primary object interface identity");
    // Actual1CC777 constructs this complete twelve-method root on a
    // sixteen-byte object with retained pointee+C and no embedded vptrs.
    // Neighbor461588 belongs to the separate9C-byte class at1CB92F.
    declare_dispatch(0x461558u,std::array{
        0x1ce5a0u,0x2e1270u,0x2e12a0u,0x305aa0u,0x2e4d60u,0xadb50u,
        0x2e9600u,0x2e9400u,0x305b40u,0x305b60u,0x1ce050u,0x32b41eu},
        "native TU1.5 authored 1CC777 primary object interface identity");
    // Actual1CB4FD constructs this complete fifteen-method interface
    // on its C-byte source-owned object. 46160C is a distinct two-method
    // stack-local interface installed by1CE2A9, excluding those words.
    declare_dispatch(0x4615d0u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x305ba0u,0x2e4d60u,0x31d520u,
        0x305d30u,0x305e30u,0x305be0u,0x305f00u,0x305bb0u,
        0x1cb560u,0x1cb560u,0x1cb560u,0x212330u},
        "native TU1.5 authored 1CB4FD primary object interface identity");
    // Actual2E97D1/2E9842 install this thirteen-method primary interface;
    // teardown2E96CD restores it. The next461E58 owner is independently
    // installed3082C2, excluding that following interface.
    declare_dispatch(0x461e24u,std::array{
        0x1f3660u,0x2e1270u,0x2e12a0u,0x308580u,0x2e4d60u,0xa9be0u,
        0x2e9600u,0x2e9400u,0x2e9710u,0x2e9750u,0x2e9610u,0x2e9640u,0x2e9670u},
        "native TU1.5 authored 2E97D1 primary object interface identity");
    // Actual31F828 installs this complete eleven-method interface; the
    // following 41EDE8 words begin reflection enum records, not methods.
    // The same constructor installs the separate four-method subobject
    // interface at +8. Neither table contains constructed object state.
    declare_dispatch(0x41edbcu,std::array{
        0x31f9f0u,0x2e1270u,0x2e12a0u,0xa3850u,0x2e4d60u,
        0xa3850u,0xa3850u,0x325d80u,0xaabb0u,0x31f860u,0x31f840u},
        "native TU1.5 authored 31F828 retained object interface identity");
    declare_dispatch(0x41ed80u,std::array{0x31f760u,0x2e4d60u,0x2e4d60u,0x2e4d60u},
        "native TU1.5 authored 31F82E embedded object interface identity");
    // Actual31E84B installs this eight-method contact listener. The next
    // word is its distinct reflection-data pointer, not another method.
    declare_dispatch(0x41eb6cu,std::array{
        0x31e7f0u,0x2e1270u,0x2e12a0u,0x31e480u,
        0x2e4d60u,0x31e5c0u,0x31e690u,0x31d520u},
        "native TU1.5 authored 31E84B contact listener typed dispatch identity");
    // Canonical3001BA installs these eight contact query methods; the
    // distinct actual30411A constructor installs the next class at41D03C.
    declare_dispatch(0x41d01cu,std::array{
        0x3051a0u,0x304b90u,0x304c10u,0x303760u,
        0x304350u,0x2f3100u,0x2e38c0u,0x3038f0u},
        "native TU1.5 authored canonical 3001BA physical query typed dispatch identity");
    // Canonical1EE85F/1EE8A9 install this ten-method physical interface.
    // Actual306F79 installs the next distinct class at461EC0.
    declare_dispatch(0x461e98u,std::array{
        0x1f3770u,0x2e1270u,0x2e12a0u,0x308650u,0x2e4d60u,
        0xad1d0u,0x2e9600u,0x2e9400u,0x308730u,0x308750u},
        "native TU1.5 authored canonical 1EE85F physical shape typed dispatch identity");
    // Canonical1EE7C3 installs this thirteen-method physical shape. The
    // next interface at461F64 is separately installed by1EE8BA.
    declare_dispatch(0x461f30u,std::array{
        0x1f2c60u,0x2e1270u,0x2e12a0u,0x308580u,0x2e4d60u,0xa9be0u,
        0x2e9600u,0x2e9400u,0x2e9710u,0x2e9750u,0x2e9610u,0x2e9640u,0x2e9670u},
        "native TU1.5 authored canonical 1EE7C3 physical shape typed dispatch identity");
    // Constructor2E1407 installs this nine-method contact query interface;
    // its last slot is the actual reached3009FD +20 dispatch. The following
    // word begins authored text, not another function identity.
    declare_dispatch(0x41c370u,std::array{
        0x2fc540u,0x2f8410u,0x2f8ef0u,0x308ca0u,0x2e5320u,
        0x2f82d0u,0x2e38c0u,0x2e4ea0u,0x2e4fe0u},
        "native TU1.5 authored 2E1407 physical contact query typed dispatch identity");
    // Actual2F7643 installs this eight-method interface;2F7703
    // independently installs the following interface at41CC84.
    declare_dispatch(0x41cc64u,std::array{
        0x2fc540u,0x2f6f40u,0x2f6e30u,0x2f6d20u,
        0x2f6ca0u,0x2f6c80u,0x2e38c0u,0x2f7050u},
        "native TU1.5 authored 2F7643 physical contact query typed dispatch identity");
    declare_dispatch(0x41cc84u,std::array{
        0x2fc540u,0x2f73c0u,0x2f7420u,0x2f7500u,
        0x2f7480u,0x2f6c80u,0x325d80u,0x2f74e0u},
        "native TU1.5 authored 2F7703 physical contact query typed dispatch identity");
    // Actual2F557D installs this eight-method contact interface; the
    // following FLT_MAX operand and authored text are separate data.
    declare_dispatch(0x41cb9cu,std::array{
        0x3051a0u,0x2f51c0u,0x2f5280u,0x2f5440u,
        0x2f59e0u,0x2f3100u,0x325d80u,0x2f36d0u},
        "native TU1.5 authored 2F557D physical contact query typed dispatch identity");
    // Actual2F7D4E installs this eight-method interface;2F8249
    // installs the independent following class at41CCE8.
    declare_dispatch(0x41ccc8u,std::array{
        0x2f8090u,0x2f7b80u,0x2f7ab0u,0x2f7b10u,
        0x2f7a50u,0x2f7a20u,0x2e38c0u,0x2f7bf0u},
        "native TU1.5 authored 2F7D4E physical contact query typed dispatch identity");
    // Actual2F0E21 installs this eight-method contact interface.2F7A9A
    // dispatches slot4 during the MCC Lockout gravity-lift contact. The
    // original following bytes are literal strings, outside this interface.
    declare_dispatch(0x41ca9cu,std::array{
        0x2fc540u,0x2f16f0u,0x2f13f0u,0x2f10b0u,
        0x2f0f10u,0x2f0da0u,0x2e38c0u,0x2f1850u},
        "native TU1.5 authored 2F0E21 physical contact typed dispatch identity");
    // Original2F0F7E/2F1122 publish these distinct two-method stack query
    // interfaces. Their following authored interfaces begin41CA94/41CA9C.
    declare_dispatch(0x41ca8cu,std::array{0x2f0a50u,0x30ba80u},
        "native TU1.5 authored 2F0F7E contact stack query identity");
    declare_dispatch(0x41ca94u,std::array{0x2f0a20u,0x30ba90u},
        "native TU1.5 authored 2F1122 contact stack query identity");
    declare_dispatch(0x41cce8u,std::array{
        0x2f81b0u,0x2f7ef0u,0x2f7f50u,0x2f80c0u,
        0x2f7fb0u,0x2f7a20u,0x325d80u,0x2f8010u},
        "native TU1.5 authored 2F8249 physical contact query typed dispatch identity");
    // Actual2FBB5C installs this eight-method scene contact interface;
    // the distinct following constraint interface begins41CE80.
    declare_dispatch(0x41ce60u,std::array{
        0x2fc540u,0x2fc520u,0x2fbec0u,0x308ca0u,
        0x2fbba0u,0x2fbb10u,0x2e38c0u,0x2e4ea0u},
        "native TU1.5 authored 2FBB5C physical scene contact typed dispatch identity");
    declare_dispatch(0x41c3d0u,std::array{
        0x2e6000u,0x2e1270u,0x2e12a0u,0x2e61b0u,0x2e4d60u,0xa9520u,
        0x2e5e80u,0x2e90e0u,0x2e5f00u,0x2e9140u,0x2e5f60u,0x2e5f70u,
        0x2e5db0u,0x2e5e20u,0x2e5da0u,0x1cb030u,0x2e5f20u},
        "native TU1.5 authored 2E2557 physical shape typed dispatch identity");
    // Actual315BFD constructs this14-method physical interface. The
    // next independent class begins41D914, installed by315F0E/315FBE.
    declare_dispatch(0x41d8dcu,std::array{
        0x315c60u,0x2e1270u,0x2e12a0u,0x315670u,0x2e4d60u,0xad740u,
        0x31c810u,0x315b00u,0x315a70u,0x31d400u,0x315860u,0x315990u,
        0x315a00u,0x3157d0u
    },"native TU1.5 authored 315BFD physical shape typed dispatch identity");
    // Actual2E8FD6 constructs this16-method physical shape interface;
    // the independent next interface begins4613E0 (publishedby2E9034).
    declare_dispatch(0x4613a0u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x2e9330u,0x2e4d60u,0x2e8cf0u,
        0x2e8d90u,0x2e90e0u,0x2e8dd0u,0x2e9140u,0x2e8d00u,0x2e8fe0u,
        0x2e8d20u,0x2e8d40u,0x2e8d70u,0xad740u
    },"native TU1.5 authored 2E8FD6 physical shape typed dispatch identity");
    // Actual2E80EE installs this complete sixteen-method physical shape
    // interface;31C92F dispatches its slot5. The distinct next interface at
    //461460 is installed by2E5F88/2E8786 (offline original audit v779).
    declare_dispatch(0x461420u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x2e9250u,0x2e4d60u,0x31ede0u,
        0x2e8120u,0x2e90e0u,0x2e81e0u,0x2e9140u,0x2e7d10u,0x306bb0u,
        0x2e7d90u,0x3070f0u,0x2e7e10u,0x31d520u},
        "native TU1.5 authored 2E80EE physical shape typed dispatch identity");
    // Actual2E88C8/2E87ED install this ten-method physical interface;
    // the following independent class starts4614B0.
    declare_dispatch(0x461488u,std::array{
        0x1cb060u,0x2e1270u,0x2e12a0u,0x2e94e0u,0x2e4d60u,
        0xae690u,0x2e8870u,0x2e8900u,0x2e8a00u,0x2e8b40u},
        "native TU1.5 authored 2E88C8 physical shape typed dispatch identity");
    // Actual1CAF40 publishes these two distinct interfaces at embedded
    // offsets20/30. The next class starts461518, bounding10/16 methods.
    declare_dispatch(0x4614b0u,std::array{
        0x1caf30u,0x2e1270u,0x2e12a0u,0x2e94e0u,0x2e4d60u,
        0xae690u,0x2e8870u,0x2e8900u,0x2e8a00u,0x2e8b40u},
        "native TU1.5 authored 1CAF7C embedded physical shape typed dispatch identity");
    declare_dispatch(0x4614d8u,std::array{
        0x1caf20u,0x2e1270u,0x2e12a0u,0x2e9330u,0x2e4d60u,0x2e8cf0u,
        0x2e8d90u,0x2e90e0u,0x2e8dd0u,0x2e9140u,0x2e8d00u,0x2e8fe0u,
        0x2e8d20u,0x2e8d40u,0x2e8d70u,0xad740u},
        "native TU1.5 authored 1CAF65 embedded physical shape typed dispatch identity");
    declare_dispatch(0x461518u,std::array{
        0x1caf10u,0x2e1270u,0x2e12a0u,0x2e9250u,0x2e4d60u,0x31ede0u,
        0x2e8120u,0x2e90e0u,0x2e81e0u,0x2e9140u,0x2e7d10u,0x306bb0u,
        0x2e7d90u,0x3070f0u,0x2e7e10u,0x31d520u},
        "native TU1.5 authored 1CAFAE physical shape typed dispatch identity");
    // Canonical1EE51F/1EE535 publish this16-method embedded interface;
    // the distinct next authored class begins4620D0.
    // Actual24DEDF installs this ten-method nested query interface;
    // the distinct stack query class starts at466284.
    declare_dispatch(0x46625cu,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x2e9220u,0x2e4d60u,
        0x1cb560u,0x1cb560u,0x1cb560u,0x1cb560u,0x24def0u},
        "native TU1.5 authored 24DEDF nested collision query typed dispatch identity");
    // Actual24DCE6 installs this17-method stack query interface. The
    // following authored bytes at4662C8 begin the separate particles text.
    declare_dispatch(0x466284u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x305ba0u,0x2e4d60u,0x31d520u,
        0x1cb560u,0x1cb560u,0x1cb560u,0x1cb560u,0x1cb560u,0x1cb560u,
        0x1cb560u,0x24dec0u,0x212330u,0x24df20u,0x24dc70u},
        "native TU1.5 authored 24DCE6 stack collision query typed dispatch identity");
    declare_dispatch(0x462090u,std::array{
        0x1f2c20u,0x2e1270u,0x2e12a0u,0x3065a0u,0x2e4d60u,0x31edf0u,
        0x306760u,0x2e90e0u,0x306890u,0x2e9140u,0x306670u,0x306830u,
        0x306790u,0x3067d0u,0x306750u,0x1cb030u},
        "native TU1.5 authored canonical 1EE51F embedded shape typed dispatch identity");
    // Actual20D837 binds every inline vehicle phantom shape to this
    // twelve-method interface. The separately installed462090 bounds it.
    declare_dispatch(0x462060u,std::array{
        0x1f2c10u,0x2e1270u,0x2e12a0u,0x306a80u,0x2e4d60u,0x31cae0u,
        0x306de0u,0x2e9400u,0x306bc0u,0x306ed0u,0x306b90u,0x306bb0u},
        "native TU1.5 authored 20D837 inline physical shape typed dispatch identity");
    // Original31E3BE installs eleven callable entries; the following word
    // at41EA68 points into reflection data, outside executable code.320E81
    // dispatches its final slot.31E3C4 installs the four-entry embedded
    // interface too, bounded by the distinct31CE20 primary table41E7AC.
    declare_dispatch(0x41ea3cu,std::array{
        0x31cea0u,0x2e1270u,0x2e12a0u,0x31e1e0u,0x2e4d60u,0x31e2e0u,
        0x31e2f0u,0x325d80u,0xa3850u,0x31e360u,0x31e330u},
        "native TU1.5 authored 31E3BE physical typed dispatch identity");
    declare_dispatch(0x41e79cu,std::array{0x31d700u,0x2e4d60u,0x325aa0u,0x2e4d60u},
        "native TU1.5 authored 31E3C4 embedded physical callback identity");
    declare_dispatch(0x4620d0u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x2e91f0u,0x2e4d60u,0xab4b0u,
        0x1f61b0u,0x2e90e0u,0x1f6220u,0x2e9140u,0x1cb560u,0x1cb560u,
        0x1f6610u,0x1f6690u,0x2e7e10u,0x1cb030u},
        "native TU1.5 authored canonical 1F1731 contact shape typed dispatch identity");
    // Actual20D893 installs this physical interface. Its17 authored
    // method identities end before padding and the next461FE0 class;
    // preserve the already declared identical interior slot aliases.
    declare_dispatch(0x461f98u,std::array{
        0x1f2c50u,0x2e1270u,0x2e12a0u,0x3083c0u,0x2e4d60u,0xad710u,
        0x306170u,0x305e30u,0x305be0u,0x305f00u,0x3061f0u,0xa3850u,
        0x306200u,0x306220u,0x306230u,0x306240u,0x2e4d60u},
        "native TU1.5 authored 20D893 physical shape typed dispatch identity");
    // Actual30639B/30648E install this17-method interface. The next
    // distinct authored interface begins461E24, outside this owner.
    declare_dispatch(0x461de0u,std::array{
        0x1f3690u,0x2e1270u,0x2e12a0u,0x3083c0u,0x2e4d60u,0xad710u,
        0x306170u,0x305e30u,0x305be0u,0x305f00u,0x3061f0u,0xa3850u,
        0x306200u,0x306220u,0x306230u,0x306240u,0x2e4d60u},
        "native TU1.5 authored 30639B physical shape typed dispatch identity");
    // The actual normal map also reaches this constructor-owned class.
    // Canonical1EE5EF installs461FE0; the distinct1EE58A constructor
    // installs462020, bounding these16 authored method identities.
    declare_dispatch(0x461fe0u,std::array{
        0x1f2c40u,0x2e1270u,0x2e12a0u,0x307630u,0x2e4d60u,0xad390u,
        0x307e30u,0x2e90e0u,0x307b50u,0x2e9140u,0x3077a0u,0x307cc0u,
        0x3077f0u,0x3079c0u,0x3077c0u,0x1cb030u},
        "native TU1.5 authored canonical 1EE5EF physical shape typed dispatch identity");
    // Canonical1D5222 installs this eight-method physical listener;
    // actual1DCF00 independently installs the next interface at461990.
    declare_dispatch(0x461970u,std::array{
        0x1d9c80u,0x2e1270u,0x2e12a0u,0x31e480u,
        0x2e4d60u,0x1d9b20u,0x1d9b70u,0x31d520u},
        "native TU1.5 authored canonical 1D5222 physical listener typed dispatch identity");
    // Actual322CAC installs this distinct two-method interface; the
    // following41F118 is the separately owned authored numeric operand.
    declare_dispatch(0x41f110u,std::array{0x322b70u,0x322cc0u},
        "native TU1.5 authored 322CAC retained object interface identity");
    // Actual32319C installs this complete two-method interface. The
    // preceding41F118 is an authored numeric operand and the next
    // eight-method table41F124 is separately installed by32325B.
    declare_dispatch(0x41f11cu,std::array{0x322b70u,0x3232e0u},
        "native TU1.5 authored 32319C retained object interface identity");
    // Canonical31F65F installs this eight-method contact owner. The next
    // word points to reflection data rather than another function identity.
    declare_dispatch(0x41f124u,std::array{
        0x3232b0u,0x322d20u,0x322f70u,0x3230c0u,
        0x323220u,0x323040u,0x323020u,0x3230b0u},
        "native TU1.5 authored canonical 31F65F contact owner typed dispatch identity");
    // The same constructor installs this embedded three-method interface
    // at offset1C, separately from its primary interface at41F080.
    declare_dispatch(0x41f074u,std::array{0x321390u,0x320ef0u,0x320260u},
        "native TU1.5 authored canonical 31D757 embedded contact query typed dispatch identity");
    // Canonical31D751 installs these six contact query methods. The next
    // bytes begin the authored StExamine string, bounding the interface.
    declare_dispatch(0x41f080u,std::array{
        0x322100u,0x3201b0u,0x3200b0u,0x320100u,0x320130u,0x320140u},
        "native TU1.5 authored canonical 31D751 physical contact query typed dispatch identity");
    // Canonical32259F installs these eleven physical constraint methods;
    // the following bytes begin the authored MinNumJacobians string.
    declare_dispatch(0x41f478u,std::array{
        0x3261d0u,0x2e1270u,0x2e12a0u,0xa3850u,0x2e4d60u,0xa3850u,
        0xa3850u,0x325d80u,0xaabb0u,0x325ef0u,0x325e90u},
        "native TU1.5 authored canonical 32259F physical constraint typed dispatch identity");
    // Canonical320CF7 installs these32 methods. The separately constructed
    // interface at41F2E0 bounds this complete physical owner declaration.
    // Canonical31F9A7 (actual323527) installs this32-method interface;
    // the separately constructed41F260 class bounds its authored extent.
    declare_dispatch(0x41f1e0u,std::array{
        0x3235c0u,0x323730u,0x8fdb0u,0x323560u,0x3235f0u,0x323420u,
        0x31edf0u,0x323860u,0x3238a0u,0x3248b0u,0x3248b0u,0x2e4d60u,
        0x2e4d60u,0x3248b0u,0x3248b0u,0x3238b0u,0x323900u,0x323950u,
        0x3239a0u,0x323b20u,0x3239f0u,0x323a00u,0x323a10u,0x2e4d60u,
        0x8fdb0u,0x2e4d60u,0x8fdb0u,0x2e4d60u,0x2e4d60u,0x8fdb0u,
        0x323ab0u,0x324910u},
        "native TU1.5 authored canonical 31F9A7 physical constraint typed dispatch identity");
    declare_dispatch(0x41f260u,std::array{
        0x324870u,0x2e4d60u,0x8fdb0u,0x3248d0u,0x324800u,0x323420u,
        0x31ede0u,0x323860u,0x3238a0u,0x3248b0u,0x3248b0u,0x2e4d60u,
        0x2e4d60u,0x3248b0u,0x3248b0u,0x3238b0u,0x323900u,0x323950u,
        0x3239a0u,0x323b20u,0x3239f0u,0x323a00u,0x323a10u,0x2e4d60u,
        0x8fdb0u,0x2e4d60u,0x8fdb0u,0x2e4d60u,0x2e4d60u,0x8fdb0u,
        0x324950u,0x324910u},
        "native TU1.5 authored canonical 320CF7 physical owner typed dispatch identity");
    // Canonical320E64 installs this related31-method physical owner. Its
    // final zero padding precedes the distinct321340 interface at41F360.
    declare_dispatch(0x41f2e0u,std::array{
        0x3235c0u,0x325170u,0x3252d0u,0x325410u,0x3249e0u,0x323420u,
        0x31dd70u,0x324bc0u,0x324d90u,0x324be0u,0x324d30u,0x324f00u,
        0x324f50u,0x324c40u,0x324c90u,0x3238b0u,0x323900u,0x323950u,
        0x3239a0u,0x323b20u,0x3239f0u,0x323a00u,0x323a10u,0x323a70u,
        0x324f80u,0x3250a0u,0x324de0u,0x324db0u,0x324e60u,0x325810u,0x323ab0u},
        "native TU1.5 authored canonical 320E64 physical owner typed dispatch identity");
    declare_dispatch(0x41f360u,std::array{
        0x3235c0u,0x325170u,0x3252d0u,0x325410u,0x3255d0u,0x323420u,
        0x2e8cf0u,0x324bc0u,0x324d90u,0x324be0u,0x324d30u,0x324f00u,
        0x324f50u,0x324c40u,0x324c90u,0x3238b0u,0x323900u,0x323950u,
        0x3239a0u,0x323b20u,0x3239f0u,0x323a00u,0x323a10u,0x323a70u,
        0x324f80u,0x3250a0u,0x324de0u,0x324db0u,0x324e60u,0x325810u,0x323ab0u},
        "native TU1.5 authored canonical 321340 physical owner typed dispatch identity");
    std::array<std::byte,4> binding{};
    for(unsigned i=0;i<4;++i)binding[i]=std::byte(tick.address>>(i*8));
    memory.mount(0x41b774u,binding,SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 world tick-count data binding");
    memory.mount(tick.address,tick.bytes,SourceRuntimeMemory::Access::ReadWrite,tick.owner);
    // Complete source environment cold declarations: loader-zeroed writable
    // destinations and authenticated immutable operands, never warm view data.
    memory.mount(0x4f2d88u,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold environment flags and current view position");
    memory.mount(0x4f2d98u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold environment membership control");
    memory.mount(0x4f3008u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold environment interpolation scalars");
    memory.mount(0x556554u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold CRT directional math scalar destination");
    memory.mount(0x556e10u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold CRT fog spread scalar destination");
    share_immutable_word(memory,0x46a0ccu,0x3f000000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a0d0u,0x3f800000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a0dcu,0xbf800000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a0e8u,0x0u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a0ecu,0x38d1b717u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a0f0u,0x0u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a0f4u,0x0u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a110u,0x41200000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a188u,0x3f7ff972u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a1d8u,0x3e000000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a2ecu,0x40490fdbu,
        "native TU1.5 authored complete environment producer operand");
    // Whole18B580 packs the sound-scenery inner/outer cone angles reached
    // by18F9B0. Literal TU1.5 .rdata bits, independently pinned by the CPU
    // fixture; no spatial admission, listener or voice outcome is supplied.
    share_immutable_word(memory,0x46a584u,0x46a2f8e0u,
        "native TU1.5 authored sound-scenery angle scale operand");
    share_immutable_word(memory,0x46a43cu,0x447fc000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x46a440u,0x44800000u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x474c34u,0x44aaf8u,
        "native TU1.5 authored complete environment producer operand");
    share_immutable_word(memory,0x44aaf8u,0x3f800000u,
        "native TU1.5 authored environment default vector");
    share_immutable_word(memory,0x44aafcu,0x0u,
        "native TU1.5 authored environment default vector");
    share_immutable_word(memory,0x44ab00u,0x0u,
        "native TU1.5 authored environment default vector");
    share_immutable_word(memory,0x44ab04u,0x0u,
        "native TU1.5 authored environment default vector");
    // Original2BA8C..2BA96 publishes exactly0x48dwords here. This
    // complete288-byte destination lies in authenticated .data loader-zero
    // BSS, beyond the raw section extent. No producer result is initialized.
    memory.mount(0x4c6710u,std::array<std::byte,0x120>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold complete main environment publication destination");
    // Direct four-byte floating inputs in the already reached complete
    // FADC0/320510/1F5780/1F4970 bodies. Existing exact immutable owners are
    // borrowed; only absent literal operands receive new cold declarations.
    for(const auto [address,bits]:std::array{
        std::pair{0x41f064u,0x7f7fffffu},
        std::pair{0x449fd4u,0x3fc00000u},
        std::pair{0x44a8a0u,0x3e19999au},
        std::pair{0x44aac8u,0xff7fffffu},
        std::pair{0x46a0ccu,0x3f000000u},
        std::pair{0x46a0d0u,0x3f800000u},
        std::pair{0x46a0e4u,0x40400000u},
        std::pair{0x46a0e8u,0x00000000u},
        std::pair{0x46a108u,0x41700000u},
        std::pair{0x46a11cu,0x40000000u},
        std::pair{0x46a124u,0x41400000u},
        std::pair{0x46a12cu,0x3d4ccccdu},
        std::pair{0x46a16cu,0x3e4ccccdu},
        std::pair{0x46a180u,0x3a83126fu},
        std::pair{0x46a18cu,0x3c23d70au},
        std::pair{0x46a19cu,0x40100000u},
        std::pair{0x46a240u,0x3ecccccdu},
        std::pair{0x46a2b4u,0x3fe66666u},
        std::pair{0x46a610u,0x322bcc76u},
        std::pair{0x46a794u,0x3f3504f3u},
        std::pair{0x46aa2cu,0xbd4ccccdu},
        std::pair{0x46aaf8u,0x40500000u},
        std::pair{0x46aafcu,0xbfb33333u},
        std::pair{0x46ab00u,0xbfe66666u},
        std::pair{0x46ab04u,0x3f350481u}})
        share_immutable_word(memory,address,bits,
            "native TU1.5 authored reached-body direct floating operand");
    // Preserve already published exact455A90/+4 immutable operand owners;
    // only the existing input constructor's cold destinations are composed.
    mount_native_tu15_input_startup_data(memory);
    // Actual15805F..158076 bounds the submitted mode to0..4 before this
    // five-entry readonly dispatch. These are authored .text data words;
    // the existing157FE0 body still selects and executes each real branch.
    memory.mount(0x158220u,encode(std::array<std::uint32_t,5>{
        0x15807du,0x15807du,0x1580d0u,0x158089u,0x1580dfu}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored five camera mode dispatch targets");
    // Absolute scalar operands of the authenticated vehicle body. Their
    // actual TU1.5 instruction addresses, initial bits and section permissions
    // are recorded in original-vehicle-render-operands.json.
    for(const auto&[address,bits]:std::array{
        std::pair{0x46a0d0u,0x3f800000u},
        std::pair{0x46a0dcu,0xbf800000u},
        std::pair{0x46a0e8u,0x0u},
        std::pair{0x46a0ecu,0x38d1b717u},
        std::pair{0x46a10cu,0x40c00000u},
        std::pair{0x46a164u,0x3c8efa35u},
        std::pair{0x46a2ecu,0x40490fdbu},
        std::pair{0x46a488u,0xc0490fdbu},
        std::pair{0x46a4c0u,0x3db851ecu},
        std::pair{0x46a518u,0xbf490fdbu},
        std::pair{0x46a550u,0x3f490fdbu},
        std::pair{0x46a5f4u,0x3fa2f983u},
        std::pair{0x46a5f8u,0x3ff5be0bu}})
        share_immutable_word(memory,address,bits,"native TU1.5 authored vehicle scalar operand");
    memory.mount(0x473e1cu,encode(std::array{0x3fc00000u,0x3fc00000u}),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 authored writable vehicle scale operands");
    memory.mount(0x473e34u,encode(std::array{0x3ecccccdu}),
        SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 authored writable vehicle clamp operand");
    // Complete original vehicle FP arithmetic also reads these authored doubles.
    memory.mount(0x46a600u,encode(std::array{0xa0000000u,0x3f91df46u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored complete vehicle double operand");
    memory.mount(0x46ab70u,encode(std::array{0x40000000u,0x3fd33333u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored complete vehicle double operand");
    // Actual F25D3/F25D6 bounds the authored vehicle kind to0..6.
    // The retained body still executes all seven original branch targets.
    memory.mount(0xf2bfcu,encode(std::array{0xf25dfu,0xf25f0u,0xf2649u,0xf2600u,0xf2610u,0xf2625u,0xf2635u}),
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored seven vehicle constructor dispatch targets");
    // Only verified uncovered literal inputs of the admitted vehicle closure.
#include "gameplay/vehicles/native_tu15_vehicle_closure_operands.inc"
    // Admit shared words after the complete earlier owners have been mounted.
    // Public HS squad placement retains the original difficulty/type switches;
    // the shared arithmetic table must not acquire a second overlapping owner.
#include "campaign/native_tu15_campaign_runtime_tables.inc"
#include "gameplay/weapons/native_tu15_weapon_runtime_tables.inc"
    // Authenticated22D78 MOVSS reads exactly this DWORD from TU1.5 .rdata.
    // The original22CD0 PRT arithmetic and every produced result stay intact.
    share_immutable_word(memory,0x46a680u,0x49800000u,
        "native TU1.5 authored22CD0 PRT arithmetic scalar");
#include "rendering/models/native_tu15_model_render_readonly_literals.inc"
    // Share exact physics literals only after the named immutable providers
    // have declared their owners; the profile validates before filling holes.
    mount_native_tu15_physics_branch_data(memory);
    // Pickup/action notifications read the defaults owned by680C0 even in
    // a local match. Only loader-zero storage is admitted here; the live
    // executor runs that genuine constructor before publishing game objects.
    mount_native_tu15_network_configuration_storage(memory);
    //165860 reads the saved-session initialized byte during actual player
    //continuation. Share53240's whole cold destination with network startup.
    mount_native_tu15_network_saved_session_storage(memory);
    // Weapon prompts enter1A9D90 directly before the full cache startup.
    // Keep the authored unset marker; its real platform query and11F440
    // conversion still own publishing a resolved game-language value.
    mount_native_tu15_game_language_storage(memory);
    mount_native_tu15_multiplayer_branch_data(memory);
#include "gameplay/multiplayer/native_tu15_objective_presentation_data.inc"
    return memory;
}
}
