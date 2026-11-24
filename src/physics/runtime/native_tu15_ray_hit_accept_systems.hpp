#pragma once
// Native TU1.5 ray hit accept, entered at 185820 (physics live support unit):
// a ray query's candidate hit kept when it is nearer and faces the ray.
//
//   ray_hit_accept::accept   185820(ECX = query; body, hit*) -> RET 8
//
// A hit before the ray's end (its fraction +0x14 below [46A0D0]) and nearer
// than the best so far (the query's +8 record, fraction +0x18) is weighed:
// with an ignored object (the query's +0xC not NONE) a hit whose body
// (186C10, ECX the body) is an object's (+0x18 = 1) and whose object (+0x20,
// 1D7530 through the dispatch) is the ignored one is passed over. A hit
// whose normal (+0, +4, +8) faces against the ray (the dot with the
// record's direction +0xC, +0x10, +0x14 below 0; COMISS) becomes the best:
// the query's hit flag (+0x10) is set, the record takes the fraction and the
// normal (+0x1C), and the query's fraction (+4) the fraction. The new
// fraction and normal live in the frame and the hit argument's slot.
//
// The single-precision arithmetic is the original's scalar SSE, in its order
// (the first operand the destination), on the live MXCSR. This is the game
// logic only. The game's memory is the retained TU1.5 bank, reached through
// a Memory object with typed loads and stores; every access names its role
// (Access); the boundary adapter (native_tu15_ray_hit_accept.hpp) maps the
// role to the original instruction and owns the ABI of the children. The
// values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_ray_hit_accept {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}
namespace hit {inline constexpr std::uint32_t nx=0u,ny=4u,nz=8u,fraction=0x14u;}
namespace query {inline constexpr std::uint32_t fraction=4u,best=8u,ignored=0xcu,found=0x10u;}
namespace best {inline constexpr std::uint32_t dx=0xcu,dy=0x10u,dz=0x14u,fraction=0x18u,normal=0x1cu;}
namespace body {inline constexpr std::uint32_t kind=0x18u,object=0x20u;inline constexpr std::uint32_t object_kind=1u;}
namespace argument {inline constexpr std::uint32_t body=0u,hit=1u;}
// The frame's slots, by offset from the return address: the normal.
namespace local {inline constexpr std::int32_t nx=-0xc,ny=-8,nz=-4;}
inline constexpr std::uint32_t zero_float=0u;

namespace ray_hit_accept {
enum class Access {one,hit_argument,hit_fraction,hit_fraction_again,best,best_fraction,fraction_store,ignored,body_argument,body_kind,
    fraction_load,body_object,hit_nz,hit_ny,best_dz,hit_nx,nz_store,best_dy,ny_store,best_dx,nx_store,nx_load,ny_load,found_store,
    best_fraction_store,best_again,normal_x,nz_load,normal_y,normal_z,query_fraction};
enum class Outcome {beyond,farther,ignored,facing,accepted};
// The original's EBX (the hit), EBP (the ignored object), ESI (the query) and EDI (the best) across the children.
struct Kept {std::uint32_t hit,ignored,query,best;};
// Frame: the hooks name the original instructions they stand for; the children refresh kept.
template<class Memory,class Frame> Outcome accept(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    const auto one_value=memory.load32(Access::one,globals::one);
    frame.framing(one_value);
    kept.hit=memory.load32(hit_argument,frame.argument(argument::hit));
    const auto fraction=memory.load32(hit_fraction,at(kept.hit,hit::fraction));
    frame.compared(one_value,fraction);
    frame.saving(1u);
    if(sse::not_above(one_value,fraction)){frame.restoring(0u);return Outcome::beyond;}
    const auto new_fraction=memory.load32(hit_fraction_again,at(kept.hit,hit::fraction));
    frame.saving(2u);
    kept.best=memory.load32(Access::best,at(kept.query,query::best));
    const auto best_value=memory.load32(best_fraction,at(kept.best,best::fraction));
    frame.weighed(best_value,new_fraction);
    memory.store32(fraction_store,frame.argument(argument::hit),new_fraction);
    if(sse::not_above(best_value,new_fraction)){frame.restoring(1u);return Outcome::farther;}
    frame.saving(3u);
    kept.ignored=memory.load32(Access::ignored,at(kept.query,query::ignored));
    frame.compared32(kept.ignored,none);
    std::uint32_t object_handle=0u;
    auto fraction_now=new_fraction;
    if(kept.ignored!=none) {
        const auto body_value=memory.load32(body_argument,frame.argument(argument::body));
        const auto entry=frame.body(body_value,kept);
        const auto kind=memory.load32(body_kind,at(entry,body::kind));
        frame.compared32(kind,body::object_kind);
        fraction_now=memory.load32(fraction_load,frame.argument(argument::hit));
        frame.reloaded(fraction_now);
        if(kind==body::object_kind)object_handle=memory.load32(body_object,at(entry,body::object));
        else frame.cleared();
    } else frame.cleared();
    frame.tested(object_handle);
    if(object_handle) {
        const auto owner=frame.object_of(object_handle,kept);
        fraction_now=frame.xmm3();   // the fraction, in XMM3 as 1D7530 returns it
        frame.compared32(owner,kept.ignored);
        if(owner==kept.ignored){frame.restoring(2u);return Outcome::ignored;}
    }
    // Facing: the hit's normal against the best record's direction, summed z, y, x.
    const auto nz=memory.load32(hit_nz,at(kept.hit,hit::nz));
    const auto ny=memory.load32(hit_ny,at(kept.hit,hit::ny));
    const auto dz=memory.load32(best_dz,at(kept.best,best::dz));
    const auto nx=memory.load32(hit_nx,at(kept.hit,hit::nx));
    const auto z_term=sse::multiply(dz,nz);
    memory.store32(nz_store,frame.local(local::nz),nz);
    const auto dy=memory.load32(best_dy,at(kept.best,best::dy));
    const auto y_term=sse::multiply(dy,ny);
    memory.store32(ny_store,frame.local(local::ny),ny);
    const auto dx=memory.load32(best_dx,at(kept.best,best::dx));
    const auto x_term=sse::multiply(dx,nx);
    memory.store32(nx_store,frame.local(local::nx),nx);
    const auto dot=sse::add(sse::add(z_term,y_term),x_term);
    frame.dotted(dot,y_term,x_term);
    if(sse::not_above(zero_float,dot)){frame.restoring(2u);return Outcome::facing;}
    const auto nx_value=memory.load32(nx_load,frame.local(local::nx));
    const auto ny_value=memory.load32(ny_load,frame.local(local::ny));
    memory.store8(found_store,at(kept.query,query::found),1u);
    memory.store32(best_fraction_store,at(kept.best,best::fraction),fraction_now);
    const auto normal=frame.normal_slot(memory.load32(best_again,at(kept.query,query::best)));
    memory.store32(normal_x,normal,nx_value);
    const auto nz_value=memory.load32(nz_load,frame.local(local::nz));
    memory.store32(normal_y,normal+4u,ny_value);
    memory.store32(normal_z,normal+8u,nz_value);
    memory.store32(query_fraction,at(kept.query,query::fraction),fraction_now);
    frame.restoring(2u);
    return Outcome::accepted;
}
} // namespace ray_hit_accept
} // namespace halo2::native_ray_hit_accept
