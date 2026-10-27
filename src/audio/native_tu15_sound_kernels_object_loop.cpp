// Native kernels: object looping-sound update (TU1.5 sound manager). The
// routines live in the object_loop_{frame,update,sources} files; this file
// resolves their original addresses and registers the group's entries.
#include "audio/native_tu15_sound_object_loop.hpp"
#include "halo2/source_object_loop_audio_tu15.hpp"
#include <cstdlib>
#include <cstring>
#include <map>

namespace halo2::native_tu15_sound {
namespace {
thread_local const Views *bound_views{};
const std::map<std::uint32_t,LoopRoutine> &routines() {
    static const auto table=[]{
        // HALO2_NATIVE_OBJECT_LOOP_PARTS (frame,update,sources) narrows the
        // native parts to bisect a live oracle difference; default all.
        const auto *parts=std::getenv("HALO2_NATIVE_OBJECT_LOOP_PARTS");
        const auto enabled=[&](const char *part){return !parts || std::strstr(parts,part);};
        std::map<std::uint32_t,LoopRoutine> t;
        const std::pair<const char *,std::span<const LoopRoutineEntry>> groups[]{
            {"frame",object_loop_frame_routines()},{"update",object_loop_update_routines()},
            {"sources",object_loop_source_routines()}};
        for(const auto &[part,group]:groups)if(enabled(part))
            for(const auto &routine:group)if(!t.emplace(routine.address,routine.run).second)
                throw FormatError("Object loop routine registered twice");
        return t;
    }();
    return table;
}
template<std::uint32_t address> void entry(M &m,Abi &a,FP fp,const O &o,const Services &services) {
    LoopContext c{m,fp,o,services,bound_views};
    object_loop_call(c,a,address);
}
// Every entry of the lifted group, registered once its routine is native.
constexpr std::uint32_t entries[]{0x129170u,0x129dd0u,0x18b580u,0x18d390u,0x18d3d0u,0x18d5a0u,0x18d960u,0x18dc70u,
    0x18e970u,0x18ed80u,0x18ee50u,0x18f510u,0x18f900u,0x18f9b0u,0x18ffb0u,0x190460u,0x1904d0u,0x190a80u,
    0x220640u,0x220c60u,0x223da0u,0x226370u,0x22c560u};
template<std::size_t... n> constexpr auto all(std::index_sequence<n...>) {
    // The group calls into the rest of the live world, so its live check is
    // the frame-level snapshot oracle, not a copy-on-write prediction.
    return std::array<Kernel,sizeof...(n)>{Kernel{entries[n],entry<entries[n]>,{},true}...};
}
constexpr auto group_kernels=all(std::make_index_sequence<std::size(entries)>{});
} // namespace

void object_loop_call(LoopContext &c,Abi &a,std::uint32_t address) {
    if(const auto found=routines().find(address);found!=routines().end()){found->second(c,a);return;}
    if(address==0x253070u && native_tu15_sound_manager_owns_entry(address)) {   // listener group
        native_tu15_sound_manager_run(c.m,address,a,c.fp,c.o,c.services);return;
    }
    if(source_object_loop_audio_tu15_owns_entry(address)) {
        // Only a part narrowed away by HALO2_NATIVE_OBJECT_LOOP_PARTS runs its
        // lifted original; with every part enabled the group is native.
        if(!std::getenv("HALO2_NATIVE_OBJECT_LOOP_PARTS"))
            throw FormatError("Object loop routine has no native implementation");
        source_run_object_loop_audio_tu15(c.m,address,a,c.fp,c.services,c.o,c.views);return;
    }
    // Every other callee runs in the live world, whose hooks (the 220C60
    // loop-sound request observer among them) see the call as the original's.
    if(!c.services.required_owner)throw FormatError("Object loop callee has no live owner");
    c.services.required_owner(c.m,{u32(c.m,a.gp[4])-5u,address,false},a,c.o);
}
const Views *object_loop_views(){return bound_views;}
std::span<const Kernel> object_loop_kernels() {
    // Registered only once every routine of the group is native, so the
    // live world never routes a partially converted group natively.
    static const bool complete=[]{
        for(const auto address:entries)if(!routines().contains(address))return false;
        return true;
    }();
    return complete?std::span<const Kernel>(group_kernels):std::span<const Kernel>{};
}
} // namespace halo2::native_tu15_sound

namespace halo2 {
void native_tu15_object_loop_run(SourcePhysicsContactMemory &m,std::uint32_t address,SourcePhysicsContactRuntimeAbi &a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices &services,
    const SourceObjectAudioViewsTu15 *views) {
    using namespace native_tu15_sound;
    const auto *previous=bound_views;bound_views=views;
    struct Restore{const Views *p;~Restore(){bound_views=p;}} restore{previous};
    const O none{};
    LoopContext c{m,fp,none,services,views};
    object_loop_call(c,a,address);
}
// The native object-audio frame (18EE50) with its camera views.
void native_tu15_object_audio_frame(SourcePhysicsContactMemory &m,SourcePhysicsContactRuntimeAbi &a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices &services,
    const SourceObjectAudioViewsTu15 *views) {
    native_tu15_object_loop_run(m,0x18ee50u,a,fp,services,views);
}
} // namespace halo2
