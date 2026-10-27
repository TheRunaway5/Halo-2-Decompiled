#pragma once
#include "halo2/native_tu15_contact_agent_dispatch.hpp"
#include "halo2/native_tu15_array_growth.hpp"
namespace halo2 {
// Actual constant methods0A9520 and0AB4B0. Their incoming receiver and FP
// state are unread; these are native owners of those authenticated bodies.
std::uint32_t native_tu15_contact_child_class_23() noexcept;
std::uint32_t native_tu15_contact_child_class_24() noexcept;
enum class NativeTu15ContactAgentFactory {owner_303c10,owner_3040d0};
struct NativeTu15ContactAgentFactoryArguments {
    // The source reads its fourth cdecl input after the real allocation and
    // size-word publication. This reads input only, never an expected result.
    std::function<std::uint32_t()> user_data;
};
// Actual2F3620. Only the verified fields are written, in source order. The
// header bytes04..07 and18..1F remain their existing input values.
// The two actual MOVAPS stores require16-byte object alignment at their own
// publication phases, after the preceding scalar stores have committed.
NativeTu15ContactAgent native_tu15_initialize_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgent,std::uint32_t user_data,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2F0380. Reread the genuine allocator/table and16-bit size+4, then
// release through the actual allocator+14 owner with memory class1A.
void native_tu15_release_contact_agent(SourcePhysicsContactMemory&,NativeTu15ContactAgent,
    NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&);
// Actual303C10/3040D0. First three cdecl inputs are unused by both source
// owners. Allocate64 genuine bytes with class1A, publish the16-bit size40,
// read user_data, arm native cleanup, construct2F3620, then publish the actual
// derived vtable. No omitted byte is initialized and no FS chain is emulated.
NativeTu15ContactAgent native_tu15_create_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgentFactory,const NativeTu15ContactAgentFactoryArguments&,
    NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
NativeTu15ContactAgent native_tu15_create_contact_agent(SourcePhysicsContactMemory&,
    NativeTu15ContactAgentFactory,std::uint32_t user_data,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
