#pragma once
// Native TU1.5 particle update systems (group E of
// docs/native_replacement/REMAINING_TRANSLATED_20261115.md):
//
//   location_update::run    251A60  one particle location's step: every
//                                   emitter's particles (emitter_update), the
//                                   release of a location nobody drew, and the
//                                   location's bounding sphere
//   location_update::emitter_update
//                           250450  one emitter's particles over the time step
//
// The particle state lives in three data arrays (the names follow
// source_particle_render_state_tu15.cpp, which reads the same records):
// particle systems (51DF74, 84 bytes) own a chain of locations (52D71C, 52
// bytes), each location owns a chain of emitters (52D718, 76 bytes), and each
// emitter owns a chain of particles (52D714, 64 bytes). The particle system
// update (1778D0) calls location_update::run for every location of a system
// with the system's definition.
//
// This is the game logic only, in the style of native_tu15_effect_owners_
// systems.hpp: memory is reached through a Memory object with one access
// role per original access, in the original order and width; the stack
// arguments and locals, the genuine child calls and the inline call's entry
// and return come from a Frame object; registers, flags and stack slots
// belong to the boundary adapter (native_tu15_particle_update.hpp).
//
// What stays exact, because translated neighbours observe it:
//   - every access happens in the original order and width, including the
//     re-reads of the arguments and locals the original makes;
//   - the single-precision arithmetic runs the original SSE instructions with
//     the original operand order (sse::), each between the same two memory
//     accesses as in the original, so NaN payloads, rounding, DAZ/FTZ and the
//     MXCSR sticky flags match, at a faulting access too;
//   - the bounding radius is the original x87 sequence, in extended precision
//     under the live control word, with its two single-precision stores;
//   - the values the original keeps in callee-saved registers across a child
//     call are "kept" structs, handed back by the adapter after the call, so
//     the logic continues exactly as the original does even when a child does
//     not preserve them.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include "rendering/effects/native_tu15_effect_owners_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

namespace halo2::native_effects {
namespace sse {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// COMISS a,b then JB: a < b, or unordered.
inline bool below_or_unordered(float a,float b) {
    unsigned char result;
    asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(result):"x"(_mm_set_ss(a)),"x"(_mm_set_ss(b)):"cc","memory");
    return result;
}
#else
inline bool below_or_unordered(float,float){unavailable();}
#endif
} // namespace sse

namespace x87 {
// The location's bounding radius (251A60's tail): the sprite or model's
// radius scale (the value its +4C method leaves in ST(0)) times the largest
// particle size, plus the length of the box's half extent. The original
// loads every single-precision operand with FLD m32 and multiplies from a
// register; it spills the length (FST m32) before the final sum and stores
// the sum (FSTP m32). Both stores report their C1.
struct Radius {
    float extent;            // FST m32: the half extent's length
    bool extent_rounded_up;
    float radius;            // FSTP m32: the bounding radius
    bool radius_rounded_up;
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline Radius bounding_radius(long double scale,float size,float z,float z_again,float y,float y_again,float x,
                              float x_again) {
    Radius out;std::uint16_t first,second;
    asm volatile(
        "fldt %[scale]\n\tflds %[size]\n\tfmulp %%st,%%st(1)\n\t"
        "flds %[z]\n\tflds %[z2]\n\tfmulp %%st,%%st(1)\n\t"
        "flds %[y]\n\tflds %[y2]\n\tfmulp %%st,%%st(1)\n\tfaddp %%st,%%st(1)\n\t"
        "flds %[x]\n\tflds %[x2]\n\tfmulp %%st,%%st(1)\n\tfaddp %%st,%%st(1)\n\t"
        "fsqrt\n\tfsts %[extent]\n\tfnstsw %[first]\n\t"
        "faddp %%st,%%st(1)\n\tfstps %[radius]\n\tfnstsw %[second]"
        :[extent]"=m"(out.extent),[radius]"=m"(out.radius),[first]"=m"(first),[second]"=m"(second)
        :[scale]"m"(scale),[size]"m"(size),[z]"m"(z),[z2]"m"(z_again),[y]"m"(y),[y2]"m"(y_again),[x]"m"(x),
         [x2]"m"(x_again)
        :"st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","memory");
    out.extent_rounded_up=(first&0x200u)!=0;out.radius_rounded_up=(second&0x200u)!=0;
    return out;
}
#else
inline Radius bounding_radius(long double,float,float,float,float,float,float,float) {
    throw std::runtime_error("Native effect systems need the x86 x87 backend");
}
#endif
} // namespace x87

namespace location_update {
namespace globals {
inline constexpr std::uint32_t particles=0x52d714u;       // DataArray* of ParticleDatum
inline constexpr std::uint32_t emitters=0x52d718u;        // DataArray* of EmitterDatum
inline constexpr std::uint32_t tag_instances=0x4f0414u;   // TagInstance*: indexed by a tag handle's low word
inline constexpr std::uint32_t frame_stamp=0x4c6854u;     // int32: compared with a location's drawn_frame
inline constexpr std::uint32_t one=0x46a0d0u;             // float 1
inline constexpr std::uint32_t half=0x46a0ccu;            // float 0.5
inline constexpr std::uint32_t zero=0x46a0e8u;            // float 0
inline constexpr std::uint32_t rest_speed_squared=0x46a4b0u;  // float 1/16: a landed particle slower than this rests
// The two particle definition interfaces 13A6B0 selects by the particle
// tag's group; their vtables hold the methods this code calls.
inline constexpr std::uint32_t sprite_interface=0x48603cu;
inline constexpr std::uint32_t model_interface=0x486048u;
}
// Tag groups of a particle system's particle tag.
inline constexpr std::uint32_t sprite_group=0x70727433u;  // 'prt3'
inline constexpr std::uint32_t model_group=0x5052544du;   // 'PRTM'
// Methods of a particle definition interface (vtable offsets).
namespace interface_methods {
inline constexpr std::uint32_t resting_particle_dies=0x34u;  // (particle) -> bool, see emitter_update
inline constexpr std::uint32_t radius_scale=0x4cu;           // () -> float in ST(0)
inline constexpr std::uint32_t select_tag=0x50u;             // (tag handle), callee pops it
}

// A tag instance (16 bytes): the group tag at +0, the loaded definition at +8.
struct TagInstance {
    std::uint32_t group;
    std::uint8_t unknown_04[4];
    std::uint32_t address;
    std::uint8_t unknown_0c[4];
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,address)==8);

// A particle (64 bytes; ParticleSimulationState in particle_creation.hpp).
struct ParticleDatum {
    std::uint16_t salt;
    std::uint16_t flags;          // +2: particle_flags
    std::uint32_t next;           // +4: the next particle of the emitter
    float age;                    // +8: 0 at birth, 1 at the end of its life
    float inverse_lifetime;       // +C: the age gained per second
    float system_fraction;        // +10
    std::uint16_t random[4];      // +14
    float position[3];            // +1C
    float velocity[3];            // +28
    float rotation_rate;          // +34: the phase gained per second
    float phase;                  // +38
    std::uint32_t seed_color;     // +3C
};
static_assert(sizeof(ParticleDatum)==0x40 && offsetof(ParticleDatum,flags)==2 && offsetof(ParticleDatum,next)==4 &&
              offsetof(ParticleDatum,age)==8 && offsetof(ParticleDatum,inverse_lifetime)==0xc &&
              offsetof(ParticleDatum,position)==0x1c && offsetof(ParticleDatum,velocity)==0x28 &&
              offsetof(ParticleDatum,rotation_rate)==0x34 && offsetof(ParticleDatum,phase)==0x38);
namespace particle_flags {
inline constexpr std::uint32_t dead=0x1u;      // released at the end of this step
inline constexpr std::uint32_t resting=0x8u;   // stopped moving; its interface may kill it
inline constexpr std::uint32_t landed=0x10u;   // the collision step (2C40D0) put it on a floor
}

// An emitter (76 bytes; ParticleEmitterState in particle_emitter.hpp).
struct EmitterDatum {
    std::uint16_t salt;
    std::uint16_t particle_count;  // +2
    std::uint32_t first_particle;  // +4
    std::uint32_t next_emitter;    // +8
    std::uint8_t unknown_0c[0x40];
};
static_assert(sizeof(EmitterDatum)==0x4c && offsetof(EmitterDatum,first_particle)==4 &&
              offsetof(EmitterDatum,next_emitter)==8);

// A particle location (52 bytes).
struct LocationDatum {
    std::uint8_t unknown_00[4];
    std::uint32_t first_emitter;   // +4
    std::uint8_t unknown_08[8];
    float position[3];             // +10: where its emitters are placed
    float lod;                     // +1C: positive while the location is drawn in detail
    float center[3];               // +20: the bounding sphere this step computes
    float radius;                  // +2C
    std::int32_t drawn_frame;      // +30: the last frame stamp that drew it
};
static_assert(sizeof(LocationDatum)==0x34 && offsetof(LocationDatum,first_emitter)==4 &&
              offsetof(LocationDatum,position)==0x10 && offsetof(LocationDatum,lod)==0x1c &&
              offsetof(LocationDatum,center)==0x20 && offsetof(LocationDatum,radius)==0x2c &&
              offsetof(LocationDatum,drawn_frame)==0x30);

// A particle system (84 bytes): only the flags byte at +C is read here.
struct SystemDatum {
    std::uint8_t unknown_00[0xc];
    std::uint8_t flags;            // +C: system_flags
    std::uint8_t unknown_0d[0x47];
};
static_assert(sizeof(SystemDatum)==0x54 && offsetof(SystemDatum,flags)==0xc);
namespace system_flags {
inline constexpr std::uint32_t cycling=0x1u;     // with `cycles`: ages wrap instead of ending (meaning not established)
inline constexpr std::uint32_t updating=0x8u;    // the particles step; cleared once one dies, if the definition asks
inline constexpr std::uint32_t cycles=0x40u;     // see `cycling`
}

// A particle system definition (a 0x38-byte row; ParticleSystemRenderDefinition).
struct SystemDefinition {
    std::uint8_t unknown_00[4];
    std::uint32_t particle_tag;      // +4: a sprite ('prt3') or model ('PRTM') particle
    std::uint8_t unknown_08[0xe];
    std::uint8_t flags;              // +16: definition_flags (the low byte of its flags word)
    std::uint8_t flags_high;         // +17: definition_flags_high
    std::uint8_t unknown_18[0x18];
    std::int32_t emitter_count;      // +30
    std::uint32_t emitters;          // +34: EmitterDefinition[emitter_count]
};
static_assert(sizeof(SystemDefinition)==0x38 && offsetof(SystemDefinition,particle_tag)==4 &&
              offsetof(SystemDefinition,flags)==0x16 && offsetof(SystemDefinition,emitter_count)==0x30 &&
              offsetof(SystemDefinition,emitters)==0x34);
namespace definition_flags {
inline constexpr std::uint32_t stop_on_death=0x1u;     // a dying particle stops the system's updating
inline constexpr std::uint32_t keep_undrawn=0x2u;      // keep the particles of a location nobody drew
}
namespace definition_flags_high {
inline constexpr std::uint32_t cycle_while_drawn=0x1u;  // ages wrap while the location's LOD is positive
}

// An emitter definition (0xB8 bytes).
struct EmitterDefinition {
    std::uint8_t unknown_00[4];
    std::uint32_t physics_tag;       // +4: the particle physics (2C40D0's definition), NONE for none
    std::uint8_t unknown_08[0x48];
    std::uint32_t size_function_size;  // +50
    std::uint32_t size_function;     // +54: SizeFunction*
    std::uint8_t unknown_58[0x60];
};
static_assert(sizeof(EmitterDefinition)==0xb8 && offsetof(EmitterDefinition,physics_tag)==4 &&
              offsetof(EmitterDefinition,size_function)==0x54);
// The head of the emitter's particle size function: its range flags and its
// lower and upper values. A function whose flags' high nibble is set has the
// range [0, 1] instead.
struct SizeFunction {
    std::uint8_t type;
    std::uint8_t flags;              // +1
    std::uint8_t unknown_02[2];
    float lower;                     // +4
    float upper;                     // +8
};
static_assert(offsetof(SizeFunction,flags)==1 && offsetof(SizeFunction,lower)==4 && offsetof(SizeFunction,upper)==8);
constexpr bool unit_range(std::uint32_t flags){return ((flags&0xffu)>>4)!=0;}

// The location's box, grown by every live particle (the caller's locals).
struct Bounds {
    float x_minimum,x_maximum,y_minimum,y_maximum,z_minimum,z_maximum;
};
static_assert(sizeof(Bounds)==0x18);

// ---- emitter_update (250450) ------------------------------------------------
// The stack arguments: the emitter, the particle system, the location, the
// system definition, the time step and the location's box (whose slot's low
// byte is reused as the "a particle died" flag).
enum class Argument {emitter,system,location,definition,time_step,bounds};
// The locals of the original's frame.
enum class Local {physics,interface,previous,next,cycle_spill,lod_spill};
enum class EmitterAccess {
    physics_tag,bounds_argument,tag_table,physics_definition,physics_store,physics_clear,
    definition_argument,particle_tag,system_argument,system_updating,interface_store,
    emitter_argument,first_particle,previous_clear,died_clear,
    // per particle
    interface_reload,particle_table,time_step,one,particle_data,inverse_lifetime,next,age,next_store,age_store,
    system_cycles,system_cycling,cycle_spill,cycle_reload,
    definition_reload,definition_cycles,location_argument,location_lod,zero,lod_spill,lod_reload,
    age_reload,age_fraction,
    dead_flags,dead_flags_store,died_set,
    flags,age_check,velocity_x,velocity_y,velocity_z,rest_threshold,rest_store,
    move_velocity_x,move_position_x,move_position_x_store,move_velocity_y,move_position_y,move_position_y_store,
    move_velocity_z,move_position_z,move_position_z_store,rotation_rate,phase,unlanded_store,phase_store,
    vtable,rest_method,killed_flags,killed_flags_store,
    alive,
    x_minimum,x_check,previous_store,x_from_particle,x_kept,x_minimum_store,
    y_minimum,y_check,y_from_particle,y_kept,y_minimum_store,
    z_minimum,z_check,z_from_particle,z_kept,z_minimum_store,
    x_maximum,x_maximum_check,x_maximum_from_particle,x_maximum_store,
    y_maximum,y_maximum_check,y_maximum_from_particle,y_maximum_store,
    z_maximum,z_maximum_check,z_maximum_store,z_maximum_from_particle,z_maximum_particle_store,
    // removal of a dead particle
    release_emitter,release_system,delete_table,previous_reload,link_next,previous_link,first_next,
    first_emitter,first_link,
    // the end of each particle
    next_reload,system_reload,
    // after the particles
    physics_reload,physics_emitter,physics_first,physics_time_step,physics_location,
    definition_final,definition_stops,died_check,system_final,system_flags,system_flags_store
};

// What one call receives in its arguments (the caller pushes them).
struct EmitterCall {
    std::uint32_t definition_entry;  // the emitter's definition (EmitterDefinition*)
    std::uint32_t emitter;           // EmitterDatum*
    std::uint32_t system;            // SystemDatum*
    std::uint32_t location;          // LocationDatum*
    std::uint32_t definition;        // SystemDefinition*
    std::uint32_t time_step;         // float bits
    std::uint32_t bounds;            // Bounds*
};
// Values the original keeps in callee-saved registers across its children:
// the location's box, the current particle and its handle, and `mark`, the
// value whose low bits the original uses for its single-bit flag tests and
// updates (1).
struct EmitterKept {
    std::uint32_t bounds;
    std::uint32_t particle;
    std::uint32_t handle;
    std::uint32_t mark;
};
// The scratch values of the speed test (for the adapter).
struct SpeedScratch {
    float x_squared_sum;   // vx*vx + vy*vy, then + vz*vz
    float y_squared;
    float z_squared;
};
// The age wrap's values (for the adapter): the whole part and its float.
struct CycleScratch {
    std::int32_t whole;
    float converted;
};
// How the emitter's step ended: the system was not updating; or the system
// keeps updating (the definition does not stop on death, or none died); or a
// particle died and the system stops updating.
enum class Ending {idle,running,no_deaths,stopped};
// How a particle's step went (for the adapter's coverage).
enum class Fate {aging,expired_cycled,expired_lod_cycled,expired_died};
enum class Motion {skipped,rested,still_landed,moved};
struct ParticleStep {
    Fate fate;
    bool cycles_flagged,cycling,cycle_while_drawn,lod_positive;
    Motion motion;
    bool aged_out;               // the age check (1 < age, or unordered) skipped the motion
    bool rest_checked,rest_killed;
    bool dead;
    std::uint32_t grew;          // bit n: bounds field n took the particle's value
    bool removed_first;
};

// One emitter's particles over the time step, for every particle in the
// emitter's chain:
//   age += inverse_lifetime * dt; past 1 the particle either cycles (the age
//   keeps its fractional part: the system's `cycles` and `cycling` flags, or
//   the definition's cycle_while_drawn while the location's LOD is positive)
//   or dies;
//   a live particle whose age is within its life moves: a landed particle
//   slower than the rest speed comes to rest instead; otherwise position +=
//   velocity * dt, phase += rotation_rate * dt, and it is no longer landed;
//   a resting particle asks its interface (+34) whether it dies now;
//   a live particle grows the location's box, a dead one is unlinked,
//   released (251650) and deleted (16E8A0).
// Then the emitter's physics (2C40D0) runs over its remaining particles, and
// when a particle died and the definition stops on death, the system stops
// updating.
// Frame: argument(a) and local(l) (slot addresses), particle_interface (13A6B0),
// speed_checked, cycle_converted, resting_particle_dies (the dispatch),
// release_particle (251650), delete_particle (16E8A0), move_particles
// (2C40D0), particle_stepped (coverage) and leave_emitter_update (RET 18).
template<class Memory,class Frame> void emitter_update(Memory &memory,Frame &frame,const EmitterCall &call,EmitterKept &kept) {
    using enum EmitterAccess;
    const auto argument=[&](Argument a){return frame.argument(a);};
    const auto local=[&](Local l){return frame.local(l);};
    const auto physics_tag_value=memory.load32(physics_tag,at(call.definition_entry,offsetof(EmitterDefinition,physics_tag)));
    kept.bounds=memory.load32(bounds_argument,argument(Argument::bounds));
    if(physics_tag_value!=none) {
        const auto tags=memory.load32(tag_table,globals::tag_instances);
        const auto physics=memory.load32(physics_definition,at(element(tags,handle_index(physics_tag_value),sizeof(TagInstance)),
                                                                offsetof(TagInstance,address)));
        memory.store32(physics_store,local(Local::physics),physics);
    } else memory.store32(physics_clear,local(Local::physics),0u);
    const auto definition=memory.load32(definition_argument,argument(Argument::definition));
    auto interface=frame.particle_interface(memory.load32(particle_tag,at(definition,offsetof(SystemDefinition,particle_tag))),kept);
    auto system=memory.load32(system_argument,argument(Argument::system));
    const auto updating=memory.load8(system_updating,at(system,offsetof(SystemDatum,flags)));
    memory.store32(interface_store,local(Local::interface),interface);
    kept.mark=1u;
    if(!((updating>>3)&kept.mark&0xffu))return frame.leave_emitter_update(Ending::idle);
    const auto emitter=memory.load32(emitter_argument,argument(Argument::emitter));
    kept.handle=memory.load32(first_particle,at(emitter,offsetof(EmitterDatum,first_particle)));
    memory.store32(previous_clear,local(Local::previous),0u);
    memory.store8(died_clear,argument(Argument::bounds),0u);
    for(bool first=true;kept.handle!=none;first=false) {
        ParticleStep step{};
        if(!first)interface=memory.load32(interface_reload,local(Local::interface));
        const auto particles=memory.load32(particle_table,globals::particles);
        const auto dt=memory.load_float(time_step,argument(Argument::time_step));
        const auto one_value=memory.load_float(one,globals::one);
        kept.particle=element(memory.load32(particle_data,at(particles,offsetof(DataArray,data))),handle_index(kept.handle),
                              sizeof(ParticleDatum));
        const auto field=[&](std::size_t offset){return at(kept.particle,offset);};
        const auto rate=memory.load_float(inverse_lifetime,field(offsetof(ParticleDatum,inverse_lifetime)));
        const auto next_handle=memory.load32(next,field(offsetof(ParticleDatum,next)));
        const auto gained=sse::multiply(rate,dt);
        const auto aged=sse::add(gained,memory.load_float(age,field(offsetof(ParticleDatum,age))));
        const bool expired=sse::greater(aged,one_value);
        memory.store32(next_store,local(Local::next),next_handle);
        memory.store_float(age_store,field(offsetof(ParticleDatum,age)),aged);
        if(expired) {
            // Keep the fractional part of the age: the whole part goes through
            // the spill slot and CVTTSS2SI, and the age is read again.
            const auto cycle=[&](EmitterAccess spill,EmitterAccess reload,Local slot) {
                memory.store_float(spill,local(slot),aged);
                CycleScratch scratch{};
                scratch.whole=sse::truncate(memory.load_float(reload,local(slot)));
                const auto current=memory.load_float(age_reload,field(offsetof(ParticleDatum,age)));
                scratch.converted=sse::from_integer(scratch.whole);
                const auto fraction=sse::subtract(current,scratch.converted);
                frame.cycle_converted(scratch);
                memory.store_float(age_fraction,field(offsetof(ParticleDatum,age)),fraction);
            };
            const auto system_bits=memory.load8(system_cycles,at(system,offsetof(SystemDatum,flags)));
            step.cycles_flagged=((system_bits>>6)&kept.mark&0xffu)!=0;
            if(step.cycles_flagged)
                step.cycling=(memory.load8(system_cycling,at(system,offsetof(SystemDatum,flags)))&kept.mark&0xffu)!=0;
            if(step.cycling) {
                step.fate=Fate::expired_cycled;
                cycle(cycle_spill,cycle_reload,Local::cycle_spill);
            } else {
                const auto definition_again=memory.load32(definition_reload,argument(Argument::definition));
                step.cycle_while_drawn=(memory.load8(definition_cycles,at(definition_again,offsetof(SystemDefinition,flags_high)))&
                                        kept.mark&0xffu)!=0;
                if(step.cycle_while_drawn) {
                    const auto location=memory.load32(location_argument,argument(Argument::location));
                    const auto lod=memory.load_float(location_lod,at(location,offsetof(LocationDatum,lod)));
                    step.lod_positive=sse::greater(lod,memory.load_float(zero,globals::zero));
                }
                if(step.lod_positive) {
                    step.fate=Fate::expired_lod_cycled;
                    cycle(lod_spill,lod_reload,Local::lod_spill);
                } else {
                    // The particle dies; the slot of the box argument records it.
                    step.fate=Fate::expired_died;
                    const auto flags_now=memory.load16(dead_flags,field(offsetof(ParticleDatum,flags)));
                    memory.store16(dead_flags_store,field(offsetof(ParticleDatum,flags)),flags_now|(kept.mark&0xffffu));
                    memory.store8(died_set,argument(Argument::bounds),kept.mark);
                }
            }
        }
        // Motion. A dead or resting particle, or one past its life (1 < age,
        // or unordered), does not move.
        const auto flags_now=memory.load16(flags,field(offsetof(ParticleDatum,flags)));
        if(flags_now&(particle_flags::dead|particle_flags::resting))step.motion=Motion::skipped;
        else if(sse::below_or_unordered(one_value,memory.load_float(age_check,field(offsetof(ParticleDatum,age))))) {
            step.motion=Motion::skipped;step.aged_out=true;
        } else {
            bool moves=true;
            if(flags_now&particle_flags::landed) {
                // |velocity|^2 = (vx*vx + vy*vy) + vz*vz.
                const auto vx=memory.load_float(velocity_x,field(offsetof(ParticleDatum,velocity)));
                const auto vy=memory.load_float(velocity_y,field(offsetof(ParticleDatum,velocity)+4u));
                const auto vz=memory.load_float(velocity_z,field(offsetof(ParticleDatum,velocity)+8u));
                SpeedScratch speed{};
                speed.x_squared_sum=sse::multiply(vx,vx);
                speed.y_squared=sse::multiply(vy,vy);
                speed.x_squared_sum=sse::add(speed.x_squared_sum,speed.y_squared);
                speed.z_squared=sse::multiply(vz,vz);
                speed.x_squared_sum=sse::add(speed.x_squared_sum,speed.z_squared);
                frame.speed_checked(speed);
                moves=sse::greater(speed.x_squared_sum,memory.load_float(rest_threshold,globals::rest_speed_squared));
                if(!moves) {
                    step.motion=Motion::rested;
                    memory.store16(rest_store,field(offsetof(ParticleDatum,flags)),flags_now|particle_flags::resting);
                } else step.motion=Motion::still_landed;
            } else step.motion=Motion::moved;
            if(moves) {
                // position += velocity * dt, axis by axis; phase += rotation_rate * dt.
                const auto advance=[&](EmitterAccess rate_access,std::size_t rate_field,EmitterAccess value_access,
                                       EmitterAccess store_access,std::size_t value_field) {
                    const auto scaled=sse::multiply(memory.load_float(rate_access,field(rate_field)),dt);
                    const auto moved=sse::add(scaled,memory.load_float(value_access,field(value_field)));
                    memory.store_float(store_access,field(value_field),moved);
                };
                advance(move_velocity_x,offsetof(ParticleDatum,velocity),move_position_x,move_position_x_store,
                        offsetof(ParticleDatum,position));
                advance(move_velocity_y,offsetof(ParticleDatum,velocity)+4u,move_position_y,move_position_y_store,
                        offsetof(ParticleDatum,position)+4u);
                advance(move_velocity_z,offsetof(ParticleDatum,velocity)+8u,move_position_z,move_position_z_store,
                        offsetof(ParticleDatum,position)+8u);
                const auto spin=sse::multiply(memory.load_float(rotation_rate,field(offsetof(ParticleDatum,rotation_rate))),dt);
                const auto turned=sse::add(spin,memory.load_float(phase,field(offsetof(ParticleDatum,phase))));
                memory.store16(unlanded_store,field(offsetof(ParticleDatum,flags)),flags_now&0xffefu);
                memory.store_float(phase_store,field(offsetof(ParticleDatum,phase)),turned);
            }
        }
        if(step.motion==Motion::skipped)frame.rest_flag_tested(flags_now);
        if(step.motion==Motion::skipped && ((flags_now&0xffu)>>3)&kept.mark&0xffu) {
            // A resting particle: its interface decides whether it dies.
            step.rest_checked=true;
            const auto table_address=memory.load32(vtable,interface);
            const auto method=memory.load32(rest_method,at(table_address,interface_methods::resting_particle_dies));
            step.rest_killed=frame.resting_particle_dies(interface,table_address,method,kept);
            if(step.rest_killed) {
                const auto killed=memory.load16(killed_flags,at(kept.particle,offsetof(ParticleDatum,flags)));
                memory.store16(killed_flags_store,at(kept.particle,offsetof(ParticleDatum,flags)),killed|(kept.mark&0xffffu));
            }
        }
        step.dead=(memory.load8(alive,at(kept.particle,offsetof(ParticleDatum,flags)))&kept.mark&0xffu)!=0;
        if(!step.dead) {
            // A live particle grows the box: each minimum takes the particle's
            // coordinate when the minimum is greater (COMISS ... JA), each
            // maximum when it is not greater. The previous particle is
            // remembered between the first comparison and its result.
            const auto bounds=kept.bounds;const auto particle=kept.particle;
            const auto position=[&](unsigned axis){return at(particle,offsetof(ParticleDatum,position)+axis*4u);};
            const auto minimum=[&](unsigned bit,EmitterAccess load,EmitterAccess check,EmitterAccess from_particle,EmitterAccess kept_access,
                                   EmitterAccess store,std::size_t offset,unsigned axis,bool remember) {
                const auto current=memory.load_float(load,at(bounds,offset));
                const bool grows=sse::greater(current,memory.load_float(check,position(axis)));
                if(remember)memory.store32(previous_store,local(Local::previous),particle);
                const auto value=grows?memory.load_float(from_particle,position(axis)):memory.load_float(kept_access,at(bounds,offset));
                if(grows)step.grew|=1u<<bit;
                memory.store_float(store,at(bounds,offset),value);
            };
            minimum(0,x_minimum,x_check,x_from_particle,x_kept,x_minimum_store,offsetof(Bounds,x_minimum),0,true);
            minimum(1,y_minimum,y_check,y_from_particle,y_kept,y_minimum_store,offsetof(Bounds,y_minimum),1,false);
            minimum(2,z_minimum,z_check,z_from_particle,z_kept,z_minimum_store,offsetof(Bounds,z_minimum),2,false);
            const auto maximum=[&](unsigned bit,EmitterAccess load,EmitterAccess check,EmitterAccess from_particle,EmitterAccess store,
                                   std::size_t offset,unsigned axis) {
                auto value=memory.load_float(load,at(bounds,offset));
                if(!sse::greater(value,memory.load_float(check,position(axis)))) {
                    value=memory.load_float(from_particle,position(axis));step.grew|=1u<<bit;
                }
                memory.store_float(store,at(bounds,offset),value);
            };
            maximum(3,x_maximum,x_maximum_check,x_maximum_from_particle,x_maximum_store,offsetof(Bounds,x_maximum),0);
            maximum(4,y_maximum,y_maximum_check,y_maximum_from_particle,y_maximum_store,offsetof(Bounds,y_maximum),1);
            // The z maximum is stored from either branch's own store.
            auto z=memory.load_float(z_maximum,at(bounds,offsetof(Bounds,z_maximum)));
            if(sse::greater(z,memory.load_float(z_maximum_check,position(2))))
                memory.store_float(z_maximum_store,at(bounds,offsetof(Bounds,z_maximum)),z);
            else {
                z=memory.load_float(z_maximum_from_particle,position(2));step.grew|=1u<<5;
                memory.store_float(z_maximum_particle_store,at(bounds,offsetof(Bounds,z_maximum)),z);
            }
        } else {
            // Unlink the dead particle from the chain, release it and delete it.
            const auto owner=memory.load32(release_emitter,argument(Argument::emitter));
            const auto system_now=memory.load32(release_system,argument(Argument::system));
            frame.release_particle(system_now,owner,kept);
            frame.delete_particle(memory.load32(delete_table,globals::particles),kept);
            kept.mark=1u;
            const auto previous=memory.load32(previous_reload,local(Local::previous));
            if(previous) {
                memory.store32(previous_link,at(previous,offsetof(ParticleDatum,next)),memory.load32(link_next,local(Local::next)));
            } else {
                step.removed_first=true;
                const auto following=memory.load32(first_next,local(Local::next));
                memory.store32(first_link,at(memory.load32(first_emitter,argument(Argument::emitter)),
                                                offsetof(EmitterDatum,first_particle)),following);
            }
        }
        frame.particle_stepped(step);
        kept.handle=memory.load32(next_reload,local(Local::next));
        system=memory.load32(system_reload,argument(Argument::system));
    }
    // The emitter's physics, over the particles that remain.
    const auto physics=memory.load32(physics_reload,local(Local::physics));
    if(physics) {
        const auto owner=memory.load32(physics_emitter,argument(Argument::emitter));
        const auto first_remaining=memory.load32(physics_first,at(owner,offsetof(EmitterDatum,first_particle)));
        if(first_remaining!=none) {
            const auto dt_bits=memory.load32(physics_time_step,argument(Argument::time_step));
            const auto location=memory.load32(physics_location,argument(Argument::location));
            frame.move_particles(physics,first_remaining,system,location,dt_bits,kept);
        }
    }
    const auto definition_last=memory.load32(definition_final,argument(Argument::definition));
    if(!(memory.load8(definition_stops,at(definition_last,offsetof(SystemDefinition,flags)))&kept.mark&0xffu))
        return frame.leave_emitter_update(Ending::running);
    if(!memory.load8(died_check,argument(Argument::bounds)))return frame.leave_emitter_update(Ending::no_deaths);
    const auto system_last=memory.load32(system_final,argument(Argument::system));
    const auto bits=memory.load8(system_flags,at(system_last,offsetof(SystemDatum,flags)));
    memory.store8(system_flags_store,at(system_last,offsetof(SystemDatum,flags)),bits&~system_flags::updating);
    frame.leave_emitter_update(Ending::stopped);
}

// ---- run (251A60) -------------------------------------------------------------
enum class LocationAccess {
    position_x,position_y,first_emitter,size_store,position_z,
    x_minimum_init,x_maximum_init,y_maximum_init,y_minimum_init,z_maximum_init,z_minimum_init,index_init,
    // per emitter
    emitter_table,emitter_data,index,emitter_count,emitter_definitions,size_function,size_flags,size_lower,size_upper,
    size_unit_upper,size_maximum,size_store_grown,
    time_step,emitter_definitions_again,system,
    next_emitter,index_reload,index_store,
    // after the emitters
    z_minimum,z_maximum,y_minimum,y_maximum,x_minimum,x_maximum,
    drawn_frame,frame_stamp,keep_flags,release_first,release_table,release_data,release_next,
    half,tag_table,center_x,center_y,center_z,particle_tag,group,extent_x,extent_y,extent_z,
    vtable,select_method,radius_vtable,radius_method,
    size_for_radius,extent_z_value,extent_z_again,extent_y_value,extent_y_again,extent_x_value,extent_x_again,
    extent_spill,radius
};
// The locals of 251A60's frame: the emitter index, the largest particle
// size, the box (whose first three floats later hold the half extent).
enum class LocationLocal {index,size,bounds,extent_x,extent_y,extent_z};
// The location's arguments: the particle system and the time step (whose
// slot the original reuses to spill the half extent's length).
enum class LocationArgument {system,time_step};
// Values the original keeps in callee-saved registers across its children,
// handed back after each child call:
//   location     throughout;
//   definition   the system definition, then (for the radius) the particle tag;
//   emitter_offset, emitter
//                in the emitter loop: the emitter definition's offset and the
//                current emitter;
//   released, emitter_table
//                in the release walk: the emitter whose particles go and the
//                emitter array;
//   interface    for the radius: the particle definition interface.
struct LocationKept {
    std::uint32_t location;
    std::uint32_t definition;
    std::uint32_t emitter_offset,emitter;
    std::uint32_t released,emitter_table;
    std::uint32_t interface;
};
// The scratch values of one emitter's size test (for the adapter).
struct SizeScratch {
    bool unit_range;
    float lower,upper;   // read, or the [0, 1] range's 0 and 1
    bool lower_greater;  // the larger of the two is the lower
    bool grew;           // it raised the largest size
};
enum class Interface {model,sprite,other};
struct LocationResult {
    bool emitters;
    bool undrawn,kept_undrawn,released;
    Interface interface;
};

// 251A60: one location's step. Its box starts at the location's position;
// for every emitter of the location, the emitter definition's particle size
// bound (the larger of its size function's lower and upper values) raises the
// largest particle size, and emitter_update steps the emitter's particles and
// grows the box. A location that was not drawn in the previous frame loses
// all its particles (2516C0 for every emitter) unless its definition keeps
// them. Then center = the box's middle, and radius = |box half extent| + the
// largest particle size times the particle interface's radius scale (+4C,
// after selecting the particle tag with +50). A particle tag that is neither
// a sprite nor a model leaves no interface: the original then reads the
// vtable of a null interface (a fault).
// The box is kept in SSE registers across the release walk, so the release
// children hand it back too.
// Frame: argument(a) and local(l) (slot addresses), size_checked,
// pass_emitter_arguments / emitter_updated / emitter_counted (the inline
// call and the loop), stamp_compared, release_emitter_particles (2516C0),
// group_compared, select_particle_tag (+50), radius_scale (+4C) and
// scale_on_stack (the value it leaves), x87_load (the x87 stack checks),
// extent_spilled and leave_location_update (RET 8).
template<class Memory,class Frame> LocationResult run(Memory &memory,Frame &frame,LocationKept &kept) {
    using enum LocationAccess;
    const auto local=[&](LocationLocal l){return frame.local(l);};
    LocationResult result{};
    const auto position=[&](unsigned axis){return at(kept.location,offsetof(LocationDatum,position)+axis*4u);};
    const float zero=0.0f;
    const auto x=memory.load_float(position_x,position(0));
    const auto y=memory.load_float(position_y,position(1));
    auto emitter_handle=memory.load32(first_emitter,at(kept.location,offsetof(LocationDatum,first_emitter)));
    memory.store_float(size_store,local(LocationLocal::size),zero);
    const auto z=memory.load_float(position_z,position(2));
    kept.emitter_offset=0;
    const auto box=local(LocationLocal::bounds);
    memory.store_float(x_maximum_init,at(box,offsetof(Bounds,x_maximum)),x);
    memory.store_float(x_minimum_init,at(box,offsetof(Bounds,x_minimum)),x);
    memory.store_float(y_maximum_init,at(box,offsetof(Bounds,y_maximum)),y);
    memory.store_float(y_minimum_init,at(box,offsetof(Bounds,y_minimum)),y);
    memory.store_float(z_maximum_init,at(box,offsetof(Bounds,z_maximum)),z);
    memory.store_float(z_minimum_init,at(box,offsetof(Bounds,z_minimum)),z);
    memory.store32(index_init,local(LocationLocal::index),kept.emitter_offset);
    Bounds grown{x,x,y,y,z,z};
    result.emitters=emitter_handle!=none;
    if(result.emitters) {
        do {
            const auto table=memory.load32(emitter_table,globals::emitters);
            kept.emitter=element(memory.load32(emitter_data,at(table,offsetof(DataArray,data))),handle_index(emitter_handle),
                                 sizeof(EmitterDatum));
            const auto index_value=memory.load32(LocationAccess::index,local(LocationLocal::index));
            const bool sized=signed_value(index_value)<signed_value(memory.load32(emitter_count,
                at(kept.definition,offsetof(SystemDefinition,emitter_count))));
            if(sized) {
                // The larger of the size function's lower and upper values
                // (COMISS lower, upper: the upper unless the lower is greater)
                // raises the largest size.
                SizeScratch size{};
                const auto entry=at(memory.load32(emitter_definitions,at(kept.definition,offsetof(SystemDefinition,emitters))),
                                    kept.emitter_offset);
                const auto function=memory.load32(size_function,at(entry,offsetof(EmitterDefinition,size_function)));
                size.unit_range=unit_range(memory.load8(size_flags,at(function,offsetof(SizeFunction,flags))));
                size.lower=size.unit_range?zero:memory.load_float(size_lower,at(function,offsetof(SizeFunction,lower)));
                size.upper=size.unit_range?memory.load_float(size_unit_upper,globals::one):
                                           memory.load_float(size_upper,at(function,offsetof(SizeFunction,upper)));
                size.lower_greater=sse::greater(size.lower,size.upper);
                const auto largest=size.lower_greater?size.lower:size.upper;
                size.grew=sse::greater(largest,memory.load_float(size_maximum,local(LocationLocal::size)));
                frame.size_checked(size);
                if(size.grew)memory.store_float(size_store_grown,local(LocationLocal::size),largest);
            }
            EmitterCall call{};
            call.time_step=memory.load32(LocationAccess::time_step,frame.argument(LocationArgument::time_step));
            call.definition_entry=at(memory.load32(emitter_definitions_again,at(kept.definition,offsetof(SystemDefinition,emitters))),
                                     kept.emitter_offset);
            call.bounds=box;
            call.system=memory.load32(LocationAccess::system,frame.argument(LocationArgument::system));
            call.definition=kept.definition;call.location=kept.location;call.emitter=kept.emitter;
            frame.pass_emitter_arguments(call,kept);
            EmitterKept emitter_kept{};
            emitter_update(memory,frame,call,emitter_kept);
            frame.emitter_updated(kept);
            emitter_handle=memory.load32(next_emitter,at(kept.emitter,offsetof(EmitterDatum,next_emitter)));
            const auto counted=memory.load32(index_reload,local(LocationLocal::index))+1u;
            kept.emitter_offset+=sizeof(EmitterDefinition);
            frame.emitter_counted(counted,emitter_handle,kept);
            memory.store32(index_store,local(LocationLocal::index),counted);
        } while(emitter_handle!=none);
        grown.z_minimum=memory.load_float(z_minimum,at(box,offsetof(Bounds,z_minimum)));
        grown.z_maximum=memory.load_float(z_maximum,at(box,offsetof(Bounds,z_maximum)));
        grown.y_minimum=memory.load_float(y_minimum,at(box,offsetof(Bounds,y_minimum)));
        grown.y_maximum=memory.load_float(y_maximum,at(box,offsetof(Bounds,y_maximum)));
        grown.x_minimum=memory.load_float(x_minimum,at(box,offsetof(Bounds,x_minimum)));
        grown.x_maximum=memory.load_float(x_maximum,at(box,offsetof(Bounds,x_maximum)));
    }
    // A location not drawn since the previous frame (drawn_frame + 1 < the
    // frame stamp) loses its particles, unless its definition keeps them.
    const auto drawn=memory.load32(drawn_frame,at(kept.location,offsetof(LocationDatum,drawn_frame)))+1u;
    const auto stamp=memory.load32(LocationAccess::frame_stamp,globals::frame_stamp);
    frame.stamp_compared(drawn);
    result.undrawn=signed_value(drawn)<signed_value(stamp);
    if(result.undrawn) {
        result.kept_undrawn=(memory.load8(keep_flags,at(kept.definition,offsetof(SystemDefinition,flags)))&
                             definition_flags::keep_undrawn)!=0;
        if(!result.kept_undrawn) {
            auto handle=memory.load32(release_first,at(kept.location,offsetof(LocationDatum,first_emitter)));
            if(handle!=none) {
                result.released=true;
                kept.emitter_table=memory.load32(release_table,globals::emitters);
                do {
                    kept.released=element(memory.load32(release_data,at(kept.emitter_table,offsetof(DataArray,data))),
                                          handle_index(handle),sizeof(EmitterDatum));
                    frame.release_emitter_particles(kept,grown);
                    handle=memory.load32(release_next,at(kept.released,offsetof(EmitterDatum,next_emitter)));
                    kept.released=handle;
                } while(handle!=none);
            }
        }
    }
    // The bounding sphere: center = (minimum + maximum) * 0.5 per axis.
    const auto half_value=memory.load_float(LocationAccess::half,globals::half);
    const auto tags=memory.load32(tag_table,globals::tag_instances);
    auto cx=sse::add(grown.x_minimum,grown.x_maximum);
    auto cy=sse::add(grown.y_minimum,grown.y_maximum);
    auto cz=sse::add(grown.z_minimum,grown.z_maximum);
    cx=sse::multiply(cx,half_value);cy=sse::multiply(cy,half_value);cz=sse::multiply(cz,half_value);
    memory.store_float(center_x,at(kept.location,offsetof(LocationDatum,center)),cx);
    memory.store_float(center_y,at(kept.location,offsetof(LocationDatum,center)+4u),cy);
    memory.store_float(center_z,at(kept.location,offsetof(LocationDatum,center)+8u),cz);
    // The particle tag's group selects the interface (the handle's low word
    // is sign-extended here).
    kept.definition=memory.load32(particle_tag,at(kept.definition,offsetof(SystemDefinition,particle_tag)));
    const auto group_value=memory.load32(group,element(tags,widen16(kept.definition),sizeof(TagInstance)));
    // The half extent: maximum - center.
    const auto ex=sse::subtract(grown.x_maximum,cx);
    const auto ey=sse::subtract(grown.y_maximum,cy);
    const auto ez=sse::subtract(grown.z_maximum,cz);
    memory.store_float(extent_x,local(LocationLocal::extent_x),ex);
    memory.store_float(extent_y,local(LocationLocal::extent_y),ey);
    memory.store_float(extent_z,local(LocationLocal::extent_z),ez);
    result.interface=group_value==model_group?Interface::model:group_value==sprite_group?Interface::sprite:Interface::other;
    kept.interface=result.interface==Interface::model?globals::model_interface:
                   result.interface==Interface::sprite?globals::sprite_interface:0u;
    frame.group_compared(group_value,kept);
    if(result.interface!=Interface::other) {
        const auto table_address=memory.load32(vtable,kept.interface);
        frame.select_particle_tag(memory.load32(select_method,at(table_address,interface_methods::select_tag)),kept);
    }
    const auto radius_table=memory.load32(radius_vtable,kept.interface);
    frame.radius_scale(memory.load32(radius_method,at(radius_table,interface_methods::radius_scale)),kept);
    // radius = scale * largest size + sqrt((ez*ez + ey*ey) + ex*ex), with the
    // scale the method left on the x87 stack; each extent is read twice, as in
    // the original.
    const auto size_value=memory.load_float(size_for_radius,local(LocationLocal::size));
    const auto scale=frame.scale_on_stack();
    const auto z1=memory.load_float(extent_z_value,local(LocationLocal::extent_z));
    frame.x87_load(1);
    const auto z2=memory.load_float(extent_z_again,local(LocationLocal::extent_z));
    const auto y1=memory.load_float(extent_y_value,local(LocationLocal::extent_y));
    frame.x87_load(2);
    const auto y2=memory.load_float(extent_y_again,local(LocationLocal::extent_y));
    const auto x1=memory.load_float(extent_x_value,local(LocationLocal::extent_x));
    frame.x87_load(2);
    const auto x2=memory.load_float(extent_x_again,local(LocationLocal::extent_x));
    const auto radius_value=x87::bounding_radius(scale,size_value,z1,z2,y1,y2,x1,x2);
    memory.store_float(extent_spill,frame.argument(LocationArgument::time_step),radius_value.extent);
    frame.extent_spilled(radius_value.extent_rounded_up);
    memory.store_float(LocationAccess::radius,at(kept.location,offsetof(LocationDatum,radius)),radius_value.radius);
    frame.leave_location_update(radius_value.radius_rounded_up);
    return result;
}
} // namespace location_update
} // namespace halo2::native_effects
