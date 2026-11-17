#pragma once
// Boundary adapters for the group H physics world leaves
// (native_tu15_world_leaves_systems.hpp). Their callers are still translated
// TU1.5 code in physics_world_runtime_generated.cpp, so each adapter owns that
// caller's ABI and nothing else:
//   - arguments from ECX, EDI and stack slots, results back into the
//     original's registers (including partial-register writes and the values
//     the original leaves in scratch XMM registers);
//   - the final EFLAGS, from the instruction that decides them in the original;
//   - the original's stack frame: register saves and restores, stack alignment
//     and the frame slots whose writes are part of the original's store order;
//   - the diagnostic cursor: each memory access publishes its original
//     instruction (a per-leaf table of access role -> instruction), and the
//     RET publishes the exit;
//   - every store goes through the unit's store helper, so guards and the store
//     observer see the original store stream.
// Delete an adapter when its callers go native.
//
// tools/native_world_leaves_replacements.py installs the bridges into the
// build-local specialized copy of the physics world unit, bound to the exact
// canonical bodies they replace: 1F5FE0 (run_1f16f0), 2E4550 (run_2e0a60),
// 303BA0 (run_300100) and 2E47B0 (run_2e0cc0).
// HALO2_REFERENCE_PHYSICS_WORLD_LEAVES=1 restores the translated bodies.
// HALO2_VERIFY_PHYSICS_WORLD_LEAVES=1 runs, for every admitted call, the native
// leaf on the live bank while recording its ordered accesses (kind, pc,
// address, width, bytes), then replays the unmodified translated body on a
// replay bank: its reads are served from the record and must match in order,
// address and width, its stores must match pc, address and bytes, and the
// final registers, flags, XMM, x87 and MXCSR must be identical. A summary line
// prints at exit.
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves_systems.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <emmintrin.h>
#include <xmmintrin.h>

namespace halo2::native_world_leaves_detail {
namespace logic=halo2::native_world_leaves;

inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PHYSICS_WORLD_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES");return p && *p;}();
    return value;
}

// Decisions of the leaves reached in verification (coverage). The adapters
// read them from the logic's accesses; the logic carries no instrumentation.
enum Branch : unsigned {
    // 1F5FE0
    a_identity, a_unit_scale, a_scaled, a_first_side, a_second_side, a_next, a_closed,
    // 2E4550, 303BA0, 2E47B0 (straight-line or fixed-count bodies)
    b_row, b_translation, c_copy, d_rotate,
    branch_count
};
// 303EE0 (validated by tools/validate_native_contact_array_tu15.py).
enum ArrayBranch : unsigned {e_room=branch_count, e_grow, array_branch_end};
// The coverage bits are coverage_words 64-bit words: branch b is bit b%64 of
// word b/64. Word 0 keeps every owner before 1F76A0 at its original bit, so
// their validators read it unchanged; the owners from 2F8630 to 2F36F0 read
// words 0 and 1, and E8D80 reads word 2.
inline constexpr unsigned coverage_words=3u,coverage_bits=coverage_words*64u;
static_assert(array_branch_end<=coverage_bits);
inline std::atomic<std::uint64_t> &coverage(unsigned word=0u) {
    static std::atomic<std::uint64_t> value[coverage_words]{};
    return value[word];
}

// The translated reference keeps explicit-destination packed SSE (the binder
// rewrites its _mm_sub_ps to this, and its _mm_mul_ps/_mm_add_ps to the
// matrix leaves' ordered forms).
inline __m128 ordered_sub_ps(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subps %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_sub_ps(a,b);
#endif
}

// ADDSS: lane 0 is a + b with a's payload first; lanes 1-3 are a's.
inline __m128 ordered_add_ss(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addss %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_add_ss(a,b);
#endif
}

enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
// MOVSS xmm,m32 and the scalar SSE results: the bits in lane 0, upper lanes zero.
inline __m128 scalar(float value){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(std::bit_cast<std::uint32_t>(value))));}

// The host's x87 exception flags (status word bits 0-5). The state's x87
// status reads them (FNSTSW), and an FCOM records them, so they belong to the
// register file as MXCSR's sticky flags do.
inline std::uint16_t host_x87_exceptions() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t word;asm volatile("fnstsw %0":"=m"(word));return word&0x3fu;
#else
    return 0;
#endif
}
inline void set_host_x87_exceptions(std::uint16_t flags) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::array<std::uint32_t,7> environment;  // FNSTENV: the status word is at +4
    asm volatile("fnstenv %0":"=m"(environment)::"memory");
    environment[1]=(environment[1]&~0x3fu)|(flags&0x3fu);
    asm volatile("fldenv %0"::"m"(environment):"memory");
#else
    (void)flags;
#endif
}
// Complete register file at entry, exit and child boundaries.
template<class State> struct Registers {
    std::array<std::uint32_t,8> gp{};
    unsigned flags{};std::uint32_t other{},fs{},mxcsr{};std::uint16_t x87_exceptions{};
    std::array<std::uint32_t,32> xmm{};
    decltype(std::declval<State&>().fp) fp{};
    static Registers capture(const State &s) {
        Registers r;r.gp=s.gp;r.flags=s.flags.bits();r.other=s.other_flags;r.fs=s.fs_base;r.fp=s.fp;r.mxcsr=_mm_getcsr();
        r.x87_exceptions=host_x87_exceptions();
        std::memcpy(r.xmm.data(),s.xmm,sizeof r.xmm);return r;
    }
    void apply(State &s)const {
        s.gp=gp;s.flags.load(flags);s.other_flags=other;s.fs_base=fs;s.fp=fp;std::memcpy(s.xmm,xmm.data(),sizeof xmm);_mm_setcsr(mxcsr);
        set_host_x87_exceptions(x87_exceptions);
    }
    bool same(const Registers &o)const {
        bool equal=gp==o.gp && flags==o.flags && other==o.other && fs==o.fs && mxcsr==o.mxcsr && xmm==o.xmm &&
            x87_exceptions==o.x87_exceptions && fp.count==o.fp.count && fp.status==o.fp.status;
        if constexpr(requires{fp.top;})equal=equal && fp.top==o.fp.top;
        for(unsigned i=0;i<fp.count && i<8;++i)equal=equal && !std::memcmp(&fp.lanes[i],&o.fp.lanes[i],10);
        return equal;
    }
};
// One ordered access of a native leaf (verification): kind 0 read, 1 store.
struct Event {std::uint32_t kind,pc,address,width;std::array<std::byte,16> bytes;};
// A verified call: its ordered accesses and its translated children's
// boundaries (target, position in the accesses, entry and exit registers).
template<class State> struct Trace {
    struct Call {std::uint32_t target;std::size_t at;Registers<State> entry,exit;};
    std::vector<Event> events;std::vector<Call> calls;
    std::vector<std::uint64_t> time_stamps;  // the RDTSC service's results, served to the replay in order
};

// The original's memory, general registers and cursor, as one leaf call sees
// them. Reads and stores use the unit's helpers; each first publishes its
// instruction to the cursor (so a faulting access reports the original
// instruction) and, in verify mode, is recorded.
template<class State> struct Port {
    using Flags=std::remove_cvref_t<decltype(std::declval<State&>().flags)>;
    State &s;std::uint32_t *cursor;Trace<State> *trace;
    std::array<std::uint32_t,8> gp;Flags flags;
    std::array<std::uint64_t,coverage_words> hit{};
    Port(State &state,std::uint32_t *c,Trace<State> *t):s(state),cursor(c),trace(t),gp(state.gp),flags(state.flags){}
    void reached(unsigned branch){hit[branch/64u]|=std::uint64_t(1)<<(branch%64u);}
    void at(std::uint32_t pc){if(cursor)*cursor=pc;}
    void record(std::uint32_t kind,std::uint32_t pc,std::uint32_t address,unsigned width,const void *bytes) {
        if(!trace)return;
        Event e{kind,pc,address,width,{}};std::memcpy(e.bytes.data(),bytes,width);trace->events.push_back(e);
    }
    // A genuine translated child, entered through the unit's own route after
    // the CALL's return address is pushed: it runs on the live state with the
    // exact register, flag, XMM and x87 ABI, and every register comes back.
    template<class Child> void call_child(std::uint32_t target,Child &child) {
        s.gp=gp;s.flags=flags;
        if(trace)trace->calls.push_back({target,trace->events.size(),Registers<State>::capture(s),{}});
        child(s);
        if(trace)trace->calls.back().exit=Registers<State>::capture(s);
        gp=s.gp;flags=s.flags;
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width) {
        at(pc);const auto value=physics_step_detail::r(s.memory,address,width);record(0,pc,address,width,&value);return value;
    }
    // MOVAPS xmm,m128: one 16-byte read.
    __m128 read_vector(std::uint32_t pc,std::uint32_t address) {
        at(pc);const auto value=physics_step_detail::v(s.memory,address);record(0,pc,address,16,&value);return value;
    }
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint64_t value) {
        at(pc);physics_step_detail::w(s.memory,s.observe,pc,address,width,value);record(1,pc,address,width,&value);
    }
    // MOVAPS m128,xmm: the unit's two ordered 8-byte stores.
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value) {
        at(pc);physics_step_detail::vw(s.memory,s.observe,pc,address,value);
        std::uint64_t half[2];std::memcpy(half,&value,16);record(1,pc,address,8,&half[0]);record(1,pc,address+8u,8,&half[1]);
    }
    void push(std::uint32_t pc,std::uint32_t value){gp[esp]-=4u;store(pc,gp[esp],4,value);}
    std::uint32_t pop(std::uint32_t pc){const auto value=read(pc,gp[esp],4);gp[esp]+=4u;return value;}
    // RET n: the return address read, then ESP past it and n argument bytes.
    void ret(std::uint32_t pc,std::uint32_t bytes){read(pc,gp[esp],4);gp[esp]+=4u+bytes;}
    void set_xmm(unsigned index,__m128 value){s.xmm[index]=value;}
    // RDTSC through the unit's service (the caller checks that it exists).
    std::uint64_t time_stamp_counter() {
        const auto value=s.services.timestamp_counter();
        if(trace)trace->time_stamps.push_back(value);
        return value;
    }
    void publish(std::uint32_t final_pc){s.gp=gp;s.flags=flags;at(final_pc);}
    // A fault leaves the general registers of the completed work.
    void abandon(){s.gp=gp;s.flags=flags;}
};

// The logic's Memory: typed accesses on a Port. Each access asks the leaf's
// Schedule for the original instruction of its role. The Schedule also reads
// coverage from the roles, and performs any register save or restore that the
// original places right before that access. A Schedule may also see each
// access complete (loaded(role,value), loaded_vector(role,value),
// stored(role)): the larger owners use this for the registers the original
// loads and for the frame traffic that follows an access.
template<class P,class Schedule> struct Memory {
    P &port;Schedule &schedule;
    template<class A> std::uint32_t load(A a,std::uint32_t address,unsigned width) {
        const auto value=port.read(schedule.instruction(a,address),address,width);
        if constexpr(requires{schedule.loaded(a,value);})schedule.loaded(a,value);
        return value;
    }
    template<class A> void store(A a,std::uint32_t address,unsigned width,std::uint32_t value) {
        port.store(schedule.instruction(a,address),address,width,value);
        if constexpr(requires{schedule.stored(a);})schedule.stored(a);
    }
    template<class A> std::uint32_t load32(A a,std::uint32_t address){return load(a,address,4);}
    template<class A> std::uint32_t load16(A a,std::uint32_t address){return load(a,address,2);}
    template<class A> std::uint32_t load8(A a,std::uint32_t address){return load(a,address,1);}
    template<class A> float load_float(A a,std::uint32_t address){return std::bit_cast<float>(load32(a,address));}
    template<class A> __m128 load_vector(A a,std::uint32_t address) {
        const auto value=port.read_vector(schedule.instruction(a,address),address);
        if constexpr(requires{schedule.loaded_vector(a,value);})schedule.loaded_vector(a,value);
        return value;
    }
    template<class A> void store32(A a,std::uint32_t address,std::uint32_t value){store(a,address,4,value);}
    template<class A> void store16(A a,std::uint32_t address,std::uint32_t value){store(a,address,2,value&0xffffu);}
    template<class A> void store8(A a,std::uint32_t address,std::uint32_t value){store(a,address,1,value&0xffu);}
    template<class A> void store_float(A a,std::uint32_t address,float value){store32(a,address,std::bit_cast<std::uint32_t>(value));}
    template<class A> void store_vector(A a,std::uint32_t address,__m128 value) {
        port.store_vector(schedule.instruction(a,address),address,value);
        if constexpr(requires{schedule.stored(a);})schedule.stored(a);
    }
};

// XMM0..XMM7 as the original leaves them after rotating a vector by an inverse
// rotation (2E47B0, and 2E4550's translation step): the sum, the input, the
// three broadcast/product registers and the two shuffles of column 2.
template<class P> void publish_inverse_rotation(P &port,const logic::havok_rotation::InverseRotated &r,__m128 input_register) {
    const auto c=logic::lanes(r.last_column);
    port.set_xmm(0,r.value);port.set_xmm(1,input_register);port.set_xmm(2,r.z_terms);
    port.set_xmm(3,logic::splat(r.input,0));port.set_xmm(4,logic::vector({c[0],c[1],c[0],c[1]}));
    port.set_xmm(5,r.y_terms);port.set_xmm(6,logic::splat(r.input,2));port.set_xmm(7,logic::vector({c[2],c[3],c[2],c[3]}));
}

// ---- 1F5FE0 surface polygon ------------------------------------------------
// ECX = the shape, EDI = the matrix or 0. Stack arguments: [ESP+4] BSP, [+8]
// key type (byte), [+C] owner, [+10] surface index, [+14] shape key. RET 14.
// SUB ESP,0C makes the local point at [entry-0C]; EBX, EBP and ESI are saved.
// The surface's address is kept in the BSP argument's slot and the first edge
// in the key type's. At exit EAX = ECX and EDX = the shape key, and EFLAGS
// come from the final ADD ESP,0C. XMM0/XMM1 hold the last corner's z and y,
// XMM3 the unit scale and XMM4 zero; a transformed corner also leaves XMM2,
// XMM5 and XMM6 (the logic's TransformResidue).
constexpr std::uint32_t polygon_instruction(logic::surface_polygon::Access a) {
    using enum logic::surface_polygon::Access;
    switch(a) {
    case convex_radius:return 0x1f5fe3u;case unit_scale:return 0x1f5fefu;case clear_user_data:return 0x1f5ffcu;
    case clear_vertex_count:return 0x1f5fffu;case key_type:return 0x1f6006u;case owner:return 0x1f6013u;
    case reference_count:return 0x1f6016u;case radius:return 0x1f601cu;case vtable:return 0x1f6021u;
    case surface_index:return 0x1f6027u;case surface_table:return 0x1f602au;case first_edge:return 0x1f6034u;
    case edge_table:return 0x1f6043u;case right_surface:return 0x1f604cu;case edge_vertex:return 0x1f6058u;
    case vertex_table:return 0x1f605cu;
    case scale:return 0x1f606cu;case vertex_x:return 0x1f6070u;case vertex_y:return 0x1f6074u;case vertex_z:return 0x1f6079u;
    case scale_y:return 0x1f608eu;case scale_z:return 0x1f6099u;
    case up_x:return 0x1f60a4u;case left_x:return 0x1f60a9u;case forward_x:return 0x1f60bau;case position_x:return 0x1f60c7u;
    case left_y:return 0x1f60ccu;case world_x:return 0x1f60d1u;
    case up_y:return 0x1f60d7u;case forward_y:return 0x1f60e8u;case position_y:return 0x1f60f5u;case world_y:return 0x1f60fau;
    case up_z:return 0x1f6100u;case left_z:return 0x1f6109u;case forward_z:return 0x1f6112u;case position_z:return 0x1f6123u;
    case world_z:return 0x1f6128u;
    case vertex_count:return 0x1f6132u;case corner_z:return 0x1f6135u;case corner_y:return 0x1f613au;case corner_x:return 0x1f613fu;
    case point_x:return 0x1f6147u;case point_y:return 0x1f614cu;case point_z:return 0x1f6151u;case point_w:return 0x1f6156u;
    case id_count:return 0x1f615bu;case vertex_id:return 0x1f6169u;case count_before_increment:return 0x1f616du;
    case vertex_count_store:return 0x1f6175u;case next_edge:return 0x1f6178u;
    case material:return 0x1f6189u;case material_store:return 0x1f618du;case surface_flags:return 0x1f6191u;
    case surface_flags_store:return 0x1f619au;case user_data:return 0x1f619du;
    }
    return 0;
}
template<class P> struct PolygonSchedule {
    P &port;std::uint32_t right_surface_at{};bool scaled{};
    std::uint32_t instruction(logic::surface_polygon::Access a,std::uint32_t address) {
        using enum logic::surface_polygon::Access;
        // Coverage: the side is where the vertex read lands relative to the
        // edge; the scale decision is whether a second scale read precedes
        // the rotation.
        if(a==right_surface)right_surface_at=address;
        else if(a==edge_vertex)port.reached(address==right_surface_at-0xau?a_first_side:a_second_side);
        else if(a==scale)scaled=false;
        else if(a==scale_y)scaled=true;
        else if(a==up_x)port.reached(scaled?a_scaled:a_unit_scale);
        return polygon_instruction(a);
    }
};
template<class P> struct PolygonFrame {
    P &port;std::uint32_t entry;std::uint32_t key{};
    std::uint32_t surface_index(){return port.read(0x1f5febu,entry+0x10u,4);}
    std::uint32_t key_type(){return port.read(0x1f6002u,entry+0x8u,1);}
    std::uint32_t owner(){return port.read(0x1f6009u,entry+0xcu,4);}
    // The original saves EBX and EBP just before it reads the BSP argument.
    std::uint32_t bsp() {
        port.push(0x1f600du,port.gp[ebx]);port.push(0x1f600eu,port.gp[ebp]);
        return port.read(0x1f600fu,entry+0x4u,4);
    }
    void keep_surface(std::uint32_t surface){port.store(0x1f6030u,entry+0x4u,4,surface);}
    // ESI is saved just before the first edge is kept.
    void keep_first_edge(std::uint32_t edge){port.push(0x1f6038u,port.gp[esi]);port.store(0x1f6039u,entry+0x8u,4,edge);}
    std::uint32_t reload_surface_index(){port.reached(a_next);return port.read(0x1f603fu,entry+0x10u,4);}
    std::uint32_t reload_first_edge(){return port.read(0x1f6170u,entry+0x8u,4);}
    std::uint32_t reload_surface(){port.reached(a_closed);return port.read(0x1f6185u,entry+0x4u,4);}
    std::uint32_t transformed_corner()const{return entry-0xcu;}
    // ESI and EBP are restored right after the shape key is read.
    std::uint32_t shape_key() {
        key=port.read(0x1f6194u,entry+0x14u,4);
        port.gp[esi]=port.pop(0x1f6198u);port.gp[ebp]=port.pop(0x1f6199u);
        return key;
    }
};
template<class State> void kernel_1f5fe0(Port<State> &port) {
    auto &gp=port.gp;
    const auto entry=gp[esp],shape=gp[ecx],matrix=gp[edi];
    if(!matrix)port.reached(a_identity);
    gp[esp]-=0xcu;  // SUB ESP,0C; ADD ESP,0C decides the final flags
    PolygonFrame<Port<State>> frame{port,entry};
    PolygonSchedule<Port<State>> schedule{port};
    Memory<Port<State>,PolygonSchedule<Port<State>>> memory{port,schedule};
    const auto residue=logic::surface_polygon::build(memory,frame,shape,matrix);
    gp[eax]=shape;gp[edx]=frame.key;
    gp[ebx]=port.pop(0x1f61a2u);
    gp[esp]=port.flags.plus(gp[esp],0xcu,4);
    port.set_xmm(0,scalar(residue.last_z));port.set_xmm(1,scalar(residue.last_y));
    port.set_xmm(3,scalar(residue.unit_scale));port.set_xmm(4,_mm_setzero_ps());
    if(const auto &t=residue.transformed) {
        port.set_xmm(2,scalar(t->left_z_times_y));port.set_xmm(5,scalar(t->world_z));port.set_xmm(6,scalar(t->forward_y_times_x));
    }
    port.ret(0x1f61a6u,0x14u);port.publish(0x1f61a6u);
}

// ---- 2E4550 relative transform --------------------------------------------
// ECX = out, [ESP+4] = a, [ESP+8] = b. PUSH EBP; MOV EBP,ESP; AND ESP,-16;
// SUB ESP,8. ESI and EDI are saved right after b is read, and EDI is restored
// between loading out's translation and storing the result. At exit EAX =
// b+40 (the row pointer), EDX = a; EFLAGS are the row loop's last ADD EAX,10
// (CF) and DEC EDI (the rest). RET 8.
constexpr std::uint32_t multiply_instruction(logic::havok_rotation::MultiplyAccess a) {
    using enum logic::havok_rotation::MultiplyAccess;
    switch(a) {
    case a_column_2:return 0x2e455cu;case a_column_0:return 0x2e4560u;case a_column_1:return 0x2e4563u;
    case b_row:return 0x2e45a0u;case out_row:return 0x2e45d0u;
    case a_column_2_again:return 0x2e45dau;case a_translation:return 0x2e45deu;case a_column_0_again:return 0x2e45e2u;
    case a_column_1_again:return 0x2e45e5u;case out_translation:return 0x2e4625u;case translation_store:return 0x2e463du;
    }
    return 0;
}
template<class P> struct MultiplySchedule {
    P &port;
    std::uint32_t instruction(logic::havok_rotation::MultiplyAccess a,std::uint32_t) {
        using enum logic::havok_rotation::MultiplyAccess;
        if(a==b_row)port.reached(Branch::b_row);
        if(a==translation_store){port.reached(b_translation);port.gp[edi]=port.pop(0x2e4639u);}
        return multiply_instruction(a);
    }
};
template<class P> struct MultiplyFrame {
    P &port;std::uint32_t entry;std::uint32_t a{};
    std::uint32_t first(){return a=port.read(0x2e4559u,entry+0x4u,4);}
    std::uint32_t second() {
        const auto b=port.read(0x2e4567u,entry+0x8u,4);
        port.push(0x2e457fu,port.gp[esi]);port.push(0x2e4591u,port.gp[edi]);
        return b;
    }
};
template<class State> void kernel_2e4550(Port<State> &port) {
    auto &gp=port.gp;
    const auto entry=gp[esp],out=gp[ecx];
    port.push(0x2e4550u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=(gp[esp]&0xfffffff0u)-8u;  // AND ESP,-16; SUB ESP,8 (flags overwritten by the loop)
    MultiplyFrame<Port<State>> frame{port,entry};
    MultiplySchedule<Port<State>> schedule{port};
    Memory<Port<State>,MultiplySchedule<Port<State>>> memory{port,schedule};
    const auto result=logic::havok_rotation::multiply_inverse(memory,frame,out);
    gp[eax]=result.b+0x40u;gp[edx]=frame.a;
    port.flags.plus(result.b+0x30u,0x10u,4);port.flags.increment(1u,4,true);
    gp[esi]=port.pop(0x2e4641u);gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2e4644u);
    publish_inverse_rotation(port,result.translation,result.result_translation);
    port.ret(0x2e4645u,8u);port.publish(0x2e4645u);
}

// ---- 303BA0 contact record copy -------------------------------------------
// ECX = destination, [ESP+4] = source. EAX = destination; EDX = the word at
// +78 with DL the state byte; ECX = source with CL the byte at +7D; XMM0 the
// last vector. No flags change. RET 4.
constexpr std::uint32_t copy_instruction(logic::contact_record::Access a) {
    // Read and store instruction of each field, in copy order.
    constexpr std::uint32_t reads[]{0x303ba6u,0x303bacu,0x303bb4u,0x303bbau,0x303bc0u,0x303bc8u,0x303bd0u,
                                    0x303bd8u,0x303be0u,0x303be6u,0x303becu,0x303bf2u,0x303bf8u};
    constexpr std::uint32_t stores[]{0x303ba9u,0x303bb0u,0x303bb7u,0x303bbdu,0x303bc4u,0x303bccu,0x303bd4u,
                                     0x303bdcu,0x303be3u,0x303be9u,0x303befu,0x303bf5u,0x303bfbu};
    const auto field=unsigned(a.field);
    return a.store?stores[field]:reads[field];
}
struct CopySchedule {
    std::uint32_t instruction(logic::contact_record::Access a,std::uint32_t){return copy_instruction(a);}
};
template<class State> void kernel_303ba0(Port<State> &port) {
    auto &gp=port.gp;
    port.reached(c_copy);
    const auto destination=gp[ecx];
    gp[eax]=destination;
    const auto source=port.read(0x303ba2u,gp[esp]+4u,4);
    CopySchedule schedule;
    Memory<Port<State>,CopySchedule> memory{port,schedule};
    const auto residue=logic::contact_record::copy(memory,destination,source);
    gp[ecx]=(source&~0xffu)|residue.byte_7d;
    gp[edx]=(residue.word_78&~0xffu)|residue.state;
    port.set_xmm(0,residue.last_vector);
    port.ret(0x303bfeu,4u);port.publish(0x303bfeu);
}

// ---- 2E47B0 transposed rotation -------------------------------------------
// ECX = out, [ESP+4] = rotation, [ESP+8] = vector. PUSH EBP; MOV EBP,ESP;
// AND ESP,-16 decides EFLAGS. EAX = the vector's address. RET 8.
constexpr std::uint32_t rotate_instruction(logic::havok_rotation::RotateAccess a) {
    using enum logic::havok_rotation::RotateAccess;
    switch(a) {
    case column_2:return 0x2e47b9u;case column_1:return 0x2e47bdu;case column_0:return 0x2e47c1u;
    case input:return 0x2e47c7u;case result:return 0x2e4816u;
    }
    return 0;
}
struct RotateSchedule {
    std::uint32_t instruction(logic::havok_rotation::RotateAccess a,std::uint32_t){return rotate_instruction(a);}
};
template<class P> struct RotateFrame {
    P &port;std::uint32_t entry;std::uint32_t vector_address{};
    std::uint32_t rotation(){return port.read(0x2e47b6u,entry+0x4u,4);}
    std::uint32_t vector(){return vector_address=port.read(0x2e47c4u,entry+0x8u,4);}
};
template<class State> void kernel_2e47b0(Port<State> &port) {
    auto &gp=port.gp;
    port.reached(d_rotate);
    const auto entry=gp[esp],out=gp[ecx];
    port.push(0x2e47b0u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=port.flags.logic(gp[esp]&0xfffffff0u,4);
    RotateFrame<Port<State>> frame{port,entry};
    RotateSchedule schedule;
    Memory<Port<State>,RotateSchedule> memory{port,schedule};
    const auto rotated=logic::havok_rotation::rotate_inverse(memory,frame,out);
    gp[eax]=frame.vector_address;
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2e481bu);
    publish_inverse_rotation(port,rotated,rotated.input);
    port.ret(0x2e481cu,8u);port.publish(0x2e481cu);
}

// ---- 303EE0 contact record append -----------------------------------------
// ECX = the array. PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,8C makes the
// local record at [aligned-80]; ESI is saved and holds the array. Children:
//   2E3250 (cdecl: the array, the element size; ADD ESP,8 after) is entered
//   through the unit's route as a genuine call, with EAX = ECX = the full
//   size and the flags of the CMP that found it full;
//   303BA0 (ECX = the new record, [ESP+4] = the local; RET 4) runs its native
//   kernel inline at its own boundary. The original pushes its argument
//   before it stores the new size.
// At exit EAX = the last record and ECX = the array's data; EDX and XMM0 are
// 303BA0's. EFLAGS come from SHL EAX,7 (OF from the INC before it). RET.
constexpr std::uint32_t append_instruction(logic::contact_array::Access a) {
    using enum logic::contact_array::Access;
    switch(a) {
    case size:return 0x303ef1u;case clear_byte_7d:return 0x303ef4u;case clear_state:return 0x303efbu;
    case capacity:return 0x303f02u;case grown_size:return 0x303f1cu;case data:return 0x303f1fu;
    case size_store:return 0x303f2eu;case result_size:return 0x303f36u;case result_data:return 0x303f39u;
    }
    return 0;
}
template<class P> struct AppendSchedule {
    P &port;std::uint32_t local;
    std::uint32_t instruction(logic::contact_array::Access a,std::uint32_t) {
        if(a==logic::contact_array::Access::size_store)port.push(0x303f2du,local);  // 303BA0's argument
        return append_instruction(a);
    }
};
template<class P,class Reserve> struct AppendFrame {
    P &port;std::uint32_t local;Reserve &reserve;bool grew{};
    std::uint32_t local_record()const{return local;}
    void reserve_more(std::uint32_t array,std::uint32_t element_size,std::uint32_t full_size) {
        grew=true;
        port.gp[eax]=full_size;port.gp[ecx]=full_size;port.flags.minus(full_size,full_size,4);
        port.push(0x303f0eu,element_size);port.push(0x303f13u,array);port.push(0x303f14u,0x303f19u);
        port.call_child(0x2e3250u,reserve);
        port.gp[esp]=port.flags.plus(port.gp[esp],8u,4);
    }
    void copy_record(std::uint32_t record,std::uint32_t source) {
        port.gp[ecx]=record;port.gp[edx]=source;
        port.push(0x303f31u,0x303f36u);
        kernel_303ba0(port);
    }
};
template<class State,class Reserve> void kernel_303ee0(Port<State> &port,Reserve &reserve) {
    auto &gp=port.gp;
    const auto array=gp[ecx];
    port.push(0x303ee0u,gp[ebp]);gp[ebp]=gp[esp];
    const auto aligned=gp[esp]&0xfffffff0u;
    gp[esp]=aligned-0x8cu;  // AND ESP,-16; SUB ESP,8C (flags overwritten below)
    port.push(0x303eeeu,gp[esi]);gp[esi]=array;
    AppendFrame<Port<State>,Reserve> frame{port,aligned-0x80u,reserve};
    AppendSchedule<Port<State>> schedule{port,aligned-0x80u};
    Memory<Port<State>,AppendSchedule<Port<State>>> memory{port,schedule};
    const auto result=logic::contact_array::expand_one(memory,frame,array);
    port.reached(frame.grew?e_grow:e_room);
    gp[eax]=result.record;gp[ecx]=result.data;
    // SHL EAX,7 decides the flags, except OF: a shift by more than one leaves
    // it as the INC EAX that counted the new record set it.
    port.flags.increment(result.index,4,false);port.flags.shift(result.size,7u,4);
    gp[esi]=port.pop(0x303f42u);gp[esp]=gp[ebp];gp[ebp]=port.pop(0x303f45u);
    port.ret(0x303f46u,0u);port.publish(0x303f46u);
}

// ---- Calls through the unit's dispatch --------------------------------------
// A virtual or function-pointer call: the target read from memory is entered
// through the unit's dispatch, with the original's canonical call site (the
// unit reports it when the target has no owner). Its RET pops its own
// arguments; every register comes back.
template<class State,class Dispatch> void call_dispatch(Port<State> &port,std::uint32_t target,std::uint32_t site,Dispatch &dispatch) {
    auto child=[&](State &state){dispatch(state,target,site);};
    port.call_child(target,child);
}
// An MSVC exception frame, opened at `pc` (PUSH -1; PUSH handler; MOV EAX,
// fs:[0]; PUSH EAX; MOV fs:[0],ESP). The pushed -1 is the unwind state, which
// the owner sets as its locals come to life. Returns the previous frame.
template<class State> std::uint32_t open_exception_frame(Port<State> &port,std::uint32_t pc,std::uint32_t handler) {
    port.push(pc,0xffffffffu);
    port.push(pc+2u,handler);
    const auto previous=port.read(pc+7u,port.s.fs_base,4);
    port.gp[eax]=previous;
    port.push(pc+0xdu,previous);
    port.store(pc+0xeu,port.s.fs_base,4,port.gp[esp]);
    return previous;
}

// ---- 303C70 / 303D00 local shape buffers -----------------------------------
// ECX = the hkLocalArray. Both open an exception frame (handler 37ED68), save
// ECX and two registers, and store the array over the saved ECX (the
// unwinder's `this`). The unwind state is [entry-4]. The exit reloads the
// previous frame from [entry-C] into ECX, restores fs:[0], and ADD ESP,10
// decides the flags.
//   303C70 (RET 4; [ESP+4] = the capacity) saves EBX and ESI and holds the
//   capacity in EBX. The bump saves EBP and EDI around its arithmetic. The
//   virtual allocate_stack() (thiscall, RET 4) is entered with EAX = the
//   bytes, ECX = the thread memory, EDX = its vtable and the flags of CMP
//   bytes,left. EAX = the array at exit; EDX = the stack left, or the
//   callee's.
//   303D00 (RET) saves ESI and EDI. deallocate_stack() (RET 8) is entered with
//   EAX = the bytes, EDX = the block, EDI = the vtable and the flags of CMP
//   block,mark (TEST mark,mark when they differ); deallocate() (RET C) with
//   EAX = the data, EDX = the vtable and SHL's flags, after the class and size
//   were pushed. EAX = the capacity read at exit, or deallocate()'s.
inline constexpr std::uint32_t local_buffers_handler=0x37ed68u;
constexpr std::uint32_t local_buffers_instruction(logic::local_buffers::Access a) {
    using enum logic::local_buffers::Access;
    switch(a) {
    case clear_data:return 0x303c90u;case clear_size:return 0x303c92u;case clear_capacity:return 0x303c95u;
    case memory_pointer:return 0x303ca0u;case stack_left:return 0x303ca6u;case stack_next:return 0x303cc6u;
    case stack_next_store:return 0x303cd1u;case stack_left_store:return 0x303cd4u;case data_store:return 0x303cdau;
    case block_store:return 0x303ce2u;case capacity_store:return 0x303ce5u;case initial_capacity_store:return 0x303cecu;
    case initial_capacity_read:return 0x303d1eu;case block_read:return 0x303d21u;case memory_pointer_again:return 0x303d24u;
    case stack_mark:return 0x303d3du;case next_to_pop:return 0x303d51u;case left_to_pop:return 0x303d54u;
    case popped_next_store:return 0x303d5bu;case popped_left_store:return 0x303d5eu;case capacity_read:return 0x303d61u;
    case owner_memory_pointer:return 0x303d70u;case storage:return 0x303d83u;
    }
    return 0;
}
// 303C70 and 303D00 (validated by tools/validate_native_local_buffers_tu15.py).
enum LocalBufferBranch : unsigned {
    l_bump=array_branch_end,l_allocate,l_empty,l_pop,l_release_at_mark,l_release_unmarked,l_owned,l_borrowed,
    local_buffer_branch_end
};
static_assert(local_buffer_branch_end<=coverage_bits);
template<class P> struct ConstructSchedule {
    P &port;std::uint32_t entry;bool bumped{};
    std::uint32_t instruction(logic::local_buffers::Access a,std::uint32_t) {
        using enum logic::local_buffers::Access;
        auto &gp=port.gp;
        switch(a) {
        case stack_next:bumped=true;port.push(0x303cc4u,gp[ebp]);port.push(0x303cc5u,gp[edi]);break;
        case stack_next_store: {
            // LEA EBP,[EDI+EAX]; SUB EDX,EAX; MOV EAX,EDI; POP EDI
            const auto next=gp[edi];
            gp[ebp]=next+gp[eax];gp[edx]=port.flags.minus(gp[edx],gp[eax],4);gp[eax]=next;
            gp[edi]=port.pop(0x303cd0u);
            break;
        }
        case data_store:
            if(bumped)gp[ebp]=port.pop(0x303cd7u);
            gp[ecx]=gp[ebx];
            break;
        case block_store:gp[ecx]=port.flags.logic(gp[ecx]|logic::local_buffers::storage_not_owned,4);break;
        case initial_capacity_store:gp[ecx]=port.read(0x303ce8u,entry-0xcu,4);break;
        default:break;
        }
        return local_buffers_instruction(a);
    }
    void loaded(logic::local_buffers::Access a,std::uint32_t value) {
        using enum logic::local_buffers::Access;
        auto &gp=port.gp;
        if(a==memory_pointer)gp[ecx]=value;
        else if(a==stack_next)gp[edi]=value;
        else if(a==stack_left) {
            gp[edx]=value;
            port.store(0x303ca9u,entry-4u,4,0u);  // the hkArray base exists: unwind state 0
            // MOV EAX,EBX; SHL EAX,8; ADD EAX,F; AND EAX,-10; CMP EAX,EDX
            gp[eax]=logic::local_buffers::block_bytes(gp[ebx]);
            port.flags.minus(gp[eax],value,4);
        }
    }
};
template<class P,class Dispatch> struct ConstructFrame {
    P &port;std::uint32_t entry;Dispatch &dispatch;
    std::uint32_t capacity(){return port.gp[ebx]=port.read(0x303c9cu,entry+4u,4);}
    // MOV EDX,[ECX]; PUSH EAX; CALL [EDX+24]
    std::uint32_t allocate_stack(std::uint32_t thread,std::uint32_t bytes) {
        auto &gp=port.gp;
        gp[edx]=port.read(0x303cbcu,thread,4);
        port.push(0x303cbeu,bytes);
        const auto slot=std::uint32_t(logic::local_buffers::ThreadMemoryFunction::allocate_stack);
        const auto target=port.read(0x303cbfu,gp[edx]+slot,4);
        port.push(0x303cbfu,0x303cc2u);
        call_dispatch(port,target,0x30021fu,dispatch);
        return gp[eax];
    }
};
template<class State,class Dispatch> void kernel_303c70(Port<State> &port,Dispatch &dispatch) {
    auto &gp=port.gp;
    const auto entry=gp[esp],array=gp[ecx];
    open_exception_frame(port,0x303c70u,local_buffers_handler);
    port.push(0x303c85u,gp[ecx]);port.push(0x303c86u,gp[ebx]);port.push(0x303c87u,gp[esi]);
    gp[esi]=array;gp[eax]=port.flags.logic(0u,4);  // XOR EAX,EAX
    port.store(0x303c8cu,entry-0x10u,4,array);
    ConstructFrame<Port<State>,Dispatch> frame{port,entry,dispatch};
    ConstructSchedule<Port<State>> schedule{port,entry};
    Memory<Port<State>,ConstructSchedule<Port<State>>> memory{port,schedule};
    const auto made=logic::local_buffers::construct(memory,frame,array);
    port.reached(made.allocated?l_allocate:l_bump);
    gp[eax]=array;
    gp[esi]=port.pop(0x303cf1u);gp[ebx]=port.pop(0x303cf2u);
    port.store(0x303cf3u,port.s.fs_base,4,gp[ecx]);
    gp[esp]=port.flags.plus(gp[esp],0x10u,4);
    port.ret(0x303cfdu,4u);port.publish(0x303cfdu);
}
template<class P> struct DestroySchedule {
    P &port;std::uint32_t entry;std::uint32_t owner{},capacity{},pushed_bytes{};
    std::uint32_t instruction(logic::local_buffers::Access a,std::uint32_t) {
        auto &gp=port.gp;
        if(a==logic::local_buffers::Access::storage) {
            // MOV EDX,[ECX]; AND EAX,7FFFFFFF; PUSH 12; SHL EAX,8; PUSH EAX:
            // the class and the size are pushed before the data is read.
            gp[edx]=port.read(0x303d76u,owner,4);
            gp[eax]=port.flags.logic(capacity&logic::contact_array::capacity_mask,4);
            port.push(0x303d7du,logic::local_buffers::array_storage_class);
            gp[eax]=pushed_bytes=port.flags.shift(gp[eax],8u,4);
            port.push(0x303d82u,gp[eax]);
        }
        return local_buffers_instruction(a);
    }
    void loaded(logic::local_buffers::Access a,std::uint32_t value) {
        using enum logic::local_buffers::Access;
        auto &gp=port.gp;
        switch(a) {
        case initial_capacity_read:gp[eax]=value;break;
        case block_read:gp[edx]=value;break;
        case memory_pointer_again:
            gp[ecx]=value;
            // SHL EAX,8; ADD EAX,F; AND EAX,-10: ZF marks an empty block.
            gp[eax]=port.flags.logic(logic::local_buffers::block_bytes(gp[eax]),4);
            port.store(0x303d33u,entry-4u,4,0u);  // unwind state 0
            break;
        case stack_mark:
            gp[edi]=value;
            port.flags.minus(gp[edx],value,4);            // CMP EDX,EDI
            if(gp[edx]!=value)port.flags.logic(value,4);  // TEST EDI,EDI
            break;
        case next_to_pop:gp[edi]=value;break;
        case left_to_pop:
            gp[edx]=value;
            gp[edi]=port.flags.minus(gp[edi],gp[eax],4);  // SUB EDI,EAX
            gp[edx]=port.flags.plus(gp[edx],gp[eax],4);   // ADD EDX,EAX
            break;
        case capacity_read:
            gp[eax]=port.flags.logic(value,4);capacity=value;  // TEST EAX,EAX
            port.store(0x303d66u,entry-4u,4,0xffffffffu);      // unwind state -1
            break;
        case owner_memory_pointer:gp[ecx]=owner=value;break;
        case storage:gp[eax]=value;break;
        default:break;
        }
    }
};
template<class P,class Dispatch> struct DestroyFrame {
    P &port;DestroySchedule<P> &schedule;Dispatch &dispatch;
    // MOV EDI,[ECX]; PUSH EAX; PUSH EDX; CALL [EDI+28]
    void deallocate_stack(std::uint32_t thread,std::uint32_t block,std::uint32_t bytes) {
        auto &gp=port.gp;
        gp[edi]=port.read(0x303d48u,thread,4);
        port.push(0x303d4au,bytes);port.push(0x303d4bu,block);
        const auto slot=std::uint32_t(logic::local_buffers::ThreadMemoryFunction::deallocate_stack);
        const auto target=port.read(0x303d4cu,gp[edi]+slot,4);
        port.push(0x303d4cu,0x303d4fu);
        call_dispatch(port,target,0x3002acu,dispatch);
    }
    // PUSH EAX; CALL [EDX+14]. The schedule pushed the class and the size
    // (computed as the original does) before the data was read.
    void deallocate(std::uint32_t thread,std::uint32_t data,std::uint32_t bytes,std::uint32_t storage_class) {
        if(thread!=schedule.owner || bytes!=schedule.pushed_bytes || storage_class!=logic::local_buffers::array_storage_class)
            throw std::logic_error("Native 303D00 deallocate() arguments differ from the pushed ones");
        auto &gp=port.gp;
        port.push(0x303d85u,data);
        const auto slot=std::uint32_t(logic::local_buffers::ThreadMemoryFunction::deallocate);
        const auto target=port.read(0x303d86u,gp[edx]+slot,4);
        port.push(0x303d86u,0x303d89u);
        call_dispatch(port,target,0x3002e6u,dispatch);
    }
};
template<class State,class Dispatch> void kernel_303d00(Port<State> &port,Dispatch &dispatch) {
    auto &gp=port.gp;
    const auto entry=gp[esp],array=gp[ecx];
    open_exception_frame(port,0x303d00u,local_buffers_handler);
    port.push(0x303d15u,gp[ecx]);port.push(0x303d16u,gp[esi]);gp[esi]=array;port.push(0x303d19u,gp[edi]);
    port.store(0x303d1au,entry-0x10u,4,array);
    DestroySchedule<Port<State>> schedule{port,entry};
    DestroyFrame<Port<State>,Dispatch> frame{port,schedule,dispatch};
    Memory<Port<State>,DestroySchedule<Port<State>>> memory{port,schedule};
    const auto done=logic::local_buffers::destroy(memory,frame,array);
    if(!done.bytes)port.reached(l_empty);
    else if(!done.released_virtually)port.reached(l_pop);
    else port.reached(*done.mark?l_release_at_mark:l_release_unmarked);
    port.reached(done.capacity&logic::local_buffers::storage_not_owned?l_borrowed:l_owned);
    gp[ecx]=port.read(0x303d89u,entry-0xcu,4);
    gp[edi]=port.pop(0x303d8du);gp[esi]=port.pop(0x303d8eu);
    port.store(0x303d8fu,port.s.fs_base,4,gp[ecx]);
    gp[esp]=port.flags.plus(gp[esp],0x10u,4);
    port.ret(0x303d99u,0u);port.publish(0x303d99u);
}

// ---- 304350 welded collection agent ----------------------------------------
// thiscall: ECX = the agent; [ESP+4] body A, [+8] body B, [+C] the input, [+10]
// the output; RET 10. PUSH EBP; MOV EBP,ESP; AND ESP,-16; an exception frame
// (handler 37EDF6; its unwind state at [S+32C] marks the shape buffers alive);
// SUB ESP,318; PUSH EBX; PUSH ESI (ESI = this); PUSH EDI. S, the ESP after
// that, is the base of the locals (CollectionAgentLocals below).
//
// The original's registers are observable only where it calls out (every
// child, every virtual and function-pointer call, and the native children it
// runs inline) and at its exit. The adapter sets them there, from the logic's
// values and from what the role hooks saw: each hook mirrors the register the
// original instruction loads, and the register arithmetic between two
// accesses. EBX, ESI and EDI are mirrored at every write, because they carry
// from one phase to the next (EBX holds the record count the welds leave).
// Children must keep EBX, ESI, EDI, EBP and pop their own arguments; a child
// that does not makes the native throw rather than diverge.
//
// x87: the logic computes with x87 values (logic::x87); the hooks keep the
// state's x87 stack exactly as the original's instructions leave it (FLD
// pushes, FCOM sets the status, FSTP pops, FXCH exchanges), so its depth,
// TOP and status are the original's at every call and at the exit. FNSTSW AX
// and the SETB AL/TEST AL,AL pairs leave values that are overwritten before
// any observation.
inline constexpr std::uint32_t collection_agent_handler=0x37edf6u;
constexpr std::uint32_t collection_agent_instruction(logic::collection_agent::Access a) {
    using enum logic::collection_agent::Access;
    switch(a) {
    case begin_next:return 0x304375u;case begin_end:return 0x30437cu;case begin_name:return 0x30438du;
    case begin_time_store:return 0x304395u;case begin_time_load:return 0x304399u;case begin_time:return 0x30439du;
    case begin_advance:return 0x3043a3u;
    case end_next:return 0x304a2fu;case end_end:return 0x304a35u;case end_name:return 0x304a42u;
    case end_time_store:return 0x304a4au;case end_time_load:return 0x304a4eu;case end_time:return 0x304a52u;
    case end_advance:return 0x304a58u;
    case body_b_argument:return 0x3043a9u;case body_b_motion:return 0x3043acu;case body_b_shape:return 0x3043afu;
    case wrapper_motion:return 0x3043b1u;case wrapper_parent:return 0x3043b5u;case collection_shape_read:return 0x3043b9u;
    case body_b_key:return 0x3043bcu;case wrapper_shape:return 0x3043c4u;case input_argument:return 0x3043c8u;
    case wrapper_key:return 0x3043d0u;case body_a_argument:return 0x3043d4u;
    case records_data_init:return 0x3043e9u;case records_size_init:return 0x3043f0u;case records_capacity_init:return 0x3043f7u;
    case child_count:return 0x304402u;case child_count_again:return 0x304416u;case body_b_motion_again:return 0x30441bu;
    case wrapper_shape_again:return 0x30441eu;case buffers_size:return 0x30442au;case child_body_parent:return 0x30442eu;
    case child_body_motion:return 0x304432u;case collection_shape_keep:return 0x304436u;case child_index_keep:return 0x30443au;
    case buffer_offset_keep:return 0x304444u;case entry_offset_keep:return 0x304448u;
    case children_data:return 0x304450u;case entry_agent:return 0x30445au;case collection_shape_reload:return 0x304463u;
    case collection_inner:return 0x304467u;case buffers_data:return 0x30446au;case buffer_offset_reload:return 0x30446eu;
    case child_key:return 0x304476u;case body_a_reload:return 0x30447du;case body_a_shape:return 0x304480u;
    case child_key_again:return 0x304484u;case child_body_shape:return 0x304486u;case child_body_key:return 0x30448au;
    case input_reload:return 0x30449du;case dispatcher:return 0x3044a0u;case verdict:return 0x3044afu;
    case gather_input:return 0x3044bcu;case gathered_agent:return 0x3044bfu;case output_argument:return 0x30463bu;
    case processed_agent:return 0x30463eu;case agent_reload:return 0x304653u;case child_index_reload:return 0x304657u;
    case entry_offset_reload:return 0x30465bu;case buffer_offset_next:return 0x30465fu;case children_size:return 0x304663u;
    case child_index_store:return 0x304672u;case entry_offset_store:return 0x304676u;case buffer_offset_store:return 0x30467au;
    case agent_size:return 0x3044ecu;case agent_data:return 0x3044f3u;case agent_store:return 0x3044fdu;
    case shape_size:return 0x304501u;case shape_data:return 0x304508u;case record_key:return 0x30450fu;
    case shape_store:return 0x304514u;case key_size:return 0x304518u;case key_data:return 0x30451fu;case key_store:return 0x304529u;
    case has_size:return 0x30452du;case found:return 0x304534u;case has_data:return 0x304538u;case has_store:return 0x304542u;
    case found_again:return 0x304546u;
    case contact_size:return 0x304552u;case contact_data:return 0x304559u;case contact_position:return 0x304560u;
    case contact_position_store:return 0x30456fu;case contact_normal:return 0x304572u;case contact_normal_store:return 0x30457au;
    case contact_distance:return 0x30457eu;case contact_distance_store:return 0x304585u;case contact_word_24:return 0x304588u;
    case contact_word_24_store:return 0x30458fu;case contact_vector_30:return 0x304592u;case contact_vector_30_store:return 0x30459au;
    case contact_vector_40:return 0x30459eu;case contact_vector_40_store:return 0x3045a6u;case body_motion:return 0x3045aau;
    case welded_distance:return 0x3045c6u;case welded_size:return 0x3045cdu;case welded_data:return 0x3045d4u;
    case welded_vector:return 0x3045dbu;case welded_vector_store:return 0x3045eau;case welded_normal:return 0x3045edu;
    case welded_normal_store:return 0x3045f5u;case welded_distance_store:return 0x3045f9u;case agent_kind:return 0x3045fcu;
    case state_size:return 0x304609u;case state_data:return 0x304610u;case state_store:return 0x30461cu;  // the marked path: +19
    case sort_count:return 0x304684u;case sort_index_keep:return 0x304691u;case sort_offset_keep:return 0x30469bu;
    case sort_index_reload:return 0x3046a1u;case sort_data:return 0x3046b0u;case sort_offset_reload:return 0x3046b7u;
    case first_distance:return 0x3046bbu;case group_distance:return 0x3046d2u;case group_distance_again:return 0x3046e0u;  // lane 0
    case rest_data:return 0x304737u;case rest_distance:return 0x304747u;case rest_distance_again:return 0x304754u;
    case swap_data:return 0x304761u;case swap_first_position:return 0x30476au;case swap_best_vector_50:return 0x304774u;
    case swap_best_distance:return 0x304778u;case swap_best_vector_60:return 0x30477bu;case swap_best_word_24:return 0x30477fu;
    case swap_best_shape_key:return 0x304782u;case swap_best_position:return 0x304787u;case swap_best_normal:return 0x30478au;
    case swap_best_vector_30:return 0x30478eu;case swap_best_vector_40:return 0x304792u;case swap_best_agent:return 0x304796u;
    case swap_best_child_shape:return 0x304799u;case swap_to_best_position:return 0x30479cu;case swap_first_normal:return 0x30479fu;
    case swap_to_best_normal:return 0x3047a3u;case swapped_key_keep:return 0x3047a7u;case swap_best_state:return 0x3047aeu;
    case swapped_state_keep:return 0x3047b1u;case swap_best_has:return 0x3047b8u;case swapped_has_keep:return 0x3047bbu;
    case swap_first_distance:return 0x3047c2u;case swap_to_best_distance:return 0x3047c5u;case swap_first_word_24:return 0x3047c8u;
    case swap_to_best_word_24:return 0x3047cbu;case swap_first_vector_30:return 0x3047ceu;case swap_to_best_vector_30:return 0x3047d2u;
    case swap_first_vector_40:return 0x3047d6u;case swap_to_best_vector_40:return 0x3047dau;case swap_first_vector_50:return 0x3047deu;
    case swap_to_best_vector_50:return 0x3047e2u;case swap_first_vector_60:return 0x3047e6u;case swap_to_best_vector_60:return 0x3047eau;
    case swap_first_agent:return 0x3047eeu;case swap_to_best_agent:return 0x3047f1u;case swap_first_child_shape:return 0x3047f4u;
    case swap_to_best_child_shape:return 0x3047f7u;case swap_first_shape_key:return 0x3047fau;case swap_to_best_shape_key:return 0x3047fdu;
    case swap_first_state:return 0x304800u;case swap_to_best_state:return 0x304803u;case swap_first_has:return 0x304806u;
    case swap_to_best_has:return 0x304809u;case swap_to_first_distance:return 0x30480cu;case swapped_key_reload:return 0x30480fu;
    case swapped_state_reload:return 0x304816u;case swap_to_first_word_24:return 0x30481du;case swap_to_first_position:return 0x304820u;
    case swap_to_first_normal:return 0x304823u;case swap_to_first_vector_30:return 0x304827u;case swap_to_first_vector_40:return 0x30482bu;
    case swap_to_first_vector_50:return 0x30482fu;case swap_to_first_vector_60:return 0x304833u;case swap_to_first_shape_key:return 0x304837u;
    case swapped_has_reload:return 0x30483au;case swap_to_first_agent:return 0x304841u;case sorted_offset_reload:return 0x304844u;
    case swap_to_first_child_shape:return 0x304848u;case swap_to_first_state:return 0x30484bu;case swap_to_first_has:return 0x30484eu;
    case sorted_data:return 0x304851u;case sorted_has_contact:return 0x304858u;case weld_input:return 0x304864u;
    case weld_parameter:return 0x304867u;case weld_index_reload:return 0x30486au;case next_index_reload:return 0x304886u;
    case sort_index_store:return 0x304893u;case sort_offset_store:return 0x304897u;case sorted_size:return 0x3048a1u;
    case record_count_keep:return 0x3048aau;
    case point_offset_keep:return 0x3048b4u;case points_left_keep:return 0x3048bcu;case points_data:return 0x3048c0u;
    case point_offset_reload:return 0x3048c7u;case points_agent:return 0x3048cbu;case point_count:return 0x3048cfu;
    case point_body_b:return 0x3048e0u;case point_distance:return 0x3048e3u;case point_motion:return 0x3048e6u;
    case point_position:return 0x3048e9u;case point_position_store:return 0x3048f3u;case point_distance_store:return 0x3048f8u;
    case point_input:return 0x304909u;case point_weld_parameter:return 0x30490cu;case point_normal:return 0x30490fu;
    case point_normal_store:return 0x30492cu;case point_offset_next:return 0x30493eu;case points_left_next:return 0x304942u;
    case point_offset_store:return 0x30494du;case points_left_store:return 0x304951u;
    case record_count_reload:return 0x30495bu;case emit_offset_keep:return 0x304969u;case emit_body_b:return 0x304970u;
    case emit_motion:return 0x304973u;case emit_offset_reload:return 0x304976u;case emit_parent:return 0x30497au;
    case emit_data:return 0x30497eu;case emit_motion_store:return 0x304989u;case emit_shape_key:return 0x30498du;
    case emit_child_shape:return 0x304990u;case emit_shape_store:return 0x304993u;case emit_key_store:return 0x304997u;
    case emit_agent:return 0x30499bu;case emit_has_contact:return 0x3049a0u;case distance_data:return 0x3049cdu;
    case distance_offset_reload:return 0x3049d4u;case emit_distance:return 0x3049d8u;case emit_agent_reload:return 0x3049dcu;
    case contact_manager:return 0x3049e0u;case distance_bias:return 0x3049e3u;case separation_store:return 0x3049eeu;
    case separation_load:return 0x3049f2u;case emit_offset_next:return 0x304a11u;case record_count_next:return 0x304a15u;
    case emit_offset_store:return 0x304a25u;
    case records_capacity:return 0x304a6fu;case thread_memory:return 0x304a85u;case records_storage:return 0x304a98u;
    }
    return 0;
}
// The four lanes of the grouped search (FCOM, and the FLD that holds a new
// closest) and the state's two paths.
inline constexpr std::uint32_t group_compare_pc[4]{0x3046d2u,0x3046e3u,0x3046f3u,0x30470bu};
inline constexpr std::uint32_t group_hold_pc[4]{0x3046e0u,0x3046f1u,0x304705u,0x30471du};
// The canonical call sites the unit's dispatch receives.
inline constexpr std::uint32_t child_shape_site=0x30099au,child_type_site=0x3009b2u,body_type_site=0x3009bau,
    predicate_site=0x3009c9u,closest_site=0x3009fdu,process_site=0x300b70u,deallocate_site=0x300fc0u;
// 304350 (validated by tools/validate_native_collection_agent_tu15.py).
enum CollectionAgentBranch : unsigned {
    a_timer=local_buffer_branch_end,a_no_children,a_null_agent,a_gather,a_process,a_contact,a_no_contact,a_marked,
    a_unmarked,a_no_records,a_groups,a_rest,a_replaced,a_welded,a_points,a_no_points,a_confirmed,a_owned,a_borrowed,
    collection_branch_end
};
static_assert(collection_branch_end<=coverage_bits);

template<class State,class Children> struct CollectionAgentAbi {
    using Access=logic::collection_agent::Access;
    Port<State> &port;Children &children;
    std::uint32_t frame_base;  // S
    logic::collection_agent::Locals local{};
    // What the hooks saw, for the registers at the calls.
    std::uint32_t self{},next_event{},body_a{},entry{},entry_offset{},buffer_offset{},buffers_base{},inner_vtable{},
        child{},a_shape{},kind{},contact_motion{},sorted_record{},best_index{},point{},motion{},parameter{},
        record{},agent{},output{},capacity{},owner{},pushed_bytes{};
    unsigned lane{},next_lane{};bool ended{};
    CollectionAgentAbi(Port<State> &p,Children &c,std::uint32_t s,std::uint32_t ebp):port(p),children(c),frame_base(s) {
        // The arguments above EBP; the locals from S (several share a slot in turn).
        local={.body_a=ebp+8u,.body_b=ebp+0xcu,.input=ebp+0x10u,.output=ebp+0x14u,
            .agent=s+0x1cu,.begin_time=s+0x20u,.end_time=s+0x24u,.wrapper_body=s+0x30u,.child_body=s+0x40u,
            .point_contact=s+0x40u,.buffers=s+0x6cu,.rotated_normal=s+0x80u,.contact=s+0x90u,.contact_found=s+0x18u,
            .records=s+0x110u,.record_storage=s+0x120u,
            .collection_shape=s+0x24u,.child_index=s+0x28u,.entry_offset=s+0x20u,.buffer_offset=s+0x14u,
            .sort_index=s+0x14u,.sort_offset=s+0x18u,.swapped_key=s+0x108u,.swapped_state=s+0x10cu,
            .swapped_has_contact=s+0x10du,.record_count=s+0x28u,.point_offset=s+0x18u,.points_left=s+0x20u,
            .emit_offset=s+0x14u,.separation=s+0x24u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    void byte_register(unsigned r,std::uint32_t value){reg(r)=(reg(r)&~0xffu)|(value&0xffu);}
    // x87, exactly as the translated instructions keep the state's stack.
    void fld(std::uint32_t bits){port.s.require_x87();port.s.fp.push(physics_step_detail::ext(physics_step_detail::f(bits)));}
    void fstp_check(){port.s.require_x87();port.s.fp.st(0);}
    void fstp(){port.s.fp.pop();}
    void discard(){port.s.require_x87();port.s.fp.st(0)=port.s.fp.st(0);port.s.fp.pop();}
    // A genuine child: EBX, ESI, EDI and EBP come back, and its RET pops what
    // the ABI says.
    template<class Call> void genuine(std::uint32_t esp_after,Call &&call) {
        const std::array<std::uint32_t,4> kept{reg(ebx),reg(esi),reg(edi),reg(ebp)};
        call();
        if(kept!=std::array<std::uint32_t,4>{reg(ebx),reg(esi),reg(edi),reg(ebp)} || reg(esp)!=esp_after)
            throw std::runtime_error("Native 304350 child broke the callee-saved registers or its stack cleanup");
    }
    std::uint32_t virtual_call(std::uint32_t pc,std::uint32_t slot_address,std::uint32_t return_to,std::uint32_t site,std::uint32_t esp_after) {
        const auto target=port.read(pc,slot_address,4);
        port.push(pc,return_to);
        genuine(esp_after,[&]{call_dispatch(port,target,site,children.dispatch);});
        return reg(eax);
    }

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t address) {
        using enum logic::collection_agent::Access;
        auto &flags=port.flags;
        switch(a) {
        case begin_end:port.push(0x30437bu,reg(edi));break;
        case begin_advance:case end_advance:reg(ecx)=flags.plus(next_event,0xcu,4);if(a==begin_advance)port.reached(a_timer);break;
        case wrapper_shape:port.push(0x3043bfu,logic::collection_agent::globals::candidate_filter);break;
        case records_data_init:reg(eax)=local.record_storage;reg(ebx)=flags.logic(0u,4);break;  // LEA EAX; XOR EBX,EBX
        case buffers_size:port.store(0x304422u,frame_base+0x32cu,1,1u);break;  // unwind state 1: the buffers live
        case child_key:
            // MOV EDX,[ECX] (the inner object's vtable); ADD ESI,EAX (the buffer)
            reg(edx)=inner_vtable=port.read(0x304472u,reg(ecx),4);
            reg(esi)=flags.plus(buffer_offset,buffers_base,4);
            break;
        // SHL size,7 before each field store of the appended record
        case agent_store:case has_store:reg(edx)=flags.shift(reg(edx),7u,4);break;
        case shape_store:reg(ecx)=flags.shift(reg(ecx),7u,4);break;
        case key_store:reg(eax)=flags.shift(reg(eax),7u,4);break;
        case contact_position_store:reg(edx)=flags.shift(reg(edx),7u,4);reg(eax)=reg(edx)+reg(eax)-0x80u;break;
        case welded_vector_store:reg(edx)=flags.shift(reg(edx),7u,4);reg(eax)=reg(edx)+reg(eax)-0x30u;break;
        case state_size:if(kind==1u||kind==2u)return 0x304622u;break;
        case state_data:if(kind==1u||kind==2u)return 0x304629u;break;
        case state_store:
            byte_register(eax,kind==1u||kind==2u?1u:flags.logic(0u,1));
            reg(ecx)=flags.shift(reg(ecx),7u,4);
            if(kind==1u||kind==2u)return 0x304635u;
            break;
        // The search's general registers are overwritten before any call.
        case group_distance:lane=next_lane;next_lane=(next_lane+1u)&3u;return group_compare_pc[lane];
        case group_distance_again:discard();port.reached(a_replaced);return group_hold_pc[lane];
        case rest_distance_again:discard();port.reached(a_replaced);break;
        case swap_first_position:discard();break;                                   // FSTP ST(0): the closest
        case swap_best_position:port.s.require_x87();std::swap(port.s.fp.st(0),port.s.fp.st(1));break;  // FXCH
        case welded_distance_store:case swap_to_first_distance:case swap_to_first_word_24:case point_distance_store:
            fstp_check();break;
        case point_position_store:
            // LEA EAX,[ESI+10]; PUSH EAX (2E47B0's vector); ADD EDX,20
            reg(eax)=point+0x10u;port.push(0x3048efu,reg(eax));reg(edx)=flags.plus(motion,0x20u,4);
            break;
        case point_normal_store:
            // 304130's arguments: the parameter, n, 0, the records, the contact;
            // ECX = this, re-read.
            port.push(0x304917u,parameter);
            reg(ecx)=port.read(0x304918u,local.agent,4);
            port.push(0x30491cu,reg(ebx));port.push(0x30491du,0u);
            reg(edx)=local.records;port.push(0x304926u,reg(edx));
            reg(eax)=local.point_contact;port.push(0x30492bu,reg(eax));
            break;
        case separation_store:
            fstp_check();
            reg(ecx)=output=port.read(0x3049e9u,local.output,4);
            port.push(0x3049ecu,reg(eax));port.push(0x3049edu,reg(ecx));
            break;
        case records_storage:
            // MOV EDX,[ECX]; AND EAX,7FFFFFFF; PUSH 12; SHL EAX,7; PUSH EAX
            reg(edx)=port.read(0x304a8bu,owner,4);
            reg(eax)=flags.logic(capacity&logic::contact_array::capacity_mask,4);
            port.push(0x304a92u,logic::local_buffers::array_storage_class);
            reg(eax)=pushed_bytes=flags.shift(reg(eax),7u,4);
            port.push(0x304a97u,reg(eax));
            break;
        default:break;
        }
        return collection_agent_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::collection_agent::Access;
        auto &flags=port.flags;
        switch(a) {
        case begin_next:case end_next:reg(ecx)=next_event=value;ended=a==end_next;break;
        case begin_end:case end_end:
            // CMP ECX,[end]; SETB AL; TEST AL,AL
            flags.minus(next_event,value,4);byte_register(eax,next_event<value);flags.logic(next_event<value,1);
            if(a==begin_end)port.store(0x304387u,local.agent,4,reg(esi));  // this, kept for the reloads
            break;
        case begin_time_load:reg(eax)=value;break;
        case end_time_load:reg(edx)=value;break;
        case body_b_argument:reg(edi)=value;break;
        case body_b_motion:case body_b_motion_again:reg(ecx)=value;break;
        case body_b_shape:case collection_shape_read:reg(eax)=value;break;
        case body_b_key:reg(edx)=value;break;
        case input_argument:reg(eax)=value;port.push(0x3043cbu,value);reg(ecx)=local.wrapper_body;break;
        case body_a_argument:reg(edx)=body_a=value;break;
        case child_count:reg(eax)=value;break;
        case child_count_again:reg(eax)=value;flags.minus(value,reg(ebx),4);if(std::int32_t(value)<=0)port.reached(a_no_children);break;
        case wrapper_shape_again:reg(edx)=value;break;
        case children_data:entry_offset=reg(ebx);reg(edi)=entry=flags.plus(value,reg(ebx),4);break;
        case entry_agent:
            flags.minus(value,reg(eax),4);
            if(value==reg(eax))port.reached(a_null_agent);
            break;
        case collection_shape_reload:reg(eax)=value;break;
        case collection_inner:reg(ecx)=value;break;
        case buffers_data:reg(eax)=buffers_base=value;break;
        case buffer_offset_reload:reg(esi)=buffer_offset=value;break;
        case child_key:reg(eax)=value;break;
        case body_a_reload:reg(ecx)=value;break;
        case body_a_shape:reg(ebx)=a_shape=value;reg(esi)=child;break;
        case child_key_again:reg(eax)=value;break;
        case input_reload:reg(ebx)=value;break;
        case dispatcher:reg(ecx)=value;break;
        case verdict:
            // MOV CL,[EAX]; ADD ESP,10; TEST CL,CL
            byte_register(ecx,value);reg(esp)=flags.plus(reg(esp),0x10u,4);flags.logic(value,1);
            port.reached(value?a_gather:a_process);
            break;
        case gather_input:reg(edx)=value;break;
        case gathered_agent:reg(ebx)=value;break;
        case agent_size:case has_size:case contact_size:case welded_size:reg(edx)=value;break;
        case agent_data:case contact_data:case welded_data:reg(eax)=value;break;
        case shape_size:case state_size:reg(ecx)=value;break;
        case shape_data:case state_data:reg(edx)=value;break;
        case record_key:reg(edi)=value;break;
        case key_size:reg(eax)=value;break;
        case key_data:case has_data:reg(ecx)=value;break;
        case found:byte_register(eax,value);break;
        case found_again:byte_register(eax,value);flags.logic(value,1);port.reached(value?a_contact:a_no_contact);break;
        case contact_distance:reg(ecx)=value;break;
        case contact_word_24:reg(edx)=value;break;
        case body_motion:reg(ecx)=contact_motion=value;break;
        case welded_distance:fld(value);break;
        case agent_kind:
            byte_register(ebx,value);kind=value;flags.minus(value,1u,1);if(value!=1u)flags.minus(value,2u,1);
            port.reached(value==1u||value==2u?a_marked:a_unmarked);
            break;
        case output_argument:reg(ecx)=value;break;
        case processed_agent:reg(edi)=value;break;
        case agent_reload:reg(esi)=value;break;
        case child_index_reload:reg(eax)=value;break;
        case entry_offset_reload:reg(ebx)=value;break;
        case buffer_offset_next:reg(edx)=value;break;
        case children_size:
            // INC EAX; ADD EBX,C; ADD EDX,100; CMP EAX,ECX
            reg(ecx)=value;
            reg(eax)=flags.increment(reg(eax),4,false);reg(ebx)=entry_offset=flags.plus(reg(ebx),0xcu,4);
            reg(edx)=flags.plus(reg(edx),0x100u,4);flags.minus(reg(eax),value,4);
            break;
        case sort_count:
            // XOR EDX,EDX; CMP EAX,EDX; MOV EBX,EAX
            reg(eax)=value;reg(edx)=flags.logic(0u,4);flags.minus(value,0u,4);reg(ebx)=value;
            if(std::int32_t(value)<=0)port.reached(a_no_records);
            break;
        case sort_index_reload:case weld_index_reload:reg(edx)=value;break;
        case first_distance:fld(value);next_lane=0;break;
        case group_distance:case rest_distance:
            port.s.require_x87();port.s.fp.comparison(physics_step_detail::ext(physics_step_detail::f(value)),0);
            port.reached(a==group_distance?a_groups:a_rest);
            break;
        case group_distance_again:case rest_distance_again:fld(value);break;
        case swap_best_distance:case swap_best_word_24:fld(value);break;
        case swap_best_shape_key:case swap_first_distance:case swap_first_word_24:case swap_first_agent:
        case swap_first_child_shape:case swap_first_shape_key:reg(edx)=value;break;
        case swap_best_state:case swap_best_has:case swap_first_state:case swap_first_has:case swapped_state_reload:
            byte_register(edx,value);break;
        case swap_best_agent:reg(esi)=value;break;  // (ESI and EAX were the closest record)
        case swap_best_child_shape:reg(edi)=value;break;
        case swapped_key_reload:reg(eax)=value;break;
        case swapped_has_reload:byte_register(eax,value);break;
        case sorted_offset_reload:reg(esi)=value;break;
        case sorted_data:reg(eax)=value;break;
        case sorted_has_contact:
            // TEST DL,DL; LEA ECX,[ESI+EAX+50]
            byte_register(edx,value);flags.logic(value,1);reg(ecx)=reg(esi)+reg(eax)+0x50u;
            if(value)port.reached(a_welded);
            break;
        case weld_input:reg(edx)=value;break;
        case weld_parameter:reg(eax)=value;break;
        case next_index_reload:
            // INC EAX; ADD ESI,80; CMP EAX,EBX
            reg(eax)=flags.increment(value,4,false);reg(esi)=flags.plus(reg(esi),0x80u,4);flags.minus(reg(eax),reg(ebx),4);
            break;
        case sorted_size:reg(eax)=value;break;
        case points_data:reg(ecx)=value;break;
        case point_offset_reload:reg(edx)=value;break;
        case points_agent:reg(esi)=value;break;
        case point_count:
            reg(edi)=flags.logic(value,4);
            if(std::int32_t(value)>0){reg(esi)=point=flags.plus(reg(esi),0x40u,4);port.reached(a_points);}
            else port.reached(a_no_points);
            break;
        case point_body_b:reg(ecx)=value;break;
        case point_distance:fld(value);break;
        case point_motion:reg(edx)=motion=value;break;
        case point_input:reg(eax)=value;break;
        case point_weld_parameter:reg(ecx)=parameter=value;break;
        case point_offset_next:reg(ecx)=value;break;
        case points_left_next:
            // ADD ECX,80; DEC EAX
            reg(eax)=value;reg(ecx)=flags.plus(reg(ecx),0x80u,4);reg(eax)=flags.increment(value,4,true);
            break;
        case record_count_reload:reg(eax)=value;reg(edi)=flags.logic(0u,4);flags.logic(value,4);break;
        case emit_body_b:reg(eax)=value;break;
        case emit_motion:reg(ecx)=value;break;
        case emit_offset_reload:reg(edx)=value;break;
        case emit_data:reg(eax)=record=flags.plus(value,reg(edx),4);flags.minus(reg(edi),reg(ebx),4);break;
        case emit_shape_key:reg(ecx)=value;break;
        case emit_child_shape:reg(edx)=value;break;
        case emit_agent:reg(esi)=agent=value;break;
        case emit_has_contact:byte_register(ecx,value);flags.logic(value,1);break;
        case distance_data:reg(ecx)=value;break;
        case distance_offset_reload:reg(eax)=value;break;
        case emit_distance:fld(value);break;
        case emit_agent_reload:reg(edx)=value;break;
        case contact_manager:reg(eax)=value;break;
        case distance_bias:
            port.s.require_x87();
            port.s.fp.st(0)=physics_step_detail::sub(port.s.fp.st(0),physics_step_detail::ext(physics_step_detail::f(value)));
            break;
        case separation_load:reg(eax)=value;break;
        case emit_offset_next:reg(ecx)=value;break;
        case record_count_next:
            // ADD ESP,20; INC EDI; ADD ECX,80; CMP EDI,EAX
            reg(eax)=value;reg(esp)=flags.plus(reg(esp),0x20u,4);reg(edi)=flags.increment(reg(edi),4,false);
            reg(ecx)=flags.plus(reg(ecx),0x80u,4);flags.minus(reg(edi),value,4);
            break;
        case records_capacity:
            reg(eax)=capacity=flags.logic(value,4);
            port.store(0x304a78u,frame_base+0x32cu,4,0xffffffffu);  // unwind state -1
            port.reached(value&logic::local_buffers::storage_not_owned?a_borrowed:a_owned);
            break;
        case thread_memory:reg(ecx)=owner=value;break;
        case records_storage:reg(eax)=value;break;
        default:break;
        }
    }
    void loaded_vector(Access a,__m128 value) {
        using enum logic::collection_agent::Access;
        switch(a) {
        case contact_position:case contact_normal:case contact_vector_30:case contact_vector_40:case welded_vector:
        case welded_normal:case swap_best_position:case point_position:case point_normal:port.set_xmm(0,value);break;
        case swap_best_normal:port.set_xmm(1,value);break;
        case swap_best_vector_30:port.set_xmm(2,value);break;
        case swap_best_vector_40:port.set_xmm(3,value);break;
        case swap_best_vector_50:port.set_xmm(4,value);break;
        case swap_best_vector_60:port.set_xmm(5,value);break;
        case swap_first_position:case swap_first_normal:case swap_first_vector_30:case swap_first_vector_40:case swap_first_vector_50:
        case swap_first_vector_60:port.set_xmm(6,value);break;
        default:break;
        }
    }
    void stored(Access a) {
        using enum logic::collection_agent::Access;
        switch(a) {
        case welded_distance_store:case swap_to_first_distance:case swap_to_first_word_24:case point_distance_store:
        case separation_store:fstp();break;
        case agent_store:case shape_store:case key_store:case has_store:break;
        default:break;
        }
    }

    // ---- Frame: the locals, the time stamp counter and the calls
    const logic::collection_agent::Locals &locals()const{return local;}
    // RDTSC (the unit's service): EDX:EAX.
    std::uint32_t time_stamp_counter() {
        port.at(ended?0x304a48u:0x304393u);
        if(!port.s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");
        const auto counter=port.time_stamp_counter();
        reg(eax)=std::uint32_t(counter);reg(edx)=std::uint32_t(counter>>32u);
        return reg(eax);
    }
    // 2F36F0(this, body A, &wrapper body, input, 2F3140): stdcall, RET 10.
    void update_child_agents(std::uint32_t body,std::uint32_t wrapper,std::uint32_t) {
        port.push(0x3043d7u,wrapper);port.push(0x3043d8u,body);
        reg(ecx)=reg(esi);
        port.push(0x3043dbu,0x3043e0u);
        genuine(frame_base,[&]{port.call_child(0x2f36f0u,children.update);});
    }
    // 303C70 on the buffers (inline), with the unwind state 0 stored first.
    void construct_buffers(std::uint32_t count) {
        port.push(0x304405u,count);
        reg(ecx)=local.buffers;
        port.store(0x30440au,frame_base+0x32cu,4,reg(ebx));
        port.push(0x304411u,0x304416u);
        kernel_303c70(port,children.dispatch);
    }
    // 30BAD0, hkpNullAgent::getNullAgent(), inline: CALL; MOV EAX,48CDC4; RET.
    std::uint32_t null_agent() {
        port.push(0x304455u,0x30445au);
        reg(eax)=logic::collection_agent::globals::null_agent;
        port.ret(0x30bad5u,0u);
        return reg(eax);
    }
    // inner->getChildShape(key, buffer): EAX = the key, ECX = the inner
    // object, EDX = its vtable (read before the key), EBX = the entry's offset,
    // ESI = the buffer, EDI = the entry; ADD ESI,EAX's flags. RET 8.
    std::uint32_t child_shape(std::uint32_t inner,std::uint32_t key,std::uint32_t buffer) {
        port.push(0x304478u,buffer);port.push(0x304479u,key);
        child=virtual_call(0x30447au,inner_vtable+std::uint32_t(logic::collection_agent::Virtual::child_shape),0x30447du,
            child_shape_site,frame_base);
        (void)inner;
        return child;
    }
    // child->getType(); the result is pushed at once (the predicate's last
    // argument).
    std::uint32_t child_shape_type(std::uint32_t shape) {
        reg(edx)=port.read(0x30448eu,shape,4);reg(ecx)=shape;
        const auto type=virtual_call(0x304492u,reg(edx)+std::uint32_t(logic::collection_agent::Virtual::shape_type),0x304495u,
            child_type_site,frame_base);
        port.push(0x304495u,type);
        return type;
    }
    // bodyA.shape->getType()
    std::uint32_t body_shape_type(std::uint32_t shape) {
        reg(eax)=port.read(0x304496u,shape,4);reg(ecx)=shape;
        return virtual_call(0x30449au,reg(eax)+std::uint32_t(logic::collection_agent::Virtual::shape_type),0x30449du,
            body_type_site,frame_base-4u);
    }
    // (*[46E46C])(&verdict, dispatcher, type A, type B): cdecl; returns the
    // hkBool's address. EDX = the verdict's address.
    std::uint32_t weld_predicate(std::uint32_t dispatcher,std::uint32_t type_a,std::uint32_t) {
        port.push(0x3044a2u,type_a);port.push(0x3044a3u,dispatcher);
        reg(edx)=frame_base+0x2fu;port.push(0x3044a8u,reg(edx));
        return virtual_call(0x3044a9u,logic::collection_agent::globals::weld_predicate,0x3044afu,predicate_site,frame_base-0x10u);
    }
    // agent->closestContact(&found, bodyA, &child body, input, &contact):
    // RET 14; TEST CL,CL's flags (the verdict).
    void closest_contact(std::uint32_t child_agent,std::uint32_t input) {
        reg(eax)=port.read(0x3044c2u,child_agent,4);
        reg(ecx)=local.contact;port.push(0x3044cbu,reg(ecx));
        port.push(0x3044ccu,input);
        reg(edx)=port.read(0x3044cdu,local.body_a,4);
        reg(ecx)=local.child_body;port.push(0x3044d4u,reg(ecx));
        port.push(0x3044d5u,reg(edx));
        reg(ecx)=local.contact_found;port.push(0x3044dau,reg(ecx));
        reg(ecx)=child_agent;
        virtual_call(0x3044ddu,reg(eax)+std::uint32_t(logic::collection_agent::Virtual::closest_contact),0x3044e0u,
            closest_site,frame_base);
    }
    // 303EE0 on the records (inline).
    void append_record() {
        reg(ecx)=local.records;
        port.push(0x3044e7u,0x3044ecu);
        kernel_303ee0(port,children.reserve);
    }
    // 2E47B0 inline: out = the rotated normal, [ESP+4] = body B's rotation,
    // [ESP+8] = the vector. ADD ECX,20's flags.
    void rotate_contact_normal(std::uint32_t out,std::uint32_t rotation,std::uint32_t vector) {
        reg(eax)=vector;reg(ecx)=port.flags.plus(contact_motion,motion_rotation_offset,4);
        if(reg(ecx)!=rotation)throw std::logic_error("Native 304350 rotation differs from the pushed one");
        port.push(0x3045b8u,reg(eax));port.push(0x3045b9u,reg(ecx));
        reg(ecx)=out;
        port.push(0x3045c1u,0x3045c6u);
        kernel_2e47b0(port);
    }
    void rotate_point_normal(std::uint32_t out,std::uint32_t rotation,std::uint32_t) {
        if(reg(edx)!=rotation)throw std::logic_error("Native 304350 rotation differs from the pushed one");
        port.push(0x3048fcu,reg(edx));
        reg(ecx)=out;
        port.push(0x304904u,0x304909u);
        kernel_2e47b0(port);
    }
    // agent->processCollision(bodyA, &child body, input, output): RET 10;
    // TEST CL,CL's flags (CL = 0).
    void process_child(std::uint32_t child_agent,std::uint32_t output_address) {
        reg(eax)=port.read(0x304641u,child_agent,4);
        port.push(0x304643u,output_address);
        reg(ecx)=port.read(0x304644u,local.body_a,4);
        port.push(0x304647u,reg(ebx));
        reg(edx)=local.child_body;port.push(0x30464cu,reg(edx));
        port.push(0x30464du,reg(ecx));
        reg(ecx)=child_agent;
        virtual_call(0x304650u,reg(eax)+std::uint32_t(logic::collection_agent::Virtual::process_collision),0x304653u,
            process_site,frame_base);
    }
    // 304130(this; contact, &records, first, n, parameter): thiscall, RET 14;
    // returns the new n (EBX).
    std::uint32_t weld_sorted(std::uint32_t contact,std::uint32_t parameter,std::uint32_t first,std::uint32_t n) {
        port.push(0x30486eu,parameter);port.push(0x30486fu,n);
        reg(edx)=port.flags.increment(reg(edx),4,false);
        if(reg(edx)!=first || reg(ecx)!=contact)throw std::logic_error("Native 304350 weld arguments differ from the pushed ones");
        port.push(0x304871u,reg(edx));
        reg(eax)=local.records;port.push(0x304879u,reg(eax));
        port.push(0x30487au,reg(ecx));
        reg(ecx)=port.read(0x30487bu,local.agent,4);
        port.push(0x30487fu,0x304884u);
        genuine(frame_base,[&]{port.call_child(0x304130u,children.weld);});
        return reg(ebx)=reg(eax);
    }
    // The point weld: its arguments were pushed before the contact's normal
    // was stored.
    std::uint32_t weld_point(std::uint32_t,std::uint32_t,std::uint32_t) {
        port.push(0x304931u,0x304936u);
        genuine(frame_base,[&]{port.call_child(0x304130u,children.weld);});
        // ADD ESI,30 (the next point); DEC EDI; MOV EBX,EAX
        reg(esi)=point=port.flags.plus(reg(esi),0x30u,4);
        reg(edi)=port.flags.increment(reg(edi),4,true);
        return reg(ebx)=reg(eax);
    }
    // 30CD00(bodyA, &child body, input, record, this->contact manager,
    // agent+30, agent+18): cdecl; TEST CL,CL's flags (the record's contact).
    void confirm_contact(std::uint32_t,std::uint32_t child_agent) {
        reg(ecx)=child_agent+logic::collection_agent::point_set;port.push(0x3049aau,reg(ecx));
        reg(ecx)=port.read(0x3049abu,local.agent,4);
        reg(edx)=child_agent+logic::collection_agent::point_list;port.push(0x3049b2u,reg(edx));
        reg(edx)=port.read(0x3049b3u,reg(ecx)+std::uint32_t(offsetof(logic::collection_agent::Agent,contact_manager)),4);
        port.push(0x3049b6u,reg(edx));
        reg(edx)=port.read(0x3049b7u,local.body_a,4);
        port.push(0x3049bau,reg(eax));
        reg(eax)=port.read(0x3049bbu,local.input,4);port.push(0x3049beu,reg(eax));
        reg(ecx)=local.child_body;port.push(0x3049c3u,reg(ecx));
        port.push(0x3049c4u,reg(edx));
        port.push(0x3049c5u,0x3049cau);
        genuine(frame_base-0x1cu,[&]{port.call_child(0x30cd00u,children.confirm);});
        reg(esp)=port.flags.plus(reg(esp),0x1cu,4);
        port.reached(a_confirmed);
    }
    // 30CF40(bodyA, &child body, input, separation, agent+30, agent+18,
    // output, this->contact manager): cdecl; ADD ESI,30's flags. The output
    // and the manager were pushed before the separation was stored.
    void report_contact(std::uint32_t separation,std::uint32_t child_agent,std::uint32_t) {
        reg(ecx)=port.read(0x3049f6u,local.input,4);
        reg(edx)=child_agent+logic::collection_agent::point_set;port.push(0x3049fcu,reg(edx));
        reg(esi)=port.flags.plus(reg(esi),logic::collection_agent::point_list,4);port.push(0x304a00u,reg(esi));
        port.push(0x304a01u,separation);
        reg(eax)=port.read(0x304a02u,local.body_a,4);
        port.push(0x304a05u,reg(ecx));
        reg(edx)=local.child_body;port.push(0x304a0au,reg(edx));
        port.push(0x304a0bu,reg(eax));
        port.push(0x304a0cu,0x304a11u);
        genuine(frame_base-0x20u,[&]{port.call_child(0x30cf40u,children.report);});
    }
    // 303D00 on the buffers (inline), with the unwind state 0 stored first.
    void destroy_buffers() {
        reg(ecx)=local.buffers;
        port.store(0x304a62u,frame_base+0x32cu,1,0u);
        port.push(0x304a6au,0x304a6fu);
        kernel_303d00(port,children.dispatch);
    }
    // thread memory->deallocate(data, bytes, class): RET C; SHL's flags. The
    // class and the size were pushed before the data was read.
    void deallocate_records(std::uint32_t thread,std::uint32_t data,std::uint32_t bytes,std::uint32_t storage_class) {
        if(thread!=owner || bytes!=pushed_bytes || storage_class!=logic::local_buffers::array_storage_class)
            throw std::logic_error("Native 304350 deallocate() arguments differ from the pushed ones");
        port.push(0x304a9fu,data);
        virtual_call(0x304aa0u,reg(edx)+std::uint32_t(logic::local_buffers::ThreadMemoryFunction::deallocate),0x304aa3u,
            deallocate_site,frame_base);
    }
    static constexpr std::uint32_t motion_rotation_offset=logic::collection_agent::motion_rotation;
};
template<class State,class Children> void kernel_304350(Port<State> &port,Children &children) {
    auto &gp=port.gp;
    const auto entry=gp[esp],self=gp[ecx];
    port.push(0x304350u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=port.flags.logic(gp[ebp]&0xfffffff0u,4);  // AND ESP,-16
    open_exception_frame(port,0x304356u,collection_agent_handler);
    gp[esp]=port.flags.minus(gp[esp],0x318u,4);
    port.push(0x304371u,gp[ebx]);port.push(0x304372u,gp[esi]);gp[esi]=self;
    const auto base=gp[esp]-4u;  // S: after PUSH EDI
    CollectionAgentAbi<State,Children> abi(port,children,base,gp[ebp]);
    abi.self=self;
    Memory<Port<State>,CollectionAgentAbi<State,Children>> memory{port,abi};
    const auto result=logic::collection_agent::process_collision(memory,abi,self);
    (void)result;
    // The exit: the previous exception frame, the saved registers, RET 10.
    gp[ecx]=port.read(0x304aa3u,base+0x324u,4);
    gp[edi]=port.pop(0x304aaau);gp[esi]=port.pop(0x304aabu);
    port.store(0x304aacu,port.s.fs_base,4,gp[ecx]);
    gp[ebx]=port.pop(0x304ab3u);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x304ab6u);
    if(gp[esp]!=entry)throw std::logic_error("Native 304350 frame is unbalanced");
    port.ret(0x304ab7u,0x10u);port.publish(0x304ab7u);
}

// ---- 30CF40 manifold report ------------------------------------------------
// cdecl: [ESP+4] body A, [+8] body B, [+C] the input, [+10] the separation,
// [+14] the cached points, [+18] their count, [+1C] the output, [+20] the
// contact manager. PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,24 makes S;
// EBX, ESI and EDI are pushed between the first reads. The locals are at
// S+C (the output's array, then each distance), +10 (the points kept), +14
// (the distance's copy), +18 (the drift), +1C (the squared drift tolerance)
// and +20 (the weight's copy).
//
// Registers: ESI walks the points at +10 (B's point), EBX the output, EDI is
// the count's address. Children:
//   2E31E0 (cdecl: the array, the capacity, 30; ADD ESP,C after) through the
//   unit's route, with EAX = the capacity pushed, ECX = the array, EDX = the
//   output and the flags of CMP ESI,EAX;
//   the contact manager's removal (vtable +8, thiscall, one argument, RET 4)
//   through the unit's dispatch, with EAX = the vtable, ECX = the manager,
//   EDX = the id and XOR EDX,EDX's flags.
// XMM0..XMM5 at the removal and at the exit are the check's values (see
// publish_check); the x87 stack is kept as the translated instructions keep
// it (FLD pushes, FCOMP records the status and pops, FSTP pops), and FNSTSW
// AX with TEST AH,5 or TEST AH,41 decides the flags of each test.
inline constexpr std::uint32_t manifold_remove_site=0x30965au;  // the canonical CALL [EAX+8]
constexpr std::uint32_t report_instruction(logic::manifold_report::Access a) {
    using enum logic::manifold_report::Access;
    switch(a) {
    case input_argument:return 0x30cf49u;case input_settings:return 0x30cf4cu;case drift_tolerance:return 0x30cf4fu;
    case output_argument:return 0x30cf52u;case output_array:return 0x30cf55u;case output_capacity:return 0x30cf59u;
    case output_size:return 0x30cf5fu;case drift_squared_store:return 0x30cf63u;case count_argument:return 0x30cf68u;
    case point_count:return 0x30cf6du;case kept_init:return 0x30cf78u;case array_keep:return 0x30cf80u;
    case array_reload:return 0x30cf97u;case output_data:return 0x30cf9eu;case output_size_store:return 0x30cfa6u;
    case point_count_again:return 0x30cfa9u;case points_argument:return 0x30cfb5u;
    case point_a:return 0x30cfc0u;case body_a_argument:return 0x30cfc4u;case body_a_motion:return 0x30cfc7u;
    case a_translation:return 0x30cfcau;case a_column_2:return 0x30cfceu;case body_b_argument:return 0x30cfd2u;
    case body_b_motion:return 0x30cfd5u;case b_column_2:return 0x30cfd8u;case a_column_1:return 0x30cfe6u;
    case a_column_0:return 0x30d001u;case point_b:return 0x30d00au;case b_translation:return 0x30d010u;
    case b_column_1:return 0x30d021u;case b_column_0:return 0x30d03cu;case normal:return 0x30d048u;
    case distance_store:return 0x30d072u;case distance_load:return 0x30d076u;case distance_bits:return 0x30d07au;
    case plane_input:return 0x30d07eu;case plane_tolerance:return 0x30d081u;case distance_copy_store:return 0x30d084u;
    case distance_copy_load:return 0x30d088u;case offset_store:return 0x30d0c9u;case separation_distance:return 0x30d0cfu;
    case separation_argument:return 0x30d0d3u;case offset_load:return 0x30d0ddu;case drift_squared_load:return 0x30d0e1u;
    case weight:return 0x30d0ecu;case kept_distance:return 0x30d0efu;case weight_store:return 0x30d0f3u;
    case weight_load:return 0x30d0f7u;case one:return 0x30d107u;case position_store:return 0x30d117u;
    case kept_normal:return 0x30d11au;case normal_store:return 0x30d11eu;case distance_out_store:return 0x30d122u;
    case kept_id:return 0x30d125u;case id_store:return 0x30d129u;case kept_load:return 0x30d12du;case kept_store:return 0x30d135u;
    case manager_argument:return 0x30d13eu;case removed_id:return 0x30d145u;case removed_count:return 0x30d14du;
    case removed_count_store:return 0x30d150u;case removed_points:return 0x30d154u;case last_point_a:return 0x30d15du;
    case point_a_store:return 0x30d163u;case last_point_b:return 0x30d167u;case point_b_store:return 0x30d16bu;
    case last_normal:return 0x30d16eu;case removed_output:return 0x30d172u;case point_normal_store:return 0x30d175u;
    case removed_array:return 0x30d179u;case output_size_load:case output_size_decrement:return 0x30d17bu;
    case kept_reload:return 0x30d17eu;case count_reload:return 0x30d182u;
    }
    return 0;
}
// 30CF40 (validated by tools/validate_native_manifold_tu15.py).
enum ReportBranch : unsigned {
    r_room=collection_branch_end,r_grow_doubled,r_grow_needed,r_no_points,r_kept,r_far,r_separated,r_drifted,
    report_branch_end
};
static_assert(report_branch_end<=coverage_bits);

template<class State,class Children> struct ReportAbi {
    using Access=logic::manifold_report::Access;
    Port<State> &port;Children &children;
    std::uint32_t frame_base;  // S
    logic::manifold_report::Locals local{};
    logic::manifold_report::PointCheck check{};
    std::uint32_t output{},array{};
    ReportAbi(Port<State> &p,Children &c,std::uint32_t s,std::uint32_t ebp):port(p),children(c),frame_base(s) {
        local={.body_a=ebp+8u,.body_b=ebp+0xcu,.input=ebp+0x10u,.separation=ebp+0x14u,.points=ebp+0x18u,
            .point_count=ebp+0x1cu,.output=ebp+0x20u,.manager=ebp+0x24u,
            .array=s+0xcu,.kept=s+0x10u,.distance_copy=s+0x14u,.offset_squared=s+0x18u,.drift_squared=s+0x1cu,.weight=s+0x20u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    // x87, exactly as the translated instructions keep the state's stack. An
    // x87 instruction first publishes itself, so a stack fault reports it.
    void x87_at(std::uint32_t pc){port.at(pc);port.s.require_x87();}
    void fld(std::uint32_t bits){port.s.fp.push(physics_step_detail::ext(physics_step_detail::f(bits)));}
    // FCOMP m32, then FNSTSW AX and TEST AH,mask.
    void fcomp(std::uint32_t bits,std::uint32_t mask) {
        port.s.fp.comparison(physics_step_detail::ext(physics_step_detail::f(bits)),1);
        const std::uint32_t word=port.s.fp.word();
        reg(eax)=(reg(eax)&0xffff0000u)|word;
        port.flags.logic((word>>8u)&mask,1);
    }
    // XMM0..XMM5 after a point's three tests (the drift's horizontal sum
    // leaves its y and z lanes above the sums; the distance's likewise).
    void publish_check() {
        using logic::lanes;using logic::vector;
        const auto drift=lanes(check.drift_terms),separation=lanes(check.separation_terms);
        port.set_xmm(0,check.drift_terms);port.set_xmm(1,check.world_a);port.set_xmm(2,check.world_b);
        port.set_xmm(3,vector({std::bit_cast<std::uint32_t>(check.drift_xy),drift[1],drift[1],drift[1]}));
        port.set_xmm(4,vector({std::bit_cast<std::uint32_t>(check.drift),drift[2],drift[2],drift[2]}));
        port.set_xmm(5,vector({std::bit_cast<std::uint32_t>(check.distance),separation[2],separation[2],separation[2]}));
    }

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::manifold_report::Access;
        auto &flags=port.flags;
        switch(a) {
        case drift_tolerance:case distance_load:case plane_tolerance:case separation_distance:case separation_argument:
        case offset_load:case drift_squared_load:
            x87_at(report_instruction(a));break;
        case output_capacity:x87_at(0x30cf57u);port.s.fp.push(port.s.fp.st(0));break;  // FLD ST(0)
        case output_size:
            port.s.require_x87();port.s.fp.st(0)=physics_step_detail::mul(port.s.fp.st(0),port.s.fp.st(1));  // FMUL
            port.push(0x30cf5eu,reg(ebx));
            break;
        case drift_squared_store:port.push(0x30cf62u,reg(esi));x87_at(0x30cf63u);break;
        case count_argument:port.push(0x30cf67u,reg(edi));break;
        case point_count:port.s.require_x87();port.s.fp.st(0)=port.s.fp.st(0);port.s.fp.pop();break;  // FSTP ST(0)
        case output_size_store:reg(ebx)=flags.shift(reg(ebx)*3u,4u,4);break;  // LEA EBX,[EBX+EBX*2]; SHL EBX,4
        case a_column_0:reg(eax)=flags.plus(reg(eax),0x20u,4);break;
        case b_column_0:reg(ecx)=flags.plus(reg(ecx),0x20u,4);break;
        case distance_store:reg(edx)=frame_base+0xcu;break;
        case offset_store:reg(ecx)=frame_base+0x18u;break;
        case removed_id:
            reg(eax)=port.read(0x30d141u,reg(ecx),4);   // the manager's vtable
            reg(edx)=flags.logic(0u,4);                  // XOR EDX,EDX
            break;
        case last_point_a:reg(eax)=flags.shift(reg(eax)*3u,4u,4);break;  // LEA EAX,[EAX+EAX*2]; SHL EAX,4
        default:break;
        }
        return report_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::manifold_report::Access;
        auto &flags=port.flags;
        switch(a) {
        case input_argument:reg(eax)=value;break;
        case input_settings:reg(ecx)=value;break;
        case drift_tolerance:case distance_load:case separation_distance:case offset_load:fld(value);break;
        case output_argument:reg(edx)=output=value;break;
        case output_array:reg(ecx)=array=value;break;
        case output_capacity:reg(eax)=value;break;
        case output_size:reg(ebx)=value;break;
        case count_argument:reg(edi)=value;break;
        case point_count:
            // ADD ESI,EBX; AND EAX,7FFFFFFF; CMP EAX,ESI
            reg(esi)=flags.plus(value,reg(ebx),4);
            reg(eax)=flags.logic(reg(eax)&logic::contact_array::capacity_mask,4);
            flags.minus(reg(eax),reg(esi),4);
            break;
        case array_reload:reg(ecx)=value;reg(esp)=flags.plus(reg(esp),0xcu,4);break;
        case output_data:reg(edx)=value;break;
        case point_count_again:
            // ADD EBX,EDX (the first output point); TEST EAX,EAX
            reg(eax)=value;reg(ebx)=flags.plus(reg(ebx),reg(edx),4);flags.logic(value,4);
            break;
        case points_argument:reg(esi)=flags.plus(value,0x10u,4);break;
        case body_a_argument:case body_a_motion:reg(eax)=value;break;
        case body_b_argument:case body_b_motion:reg(ecx)=value;break;
        case distance_bits:reg(eax)=value;break;
        case plane_input:reg(edx)=value;break;
        case plane_tolerance:case drift_squared_load:fcomp(value,5u);break;
        case separation_argument:fcomp(value,0x41u);break;
        case weight:reg(eax)=value;break;
        case kept_distance:reg(ecx)=value;break;
        case kept_id:case removed_id:reg(edx)=(reg(edx)&0xffff0000u)|(value&0xffffu);break;
        case kept_load:
            // INC EDX; ADD ESI,30 (the next point)
            reg(edx)=flags.increment(value,4,false);reg(esi)=flags.plus(reg(esi),0x30u,4);
            break;
        case manager_argument:reg(ecx)=value;break;
        case removed_count:reg(edx)=flags.increment(value,4,true);break;  // DEC EDX
        case removed_points:reg(edx)=value;break;
        case removed_output:case removed_array:reg(eax)=value;break;
        case output_size_load:flags.increment(value,4,true);break;  // DEC [EAX+4]
        case kept_reload:reg(ecx)=value;break;
        case count_reload:flags.minus(reg(ecx),value,4);break;  // CMP ECX,[EDI]
        default:break;
        }
    }
    void loaded_vector(Access a,__m128 value) {
        using enum logic::manifold_report::Access;
        switch(a) {
        case last_point_a:reg(eax)=port.flags.plus(reg(eax),reg(edx),4);port.set_xmm(0,value);break;  // ADD EAX,EDX
        case last_point_b:case last_normal:port.set_xmm(0,value);break;
        case kept_normal:kept_normal_value=value;break;
        default:break;
        }
    }
    void stored(Access a) {
        using enum logic::manifold_report::Access;
        switch(a) {
        case drift_squared_store:port.s.fp.pop();break;  // FSTP m32
        case removed_count_store:reg(eax)=reg(edx);break;
        case kept_store:reg(ebx)=port.flags.plus(reg(ebx),0x30u,4);break;  // ADD EBX,30
        default:break;
        }
    }

    // ---- Frame
    const logic::manifold_report::Locals &locals()const{return local;}
    // 2E31E0 (hkArrayUtil::_reserve): ADD EAX,EAX; CMP ESI,EAX; the larger
    // (signed) is pushed, then ECX = the array.
    void reserve(std::uint32_t array_address,std::uint32_t capacity,std::uint32_t element_size) {
        reg(eax)=port.flags.plus(reg(eax),reg(eax),4);
        port.flags.minus(reg(esi),reg(eax),4);
        const bool needed_more=!(std::int32_t(reg(esi))<std::int32_t(reg(eax)));
        if(needed_more)reg(eax)=reg(esi);
        grew=true;port.reached(needed_more?r_grow_needed:r_grow_doubled);
        if(reg(eax)!=capacity || reg(ecx)!=array_address)throw std::logic_error("Native 30CF40 reserve arguments differ from the registers");
        const auto esp_after=reg(esp)-0xcu;
        port.push(0x30cf8eu,element_size);port.push(0x30cf90u,reg(eax));port.push(0x30cf91u,reg(ecx));
        port.push(0x30cf92u,0x30cf97u);
        genuine(esp_after,[&]{port.call_child(0x2e31e0u,children.reserve);});
    }
    // manager->removeContactPoint(id): thiscall, RET 4.
    void remove_contact_point(std::uint32_t manager,std::uint32_t id) {
        if(reg(ecx)!=manager || (reg(edx)&0xffffu)!=id)throw std::logic_error("Native 30CF40 removal arguments differ from the registers");
        publish_check();
        const auto esp_after=reg(esp);
        port.push(0x30d149u,reg(edx));
        const auto target=port.read(0x30d14au,reg(eax)+8u,4);
        port.push(0x30d14au,0x30d14du);
        genuine(esp_after,[&]{call_dispatch(port,target,manifold_remove_site,children.dispatch);});
    }
    // A genuine child keeps EBX, ESI, EDI and EBP, and its RET pops what the
    // ABI says.
    template<class Call> void genuine(std::uint32_t esp_after,Call &&call) {
        const std::array<std::uint32_t,4> kept{reg(ebx),reg(esi),reg(edi),reg(ebp)};
        call();
        if(kept!=std::array<std::uint32_t,4>{reg(ebx),reg(esi),reg(edi),reg(ebp)} || reg(esp)!=esp_after)
            throw std::runtime_error("Native 30CF40 child broke the callee-saved registers or its stack cleanup");
    }
    // Each point's outcome. After a kept point XMM0 is its normal (re-read),
    // XMM1 the position and XMM3 weight * A; the rest are the check's.
    bool grew{},looped{};__m128 kept_normal_value{};
    void tested(const logic::manifold_report::PointCheck &c,logic::manifold_report::Failed failed) {
        using enum logic::manifold_report::Failed;
        check=c;looped=true;
        port.reached(failed==none?r_kept:failed==tolerance?r_far:failed==separation?r_separated:r_drifted);
    }
    void reported(const logic::manifold_report::PointCheck &,const logic::manifold_report::Reported &r) {
        publish_check();
        port.set_xmm(0,kept_normal_value);port.set_xmm(1,r.position);port.set_xmm(3,r.weighted_a);
    }
};
template<class State,class Children> void kernel_30cf40(Port<State> &port,Children &children) {
    auto &gp=port.gp;
    const auto entry=gp[esp];
    port.push(0x30cf40u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=port.flags.logic(gp[ebp]&0xfffffff0u,4);  // AND ESP,-16
    gp[esp]=port.flags.minus(gp[esp],0x24u,4);
    ReportAbi<State,Children> abi(port,children,gp[esp],gp[ebp]);
    Memory<Port<State>,ReportAbi<State,Children>> memory{port,abi};
    logic::manifold_report::verify_and_report(memory,abi);
    if(!abi.grew)port.reached(r_room);
    if(!abi.looped)port.reached(r_no_points);
    gp[edi]=port.pop(0x30d18au);gp[esi]=port.pop(0x30d18bu);gp[ebx]=port.pop(0x30d18cu);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x30d18fu);
    if(gp[esp]!=entry)throw std::logic_error("Native 30CF40 frame is unbalanced");
    port.ret(0x30d190u,0u);port.publish(0x30d190u);
}

// ---- 304130 contact weld ---------------------------------------------------
// thiscall: ECX = the agent; [ESP+4] the contact, [+8] the records (hkArray),
// [+C] first, [+10] n, [+14] the parameter; RET 14. PUSH EBP; MOV EBP,ESP;
// AND ESP,-16; SUB ESP,B4 makes S; EBX, ESI and EDI are pushed after the
// first read. Locals: S+7 the verdict, +8 the index, +20 this, +C..+30 the
// last record's field addresses, +A4..+B1 the held words and bytes.
//
// Registers: EDI = the record's offset, EBX = the end of the first n (both
// times 80), ESI = the records' data, EAX = n. The test (303800) is entered
// through the unit's dispatch with its canonical site: EAX = the record's
// second contact, ECX = this (re-read), EDX = the verdict's address, the
// flags of CMP [record+7C],1; RET 10. A swap leaves XMM0..XMM5 the replaced
// record's vectors and XMM6 the last record's second normal, and the x87
// stack as FLD, FLD, FXCH, FSTP, FSTP leave it.
inline constexpr std::uint32_t weld_test_site=0x3006a6u;  // the canonical CALL 303800
constexpr std::uint32_t weld_instruction(logic::contact_weld::Access a) {
    using enum logic::contact_weld::Access;
    switch(a) {
    case count_argument:return 0x30413cu;case first_argument:return 0x304142u;case self_keep:return 0x304147u;
    case index_init:return 0x30414bu;case records_argument:return 0x304160u;case records_data:return 0x304163u;
    case record_state:return 0x304165u;case parameter_argument:return 0x304170u;case contact_argument:return 0x304173u;
    case self_reload:return 0x30417du;case verdict:return 0x30418bu;case count_kept:return 0x304324u;
    case index_load:case index_store:return 0x304327u;case index_compare:return 0x304331u;
    case held_vector_50:return 0x304194u;case held_word_20:return 0x304199u;case held_vector_60:return 0x30419du;
    case held_word_24:return 0x3041a2u;case held_word_74:return 0x3041a6u;case held_word_70:return 0x3041acu;
    case last_position:return 0x3041b0u;case held_word_78:return 0x3041b5u;case held_position:return 0x3041b9u;
    case held_normal:return 0x3041bdu;case held_vector_30:return 0x3041c2u;case held_vector_40:return 0x3041c7u;
    case position_store:return 0x3041ccu;case last_normal:return 0x3041d0u;case normal_store:return 0x3041d5u;
    case held_word_78_store:return 0x3041dau;case last_word_20:return 0x3041e1u;case word_20_store:return 0x3041e5u;
    case held_word_70_store:return 0x3041e9u;case held_state:return 0x3041f0u;case held_word_74_store:return 0x3041f4u;
    case held_byte_7d:return 0x3041fbu;case held_state_store:return 0x3041ffu;case held_byte_7d_store:return 0x304206u;
    case at_word_20_store:return 0x304215u;case at_word_24_store:return 0x30421du;case last_word_24:return 0x304221u;
    case word_24_store:return 0x304223u;case last_vector_30:return 0x304227u;case vector_30_store:return 0x30422cu;
    case last_vector_40:return 0x304231u;case vector_40_store:return 0x304236u;case last_vector_50:return 0x30423bu;
    case vector_50_store:return 0x304240u;case last_vector_60:return 0x304245u;case vector_60_store:return 0x30424au;
    case at_vector_30_store:return 0x304253u;case at_vector_40_store:return 0x30425bu;case at_word_70_store:return 0x304267u;
    case last_word_70:return 0x30426bu;case word_70_store:return 0x30426du;case at_word_74_store:return 0x304275u;
    case last_word_74:return 0x304279u;case word_74_store:return 0x30427bu;case at_word_78_store:return 0x304283u;
    case last_word_78:return 0x304287u;case word_78_store:return 0x304289u;case at_state_store:return 0x304291u;
    case last_state:return 0x304295u;case state_store:return 0x304297u;case at_byte_7d_store:return 0x30429fu;
    case last_byte_7d:return 0x3042a3u;case byte_7d_store:return 0x3042a5u;
    case at_word_24_load:return 0x3042a9u;case written_normal:return 0x3042adu;case at_word_20_load:return 0x3042b0u;
    case written_word_20:return 0x3042b4u;case at_vector_30_load:return 0x3042b6u;case written_vector_30:return 0x3042bau;
    case at_word_70_load:return 0x3042bdu;case written_word_24:return 0x3042c1u;case at_vector_40_load:return 0x3042c3u;
    case written_vector_40:return 0x3042c7u;case held_word_74_load:return 0x3042cau;case written_position:return 0x3042d1u;
    case written_vector_50:return 0x3042d6u;case written_vector_60:return 0x3042d9u;case held_word_70_load:return 0x3042ddu;
    case written_word_70:return 0x3042e4u;case at_word_74_load:return 0x3042e6u;case held_word_78_load:return 0x3042eau;
    case written_word_74:return 0x3042f1u;case at_word_78_load:return 0x3042f3u;case held_state_load:return 0x3042f7u;
    case written_word_78:return 0x3042feu;case at_state_load:return 0x304300u;case held_byte_7d_load:return 0x304304u;
    case written_state:return 0x30430bu;case count_swap:return 0x30430du;case at_byte_7d_load:return 0x304310u;
    case written_byte_7d:return 0x304315u;case count_store:return 0x304317u;
    }
    return 0;
}
// 304130 (validated by tools/validate_native_manifold_tu15.py).
enum WeldBranch : unsigned {w_none=report_branch_end,w_untested,w_kept,w_swapped,w_swapped_self,weld_branch_end};
static_assert(weld_branch_end<=coverage_bits);

template<class State,class Dispatch> struct WeldAbi {
    using Access=logic::contact_weld::Access;
    Port<State> &port;Dispatch &dispatch;
    std::uint32_t frame_base;  // S
    logic::contact_weld::Locals local{};
    bool started{};
    WeldAbi(Port<State> &p,Dispatch &d,std::uint32_t s,std::uint32_t ebp):port(p),dispatch(d),frame_base(s) {
        local={.contact=ebp+8u,.records=ebp+0xcu,.first=ebp+0x10u,.count=ebp+0x14u,.parameter=ebp+0x18u,
            .self=s+0x20u,.index=s+0x8u,.verdict=s+0x7u,
            .held_word_70=s+0xa4u,.held_word_74=s+0xa8u,.held_word_78=s+0xacu,.held_state=s+0xb0u,.held_byte_7d=s+0xb1u,
            .at_word_20=s+0x30u,.at_word_24=s+0x18u,.at_vector_30=s+0x28u,.at_vector_40=s+0xcu,.at_word_70=s+0x10u,
            .at_word_74=s+0x14u,.at_word_78=s+0x1cu,.at_state=s+0x24u,.at_byte_7d=s+0x2cu};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    void byte_register(unsigned r,std::uint32_t value){reg(r)=(reg(r)&~0xffu)|(value&0xffu);}
    // The last record's field at offset: LEA [EBX+ESI+offset-80].
    std::uint32_t last_field(std::uint32_t offset){return reg(ebx)+reg(esi)+offset-0x80u;}

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::contact_weld::Access;
        auto &flags=port.flags;
        switch(a) {
        case first_argument:port.push(0x30413fu,reg(ebx));port.push(0x304140u,reg(esi));port.push(0x304141u,reg(edi));break;
        case records_argument:
            if(!started) {
                // MOV EBX,EAX; SHL EDI,7; SHL EBX,7 (MOV ECX,ECX)
                started=true;reg(ebx)=reg(eax);
                reg(edi)=flags.shift(reg(edi),7u,4);reg(ebx)=flags.shift(reg(ebx),7u,4);
            }
            break;
        case self_reload:
            // the test's arguments: the parameter, the record's second contact, the contact
            port.push(0x304176u,reg(edx));
            reg(eax)=reg(edi)+reg(esi)+std::uint32_t(offsetof(logic::contact_record::ContactRecord,second_contact));
            port.push(0x30417bu,reg(eax));port.push(0x30417cu,reg(ecx));
            break;
        case held_word_20:case held_word_24:port.at(weld_instruction(a));port.s.require_x87();break;
        case held_word_70:port.s.require_x87();std::swap(port.s.fp.st(0),port.s.fp.st(1));break;  // FXCH
        case at_word_20_store:
            reg(ecx)=last_field(0x10u);reg(eax)=last_field(0x20u);break;
        case at_word_24_store:reg(eax)=last_field(0x24u);break;
        case at_vector_30_store:reg(eax)=last_field(0x30u);break;
        case at_vector_40_store:reg(eax)=last_field(0x40u);break;
        case at_word_70_store:reg(eax)=last_field(0x50u);reg(edx)=last_field(0x70u);break;
        case at_word_74_store:reg(edx)=last_field(0x74u);break;
        case at_word_78_store:reg(edx)=last_field(0x78u);break;
        case at_state_store:reg(edx)=last_field(0x7cu);break;
        case at_byte_7d_store:reg(edx)=last_field(0x7du);break;
        case written_word_20:case written_word_24:port.s.require_x87();break;
        default:break;
        }
        return weld_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::contact_weld::Access;
        auto &flags=port.flags;
        switch(a) {
        case count_argument:case count_kept:reg(eax)=value;break;
        case first_argument:reg(edi)=value;flags.minus(value,reg(eax),4);break;  // CMP EDI,EAX
        case records_argument:reg(ecx)=value;break;
        case records_data:reg(esi)=value;break;
        case record_state:flags.minus(value,logic::contact_weld::tested_state,1);if(value!=1u)port.reached(w_untested);break;
        case parameter_argument:reg(edx)=value;break;
        case contact_argument:reg(ecx)=value;break;
        case self_reload:reg(ecx)=value;break;
        case verdict:flags.minus(value,0u,1);if(value)port.reached(w_kept);break;  // CMP byte [EAX],0
        case index_load:flags.increment(value,4,false);break;
        case index_compare:flags.minus(value,reg(eax),4);break;
        case held_word_20:case held_word_24:port.s.fp.push(physics_step_detail::ext(physics_step_detail::f(value)));break;
        case held_word_74:reg(ecx)=value;break;
        case held_word_70:reg(eax)=value;break;
        case held_word_78:case last_word_20:reg(edx)=value;break;
        case held_state:byte_register(eax,value);break;
        case held_byte_7d:byte_register(ecx,value);break;
        case last_word_24:reg(eax)=value;break;
        case last_word_70:case last_word_74:case last_word_78:reg(edx)=value;break;
        case last_state:case last_byte_7d:byte_register(edx,value);break;
        case at_word_24_load:case at_vector_40_load:case held_word_74_load:case at_word_78_load:reg(edx)=value;break;
        case at_word_20_load:case at_vector_30_load:case at_word_70_load:case held_word_78_load:case at_state_load:
        case count_swap:reg(ecx)=value;break;
        case held_word_70_load:case at_word_74_load:case at_byte_7d_load:reg(eax)=value;break;
        case held_state_load:byte_register(eax,value);break;
        case held_byte_7d_load:byte_register(edx,value);break;
        default:break;
        }
        if(a==at_byte_7d_load)reg(ecx)=flags.increment(reg(ecx),4,true);  // DEC ECX
    }
    void stored(Access a) {
        using enum logic::contact_weld::Access;
        switch(a) {
        case written_word_20:case written_word_24:port.s.fp.pop();break;  // FSTP m32
        case index_store:reg(edi)=port.flags.plus(reg(edi),0x80u,4);break;
        case count_store:reg(ebx)=port.flags.minus(reg(ebx),0x80u,4);reg(eax)=reg(ecx);break;  // SUB EBX,80; MOV EAX,ECX
        default:break;
        }
    }

    // ---- Frame
    const logic::contact_weld::Locals &locals()const{return local;}
    // this->test(&verdict, contact, &record.second_contact, parameter): the
    // three arguments were pushed before this was re-read.
    std::uint32_t test(std::uint32_t self,std::uint32_t verdict_address,std::uint32_t,std::uint32_t,std::uint32_t) {
        const auto esp_after=reg(esp)+0xcu;
        reg(edx)=verdict_address;port.push(0x304185u,reg(edx));
        port.push(0x304186u,0x30418bu);
        if(reg(ecx)!=self)throw std::logic_error("Native 304130 test receiver differs from the register");
        const std::array<std::uint32_t,4> kept{reg(ebx),reg(esi),reg(edi),reg(ebp)};
        call_dispatch(port,logic::contact_weld::weld_test,weld_test_site,dispatch);
        if(kept!=std::array<std::uint32_t,4>{reg(ebx),reg(esi),reg(edi),reg(ebp)} || reg(esp)!=esp_after)
            throw std::runtime_error("Native 304130 test broke the callee-saved registers or its stack cleanup");
        return reg(eax);
    }
    void swapped(const logic::contact_weld::Swapped &held) {
        port.set_xmm(0,held.position);port.set_xmm(1,held.normal);port.set_xmm(2,held.vector_30);
        port.set_xmm(3,held.vector_40);port.set_xmm(4,held.second_position);port.set_xmm(5,held.second_normal);
        port.set_xmm(6,held.last_second_normal);
        port.reached(reg(ebx)-0x80u==reg(edi)?w_swapped_self:w_swapped);
    }
};
template<class State,class Dispatch> void kernel_304130(Port<State> &port,Dispatch &dispatch) {
    auto &gp=port.gp;
    const auto entry=gp[esp],self=gp[ecx];
    port.push(0x304130u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=port.flags.logic(gp[ebp]&0xfffffff0u,4);  // AND ESP,-16
    gp[esp]=port.flags.minus(gp[esp],0xb4u,4);
    WeldAbi<State,Dispatch> abi(port,dispatch,gp[esp],gp[ebp]);
    Memory<Port<State>,WeldAbi<State,Dispatch>> memory{port,abi};
    gp[eax]=logic::contact_weld::weld(memory,abi,self);
    if(!abi.started)port.reached(w_none);
    gp[edi]=port.pop(0x30433bu);gp[esi]=port.pop(0x30433cu);gp[ebx]=port.pop(0x30433du);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x304340u);
    if(gp[esp]!=entry)throw std::logic_error("Native 304130 frame is unbalanced");
    port.ret(0x304341u,0x14u);port.publish(0x304341u);
}

// ---- 2F33B0 key sort -------------------------------------------------------
// cdecl: [ESP+4] the keys, [+8] low, [+C] high, [+10] the comparator. PUSH
// EBP; MOV EBP,ESP; AND ESP,-16; PUSH ECX (a slot never read); EBX, ESI and
// EDI are pushed between the argument reads. Registers: ESI = the keys, EDX =
// low, EBX = high, EAX = i, EDI = j, ECX = the pivot. The recursion on the
// lower part runs this kernel inline at its own boundary (PUSH the
// comparator, i, low, the keys; the return address; ADD ESP,10 after), so its
// registers and frame are the original's. The exit flags are CMP EDI,EBX's.
constexpr std::uint32_t sort_instruction(logic::key_sort::Access a) {
    using enum logic::key_sort::Access;
    switch(a) {
    case low_argument:return 0x2f33b7u;case high_argument:return 0x2f33bbu;case keys_argument:return 0x2f33bfu;
    case pivot:return 0x2f33c8u;case scan_up_first:return 0x2f33d0u;case scan_up:return 0x2f33d6u;
    case scan_down_first:return 0x2f33dbu;case scan_down:return 0x2f33e1u;case swap_low_key:return 0x2f33ecu;
    case swap_high_key:return 0x2f33efu;case swap_high_store:return 0x2f33f2u;case high_reload:return 0x2f33f5u;
    case swap_low_store:return 0x2f33f8u;case low_reload:return 0x2f33fbu;case comparator_argument:return 0x2f3408u;
    case low_store:return 0x2f341du;
    }
    return 0;
}
// 2F33B0 (validated by tools/validate_native_key_sort_tu15.py).
enum SortBranch : unsigned {k_scan_up=weld_branch_end,k_scan_down,k_swap,k_recurse,k_tail,sort_branch_end};
static_assert(sort_branch_end<=coverage_bits);

template<class State> struct SortAbi {
    using Access=logic::key_sort::Access;
    Port<State> &port;
    logic::key_sort::Locals local{};
    SortAbi(Port<State> &p,std::uint32_t ebp):port(p) {
        local={.keys=ebp+8u,.low=ebp+0xcu,.high=ebp+0x10u,.comparator=ebp+0x14u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::key_sort::Access;
        switch(a) {
        case low_argument:port.push(0x2f33b6u,reg(ecx));break;
        case high_argument:port.push(0x2f33bau,reg(ebx));break;
        case keys_argument:port.push(0x2f33beu,reg(esi));break;
        case scan_up:port.reached(k_scan_up);break;
        case scan_down:port.reached(k_scan_down);break;
        case swap_high_store:port.reached(k_swap);break;
        case low_store:port.flags.minus(reg(edi),reg(ebx),4);reg(edx)=reg(edi);port.reached(k_tail);break;  // CMP EDI,EBX; MOV EDX,EDI
        default:break;
        }
        return sort_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::key_sort::Access;
        switch(a) {
        case low_argument:reg(edx)=value;break;
        case high_argument:reg(ebx)=value;break;
        case keys_argument:reg(esi)=value;port.push(0x2f33c2u,reg(edi));break;
        case comparator_argument:reg(ecx)=value;break;
        default:break;
        }
    }
    // ---- Frame
    const logic::key_sort::Locals &locals()const{return local;}
    // The registers a partition leaves; CMP EDX,EAX decides the recursion.
    void partitioned(std::uint32_t pivot,std::uint32_t i,std::uint32_t j,std::uint32_t low,std::uint32_t high) {
        reg(ecx)=pivot;reg(eax)=i;reg(edi)=j;reg(edx)=low;reg(ebx)=high;
        port.flags.minus(reg(edx),reg(eax),4);
    }
    void sort_part(std::uint32_t keys,std::uint32_t low,std::uint32_t high,std::uint32_t comparator);
};
template<class State> void kernel_2f33b0(Port<State> &port) {
    auto &gp=port.gp;
    const auto entry=gp[esp];
    port.push(0x2f33b0u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=port.flags.logic(gp[ebp]&0xfffffff0u,4);  // AND ESP,-16
    SortAbi<State> abi(port,gp[ebp]);
    Memory<Port<State>,SortAbi<State>> memory{port,abi};
    logic::key_sort::sort(memory,abi);
    port.flags.minus(gp[edi],gp[ebx],4);  // CMP EDI,EBX
    gp[edi]=port.pop(0x2f3422u);gp[esi]=port.pop(0x2f3423u);gp[ebx]=port.pop(0x2f3424u);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2f3427u);
    if(gp[esp]!=entry)throw std::logic_error("Native 2F33B0 frame is unbalanced");
    port.ret(0x2f3428u,0u);port.publish(0x2f3428u);
}
// The recursion: the arguments and the return address pushed, the kernel at
// its own boundary, then ADD ESP,10.
template<class State> void SortAbi<State>::sort_part(std::uint32_t keys,std::uint32_t low,std::uint32_t high,std::uint32_t comparator) {
    if(reg(ecx)!=comparator || reg(eax)!=high || reg(edx)!=low || reg(esi)!=keys)
        throw std::logic_error("Native 2F33B0 recursion arguments differ from the registers");
    port.push(0x2f340bu,reg(ecx));port.push(0x2f340cu,reg(eax));port.push(0x2f340du,reg(edx));port.push(0x2f340eu,reg(esi));
    port.push(0x2f340fu,0x2f3414u);
    port.reached(k_recurse);
    kernel_2f33b0(port);
    reg(esp)=port.flags.plus(reg(esp),0x10u,4);
}

// ---- 1F76A0 shape key filter ----------------------------------------------
// EDX = the shape key; the result in AL (BL holds it: MOV BL,1, XOR BL,BL on
// the paths that fail). EBX and ESI are pushed, EDI around kind 1's reads.
// The kind selects a target of the jump table at 1F77AC (read at 1F76BA): the
// four known targets are the four cases; any other entry throws the
// translated route's message (the unit's canonical_branch(): unknown values
// are "Unbound TU1.5 contact dispatch target", other labels and 0 "Required
// original solver dispatch target unavailable"). Kind 1's 185AA0 is entered
// through the unit's dispatch with EAX = the breakable index (canonical site
// 1F2E0D) and returns AL. Each path leaves its own EAX upper bytes, ECX, EDX
// and flags (the hooks mirror them); the exits are 1F770A (kind 1 fails),
// 1F7754 (kind 2 fails) and 1F77A5/1F77A7 (the rest).
inline constexpr std::uint32_t filter_table=0x1f77acu,filter_breakable_target=0x186e70u,filter_breakable_site=0x1f2e0du;
constexpr std::uint32_t filter_instruction(logic::shape_key_filter::Access a) {
    using enum logic::shape_key_filter::Access;
    switch(a) {
    case instanced_structure:return 0x1f76c1u;case instances:return 0x1f76cdu;case definition_index:return 0x1f76d3u;
    case definitions:return 0x1f76d8u;case instance_surfaces:return 0x1f76e4u;case instance_flags:return 0x1f76eeu;
    case instance_breakable:return 0x1f76f9u;
    case collision_structure:return 0x1f7711u;case collision_surfaces:return 0x1f7716u;case surface_flags:return 0x1f771cu;
    case surface_breakable:return 0x1f7726u;case breakable_set:return 0x1f772fu;case breakable_state:return 0x1f7748u;
    case breakable_word:return 0x1f774eu;
    case handles:return 0x1f775bu;case handle:return 0x1f7761u;case headers:return 0x1f7769u;case headers_data:return 0x1f776fu;
    case header_flags:return 0x1f777eu;case object:return 0x1f7784u;case region_offset:return 0x1f7787u;
    case region_state:return 0x1f7795u;
    }
    return 0;
}
// 1F76A0 (validated by tools/validate_native_shape_key_filter_tu15.py).
// (Each case's deciding read; these are the last six bits of coverage word 0.)
enum FilterBranch : unsigned {
    f_solid=sort_branch_end,f_breakable_call,f_breakable_bit,f_object_missing,f_object_compare,f_always,filter_branch_end
};
static_assert(filter_branch_end<=coverage_bits);

template<class State,class Children> struct FilterAbi {
    using Access=logic::shape_key_filter::Access;
    using Kind=logic::shape_key_filter::Kind;
    Port<State> &port;Children &children;
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    void byte_register(unsigned r,std::uint32_t value){reg(r)=(reg(r)&~0xffu)|(value&0xffu);}
    // ---- Frame: the jump table, and 185AA0
    Kind kind(std::uint32_t) {
        const auto target=port.read(0x1f76bau,filter_table+reg(edx)*4u,4);
        switch(target) {
        case 0x1f76c1u:return Kind::instanced_surface;
        case 0x1f7711u:return Kind::structure_surface;
        case 0x1f775bu:return Kind::object_region;
        case 0x1f77a7u:return Kind::always;
        default:break;
        }
        children.canonical(target);  // throws for an unknown target, as the unit's canonical_branch()
        throw std::runtime_error("Required original solver dispatch target unavailable");
    }
    bool breakable_surface_intact(std::uint32_t index) {
        if((reg(eax)&0xffu)!=index)throw std::logic_error("Native 1F76A0 breakable index differs from the register");
        const std::array<std::uint32_t,4> kept{reg(ebx),reg(esi),reg(edi),reg(ebp)};
        const auto esp_after=reg(esp);
        port.push(0x1f76fdu,0x1f7702u);
        call_dispatch(port,filter_breakable_target,filter_breakable_site,children.dispatch);
        if(kept!=std::array<std::uint32_t,4>{reg(ebx),reg(esi),reg(edi),reg(ebp)} || reg(esp)!=esp_after)
            throw std::runtime_error("Native 1F76A0 child broke the callee-saved registers or its stack cleanup");
        port.flags.logic(reg(eax)&0xffu,1);  // TEST AL,AL
        port.reached(f_breakable_call);
        return (reg(eax)&0xffu)!=0;
    }
    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::shape_key_filter::Access;
        auto &flags=port.flags;
        switch(a) {
        case instances:reg(esi)=flags.product(reg(ecx),0x58u);port.push(0x1f76ccu,reg(edi));break;  // IMUL ESI,ECX,58; PUSH EDI
        case instance_surfaces:reg(esi)=flags.product(reg(esi),0xc8u);break;
        case breakable_state:
            // MOV EDX,ECX; SAR EDX,5; LEA EAX,[EDX+EAX*8]; AND ECX,1F; MOV EDX,1; SHL EDX,CL
            reg(edx)=flags.right(reg(ecx),5u,4,true);reg(eax)=reg(edx)+reg(eax)*8u;
            reg(ecx)=flags.logic(reg(ecx)&31u,4);reg(edx)=flags.shift(1u,reg(ecx)&0xffu,4);
            break;
        case header_flags:
            // AND ECX,FFFF; LEA ECX,[ECX+ECX*2]; LEA EDX,[EDX+ECX*4]
            reg(ecx)=flags.logic(reg(ecx)&0xffffu,4);reg(ecx)*=3u;reg(edx)+=reg(ecx)*4u;
            break;
        case region_state:
            // MOV ECX,EAX; AND ECX,1F; ADD EDX,ESI
            reg(ecx)=flags.logic(reg(eax)&31u,4);reg(edx)=flags.plus(reg(edx),reg(esi),4);
            break;
        default:break;
        }
        return filter_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::shape_key_filter::Access;
        auto &flags=port.flags;
        const auto word=std::uint32_t(std::int32_t(std::int16_t(value)));
        switch(a) {
        case instanced_structure:case definitions:case headers:case headers_data:case handles:reg(edx)=value;break;
        case instances:reg(edi)=value;break;
        case definition_index:reg(esi)=word;break;
        case instance_surfaces:reg(edx)=value;reg(eax)=reg(edx)+reg(eax)*8u;break;
        case instance_flags:
            flags.logic(value&logic::shape_key_filter::breakable_flag,1);
            reg(edi)=port.pop(0x1f76f2u);
            if(!(value&logic::shape_key_filter::breakable_flag))port.reached(f_solid);
            break;
        case instance_breakable:reg(eax)=value;break;
        case collision_structure:reg(eax)=value;break;
        case collision_surfaces:reg(edx)=value;reg(eax)=reg(edx)+reg(ecx)*8u;break;
        case surface_flags:
            flags.logic(value&logic::shape_key_filter::breakable_flag,1);
            if(!(value&logic::shape_key_filter::breakable_flag))port.reached(f_solid);
            break;
        case surface_breakable:reg(ecx)=value;flags.minus(value,0xffffffffu,4);break;
        case breakable_set:reg(eax)=word;break;
        case breakable_state:reg(ecx)=value;break;
        case breakable_word:flags.logic(value&reg(edx),4);port.reached(f_breakable_bit);break;
        case handle:reg(ecx)=value;flags.minus(value,0xffffffffu,4);if(value==0xffffffffu)port.reached(f_object_missing);break;
        case header_flags:
            flags.logic(value&logic::shape_key_filter::excluded_flag,1);
            if(value&logic::shape_key_filter::excluded_flag)port.reached(f_object_missing);
            break;
        case object:reg(esi)=value;break;
        case region_offset:reg(edx)=word;break;
        case region_state:
            // MOVSX ECX; SAR EAX,5; AND EAX,FF; CMP ECX,EAX
            reg(ecx)=std::uint32_t(std::int32_t(std::int8_t(value)));
            reg(eax)=flags.right(reg(eax),5u,4,true);reg(eax)=flags.logic(reg(eax)&0xffu,4);
            flags.minus(reg(ecx),reg(eax),4);
            port.reached(f_object_compare);
            break;
        default:break;
        }
    }
};
template<class State,class Children> void kernel_1f76a0(Port<State> &port,Children &children) {
    using Kind=logic::shape_key_filter::Kind;
    auto &gp=port.gp;auto &flags=port.flags;
    const auto entry=gp[esp],key=gp[edx];
    // MOV EAX,EDX; MOV ECX,EDX; SHR EAX,10; PUSH EBX; SHR EDX,1D; AND ECX,FFFF;
    // AND EAX,1FFF; PUSH ESI; MOV BL,1; DEC EDX
    gp[eax]=flags.right(key,16u,4,false);gp[ecx]=key;
    port.push(0x1f76a7u,gp[ebx]);
    gp[edx]=flags.right(key,29u,4,false);gp[ecx]=flags.logic(gp[ecx]&0xffffu,4);gp[eax]=flags.logic(gp[eax]&0x1fffu,4);
    port.push(0x1f76b6u,gp[esi]);
    gp[ebx]=(gp[ebx]&~0xffu)|1u;
    gp[edx]=flags.increment(gp[edx],4,true);
    FilterAbi<State,Children> abi{port,children};
    Memory<Port<State>,FilterAbi<State,Children>> memory{port,abi};
    const auto result=logic::shape_key_filter::may_collide(memory,abi,key);
    // The exits: XOR BL,BL where the path fails; POP ESI; MOV AL,BL; POP EBX; RET.
    std::uint32_t pop_esi=0x1f77a7u;
    if(!result.collides) {
        const auto clear=result.kind==Kind::instanced_surface?0x1f770au:result.kind==Kind::structure_surface?0x1f7754u:0x1f77a5u;
        gp[ebx]=(gp[ebx]&~0xffu)|flags.logic(0u,1);
        pop_esi=clear+2u;
    }
    if(result.kind==Kind::always)port.reached(f_always);
    gp[esi]=port.pop(pop_esi);
    gp[eax]=(gp[eax]&~0xffu)|(gp[ebx]&0xffu);
    gp[ebx]=port.pop(pop_esi+3u);
    if(gp[esp]!=entry)throw std::logic_error("Native 1F76A0 frame is unbalanced");
    port.ret(pop_esi+4u,0u);port.publish(pop_esi+4u);
}

// ---- 2E4760 rotation into world (inline only) -------------------------------
// ECX = out, [ESP+4] = the rotation, [ESP+8] = the vector. PUSH EBP; MOV
// EBP,ESP; the AND ESP,-16 before the store decides EFLAGS. EAX = the
// rotation's address; XMM0 the result, XMM1 the x coordinate's broadcast,
// XMM2 and XMM3 the z and y terms. RET 8. 2F8630 runs it inline; it is not
// bound at its own body.
constexpr std::uint32_t forward_instruction(logic::havok_rotation::ForwardAccess a) {
    using enum logic::havok_rotation::ForwardAccess;
    switch(a) {
    case input:return 0x2e4766u;case column_2:return 0x2e476cu;case column_1:return 0x2e4770u;
    case column_0:return 0x2e478fu;case result:return 0x2e479eu;
    }
    return 0;
}
template<class P> struct ForwardSchedule {
    P &port;
    std::uint32_t instruction(logic::havok_rotation::ForwardAccess a,std::uint32_t) {
        if(a==logic::havok_rotation::ForwardAccess::result)port.gp[esp]=port.flags.logic(port.gp[esp]&0xfffffff0u,4);
        return forward_instruction(a);
    }
};
template<class P> struct ForwardFrame {
    P &port;std::uint32_t ebp;std::uint32_t rotation_address{};
    std::uint32_t vector(){return port.read(0x2e4763u,ebp+0xcu,4);}
    std::uint32_t rotation(){return rotation_address=port.read(0x2e4769u,ebp+8u,4);}
};
template<class State> void kernel_2e4760(Port<State> &port) {
    auto &gp=port.gp;
    const auto out=gp[ecx];
    port.push(0x2e4760u,gp[ebp]);gp[ebp]=gp[esp];
    ForwardFrame<Port<State>> frame{port,gp[ebp]};
    ForwardSchedule<Port<State>> schedule{port};
    Memory<Port<State>,ForwardSchedule<Port<State>>> memory{port,schedule};
    const auto rotated=logic::havok_rotation::rotate(memory,frame,out);
    gp[eax]=frame.rotation_address;
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2e47a3u);
    port.set_xmm(0,rotated.value);port.set_xmm(1,rotated.x_splat);port.set_xmm(2,rotated.z_term);port.set_xmm(3,rotated.y_term);
    port.ret(0x2e47a4u,8u);port.publish(0x2e47a4u);
}

// x87 as the translated instructions keep the state's stack, for the convex
// pair adapters. Each x87 instruction first publishes itself, so a stack or
// mixed-MMX fault reports it.
template<class State> struct X87Mirror {
    Port<State> &port;
    void at(std::uint32_t pc){port.at(pc);port.s.require_x87();}
    void load(std::uint32_t bits){port.s.fp.push(physics_step_detail::ext(physics_step_detail::f(bits)));}
    void add(std::uint32_t bits){port.s.fp.st(0)=physics_step_detail::add(port.s.fp.st(0),physics_step_detail::ext(physics_step_detail::f(bits)));}
    void subtract(std::uint32_t bits){port.s.fp.st(0)=physics_step_detail::sub(port.s.fp.st(0),physics_step_detail::ext(physics_step_detail::f(bits)));}
    void subtract_from(std::uint32_t bits){port.s.fp.st(0)=physics_step_detail::sub(physics_step_detail::ext(physics_step_detail::f(bits)),port.s.fp.st(0));}
    void multiply(std::uint32_t bits){port.s.fp.st(0)=physics_step_detail::mul(port.s.fp.st(0),physics_step_detail::ext(physics_step_detail::f(bits)));}
    void negate(){port.s.require_x87();port.s.fp.st(0)=-port.s.fp.st(0);}
    void compare_pop(std::uint32_t bits){port.s.fp.comparison(physics_step_detail::ext(physics_step_detail::f(bits)),1);}
    void store_check(){port.s.fp.st(0);}  // FST/FSTP read ST(0) before the store
    void pop(){port.s.fp.pop();}
    // FNSTSW AX, then TEST AH,mask.
    void test_status(std::uint32_t mask) {
        const std::uint32_t word=port.s.fp.word();
        port.gp[eax]=(port.gp[eax]&0xffff0000u)|word;
        port.flags.logic((word>>8u)&mask,1);
    }
};
// A genuine child keeps EBX, ESI, EDI and EBP, and its RET pops what the ABI
// says; otherwise the adapter stops rather than diverge.
template<class State,class Call> void genuine_child(Port<State> &port,std::uint32_t esp_after,const char *owner,Call &&call) {
    auto &gp=port.gp;
    const std::array<std::uint32_t,4> kept{gp[ebx],gp[esi],gp[edi],gp[ebp]};
    call();
    if(kept!=std::array<std::uint32_t,4>{gp[ebx],gp[esi],gp[edi],gp[ebp]} || gp[esp]!=esp_after) {
        char text[120];std::snprintf(text,sizeof text,"Native %s child broke the callee-saved registers or its stack cleanup",owner);
        throw std::runtime_error(text);
    }
}

// ---- 2F8630 closest point --------------------------------------------------
// thiscall: ECX = the GSK agent; [ESP+4] the result (hkBool *), [+8] body A,
// [+C] body B, [+10] the input, [+14] the output. RET 14. PUSH EBP; MOV
// EBP,ESP; AND ESP,-16; SUB ESP,1D4; EBX, ESI and EDI are pushed (EDI = this);
// T is ESP after them. The locals: T+14 (the time stamp, the closing distance,
// the world normal's address, the negated values), T+18 (shape A, then the
// surfaces' distance), T+1C (shape B), T+20 (30E930's output), T+30 (B's
// transform in A's frame) and T+70 (30E930's and 30D350's object).
// Registers: EBX body A, EDI the agent, ESI the profiler's stream and then the
// output. Children:
//   2E4550 inline at its own boundary (ECX = T+30; A's and B's transforms
//   pushed; ADD EDX,20's flags);
//   30E930 through the unit's route (thiscall on T+70: shape A, shape B,
//   T+30, the cache, T+20; RET 14; 2E4550's flags), and TEST EAX,EAX after;
//   30D350 through the unit's route (thiscall on T+70: the cache, T+20, the
//   output; RET C; TEST EAX,EAX's flags);
//   2E4760 inline at its own boundary (ECX = output+40).
// Each x87 instruction is mirrored on the state's stack; FNSTSW AX with TEST
// AH,41 or TEST AH,5 decides the flags of each comparison. XMM0..XMM3 at the
// plane's exit and XMM0..XMM7 at a contact's are the logic's (publish()).
// The exits: 2F888B (a contact) and 2F88C4 (none).
constexpr std::uint32_t closest_point_instruction(logic::closest_point::Access a) {
    using enum logic::closest_point::Access;
    switch(a) {
    case body_a_argument:return 0x2f863du;case body_a_shape:return 0x2f8640u;case plane_distance:return 0x2f8646u;
    case body_b_argument:return 0x2f8649u;case plane_bound:return 0x2f864cu;case body_b_shape:return 0x2f8652u;
    case shape_a_keep:return 0x2f8654u;case shape_b_keep:return 0x2f8658u;
    case plane_next:return 0x2f8667u;case plane_end:return 0x2f866du;case plane_name:return 0x2f8675u;
    case plane_time_store:return 0x2f867du;case plane_time_load:return 0x2f8681u;case plane_time:return 0x2f8685u;
    case plane_advance:return 0x2f868bu;
    case motion_a:return 0x2f8691u;case a_before:return 0x2f8694u;case motion_b:return 0x2f8697u;case a_after:return 0x2f869au;
    case b_after:return 0x2f869eu;case plane_normal:return 0x2f86a2u;case motion_b_again:return 0x2f86a6u;
    case b_bound:return 0x2f86a9u;case b_before:return 0x2f86afu;case closing_store:return 0x2f86d8u;
    case motion_a_again:return 0x2f86dcu;case a_bound:return 0x2f86dfu;case closing_load:return 0x2f86e2u;
    case plane_distance_again:return 0x2f86e6u;case plane_distance_store:return 0x2f86e9u;case plane_bound_again:return 0x2f86ecu;
    case search_next:return 0x2f86fdu;case search_end:return 0x2f8703u;case search_name:return 0x2f870bu;
    case search_time_store:return 0x2f8713u;case search_time_load:return 0x2f8717u;case search_time:return 0x2f871bu;
    case search_advance:return 0x2f8721u;
    case search_motion_b:return 0x2f8727u;case search_motion_a:return 0x2f872au;case search_shape_a:return 0x2f8743u;
    case search_shape_b:return 0x2f874bu;case gsk_clear:return 0x2f875du;case output_argument:return 0x2f8772u;
    case normal_motion:return 0x2f8788u;case world_normal_keep:return 0x2f8793u;
    case core_distance:return 0x2f879cu;case gap_shape_a:return 0x2f879fu;case radius_a:return 0x2f87a3u;
    case gap_shape_b:return 0x2f87a6u;case radius_b:return 0x2f87aau;case input_argument:return 0x2f87adu;
    case gap_store:return 0x2f87b0u;case gap_bits:return 0x2f87b4u;case gap_load:return 0x2f87b8u;
    case distance_store:return 0x2f87bcu;case tolerance:return 0x2f87bfu;
    case contact_radius:return 0x2f87cdu;case contact_normal:return 0x2f87d0u;case relative_column_2:return 0x2f87d3u;
    case relative_column_1:return 0x2f87dau;case negated_radius_store:return 0x2f87dfu;case negated_radius_load:return 0x2f87e3u;
    case contact_gap:return 0x2f87e9u;case result_argument:return 0x2f87edu;case negated_gap_store:return 0x2f87f5u;
    case contact_point:return 0x2f87fdu;case relative_translation:return 0x2f8804u;case point_a_store:return 0x2f880cu;
    case negated_gap_load:return 0x2f8810u;case contact_normal_again:return 0x2f8816u;case contact_point_again:return 0x2f8820u;
    case relative_column_0:return 0x2f882au;case point_b_store:return 0x2f887eu;case contact_result:return 0x2f8882u;
    case plane_not_cached:return 0x2f888eu;case settings:return 0x2f8895u;case caching_enabled:return 0x2f8898u;
    case world_normal_reload:return 0x2f889du;case world_normal:return 0x2f88a1u;case plane_normal_store:return 0x2f88a4u;
    case cache_tolerance:return 0x2f88a8u;case cache_fraction:return 0x2f88abu;case cache_gap:return 0x2f88b1u;
    case cache_distance_store:return 0x2f88b5u;
    case no_contact_result_argument:return 0x2f88b8u;case no_contact_result:return 0x2f88bdu;
    }
    return 0;
}
// 2F8630 (validated by tools/validate_native_convex_pair_tu15.py). Coverage
// word 1 starts here (filter_branch_end is 64).
enum ClosestPointBranch : unsigned {
    p_plane_unused=filter_branch_end,p_plane_apart,p_plane_closed,p_features_apart,p_contact,p_plane_cached,
    p_agent_keeps_no_plane,p_settings_keep_no_plane,p_timer_written,p_timer_full,closest_point_branch_end
};
static_assert(closest_point_branch_end<=coverage_bits);

template<class State,class Children> struct ClosestPointAbi {
    using Access=logic::closest_point::Access;
    Port<State> &port;Children &children;X87Mirror<State> x87{port};
    std::uint32_t frame_base;  // T
    logic::closest_point::Locals local{};
    bool search_timer{};
    ClosestPointAbi(Port<State> &p,Children &c,std::uint32_t t,std::uint32_t ebp):port(p),children(c),frame_base(t) {
        local={.result=ebp+8u,.body_a=ebp+0xcu,.body_b=ebp+0x10u,.input=ebp+0x14u,.output=ebp+0x18u,
            .time_stamp=t+0x14u,.closing=t+0x14u,.world_normal=t+0x14u,.negated=t+0x14u,.shape_a=t+0x18u,.gap=t+0x18u,
            .shape_b=t+0x1cu,.separation=t+0x20u,.relative=t+0x30u,.gsk=t+0x70u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    static bool is_x87(Access a) {
        using enum logic::closest_point::Access;
        switch(a) {
        case plane_distance:case plane_bound:case b_bound:case a_bound:case closing_load:case plane_distance_again:
        case plane_distance_store:case plane_bound_again:case core_distance:case radius_a:case radius_b:case gap_store:
        case gap_load:case tolerance:case contact_radius:case negated_radius_store:case contact_gap:case negated_gap_store:
        case cache_tolerance:case cache_fraction:case cache_gap:case cache_distance_store:
            return true;
        default:return false;
        }
    }

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::closest_point::Access;
        auto &flags=port.flags;
        const auto pc=closest_point_instruction(a);
        switch(a) {
        case plane_distance:port.push(0x2f8642u,reg(esi));port.push(0x2f8643u,reg(edi));reg(edi)=reg(ecx);break;
        case plane_name:search_timer=false;port.reached(p_timer_written);break;
        case search_name:search_timer=true;port.reached(p_timer_written);break;
        case plane_advance:case search_advance:reg(esi)=flags.plus(reg(esi),0xcu,4);break;
        case closing_store:reg(edx)=frame_base+0x14u;break;                                   // LEA EDX,[ESP+14]
        case search_shape_a:reg(ecx)=frame_base+0x20u;port.push(0x2f8742u,reg(ecx));break;  // 30E930's last argument
        case search_shape_b:reg(eax)=reg(edi)+0xcu;port.push(0x2f874au,reg(eax));break;     // the cache
        case gsk_clear:
            reg(edx)=frame_base+0x30u;port.push(0x2f8753u,reg(edx));port.push(0x2f8754u,reg(eax));port.push(0x2f8755u,reg(ecx));
            reg(ecx)=frame_base+0x70u;
            break;
        case world_normal_keep:
            // PUSH ESI (the vector); ADD EAX,20; LEA ECX,[ESI+40]; PUSH EAX (the rotation)
            port.push(0x2f878bu,reg(esi));reg(eax)=flags.plus(reg(eax),0x20u,4);reg(ecx)=reg(esi)+0x40u;port.push(0x2f8792u,reg(eax));
            break;
        case relative_column_1:x87.negate();break;  // FCHS (the radius)
        case no_contact_result:reg(edi)=port.pop(0x2f88bbu);reg(esi)=port.pop(0x2f88bcu);break;
        default:break;
        }
        if(is_x87(a))x87.at(pc);
        switch(a) {
        case plane_distance_store:case gap_store:case negated_radius_store:case negated_gap_store:case cache_distance_store:
            x87.store_check();break;
        default:break;
        }
        return pc;
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::closest_point::Access;
        auto &flags=port.flags;
        switch(a) {
        case body_a_argument:reg(ebx)=value;break;
        case body_a_shape:case search_shape_b:case gap_bits:case normal_motion:case plane_time_load:case search_time_load:
        case motion_a:case motion_b_again:case world_normal_reload:case no_contact_result_argument:
            reg(eax)=value;break;
        case body_b_argument:case search_motion_b:case search_shape_a:case gap_shape_b:case input_argument:reg(ecx)=value;break;
        case body_b_shape:case motion_b:case motion_a_again:case search_motion_a:case gap_shape_a:case settings:reg(edx)=value;break;
        case plane_next:case search_next:reg(esi)=value;break;
        case plane_end:case search_end:
            flags.minus(reg(esi),value,4);
            if(!(reg(esi)<value))port.reached(p_timer_full);
            break;
        case output_argument:reg(esi)=value;break;
        case plane_distance:case b_bound:case core_distance:case gap_load:case contact_radius:case contact_gap:case cache_tolerance:
            x87.load(value);break;
        case plane_bound:x87.compare_pop(value);break;  // FNSTSW AX follows the two stores
        case a_bound:case closing_load:x87.add(value);break;
        case plane_distance_again:case cache_gap:x87.subtract_from(value);break;
        case plane_bound_again:
            x87.compare_pop(value);x87.test_status(0x41u);
            if(!flags.zf)port.reached(p_plane_closed);  // TEST AH,41 / JZ: still greater keeps the plane
            break;
        case radius_a:case radius_b:x87.subtract(value);break;
        case cache_fraction:x87.multiply(value);break;
        case tolerance:x87.compare_pop(value);x87.test_status(5u);break;
        case result_argument:reg(eax)=value;x87.negate();break;  // FCHS (the distance)
        case plane_not_cached:
            reg(eax)=value;flags.logic(value,4);
            if(value)port.reached(p_agent_keeps_no_plane);
            break;
        case caching_enabled:
            flags.minus(value,0u,1);
            if(!value)port.reached(p_settings_keep_no_plane);
            break;
        default:break;
        }
    }
    void loaded_vector(Access a,__m128 value) {
        if(a==Access::world_normal){port.set_xmm(0,value);port.reached(p_plane_cached);}
    }
    void stored(Access a) {
        using enum logic::closest_point::Access;
        switch(a) {
        case shape_b_keep:
            x87.test_status(0x41u);  // FNSTSW AX; TEST AH,41
            if(!port.flags.zf)port.reached(p_plane_unused);
            break;
        case gap_store:case negated_radius_store:case negated_gap_store:case cache_distance_store:x87.pop();break;
        default:break;
        }
    }

    // ---- Frame
    const logic::closest_point::Locals &locals()const{return local;}
    std::uint32_t time_stamp_counter() {
        port.at(search_timer?0x2f8711u:0x2f867bu);
        if(!port.s.services.timestamp_counter)throw std::runtime_error("Required original contact RDTSC owner unavailable");
        const auto counter=port.time_stamp_counter();
        reg(eax)=std::uint32_t(counter);reg(edx)=std::uint32_t(counter>>32u);
        return reg(eax);
    }
    // 2E4550 inline: ADD ECX,20; PUSH ECX (B's transform); ADD EDX,20; PUSH
    // EDX (A's); LEA ECX,[ESP+38] (out).
    void relative_transform(std::uint32_t out,std::uint32_t a,std::uint32_t b) {
        auto &flags=port.flags;
        reg(ecx)=flags.plus(reg(ecx),0x20u,4);
        const auto pushed_b=reg(ecx);port.push(0x2f8730u,pushed_b);
        reg(edx)=flags.plus(reg(edx),0x20u,4);port.push(0x2f8734u,reg(edx));
        reg(ecx)=frame_base+0x30u;
        if(reg(ecx)!=out || reg(edx)!=a || pushed_b!=b)throw std::logic_error("Native 2F8630 relative transform differs from the pushed one");
        port.push(0x2f8739u,0x2f873eu);
        kernel_2e4550(port);
        if(reg(esp)!=frame_base)throw std::logic_error("Native 2F8630 frame is unbalanced after 2E4550");
    }
    // 30E930 (thiscall, RET 14): the closest features; TEST EAX,EAX after.
    std::uint32_t closest_features(std::uint32_t gsk,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {
        if(reg(ecx)!=gsk)throw std::logic_error("Native 2F8630 closest features object differs from ECX");
        port.push(0x2f8765u,0x2f876au);
        genuine_child(port,frame_base,"2F8630",[&]{port.call_child(0x30e930u,children.features);});
        port.flags.logic(reg(eax),4);
        if(reg(eax))port.reached(p_features_apart);
        return reg(eax);
    }
    // 30D350 (thiscall, RET C): PUSH ESI (the output); LEA EDX,[ESP+24]; PUSH
    // EDX; LEA EAX,[EDI+C]; PUSH EAX; LEA ECX,[ESP+7C].
    void closest_points(std::uint32_t gsk,std::uint32_t cache,std::uint32_t separation,std::uint32_t output) {
        port.push(0x2f8775u,reg(esi));
        reg(edx)=frame_base+0x20u;port.push(0x2f877au,reg(edx));
        reg(eax)=reg(edi)+0xcu;port.push(0x2f877eu,reg(eax));
        reg(ecx)=frame_base+0x70u;
        if(reg(ecx)!=gsk || reg(eax)!=cache || reg(edx)!=separation || reg(esi)!=output)
            throw std::logic_error("Native 2F8630 closest points arguments differ from the registers");
        port.push(0x2f8783u,0x2f8788u);
        genuine_child(port,frame_base,"2F8630",[&]{port.call_child(0x30d350u,children.points);});
    }
    // 2E4760 inline (the pushes are world_normal_keep's).
    void rotate_into_world(std::uint32_t out,std::uint32_t rotation,std::uint32_t vector) {
        if(reg(ecx)!=out || reg(eax)!=rotation || reg(esi)!=vector)
            throw std::logic_error("Native 2F8630 world normal arguments differ from the registers");
        port.push(0x2f8797u,0x2f879cu);
        kernel_2e4760(port);
        if(reg(esp)!=frame_base)throw std::logic_error("Native 2F8630 frame is unbalanced after 2E4760");
    }
    // XMM at the exits: the plane's reduction (XMM0..XMM3), a contact's
    // rotation into B's frame (XMM0..XMM7); the other exits keep what the
    // children and the cached normal left.
    void publish(const logic::closest_point::Result &r) {
        using enum logic::closest_point::Outcome;
        using logic::lanes;using logic::vector;
        if(r.outcome==plane_apart) {
            const auto t=lanes(r.plane.closing_terms);
            port.set_xmm(0,r.plane.closing_terms);
            port.set_xmm(1,vector({std::bit_cast<std::uint32_t>(r.plane.closing_yx),t[1],t[1],t[1]}));
            port.set_xmm(2,vector({std::bit_cast<std::uint32_t>(r.plane.closing),t[2],t[2],t[2]}));
            port.set_xmm(3,r.plane.change_b);
        } else if(r.outcome==contact) {
            const auto &p=r.point_b;const auto c=lanes(p.last_column);
            port.set_xmm(0,p.input);port.set_xmm(1,p.value);port.set_xmm(2,vector({c[0],c[1],c[0],c[1]}));
            port.set_xmm(3,p.z_terms);port.set_xmm(4,logic::splat(p.input,0));port.set_xmm(5,p.y_terms);
            port.set_xmm(6,logic::splat(p.input,2));port.set_xmm(7,vector({c[2],c[3],c[2],c[3]}));
        }
    }
};
template<class State,class Children> void kernel_2f8630(Port<State> &port,Children &children) {
    using enum logic::closest_point::Outcome;
    auto &gp=port.gp;auto &flags=port.flags;
    const auto entry=gp[esp],agent=gp[ecx];
    port.push(0x2f8630u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=flags.logic(gp[esp]&0xfffffff0u,4);  // AND ESP,-16
    gp[esp]=flags.minus(gp[esp],0x1d4u,4);       // SUB ESP,1D4
    port.push(0x2f863cu,gp[ebx]);
    ClosestPointAbi<State,Children> abi(port,children,gp[esp]-8u,gp[ebp]);  // ESI and EDI follow
    Memory<Port<State>,ClosestPointAbi<State,Children>> memory{port,abi};
    const auto result=logic::closest_point::find(memory,abi,agent);
    abi.publish(result);
    switch(result.outcome) {
    case plane_apart:port.reached(p_plane_apart);break;
    case contact:port.reached(p_contact);break;
    default:break;
    }
    std::uint32_t exit;
    if(result.outcome==contact) {
        gp[edi]=port.pop(0x2f8885u);gp[esi]=port.pop(0x2f8886u);gp[ebx]=port.pop(0x2f8887u);
        gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2f888au);exit=0x2f888bu;
    } else {
        gp[ebx]=port.pop(0x2f88c0u);
        gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2f88c3u);exit=0x2f88c4u;
    }
    if(gp[esp]!=entry)throw std::logic_error("Native 2F8630 frame is unbalanced");
    port.ret(exit,0x14u);port.publish(exit);
}

// ---- 2E4FE0 swept pair -----------------------------------------------------
// thiscall: ECX = the GSK agent; [ESP+4] the result (hkBool *), [+8] body A,
// [+C] body B, [+10] the input, [+14] the output. RET 14. PUSH EBP; MOV
// EBP,ESP; AND ESP,-16; SUB ESP,64; EBX, ESI and EDI are pushed; T is ESP
// after them. The locals: T+1A (the settings' byte), T+1B (2F8630's hkBool),
// T+1C (this), T+20 (the sweep flags), T+24 (a byte: a shape swept; then
// shape A), T+28 (the squared speed, A's step time), T+2C (B's owner, B's
// step time) and T+30 (B's transform in A's frame).
// Registers: EBX body A, EDI body B (and A's owner before it), ESI the swept
// shape and then this. Children:
//   the shapes' getType() (vtable +14) through the unit's dispatch (canonical
//   sites 2E1510, 2E162C, 2E1792), and CMP EAX,17 or CMP EAX,5 after;
//   283380 through the unit's route (cdecl: the owner; ADD ESP,4 and TEST
//   AL,AL after; TEST AH,41's flags at the call);
//   2E4550 inline at its own boundary (ADD EAX,20's flags);
//   309150 through the unit's dispatch (thiscall on the cache: shape A,
//   shape B, T+30; canonical site 2E17AC) or 309060 through the unit's route
//   (the same arguments); RET C; ADD ECX,C's flags;
//   2F8630 through the unit's route (thiscall on this: T+1B, body A, body B,
//   the input, the output; RET 14).
// XMM0..XMM2 after a speed and XMM0..XMM7 after a sweep are the logic's
// (measured(), swept()); the exit keeps 2F8630's. The exit is 2E5312.
inline constexpr std::uint32_t swept_pair_cache_5_target=0x309150u;  // selected(305650)
constexpr std::uint32_t swept_pair_instruction(logic::swept_pair::Access a) {
    using enum logic::swept_pair::Access;
    switch(a) {
    case input_argument:return 0x2e4fe9u;case body_a_argument:return 0x2e4fedu;case agent_keep:return 0x2e4ff0u;
    case settings:return 0x2e4ff4u;case settings_byte:return 0x2e4ff7u;case shape_a:return 0x2e4ff9u;case vtable_a:return 0x2e4ffbu;
    case swept_clear:return 0x2e4fffu;case settings_byte_keep:return 0x2e5004u;case sweep_flags_clear:return 0x2e5008u;
    case a_parent:return 0x2e501cu;case a_swept_shape:return 0x2e5021u;case a_parent_next:return 0x2e5029u;
    case a_owner:return 0x2e5030u;case a_owner_motion:return 0x2e5033u;case a_velocity:return 0x2e5036u;
    case a_speed_store:return 0x2e505au;case a_speed_load:return 0x2e505eu;case a_threshold:return 0x2e5062u;
    case a_owner_motion_again:return 0x2e5081u;case a_motion:return 0x2e5084u;case a_column_2:return 0x2e5087u;
    case a_velocity_again:return 0x2e508bu;case a_column_0:return 0x2e508fu;case a_column_1:return 0x2e5093u;
    case a_input:return 0x2e5097u;case a_sweep_store:return 0x2e50ecu;case a_step_time:return 0x2e50f0u;
    case a_sweep_load:return 0x2e50f3u;case a_step_time_store:return 0x2e50f7u;case a_step_time_load:return 0x2e50fbu;
    case a_sweep_scaled_store:return 0x2e510bu;case a_flags_store:return 0x2e510fu;case a_swept_store:return 0x2e5117u;
    case a_sweep_zero:return 0x2e5121u;
    case body_b_argument:return 0x2e5125u;case shape_b:return 0x2e5128u;case vtable_b:return 0x2e512au;
    case b_parent:return 0x2e5138u;case b_swept_shape:return 0x2e513du;case b_parent_next:return 0x2e5145u;
    case b_owner:return 0x2e514cu;case b_owner_motion:return 0x2e514fu;case b_velocity:return 0x2e5152u;
    case b_speed_store:return 0x2e5176u;case b_speed_load:return 0x2e517au;case b_threshold:return 0x2e517eu;
    case b_owner_keep:return 0x2e5181u;case b_motion:return 0x2e51a1u;case b_column_2:return 0x2e51a4u;
    case b_column_0:return 0x2e51a8u;case b_column_1:return 0x2e51acu;case b_owner_reload:return 0x2e51b0u;
    case b_owner_motion_again:return 0x2e51b4u;case b_velocity_again:return 0x2e51b7u;case b_input:return 0x2e51bbu;
    case b_sweep_store:return 0x2e5210u;case b_step_time:return 0x2e5214u;case b_sweep_load:return 0x2e5217u;
    case b_step_time_store:return 0x2e521bu;case b_step_time_load:return 0x2e521fu;case b_flags_load:return 0x2e5225u;
    case b_sweep_scaled_store:return 0x2e5236u;case b_flags_store:return 0x2e523au;case b_swept_store:return 0x2e523eu;
    case b_sweep_zero:return 0x2e5248u;
    case agent_reload:return 0x2e524cu;case flags_reload:return 0x2e5250u;case previous_flags:return 0x2e5254u;
    case changed_input:return 0x2e525cu;case changed_settings:return 0x2e525fu;case settings_byte_clear:return 0x2e5262u;
    case swept_load:return 0x2e5267u;
    case reset_motion_b:return 0x2e526fu;case reset_shape_a:return 0x2e5272u;case reset_motion_a:return 0x2e5274u;
    case reset_shape_b:return 0x2e5277u;case shape_a_keep:return 0x2e5280u;case reset_vtable_b:return 0x2e528eu;
    case cache_5_shape_a:return 0x2e529au;case cache_5_agent:return 0x2e52a5u;case cache_shape_a:return 0x2e52b3u;
    case cache_agent:return 0x2e52b7u;case plane_agent:return 0x2e52cau;case plane_reset:return 0x2e52ceu;
    case output_argument:return 0x2e52d7u;case query_input:return 0x2e52dau;case flags_final:return 0x2e52edu;
    case input_final:return 0x2e52f1u;case flags_store:return 0x2e52f4u;case settings_final:return 0x2e52fau;
    case settings_byte_reload:return 0x2e52fdu;case result_argument:return 0x2e5301u;case settings_byte_restore:return 0x2e5305u;
    case found_load:return 0x2e5307u;case result_store:return 0x2e530cu;
    }
    return 0;
}
// 2E4FE0 (validated by tools/validate_native_convex_pair_tu15.py).
enum SweptPairBranch : unsigned {
    s_plain_a=closest_point_branch_end,s_parent_a,s_slow_a,s_refused_a,s_swept_a,
    s_plain_b,s_parent_b,s_slow_b,s_refused_b,s_swept_b,
    s_flags_changed,s_swept_reset,s_cache_kept,s_cache_5,s_cache_other,swept_pair_branch_end
};
static_assert(swept_pair_branch_end<=coverage_bits);

template<class State,class Children> struct SweptPairAbi {
    using Access=logic::swept_pair::Access;
    using Side=logic::swept_pair::Side;
    using TypeQuery=logic::swept_pair::TypeQuery;
    Port<State> &port;Children &children;X87Mirror<State> x87{port};
    std::uint32_t frame_base;  // T
    logic::swept_pair::Locals local{};
    std::uint32_t pushed_shape_a{};
    SweptPairAbi(Port<State> &p,Children &c,std::uint32_t t,std::uint32_t ebp):port(p),children(c),frame_base(t) {
        local={.result=ebp+8u,.body_a=ebp+0xcu,.body_b=ebp+0x10u,.input=ebp+0x14u,.output=ebp+0x18u,
            .settings_byte=t+0x1au,.found=t+0x1bu,.agent=t+0x1cu,.sweep_flags=t+0x20u,.swept=t+0x24u,.shape_a=t+0x24u,
            .speed=t+0x28u,.owner_b=t+0x2cu,.relative=t+0x30u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    void byte_register(unsigned r,std::uint32_t value){reg(r)=(reg(r)&~0xffu)|(value&0xffu);}

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::swept_pair::Access;
        auto &flags=port.flags;
        const auto pc=swept_pair_instruction(a);
        switch(a) {
        case body_a_argument:port.push(0x2e4fecu,reg(ebx));break;
        case swept_clear:port.push(0x2e4ffdu,reg(esi));port.push(0x2e4ffeu,reg(edi));break;
        case a_parent_next:reg(eax)=reg(ecx);port.reached(s_parent_a);break;  // MOV EAX,ECX
        case b_parent_next:reg(eax)=reg(ecx);port.reached(s_parent_b);break;
        case a_speed_store:reg(ecx)=frame_base+0x28u;break;  // LEA ECX,[ESP+28]
        case b_speed_store:reg(eax)=frame_base+0x28u;break;  // LEA EAX,[ESP+28]
        case a_speed_load:case a_threshold:case b_speed_load:case b_threshold:x87.at(pc);break;
        case a_sweep_zero:case b_sweep_zero:port.set_xmm(0,_mm_setzero_ps());break;  // XORPS XMM0,XMM0
        case shape_a_keep:
            // ADD EDX,20; PUSH EDX (B's transform); ADD EAX,20
            reg(edx)=flags.plus(reg(edx),0x20u,4);port.push(0x2e527cu,reg(edx));reg(eax)=flags.plus(reg(eax),0x20u,4);
            break;
        case cache_5_agent:
            // LEA EAX,[ESP+30]; PUSH EAX; PUSH ESI; PUSH ECX (shape A)
            reg(eax)=frame_base+0x30u;port.push(0x2e52a2u,reg(eax));port.push(0x2e52a3u,reg(esi));
            pushed_shape_a=reg(ecx);port.push(0x2e52a4u,pushed_shape_a);
            break;
        case settings_byte_restore:reg(edi)=port.pop(0x2e5304u);break;
        case result_store:reg(esi)=port.pop(0x2e530bu);break;
        default:break;
        }
        return pc;
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::swept_pair::Access;
        auto &flags=port.flags;
        switch(a) {
        case input_argument:case vtable_a:case a_owner_motion:case a_motion:case a_step_time:case b_owner_motion:case b_motion:
        case b_step_time:case changed_settings:case reset_motion_a:case cache_shape_a:case query_input:case input_final:
        case result_argument:
            reg(eax)=value;break;
        case body_a_argument:reg(ebx)=value;break;
        case settings:case shape_a:case a_owner_motion_again:case shape_b:case b_owner:case b_owner_reload:case b_owner_motion_again:
        case flags_reload:case reset_shape_a:case cache_5_shape_a:case cache_agent:case plane_agent:case settings_final:
            reg(ecx)=value;break;
        case settings_byte:case settings_byte_reload:byte_register(edx,value);break;
        case vtable_b:case changed_input:case reset_motion_b:case output_argument:case flags_final:reg(edx)=value;break;
        case a_parent:case b_parent:case a_parent_next:case b_parent_next:reg(ecx)=value;flags.logic(value,4);break;  // TEST ECX,ECX
        case a_swept_shape:reg(esi)=value;reg(eax)=reg(ebx);break;  // MOV EAX,EBX
        case b_swept_shape:reg(esi)=value;reg(eax)=reg(edi);break;  // MOV EAX,EDI
        case a_owner:case body_b_argument:reg(edi)=value;break;
        case a_speed_load:case b_speed_load:x87.load(value);break;
        case a_threshold:x87.compare_pop(value);x87.test_status(0x41u);break;
        case b_threshold:x87.compare_pop(value);break;  // FNSTSW AX follows B's owner's store
        case a_input:
            // ADD ECX,40 (the velocity); ADD EAX,20 (the transform)
            reg(edx)=value;reg(ecx)=flags.plus(reg(ecx),0x40u,4);reg(eax)=flags.plus(reg(eax),0x20u,4);
            break;
        case b_input:
            // ADD EAX,20 (the transform); ADD ECX,40 (the velocity)
            reg(edx)=value;reg(eax)=flags.plus(reg(eax),0x20u,4);reg(ecx)=flags.plus(reg(ecx),0x40u,4);
            break;
        case b_flags_load:reg(eax)=flags.plus(value,logic::swept_pair::swept_b,4);break;  // ADD EAX,2
        case agent_reload:reg(esi)=value;break;
        case previous_flags:
            flags.minus(reg(ecx),value,4);  // CMP ECX,[ESI+F0]
            port.reached(reg(ecx)!=value?s_flags_changed:s_cache_kept);
            break;
        case swept_load:
            byte_register(eax,value);flags.logic(value&0xffu,1);  // TEST AL,AL
            if(value&0xffu)port.reached(s_swept_reset);
            break;
        case reset_shape_b:reg(esi)=value;break;
        case reset_vtable_b:reg(edx)=value;reg(ecx)=reg(esi);break;  // MOV ECX,ESI
        case cache_5_agent:reg(ecx)=flags.plus(value,0xcu,4);break;  // ADD ECX,C
        case found_load:byte_register(ecx,value);break;
        default:break;
        }
    }
    void loaded_vector(Access a,__m128) {
        using enum logic::swept_pair::Access;
        if(a==a_velocity || a==b_velocity)reg(eax)=port.flags.plus(reg(eax),logic::convex_pair::motion_velocity,4);  // ADD EAX,40
    }
    void stored(Access a) {
        using enum logic::swept_pair::Access;
        switch(a) {
        case b_owner_keep:x87.test_status(0x41u);break;  // FNSTSW AX; TEST AH,41
        case plane_reset:reg(esi)=reg(ecx);break;        // MOV ESI,ECX
        default:break;
        }
    }

    // ---- Frame
    const logic::swept_pair::Locals &locals()const{return local;}
    // shape->getType(): CALL [vtable+14] through the unit's dispatch; CMP EAX,17
    // (or CMP EAX,5 for the cache) after.
    std::uint32_t shape_type(TypeQuery query,std::uint32_t shape,std::uint32_t vtable) {
        using enum logic::swept_pair::TypeQuery;
        const std::uint32_t pc=query==shape_a?0x2e5010u:query==shape_b?0x2e512cu:0x2e5292u;
        const std::uint32_t site=query==shape_a?0x2e1510u:query==shape_b?0x2e162cu:0x2e1792u;
        if(reg(ecx)!=shape || reg(query==shape_a?eax:edx)!=vtable)throw std::logic_error("Native 2E4FE0 shape type call differs from the registers");
        const auto target=port.read(pc,vtable+std::uint32_t(logic::collection_agent::Virtual::shape_type),4);
        const auto esp_after=reg(esp);
        port.push(pc,pc+3u);
        genuine_child(port,esp_after,"2E4FE0",[&]{call_dispatch(port,target,site,children.dispatch);});
        const auto type=reg(eax);
        if(query==cache_b) {
            port.flags.minus(type,logic::convex_pair::cache_type_5,4);
            port.reached(type==logic::convex_pair::cache_type_5?s_cache_5:s_cache_other);
        } else {
            port.flags.minus(type,logic::convex_pair::swept_shape_type,4);
            if(type!=logic::convex_pair::swept_shape_type)port.reached(query==shape_a?s_plain_a:s_plain_b);
        }
        return type;
    }
    // 283380(owner): cdecl; PUSH the owner (EDI for A, ECX for B); ADD ESP,4
    // and TEST AL,AL after.
    bool may_sweep(Side side,std::uint32_t owner) {
        const bool a=side==Side::a;
        if(reg(a?edi:ecx)!=owner)throw std::logic_error("Native 2E4FE0 owner differs from the register");
        port.push(a?0x2e5070u:0x2e5190u,owner);
        const auto esp_after=reg(esp);
        port.push(a?0x2e5071u:0x2e5191u,a?0x2e5076u:0x2e5196u);
        genuine_child(port,esp_after,"2E4FE0",[&]{port.call_child(0x283380u,children.may_sweep);});
        reg(esp)=port.flags.plus(reg(esp),4u,4);
        port.flags.logic(reg(eax)&0xffu,1);
        const bool agreed=(reg(eax)&0xffu)!=0;
        if(!agreed)port.reached(a?s_refused_a:s_refused_b);
        return agreed;
    }
    // The speed's horizontal sum: XMM0 the squares, XMM1 and XMM2 the two
    // ADDSS (their upper lanes the broadcast y and z).
    void measured(Side side,const logic::swept_pair::Speed &speed) {
        using logic::lanes;using logic::vector;
        const auto squared=lanes(speed.squared);
        port.set_xmm(0,speed.squared);
        port.set_xmm(1,vector({std::bit_cast<std::uint32_t>(speed.yx),squared[1],squared[1],squared[1]}));
        port.set_xmm(2,vector({std::bit_cast<std::uint32_t>(speed.value),squared[2],squared[2],squared[2]}));
        // TEST AH,41 / JNZ: the shape sweeps only when the speed is greater.
        if(port.flags.zf==false)port.reached(side==Side::a?s_slow_a:s_slow_b);
    }
    // A sweep: the rotation's registers (publish_inverse_rotation), then
    // XMM0 the step time (MOVSS), XMM1 the sweep read back, XMM2 scaled.
    void swept(Side side,const logic::swept_pair::Sweep &sweep) {
        publish_inverse_rotation(port,sweep.rotated,sweep.rotated.input);
        port.set_xmm(0,scalar(sweep.step_time));port.set_xmm(1,sweep.loaded);port.set_xmm(2,sweep.scaled);
        port.reached(side==Side::a?s_swept_a:s_swept_b);
    }
    // 2E4550 inline (the first push is shape_a_keep's): PUSH EAX (A's
    // transform); LEA ECX,[ESP+38].
    void relative_transform(std::uint32_t out,std::uint32_t a,std::uint32_t b) {
        port.push(0x2e5284u,reg(eax));
        reg(ecx)=frame_base+0x30u;
        if(reg(ecx)!=out || reg(eax)!=a || reg(edx)!=b)throw std::logic_error("Native 2E4FE0 relative transform differs from the registers");
        port.push(0x2e5289u,0x2e528eu);
        kernel_2e4550(port);
        if(reg(esp)!=frame_base)throw std::logic_error("Native 2E4FE0 frame is unbalanced after 2E4550");
    }
    // 309150 (the pushes are cache_5_agent's): through the unit's dispatch.
    void set_up_cache_5(std::uint32_t cache,std::uint32_t a,std::uint32_t b,std::uint32_t relative) {
        if(reg(ecx)!=cache || pushed_shape_a!=a || reg(esi)!=b || reg(eax)!=relative)
            throw std::logic_error("Native 2E4FE0 cache arguments differ from the registers");
        port.push(0x2e52acu,0x2e52b1u);
        genuine_child(port,frame_base,"2E4FE0",[&]{call_dispatch(port,swept_pair_cache_5_target,0x2e17acu,children.dispatch);});
    }
    // 309060: LEA EDX,[ESP+30]; PUSH EDX; PUSH ESI; PUSH EAX; ADD ECX,C.
    void set_up_cache(std::uint32_t cache,std::uint32_t a,std::uint32_t b,std::uint32_t relative) {
        reg(edx)=frame_base+0x30u;port.push(0x2e52bfu,reg(edx));port.push(0x2e52c0u,reg(esi));port.push(0x2e52c1u,reg(eax));
        reg(ecx)=port.flags.plus(reg(ecx),0xcu,4);
        if(reg(ecx)!=cache || reg(eax)!=a || reg(esi)!=b || reg(edx)!=relative)
            throw std::logic_error("Native 2E4FE0 cache arguments differ from the registers");
        port.push(0x2e52c5u,0x2e52cau);
        genuine_child(port,frame_base,"2E4FE0",[&]{port.call_child(0x309060u,children.set_up_cache);});
    }
    // 2F8630: PUSH EDX (the output), EAX (the input), EDI, EBX; LEA
    // ECX,[ESP+2B]; PUSH ECX; MOV ECX,ESI.
    void closest_point(std::uint32_t self,std::uint32_t found,std::uint32_t a,std::uint32_t b,std::uint32_t input,std::uint32_t output) {
        port.push(0x2e52ddu,reg(edx));port.push(0x2e52deu,reg(eax));port.push(0x2e52dfu,reg(edi));port.push(0x2e52e0u,reg(ebx));
        reg(ecx)=frame_base+0x1bu;port.push(0x2e52e5u,reg(ecx));
        reg(ecx)=reg(esi);
        if(reg(ecx)!=self || frame_base+0x1bu!=found || reg(ebx)!=a || reg(edi)!=b || reg(eax)!=input || reg(edx)!=output)
            throw std::logic_error("Native 2E4FE0 query arguments differ from the registers");
        port.push(0x2e52e8u,0x2e52edu);
        genuine_child(port,frame_base,"2E4FE0",[&]{port.call_child(0x2f8630u,children.closest_point);});
    }
};
template<class State,class Children> void kernel_2e4fe0(Port<State> &port,Children &children) {
    auto &gp=port.gp;auto &flags=port.flags;
    const auto entry=gp[esp],agent=gp[ecx];
    port.push(0x2e4fe0u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=flags.logic(gp[esp]&0xfffffff0u,4);  // AND ESP,-16
    gp[esp]=flags.minus(gp[esp],0x64u,4);        // SUB ESP,64
    SweptPairAbi<State,Children> abi(port,children,gp[esp]-0xcu,gp[ebp]);  // EBX, ESI and EDI follow
    Memory<Port<State>,SweptPairAbi<State,Children>> memory{port,abi};
    logic::swept_pair::closest_point(memory,abi,agent);
    gp[ebx]=port.pop(0x2e530eu);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2e5311u);
    if(gp[esp]!=entry)throw std::logic_error("Native 2E4FE0 frame is unbalanced");
    port.ret(0x2e5312u,0x14u);port.publish(0x2e5312u);
}

// ---- 1F7C10 BSP child shape --------------------------------------------------
// thiscall: ECX = the structure's collision shape; [ESP+4] the key, [+8] the
// buffer. RET 8. PUSH ECX (a slot the original then overwrites with ECX
// again) and PUSH ESI (the key); EDI is pushed once the key is accepted, and
// EBX on kind 2's path. The exits pop them back in reverse and leave EAX as
// the logic's result. Children (each entered through the unit's route or
// dispatch, with the original's registers at the call):
//   1F76A0 (EDX = the key; TEST AL,AL after; bound at its own body);
//   1F5FE0 (ECX = the buffer, EDI = 0; the BSP, the kind, -1, the index and
//   the key pushed; RET 14; bound at its own body);
//   2E88B0 (dispatch site 1F33BC; ECX = the buffer, the object pushed; RET 4);
//   283450 (cdecl: the object; ADD ESP,4 and TEST AL,AL after);
//   the object's destructor (CALL [vtable]; site 1F33FE; ECX = the object,
//   1 pushed; RET 4);
//   184E20 (dispatch sites 1F33EA and 1F3405; EAX = the shape's +10, ECX =
//   the instance; RET);
//   1F67C0 (dispatch site 1F3449; ECX = the buffer, EDX = the key, EAX = the
//   collision data, EDI = the instance, the secondary index pushed; RET 4);
//   1F7330 (dispatch site 1F345A; the key and the buffer pushed; RET 8).
inline constexpr std::uint32_t child_shape_build_target=0x2e88b0u,child_shape_place_target=0x184e20u,
    child_shape_instance_surface_target=0x1f67c0u,child_shape_other_target=0x1f7330u;
constexpr std::uint32_t child_shape_instruction(logic::bsp_child_shape::Access a) {
    using enum logic::bsp_child_shape::Access;
    switch(a) {
    case key_argument:return 0x1f7c12u;case rejected_buffer:return 0x1f7d55u;case empty_count:return 0x1f7d5du;
    case empty_user_data:return 0x1f7d63u;case empty_vtable:return 0x1f7d6au;
    case surface_buffer:return 0x1f7c46u;case collision_bsp:return 0x1f7c50u;
    case instance_structure:return 0x1f7c77u;case instance_list:return 0x1f7c80u;case instance_definition_index:return 0x1f7c88u;
    case definitions:return 0x1f7c8cu;case definition_collision:return 0x1f7c98u;case instance_buffer:return 0x1f7c9fu;
    case reference_count:case reference_count_store:return 0x1f7cb7u;case reference_count_check:return 0x1f7cbbu;
    case reference_kept:return 0x1f7cd4u;case object_vtable:return 0x1f7ce8u;
    case surface_structure:return 0x1f7d08u;case surface_instances:return 0x1f7d11u;case surface_definitions:return 0x1f7d17u;
    case surface_definition_index:return 0x1f7d21u;case surface_collision:return 0x1f7d2bu;case surface_buffer_5:return 0x1f7d33u;
    case other_buffer:return 0x1f7d44u;
    }
    return 0;
}
// 1F7C10 (validated by tools/validate_native_bsp_child_shape_tu15.py).
enum ChildShapeBranch : unsigned {
    c_rejected=swept_pair_branch_end,c_rejected_empty,c_surface,c_surface_empty,c_instance,c_instance_empty,c_released,
    c_static,c_destroyed,c_instance_surface,c_other,child_shape_branch_end
};
static_assert(child_shape_branch_end<=coverage_bits);

template<class State,class Children> struct ChildShapeAbi {
    using Access=logic::bsp_child_shape::Access;
    Port<State> &port;Children &children;
    std::uint32_t entry;
    logic::bsp_child_shape::Locals local{};
    enum class Exit {rejected,rejected_empty,surface,surface_empty,instance_kept,instance,instance_surface,other} exit{};
    bool built{};
    ChildShapeAbi(Port<State> &p,Children &c,std::uint32_t e):port(p),children(c),entry(e),local{e+4u,e+8u}{}
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    // EDI pushed: ESP for the accepted paths, which every child returns to.
    std::uint32_t accepted_esp()const{return entry-0xcu;}

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::bsp_child_shape::Access;
        auto &flags=port.flags;
        switch(a) {
        case collision_bsp:port.push(0x1f7c4eu,reg(esi));port.push(0x1f7c4fu,reg(eax));break;  // the key, the index
        case instance_structure:flags.minus(reg(edx),2u,4);break;                               // CMP EDX,2
        case instance_list:reg(eax)=flags.product(reg(eax),logic::bsp_child_shape::instance_size);break;
        case definition_collision:reg(edx)=flags.product(reg(edx),logic::bsp_child_shape::definition_size);break;
        case reference_count:
            if(!built)reg(ebx)=flags.logic(0u,4);  // XOR EBX,EBX: no buffer, no shape
            break;
        case reference_kept:reg(eax)=reg(ebx)+logic::bsp_child_shape::placed_transform;reg(ecx)=reg(edi);exit=Exit::instance_kept;break;
        case surface_structure:case other_buffer:flags.minus(reg(edx),2u,4);flags.minus(reg(edx),5u,4);break;  // CMP EDX,2; CMP EDX,5
        case surface_instances:reg(eax)=flags.product(reg(eax),logic::bsp_child_shape::instance_size);break;
        case surface_definition_index:reg(eax)=flags.plus(reg(eax),reg(edi),4);reg(edi)=reg(eax);break;  // ADD EAX,EDI; MOV EDI,EAX
        case surface_collision:reg(eax)=flags.product(reg(eax),logic::bsp_child_shape::definition_size);break;
        case surface_buffer_5:port.push(0x1f7d32u,reg(ecx));break;  // the secondary index
        default:break;
        }
        return child_shape_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::bsp_child_shape::Access;
        auto &flags=port.flags;
        const auto word=std::uint32_t(std::int32_t(std::int16_t(value)));
        switch(a) {
        case key_argument:reg(esi)=value;break;
        case rejected_buffer:
            reg(eax)=value;flags.logic(value,4);  // TEST EAX,EAX
            exit=value?Exit::rejected:Exit::rejected_empty;port.reached(value?c_rejected:c_rejected_empty);
            break;
        case surface_buffer:
            reg(ecx)=value;flags.logic(value,4);  // TEST ECX,ECX
            exit=value?Exit::surface:Exit::surface_empty;port.reached(value?c_surface:c_surface_empty);
            break;
        case collision_bsp:reg(eax)=value;break;
        case instance_structure:reg(ecx)=value;break;
        case instance_list:reg(eax)=flags.plus(reg(eax),value,4);reg(edi)=reg(eax);break;  // ADD EAX,[ECX+144]; MOV EDI,EAX
        case instance_definition_index:reg(edx)=word;break;
        case definitions:reg(eax)=value;break;
        case definition_collision:reg(esi)=value;break;
        case instance_buffer:
            // ADD ESI,40; TEST ECX,ECX; PUSH EBX
            reg(ecx)=value;reg(esi)=flags.plus(reg(esi),logic::bsp_child_shape::collision_shape_object,4);flags.logic(value,4);
            port.push(0x1f7ca8u,reg(ebx));
            exit=Exit::instance;port.reached(value?c_instance:c_instance_empty);
            break;
        case reference_count:flags.increment(value,2,true);break;  // DEC WORD [ESI+6]
        case reference_count_check:
            flags.minus(value,0u,2);  // CMP WORD [ESI+6],0
            if(value==0)port.reached(c_released);
            break;
        case object_vtable:reg(edx)=value;break;
        case surface_structure:reg(edx)=value;break;
        case surface_instances:reg(edi)=value;break;
        case surface_definitions:reg(edx)=value;break;
        case surface_definition_index:reg(eax)=word;break;
        case surface_collision:reg(eax)=value;break;
        case surface_buffer_5:reg(ecx)=value;exit=Exit::instance_surface;port.reached(c_instance_surface);break;
        case other_buffer:reg(eax)=value;exit=Exit::other;port.reached(c_other);break;
        default:break;
        }
    }

    // ---- Frame
    const logic::bsp_child_shape::Locals &locals()const{return local;}
    // 1F76A0: MOV EDX,ESI; MOV [ESP+4],ECX (the slot PUSH ECX filled). After
    // an accepted key: the kind, index and secondary index into EDX, EAX and
    // ECX, CMP EDX,1, PUSH EDI.
    bool may_collide(std::uint32_t key) {
        auto &flags=port.flags;
        if(reg(esi)!=key)throw std::logic_error("Native 1F7C10 key differs from ESI");
        reg(edx)=reg(esi);
        port.store(0x1f7c18u,entry-4u,4,reg(ecx));
        const auto esp_after=reg(esp);
        port.push(0x1f7c1cu,0x1f7c21u);
        genuine_child(port,esp_after,"1F7C10",[&]{port.call_child(0x1f76a0u,children.filter);});
        flags.logic(reg(eax)&0xffu,1);  // TEST AL,AL
        if(!(reg(eax)&0xffu))return false;
        reg(ecx)=flags.right(reg(esi),16u,4,false);reg(edx)=reg(esi);reg(eax)=reg(esi);
        reg(edx)=flags.right(reg(edx),29u,4,false);reg(eax)=flags.logic(reg(eax)&0xffffu,4);reg(ecx)=flags.logic(reg(ecx)&0x1fffu,4);
        flags.minus(reg(edx),logic::bsp_child_shape::structure_surface_kind,4);
        port.push(0x1f7c43u,reg(edi));
        return true;
    }
    // 1F5FE0 (the key and index are collision_bsp's pushes): PUSH -1; PUSH
    // EDX; PUSH EAX; XOR EDI,EDI.
    std::uint32_t surface_polygon(std::uint32_t buffer,std::uint32_t bsp,std::uint32_t kind,std::uint32_t owner,std::uint32_t index,
        std::uint32_t key) {
        port.push(0x1f7c55u,owner);port.push(0x1f7c57u,reg(edx));port.push(0x1f7c58u,reg(eax));
        reg(edi)=port.flags.logic(0u,4);
        if(reg(ecx)!=buffer || reg(eax)!=bsp || reg(edx)!=kind || reg(esi)!=key || (key&0xffffu)!=index)
            throw std::logic_error("Native 1F7C10 polygon arguments differ from the registers");
        port.push(0x1f7c5bu,0x1f7c60u);
        genuine_child(port,accepted_esp(),"1F7C10",[&]{port.call_child(0x1f5fe0u,children.polygon);});
        return reg(eax);
    }
    // 2E88B0: PUSH ESI (the object); ECX = the buffer; MOV EBX,EAX after.
    std::uint32_t build_instance_shape(std::uint32_t buffer,std::uint32_t object) {
        if(reg(ecx)!=buffer || reg(esi)!=object)throw std::logic_error("Native 1F7C10 shape build arguments differ from the registers");
        built=true;
        const auto esp_after=reg(esp);
        port.push(0x1f7cabu,reg(esi));port.push(0x1f7cacu,0x1f7cb1u);
        genuine_child(port,esp_after,"1F7C10",[&]{call_dispatch(port,child_shape_build_target,0x1f33bcu,children.dispatch);});
        reg(ebx)=reg(eax);
        return reg(ebx);
    }
    // 283450(object): cdecl; ADD ESP,4 and TEST AL,AL after.
    bool is_static(std::uint32_t object) {
        if(reg(esi)!=object)throw std::logic_error("Native 1F7C10 object differs from ESI");
        port.push(0x1f7cc2u,reg(esi));
        const auto esp_after=reg(esp);
        port.push(0x1f7cc3u,0x1f7cc8u);
        genuine_child(port,esp_after,"1F7C10",[&]{port.call_child(0x283450u,children.is_static);});
        reg(esp)=port.flags.plus(reg(esp),4u,4);
        port.flags.logic(reg(eax)&0xffu,1);
        const bool kept=(reg(eax)&0xffu)!=0;
        port.reached(kept?c_static:c_destroyed);
        return kept;
    }
    // object->~(1): PUSH 1; MOV ECX,ESI; CALL [EDX].
    void destroy(std::uint32_t object,std::uint32_t vtable) {
        if(reg(esi)!=object || reg(edx)!=vtable)throw std::logic_error("Native 1F7C10 destructor arguments differ from the registers");
        const auto esp_after=reg(esp);
        port.push(0x1f7ceau,1u);reg(ecx)=reg(esi);
        const auto target=port.read(0x1f7ceeu,reg(edx),4);
        port.push(0x1f7ceeu,0x1f7cf0u);
        genuine_child(port,esp_after,"1F7C10",[&]{call_dispatch(port,target,0x1f33feu,children.dispatch);});
    }
    // 184E20: LEA EAX,[EBX+10]; MOV ECX,EDI (reference_kept's on the static
    // path); MOV EAX,EBX after.
    void place_at_instance(std::uint32_t transform,std::uint32_t instance) {
        const bool kept=exit==Exit::instance_kept;
        if(!kept){reg(eax)=reg(ebx)+logic::bsp_child_shape::placed_transform;reg(ecx)=reg(edi);}
        if(reg(eax)!=transform || reg(ecx)!=instance)throw std::logic_error("Native 1F7C10 placement arguments differ from the registers");
        const auto esp_after=reg(esp);
        port.push(kept?0x1f7cdau:0x1f7cf5u,kept?0x1f7cdfu:0x1f7cfau);
        genuine_child(port,esp_after,"1F7C10",[&]{call_dispatch(port,child_shape_place_target,kept?0x1f33eau:0x1f3405u,children.dispatch);});
        reg(eax)=reg(ebx);
    }
    // 1F67C0 (the secondary index is surface_buffer_5's push): MOV EDX,ESI.
    std::uint32_t instance_surface_polygon(std::uint32_t buffer,std::uint32_t collision,std::uint32_t,std::uint32_t key,
        std::uint32_t instance) {
        reg(edx)=reg(esi);
        if(reg(ecx)!=buffer || reg(eax)!=collision || reg(edx)!=key || reg(edi)!=instance)
            throw std::logic_error("Native 1F7C10 instance surface arguments differ from the registers");
        port.push(0x1f7d39u,0x1f7d3eu);
        genuine_child(port,accepted_esp(),"1F7C10",[&]{call_dispatch(port,child_shape_instance_surface_target,0x1f3449u,children.dispatch);});
        return reg(eax);
    }
    // 1F7330(key, buffer): PUSH EAX; PUSH ESI.
    std::uint32_t other_shape(std::uint32_t key,std::uint32_t buffer) {
        if(reg(esi)!=key || reg(eax)!=buffer)throw std::logic_error("Native 1F7C10 shape arguments differ from the registers");
        port.push(0x1f7d48u,reg(eax));port.push(0x1f7d49u,reg(esi));port.push(0x1f7d4au,0x1f7d4fu);
        genuine_child(port,accepted_esp(),"1F7C10",[&]{call_dispatch(port,child_shape_other_target,0x1f345au,children.dispatch);});
        return reg(eax);
    }
};
template<class State,class Children> void kernel_1f7c10(Port<State> &port,Children &children) {
    using Exit=typename ChildShapeAbi<State,Children>::Exit;
    auto &gp=port.gp;auto &flags=port.flags;
    const auto entry=gp[esp];
    port.push(0x1f7c10u,gp[ecx]);port.push(0x1f7c11u,gp[esi]);
    ChildShapeAbi<State,Children> abi(port,children,entry);
    Memory<Port<State>,ChildShapeAbi<State,Children>> memory{port,abi};
    const auto result=logic::bsp_child_shape::child_shape(memory,abi);
    // The exits: the pushes popped back (POP EBX, EDI, ESI, ECX as pushed),
    // XOR EAX,EAX where there is no shape.
    std::uint32_t pc{};
    switch(abi.exit) {
    case Exit::rejected:pc=0x1f7d70u;break;
    case Exit::rejected_empty:gp[eax]=flags.logic(0u,4);pc=0x1f7d77u;break;
    case Exit::surface:gp[edi]=port.pop(0x1f7c60u);pc=0x1f7c61u;break;
    case Exit::surface_empty:gp[edi]=port.pop(0x1f7c66u);gp[eax]=flags.logic(0u,4);pc=0x1f7c69u;break;
    case Exit::instance_kept:gp[ebx]=port.pop(0x1f7ce1u);gp[edi]=port.pop(0x1f7ce2u);pc=0x1f7ce3u;break;
    case Exit::instance:gp[ebx]=port.pop(0x1f7cfcu);gp[edi]=port.pop(0x1f7cfdu);pc=0x1f7cfeu;break;
    case Exit::instance_surface:gp[edi]=port.pop(0x1f7d3eu);pc=0x1f7d3fu;break;
    case Exit::other:gp[edi]=port.pop(0x1f7d4fu);pc=0x1f7d50u;break;
    }
    gp[esi]=port.pop(pc);gp[ecx]=port.pop(pc+1u);
    if(gp[esp]!=entry)throw std::logic_error("Native 1F7C10 frame is unbalanced");
    if(gp[eax]!=result)throw std::logic_error("Native 1F7C10 result differs from EAX");
    port.ret(pc+2u,8u);port.publish(pc+2u);
}

// ---- 1F6D10 structure AABB query --------------------------------------------
// thiscall: ECX = the structure collision shape; [ESP+4] the AABB, [+8] the
// hits (hkArray). RET 8. PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,F4;
// EBX, ESI and EDI are saved between the argument reads. T, the ESP after
// them, is the base of the locals: +10 the collision data (then the shape's
// +14), +14 this, +18 the walked key's index, +1C the resume index, +20 the
// staged position, +30 the instance's transform, +70 the box's, +B0 the half
// extents, +C0 the box in the instance's frame.
// Registers: EBX = the AABB (re-read for each later key), ESI = the hits,
// EDI = the walk's index; on the expansion path EBX holds the size before the
// query, then the index of the key being filtered, and EDI that key. The
// children (each with the original's registers at the call):
//   2E9670 (canonical 2E5BC0; thiscall on the shape: the AABB, the hits;
//   RET 8) through the unit's route;
//   1F6FA0 (canonical 1F26B0; ESI = the hits, the shape pushed; RET 4)
//   through the unit's route;
//   2E4550 inline (ECX = the box in the instance's frame; the instance's
//   transform and the box's pushed);
//   the definition's queryObb (vtable +2C; ECX = the MOPP shape, EAX = its
//   vtable; the transform, the extents, the tolerance and the hits pushed;
//   RET 10) through the unit's dispatch (canonical site 1F262E);
//   1F76A0 (EDX = EDI = the instance surface key; TEST AL,AL after) through
//   the unit's route (bound at its own body).
// The exit flags are the walk's last CMP EDI,EAX (TEST EAX,EAX when the
// filtered array is empty); EAX the size read last.
inline constexpr std::uint32_t aabb_query_structure_target=0x2e9670u,aabb_query_filter_target=0x1f6fa0u,
    aabb_query_site=0x1f262eu;
constexpr std::uint32_t aabb_query_instruction(logic::structure_aabb_query::Access a) {
    using enum logic::structure_aabb_query::Access;
    switch(a) {
    case aabb_argument:return 0x1f6d1du;case hits_argument:return 0x1f6d21u;case shape_store:return 0x1f6d29u;
    case walk_size:return 0x1f6d38u;case aabb_reload:return 0x1f6d47u;case walk_data:return 0x1f6d50u;case walk_key:return 0x1f6d52u;
    case index_store:return 0x1f6d68u;case structure_global:return 0x1f6d72u;case instances:return 0x1f6d7bu;
    case definition_index:return 0x1f6d81u;case definitions:return 0x1f6d8bu;case surface_count:return 0x1f6d91u;
    case box_maximum:return 0x1f6dabu;case collision_pointer:return 0x1f6dafu;case half:return 0x1f6e32u;
    case position_staged:return 0x1f6e52u;case translation_store:return 0x1f6e5au;case box_minimum:return 0x1f6e5fu;
    case one:return 0x1f6e8bu;case collision_store:return 0x1f6e98u;case half_extents_store:return 0x1f6ea7u;
    case center_store:return 0x1f6ec7u;
    case expanded_size:return 0x1f6ed4u;case expanded_size_store:return 0x1f6edcu;case expanded_shift_data:return 0x1f6ee3u;
    case expanded_shift_next:return 0x1f6ee5u;case expanded_shift_store:return 0x1f6eecu;case expanded_shift_size:return 0x1f6eeeu;
    case collision_reload:return 0x1f6ef6u;case query_vtable:return 0x1f6efau;case query_first_new:return 0x1f6efdu;
    case resume_store:return 0x1f6f1au;case query_function:return 0x1f6f1eu;case query_size:return 0x1f6f21u;
    case shape_reload:return 0x1f6f26u;case subobject_store:return 0x1f6f2du;
    case new_data:return 0x1f6f31u;case new_key:return 0x1f6f33u;case instance_index_reload:return 0x1f6f36u;
    case accepted_data:return 0x1f6f50u;case accepted_store:return 0x1f6f52u;
    case rejected_size:return 0x1f6f57u;case rejected_size_store:return 0x1f6f5fu;case rejected_shift_data:return 0x1f6f66u;
    case rejected_shift_next:return 0x1f6f68u;case rejected_shift_store:return 0x1f6f6fu;case rejected_shift_size:return 0x1f6f71u;
    case new_size:return 0x1f6f7au;case resume_reload:return 0x1f6f82u;case walk_size_next:return 0x1f6f86u;
    case matrix_field:case transform_field:case zero_field:case box_column_store:case one_store:break;  // by address
    }
    return 0;
}
// The instructions of the accesses that differ only by address: the matrix
// field read (by its offset in the instance), and the frame stores (by their
// offset from T).
constexpr std::uint32_t aabb_query_matrix_instruction(std::uint32_t offset) {
    switch(offset) {
    case 0x10u:return 0x1f6da1u;case 0x04u:return 0x1f6da6u;case 0x14u:return 0x1f6dbbu;case 0x18u:return 0x1f6dc6u;
    case 0x1cu:return 0x1f6dd1u;case 0x20u:return 0x1f6ddcu;case 0x24u:return 0x1f6de7u;case 0x08u:return 0x1f6df2u;
    case 0x28u:return 0x1f6dfdu;case 0x2cu:return 0x1f6e08u;case 0x0cu:return 0x1f6e13u;case 0x30u:return 0x1f6e1eu;
    }
    return 0;
}
constexpr std::uint32_t aabb_query_frame_instruction(std::uint32_t offset) {
    switch(offset) {
    case 0x40u:return 0x1f6db5u;case 0x44u:return 0x1f6dc0u;case 0x48u:return 0x1f6dcbu;case 0x50u:return 0x1f6dd6u;
    case 0x54u:return 0x1f6de1u;case 0x30u:return 0x1f6decu;case 0x58u:return 0x1f6df7u;case 0x20u:return 0x1f6e02u;
    case 0x34u:return 0x1f6e0du;case 0x24u:return 0x1f6e18u;case 0x38u:return 0x1f6e23u;case 0x28u:return 0x1f6e2cu;
    case 0x3cu:return 0x1f6e3au;case 0x4cu:return 0x1f6e40u;case 0x5cu:return 0x1f6e46u;case 0x2cu:return 0x1f6e4cu;
    }
    return 0;
}
constexpr std::uint32_t aabb_query_box_instruction(std::uint32_t offset) {
    switch(offset) {
    case 0x70u:return 0x1f6e76u;case 0x80u:return 0x1f6e7bu;case 0x90u:return 0x1f6e83u;  // the zero columns
    }
    return 0;
}
constexpr std::uint32_t aabb_query_one_instruction(std::uint32_t offset) {
    switch(offset) {
    case 0x70u:return 0x1f6eafu;case 0x84u:return 0x1f6eb5u;case 0x98u:return 0x1f6ebeu;
    }
    return 0;
}
// 1F6D10 (validated by tools/validate_native_structure_aabb_query_tu15.py).
enum AabbQueryBranch : unsigned {
    q_empty=child_shape_branch_end,q_other_kind,q_too_many,q_expanded,q_expanded_shift,q_no_new,q_accepted,q_rejected,
    q_rejected_shift,aabb_query_branch_end
};
static_assert(aabb_query_branch_end<=coverage_bits);

template<class State,class Children> struct AabbQueryAbi {
    using Access=logic::structure_aabb_query::Access;
    Port<State> &port;Children &children;
    std::uint32_t t;  // ESP after the saves
    logic::structure_aabb_query::Locals local{};
    std::uint32_t instance{},pushed_a{},pushed_b{};bool kept{};
    AabbQueryAbi(Port<State> &p,Children &c,std::uint32_t ebp,std::uint32_t base):port(p),children(c),t(base) {
        local={.aabb_argument=ebp+8u,.hits_argument=ebp+0xcu,.shape=t+0x14u,.instance_index=t+0x18u,.resume_index=t+0x1cu,
            .collision=t+0x10u,.position=t+0x20u,.instance_transform=t+0x30u,.box_transform=t+0x70u,.half_extents=t+0xb0u,
            .query_transform=t+0xc0u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t address) {
        using enum logic::structure_aabb_query::Access;
        auto &flags=port.flags;
        switch(a) {
        case aabb_argument:port.push(0x1f6d1cu,reg(ebx));break;
        case hits_argument:port.push(0x1f6d20u,reg(esi));break;
        case shape_store:  // PUSH EDI; PUSH ESI; MOV EDI,ECX; PUSH EBX
            port.push(0x1f6d24u,reg(edi));port.push(0x1f6d25u,reg(esi));reg(edi)=reg(ecx);port.push(0x1f6d28u,reg(ebx));
            break;
        case index_store:  // MOV EAX,ECX; AND EAX,FFFF; AND ECX,E0000000; CMP ECX,40000000
            reg(eax)=flags.logic(reg(ecx)&0xffffu,4);reg(ecx)=flags.logic(reg(ecx)&0xe0000000u,4);
            flags.minus(reg(ecx),logic::structure_aabb_query::instance_kind_bits,4);
            if(reg(ecx)!=logic::structure_aabb_query::instance_kind_bits)port.reached(q_other_kind);
            break;
        case instances:reg(eax)=flags.product(reg(eax),logic::structure_aabb_query::instance_size);break;
        case definition_index:instance=address-logic::structure_aabb_query::instance_definition;break;
        case definitions:reg(ecx)=flags.product(reg(ecx),logic::structure_aabb_query::definition_size);break;
        case matrix_field:return aabb_query_matrix_instruction(address-instance);
        case transform_field:case zero_field:return aabb_query_frame_instruction(address-t);
        case box_column_store:reg(edx)=t+0x70u;return aabb_query_box_instruction(address-t);  // LEA EDX,[ESP+70] first
        case collision_store:pushed_b=reg(edx);port.push(0x1f6e93u,pushed_b);reg(eax)=t+0x30u;break;  // PUSH EDX; LEA EAX,[ESP+34]
        case half_extents_store:pushed_a=reg(eax);port.push(0x1f6e9fu,pushed_a);reg(ecx)=t+0xc0u;break;  // PUSH EAX; LEA ECX,[ESP+C8]
        case one_store:return aabb_query_one_instruction(address-t);
        case center_store:port.reached(q_expanded);break;
        case expanded_size_store:  // DEC EBX; MOV ECX,EBX; CMP EDI,ECX; ...; MOV EAX,EDI
            reg(ebx)=flags.increment(reg(ebx),4,true);reg(ecx)=reg(ebx);flags.minus(reg(edi),reg(ecx),4);reg(eax)=reg(edi);
            break;
        case expanded_shift_store:case rejected_shift_store:
            reg(ecx)=reg(ecx)+reg(eax)*4u;  // LEA ECX,[ECX+EAX*4]
            port.reached(a==expanded_shift_store?q_expanded_shift:q_rejected_shift);
            break;
        case resume_store:
            // PUSH ESI; PUSH tolerance; LEA EDX,[ESP+B8]; PUSH EDX; ADD ECX,50;
            // LEA EDX,[ESP+CC]; DEC EDI; PUSH EDX
            port.push(0x1f6f00u,reg(esi));port.push(0x1f6f01u,logic::structure_aabb_query::query_tolerance);
            reg(edx)=t+0xb0u;port.push(0x1f6f0du,reg(edx));
            reg(ecx)=flags.plus(reg(ecx),logic::structure_aabb_query::instance_query_shape,4);
            reg(edx)=t+0xc0u;reg(edi)=flags.increment(reg(edi),4,true);port.push(0x1f6f19u,reg(edx));
            break;
        case subobject_store:reg(eax)=flags.plus(reg(eax),logic::structure_aabb_query::filter_subobject,4);break;
        case rejected_size_store:  // DEC EDI; MOV ECX,EDI; CMP EBX,ECX; ...; MOV EAX,EBX
            reg(edi)=flags.increment(reg(edi),4,true);reg(ecx)=reg(edi);flags.minus(reg(ebx),reg(ecx),4);reg(eax)=reg(ebx);
            break;
        default:break;
        }
        return aabb_query_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::structure_aabb_query::Access;
        auto &flags=port.flags;
        switch(a) {
        case aabb_argument:case aabb_reload:reg(ebx)=value;break;
        case hits_argument:reg(esi)=value;break;
        case walk_size:  // XOR EDI,EDI; TEST EAX,EAX
            reg(eax)=value;reg(edi)=flags.logic(0u,4);flags.logic(value,4);
            if(std::int32_t(value)<=0)port.reached(q_empty);
            break;
        case walk_data:reg(eax)=value;break;
        case walk_key:reg(ecx)=value;break;
        case structure_global:reg(edx)=value;break;
        case instances:reg(eax)=flags.plus(reg(eax),value,4);break;
        case definition_index:reg(ecx)=std::uint32_t(std::int32_t(std::int16_t(value)));break;
        case definitions:reg(ecx)=flags.plus(reg(ecx),value,4);break;
        case surface_count:
            flags.minus(value,std::uint32_t(logic::structure_aabb_query::expandable_surfaces),4);
            if(std::int32_t(value)>logic::structure_aabb_query::expandable_surfaces)port.reached(q_too_many);
            break;
        case collision_pointer:reg(ecx)=value;break;
        case expanded_size:reg(ebx)=value;break;
        case rejected_size:reg(edi)=value;break;
        case expanded_shift_data:case rejected_shift_data:reg(ecx)=value;break;
        case expanded_shift_next:case rejected_shift_next:reg(edx)=value;break;
        case expanded_shift_size:case rejected_shift_size:  // INC EAX; CMP EAX,ECX
            reg(ecx)=value;reg(eax)=flags.increment(reg(eax),4,false);flags.minus(reg(eax),reg(ecx),4);
            break;
        case collision_reload:reg(ecx)=value;break;
        case query_vtable:reg(eax)=value;break;
        case query_first_new:reg(ebx)=value;break;
        case query_size:
            flags.minus(reg(ebx),value,4);  // CMP EBX,[ESI+4]
            if(!(std::int32_t(reg(ebx))<std::int32_t(value)))port.reached(q_no_new);
            break;
        case shape_reload:reg(eax)=value;break;
        case new_data:reg(ecx)=value;break;
        case new_key:reg(edi)=value;break;
        case instance_index_reload:reg(edx)=value;break;
        case accepted_data:reg(edx)=value;break;
        case new_size:  // (DEC EBX after a removal;) INC EBX; CMP EBX,EAX
            reg(eax)=value;
            if(!kept)reg(ebx)=flags.increment(reg(ebx),4,true);
            reg(ebx)=flags.increment(reg(ebx),4,false);flags.minus(reg(ebx),reg(eax),4);
            break;
        case resume_reload:reg(edi)=value;break;
        case walk_size_next:reg(eax)=value;reg(edi)=flags.increment(reg(edi),4,false);flags.minus(reg(edi),value,4);break;
        default:break;
        }
    }

    // ---- Frame
    const logic::structure_aabb_query::Locals &locals()const{return local;}
    // 2E9670: the arguments were pushed before the shape's store.
    void query_structure(std::uint32_t shape,std::uint32_t aabb,std::uint32_t hits) {
        if(reg(ecx)!=shape || reg(edi)!=shape || reg(ebx)!=aabb || reg(esi)!=hits)
            throw std::logic_error("Native 1F6D10 MOPP query arguments differ from the registers");
        port.push(0x1f6d2du,0x1f6d32u);
        genuine_child(port,t,"1F6D10",[&]{port.call_child(aabb_query_structure_target,children.query_structure);});
    }
    // 1F6FA0: PUSH EDI (the shape); ESI = the hits.
    void filter_structure_keys(std::uint32_t shape,std::uint32_t hits) {
        if(reg(edi)!=shape || reg(esi)!=hits)throw std::logic_error("Native 1F6D10 key filter arguments differ from the registers");
        port.push(0x1f6d32u,reg(edi));port.push(0x1f6d33u,0x1f6d38u);
        genuine_child(port,t,"1F6D10",[&]{port.call_child(aabb_query_filter_target,children.filter_keys);});
    }
    // 2E4550 inline (its arguments are collision_store's and
    // half_extents_store's pushes).
    void relative_transform(std::uint32_t out,std::uint32_t a,std::uint32_t b) {
        if(reg(ecx)!=out || pushed_a!=a || pushed_b!=b || reg(esp)!=t-8u)
            throw std::logic_error("Native 1F6D10 relative transform arguments differ from the pushed ones");
        port.push(0x1f6ecfu,0x1f6ed4u);
        kernel_2e4550(port);
        if(reg(esp)!=t)throw std::logic_error("Native 1F6D10 frame is unbalanced after 2E4550");
    }
    // The definition's queryObb (resume_store pushed its arguments).
    void query_instance(std::uint32_t function,std::uint32_t shape,std::uint32_t transform,std::uint32_t extents,std::uint32_t,
        std::uint32_t hits) {
        if(reg(ecx)!=shape || reg(edx)!=transform || reg(esi)!=hits || t+0xb0u!=extents)
            throw std::logic_error("Native 1F6D10 instance query arguments differ from the registers");
        port.push(0x1f6f1eu,0x1f6f21u);
        genuine_child(port,t,"1F6D10",[&]{call_dispatch(port,function,aabb_query_site,children.dispatch);});
    }
    // 1F76A0: OR EDI,FFFFA000; SHL EDI,10; OR EDI,EDX; MOV EDX,EDI; TEST AL,AL after.
    bool may_collide(std::uint32_t key) {
        auto &flags=port.flags;
        reg(edi)=flags.logic(reg(edi)|logic::structure_aabb_query::surface_kind_fill,4);
        reg(edi)=flags.shift(reg(edi),16u,4);
        reg(edi)=flags.logic(reg(edi)|reg(edx),4);
        reg(edx)=reg(edi);
        if(reg(edi)!=key)throw std::logic_error("Native 1F6D10 surface key differs from EDI");
        port.push(0x1f6f47u,0x1f6f4cu);
        genuine_child(port,t,"1F6D10",[&]{port.call_child(0x1f76a0u,children.filter);});
        flags.logic(reg(eax)&0xffu,1);
        kept=(reg(eax)&0xffu)!=0;
        port.reached(kept?q_accepted:q_rejected);
        return kept;
    }
};
template<class State,class Children> void kernel_1f6d10(Port<State> &port,Children &children) {
    auto &gp=port.gp;auto &flags=port.flags;
    const auto entry=gp[esp],shape=gp[ecx];
    port.push(0x1f6d10u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=flags.logic(gp[esp]&0xfffffff0u,4);  // AND ESP,-16
    gp[esp]=flags.minus(gp[esp],0xf4u,4);        // SUB ESP,F4
    AabbQueryAbi<State,Children> abi(port,children,gp[ebp],gp[esp]-0xcu);  // EBX, ESI and EDI follow
    Memory<Port<State>,AabbQueryAbi<State,Children>> memory{port,abi};
    logic::structure_aabb_query::query_aabb(memory,abi,shape);
    if(gp[esp]!=abi.t)throw std::logic_error("Native 1F6D10 frame is unbalanced");
    gp[edi]=port.pop(0x1f6f92u);gp[esi]=port.pop(0x1f6f93u);gp[ebx]=port.pop(0x1f6f94u);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x1f6f97u);
    if(gp[esp]!=entry)throw std::logic_error("Native 1F6D10 frame is unbalanced at its exit");
    port.ret(0x1f6f98u,8u);port.publish(0x1f6f98u);
}

// ---- 2F36F0 child agent update -----------------------------------------------
// thiscall: ECX = the agent; [ESP+4] body A, [+8] body B, [+C] the input,
// [+10] the function that creates a child agent. RET 10. PUSH EBP; MOV
// EBP,ESP; AND ESP,-16; an exception frame (handler 37E656; its unwind state
// is [T+3DC]); SUB ESP,3C8; EBX, ESI and EDI saved between the first reads.
// T, the ESP after them, is the base of the locals: +10 the removal's offset
// / the end of the entries / the move's cursor / the merged key, +14 a key's
// position / the end of the keys (merge), +18 the half tolerance / the end of
// the entries / the new size, +1F the filter's hkBool, +20 the slack / the end
// of the keys / the move's step / the contact manager (merge), +24 the moved
// lanes / the end of the keys (insertion) / the comparator, +28 this / the
// contact manager (query order), +2C the merged entries (hkLocalArray), +40
// the AABB, +60 the container, +64 the new child's CdBody, +80 its shape
// buffer, +180 the keys (hkInplaceArray, storage +18C), +390 body A in body
// B's frame. getAabb()'s tolerance is written into its pushed slot (T-8).
// Registers: EBX = body B, then the expected key (query order) or the merged
// entries' cursor; ESI = this (+C in the query order), then the keys' cursor
// (merge); EDI = body A, the tree, the container, then the entries' cursor.
// The children, each with the original's registers at the call:
//   2E4550 inline (ECX = the transform; body B's and body A's pushed);
//   2E31E0 (hkArray reserve; cdecl: the array, the capacity, 12), 2F3430
//   (hkLocalArray(count); thiscall, RET 4), 2F34D0 (its destructor) and
//   2F33B0 (cdecl: keys, 0, count - 1, the comparator; bound at its own body)
//   through the unit's route;
//   the virtual calls and the creation function through the unit's dispatch
//   with their canonical sites; 30BAD0 inline.
// The exit flags are the last child's, TEST EAX,EAX of the keys' capacity,
// or the cache test's TEST AL,AL.
inline constexpr std::uint32_t child_agents_handler=0x37e656u;
namespace child_agents_site {
inline constexpr std::uint32_t shape_aabb=0x2efca6u,query_tree=0x2efd5eu,cleanup=0x2efdf8u,child_shape=0x2eff84u,enabled=0x2effb2u,
    create=0x2effcau,merge_child_shape=0x2f0112u,merge_enabled=0x2f0143u,merge_create=0x2f015cu,gone=0x2f0190u,rest=0x2f01b3u,
    tail_child_shape=0x2f01d8u,tail_enabled=0x2f0202u,tail_create=0x2f021au,install_deallocate=0x2f026bu,install_allocate=0x2f0283u,
    keys_deallocate=0x2f02ffu;
}
constexpr std::uint32_t child_agents_instruction(logic::child_agent_update::Access a,bool null_path) {
    using enum logic::child_agent_update::Access;
    switch(a) {
    case body_b_argument:return 0x2f3712u;case body_b_motion:return 0x2f3715u;case body_a_argument:return 0x2f371au;
    case body_a_motion:return 0x2f371fu;case agent_store:return 0x2f3731u;case input_argument:return 0x2f373au;
    case tolerance:return 0x2f373du;case body_a_shape:return 0x2f3740u;case half_factor:return 0x2f3742u;
    case shape_a_vtable:return 0x2f3748u;case half_store:return 0x2f374fu;case half_reload:return 0x2f3754u;
    case tolerance_again:return 0x2f3758u;case margin_store:return 0x2f3762u;case shape_aabb_function:return 0x2f3766u;
    case cache_enabled:return 0x2f3769u;case aabb_minimum:return 0x2f3770u;case aabb_maximum:return 0x2f3775u;
    case slack_read:return 0x2f377cu;case cached_maximum:return 0x2f3780u;case absolute_mask_read:return 0x2f3784u;
    case slack_store:return 0x2f378bu;case slack_reload:return 0x2f378fu;case moved_store:return 0x2f37b1u;
    case cached_minimum:return 0x2f37b7u;case moved_store_again:return 0x2f37cbu;
    case cache_minimum_store:return 0x2f37e3u;case cache_maximum_store:return 0x2f37e7u;case tree_read:return 0x2f37ebu;
    case hits_data_init:return 0x2f37edu;case hits_size_init:return 0x2f37f4u;case hits_capacity_init:return 0x2f37fbu;
    case tree_vtable:return 0x2f3806u;case query_function:return 0x2f381eu;case query_order_read:return 0x2f3821u;
    case container_read:return 0x2f3828u;case body_b_motion_again:return 0x2f382bu;case child_parent_store:return 0x2f382eu;
    case child_motion_store:return 0x2f3832u;case container_store:return 0x2f3836u;
    case manager_read:return 0x2f3840u;case entries_data:return 0x2f3843u;case keys_data:return 0x2f3846u;
    case manager_store:return 0x2f3850u;case entries_size:return 0x2f3854u;case keys_size:return 0x2f385au;
    case entries_end_store:return 0x2f3869u;case keys_end_store:return 0x2f386fu;case entry_key:return 0x2f3884u;
    case expected_key:return 0x2f3886u;case search_start_store:return 0x2f3896u;case search_entry_key:return 0x2f389cu;
    case search_key:return 0x2f38a0u;case search_end_store:return 0x2f38afu;case found_store:return 0x2f39f2u;
    case entry_agent:return 0x2f38b3u;case child_vtable:return 0x2f38b6u;case cleanup_function:return 0x2f38b8u;
    case removal_size:return 0x2f38bbu;case removal_data:return 0x2f38beu;case removal_size_store:return 0x2f38c1u;
    case removal_size_reload:return 0x2f38cfu;case shift_offset_store:return 0x2f38e5u;case shift_data:return 0x2f38f0u;
    case shift_word0:return 0x2f38f7u;case shift_word0_store:return 0x2f38f9u;case shift_word1:return 0x2f38fbu;
    case shift_word1_store:return 0x2f38feu;case shift_word2:return 0x2f3901u;case shift_word2_store:return 0x2f3904u;
    case shift_offset_reload:return 0x2f3907u;case shift_size:return 0x2f390bu;case shift_offset_store_next:return 0x2f3914u;
    case position_reload:return 0x2f391au;case entries_end_reload:return 0x2f391eu;case keys_data_reload:return 0x2f3922u;
    case entries_end_store_again:return 0x2f392fu;case keys_end_reload:return 0x2f3933u;case entries_end_next:return 0x2f3937u;
    case insert_entries_size:return 0x2f3946u;case insert_keys_size:return 0x2f3949u;case insert_entries_data:return 0x2f3956u;
    case insert_keys_size_again:return 0x2f3958u;case insert_entries_end_store:return 0x2f396au;case insert_cursor_store:return 0x2f3970u;
    case insert_keys_end_store:return 0x2f3974u;case insert_end_read:return 0x2f3980u;case insert_entry_key:return 0x2f3986u;
    case insert_key:return 0x2f3988u;case move_count_store:return 0x2f399cu;case insert_capacity:return 0x2f39a0u;
    case new_size_store:return 0x2f39aau;case insert_data:return 0x2f39c4u;case move_count_reload:return 0x2f39c6u;
    case move_first_store:return 0x2f39d7u;case move_first_reload:return 0x2f39e3u;case move_step_store:return 0x2f39e8u;
    case move_left_store:return 0x2f39ecu;case move_step_reload:return 0x2f39feu;case move_word0:return 0x2f3a04u;
    case move_word0_store:return 0x2f3a08u;case move_word1:return 0x2f3a0au;case move_word1_store:return 0x2f3a0du;
    case move_word2:return 0x2f3a10u;case move_word2_store:return 0x2f3a13u;case move_left_reload:return 0x2f3a16u;
    case move_left_store_next:return 0x2f3a1eu;case new_size_reload:return 0x2f3a24u;case insert_cursor_reload:return 0x2f3a28u;
    case insert_data_again:return 0x2f3a2cu;case container_reload:return 0x2f3a2eu;case insert_size_store:return 0x2f3a32u;
    case insert_key_read:return 0x2f3a35u;case container_vtable:return 0x2f3a41u;case child_shape_function:return 0x2f3a44u;
    case child_key_store:return 0x2f3a47u;case insert_cursor_again:return 0x2f3a4bu;case child_shape_store:return 0x2f3a4fu;
    case filter_key:return 0x2f3a53u;case filter_input:return 0x2f3a55u;case filter_read:return 0x2f3a58u;
    case filter_vtable:return 0x2f3a5bu;case filter_container:return 0x2f3a5eu;case filter_body_a:return 0x2f3a68u;
    case filter_function:return 0x2f3a72u;case enabled_read:return 0x2f3a74u;case create_manager:return 0x2f3a79u;
    case create_input:return 0x2f3a7du;case create_function:return 0x2f3a8au;case insert_cursor_final:return 0x2f3a97u;
    case new_agent_store:return 0x2f3a9bu;case new_key:return 0x2f3a9eu;case new_key_store:return 0x2f3aa0u;
    case insert_size_reload:return 0x2f3aa2u;case insert_data_reload:return 0x2f3aa5u;case insert_keys_data_reload:return 0x2f3aadu;
    case insert_entries_end_store_again:return 0x2f3ab4u;case insert_cursor_next:return 0x2f3ab8u;
    case insert_keys_end_reload:return 0x2f3abcu;case insert_cursor_store_next:return 0x2f3ac8u;
    case sort_keys_size:return 0x2f3ad7u;case comparator_store:return 0x2f3ae1u;case comparator_read:return 0x2f3ae8u;
    case sort_keys_data:return 0x2f3aefu;case sort_keys_size_again:return 0x2f3afeu;case merge_manager_read:return 0x2f3b08u;
    case merge_entries_data:return 0x2f3b0bu;case merge_entries_size:return 0x2f3b0eu;case merge_keys_data:return 0x2f3b14u;
    case merge_manager_store:return 0x2f3b1bu;case merge_entries_end_store:return 0x2f3b22u;case merge_keys_end_store:return 0x2f3b2eu;
    case entries_capacity_read:return 0x2f3b37u;case merge_keys_size:return 0x2f3b3bu;case merge_entries_end_reload:return 0x2f3b6du;
    case entries_size_store:return 0x2f3b73u;case entries_data_read:return 0x2f3b77u;case merge_keys_end_read:return 0x2f3b81u;
    case merge_key:return 0x2f3b8bu;case merge_entry_key:return 0x2f3b8du;case merge_key_store:return 0x2f3b91u;
    case kept_word0:return 0x2f3b99u;case kept_word0_store:return 0x2f3b9du;case kept_word1:return 0x2f3b9fu;
    case kept_word1_store:return 0x2f3ba2u;case kept_word2:return 0x2f3ba5u;case kept_word2_store:return 0x2f3baeu;
    case merge_container:return 0x2f3bbfu;case merge_container_vtable:return 0x2f3bc3u;case merge_key_reload:return 0x2f3bcdu;
    case merge_child_shape_function:return 0x2f3bd2u;case merge_key_again:return 0x2f3bd5u;case merge_filter_input:return 0x2f3bd9u;
    case merge_child_key_store:return 0x2f3bdcu;case merge_filter:return 0x2f3be0u;case merge_child_shape_store:return 0x2f3be3u;
    case merge_filter_key:return 0x2f3be7u;case merge_filter_vtable:return 0x2f3be9u;case merge_filter_container:return 0x2f3becu;
    case merge_filter_body_a:return 0x2f3bf6u;case merge_filter_input_again:return 0x2f3bfau;case merge_filter_function:return 0x2f3c03u;
    case merge_enabled:return 0x2f3c05u;case merge_create_manager:return 0x2f3c0au;case merge_create_input:return 0x2f3c0eu;
    case merge_create_body_a:return 0x2f3c12u;case merge_create_function:return 0x2f3c1cu;
    // the created agent's and the null agent's paths store the entry with their own instructions
    case merge_agent_store:return null_path?0x2f3c36u:0x2f3c1fu;case merge_new_key:return null_path?0x2f3c39u:0x2f3c22u;
    case merge_new_key_store:return null_path?0x2f3c3bu:0x2f3c24u;
    case gone_agent:return 0x2f3c45u;case gone_vtable:return 0x2f3c4eu;case gone_function:return 0x2f3c50u;
    case merge_entries_end_next:return 0x2f3c56u;case rest_agent:return 0x2f3c68u;case rest_vtable:return 0x2f3c71u;
    case rest_function:return 0x2f3c73u;case rest_end:return 0x2f3c76u;
    case tail_keys_end_read:return 0x2f3c81u;case tail_key:return 0x2f3c87u;case tail_container:return 0x2f3c89u;
    case tail_container_vtable:return 0x2f3c8du;case tail_child_shape_function:return 0x2f3c98u;case tail_child_key_store:return 0x2f3c9bu;
    case tail_child_shape_store:return 0x2f3c9fu;case tail_filter_key:return 0x2f3ca3u;case tail_filter_input:return 0x2f3ca5u;
    case tail_filter:return 0x2f3ca8u;case tail_filter_vtable:return 0x2f3cabu;case tail_filter_container:return 0x2f3caeu;
    case tail_filter_body_a:return 0x2f3cb8u;case tail_filter_function:return 0x2f3cc2u;case tail_enabled:return 0x2f3cc4u;
    case tail_create_manager:return 0x2f3cc9u;case tail_create_input:return 0x2f3ccdu;case tail_create_function:return 0x2f3cdau;
    case tail_agent_store:return 0x2f3ce7u;case tail_new_key:return 0x2f3ceau;case tail_keys_end_next:return 0x2f3cecu;
    case tail_new_key_store:return 0x2f3cf0u;
    case install_agent:return 0x2f3cfcu;case install_capacity:return 0x2f3d00u;case install_size:return 0x2f3d03u;
    case install_old_data:return 0x2f3d16u;case install_memory:return 0x2f3d19u;case install_memory_vtable:return 0x2f3d1fu;
    case install_deallocate_function:return 0x2f3d2bu;case install_size_again:return 0x2f3d2eu;case install_memory_again:return 0x2f3d32u;
    case install_memory_vtable_again:return 0x2f3d38u;case install_allocate_function:return 0x2f3d43u;
    case install_size_reload:return 0x2f3d46u;case install_data_store:return 0x2f3d4au;case install_capacity_store:return 0x2f3d4du;
    case install_data:return 0x2f3d52u;case install_size_store:return 0x2f3d55u;case install_source:return 0x2f3d5au;
    case copy_word0:return 0x2f3d62u;case copy_word0_store:return 0x2f3d66u;case copy_word1:return 0x2f3d68u;
    case copy_word1_store:return 0x2f3d6bu;case copy_word2:return 0x2f3d6eu;case copy_word2_store:return 0x2f3d78u;
    case sorted_keys_data_reload:return 0x2f3d8eu;
    case hits_capacity_read:return 0x2f3d95u;case hits_memory:return 0x2f3dabu;case hits_memory_vtable:return 0x2f3db1u;
    case hits_deallocate_function:return 0x2f3dbfu;
    }
    return 0;
}
// 2F36F0 (validated by tools/validate_native_child_agents_tu15.py).
enum ChildAgentBranch : unsigned {
    u_cache_kept=aabb_query_branch_end,u_cache_moved,u_query_order,u_expected,u_found,u_removed,u_removed_shift,u_inserted,
    u_insert_grow,u_insert_move,u_created,u_null,u_sort,u_kept,u_merged_new,u_gone,u_tail,u_install_grow,u_keys_freed,
    child_agent_branch_end
};
static_assert(child_agent_branch_end<=coverage_bits);

template<class State,class Children> struct ChildAgentAbi {
    using Access=logic::child_agent_update::Access;
    Port<State> &port;Children &children;X87Mirror<State> x87{port};
    std::uint32_t t;  // ESP after the saves
    logic::child_agent_update::Locals local{};
    std::uint32_t pushed_b{},pushed_a{};
    // The arguments pushed for the next call, in push order: each call checks
    // them against the logic's.
    std::array<std::uint32_t,6> pushed{};unsigned pushes{};
    unsigned phase{};  // which site makes a new key's agent: 0 insertion, 1 merge, 2 the keys after the entries
    bool null_path{},gone_pending{},create_pending{};
    ChildAgentAbi(Port<State> &p,Children &c,std::uint32_t ebp,std::uint32_t base):port(p),children(c),t(base) {
        local={.body_a=ebp+8u,.body_b=ebp+0xcu,.input=ebp+0x10u,.create_agent=ebp+0x14u,.agent=t+0x28u,.half_tolerance=t+0x18u,
            .aabb_tolerance=t-8u,.slack=t+0x20u,.moved=t+0x24u,.aabb=t+0x40u,.container=t+0x60u,.child_body=t+0x64u,
            .child_buffer=t+0x80u,.hits=t+0x180u,.hit_storage=t+0x18cu,.entries=t+0x2cu,.a_in_b=t+0x390u,.enabled=t+0x1fu,
            .comparator=t+0x24u,.contact_manager=t+0x28u,.merge_contact_manager=t+0x20u,.entries_end=t+0x18u,.keys_end=t+0x20u,
            .key_position=t+0x14u,.merge_keys_end=t+0x14u,.shift_offset=t+0x10u,.insert_entries_end=t+0x10u,.insert_keys_end=t+0x24u,
            .new_size=t+0x18u,.move_count=t+0x20u,.move_cursor=t+0x10u,.move_step=t+0x20u,.merged_key=t+0x10u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    static bool is_x87(Access a) {
        using enum logic::child_agent_update::Access;
        return a==tolerance || a==half_factor || a==half_store || a==half_reload || a==tolerance_again || a==margin_store;
    }
    // MOV AL,[m8]; TEST AL,AL.
    void byte_flag(std::uint32_t value) {
        reg(eax)=(reg(eax)&0xffffff00u)|(value&0xffu);
        port.flags.logic(value&0xffu,1);
    }
    // ADD ESP,n after a cdecl child.
    void drop(std::uint32_t bytes){reg(esp)=port.flags.plus(reg(esp),bytes,4);}
    void genuine(std::uint32_t esp_after,auto &&call){genuine_child(port,esp_after,"2F36F0",call);}
    void argument(std::uint32_t pc,std::uint32_t value){port.push(pc,value);if(pushes<pushed.size())pushed[pushes++]=value;}
    void arguments(std::initializer_list<std::uint32_t> expected,const char *call) {
        const bool same=pushes==expected.size() && std::equal(expected.begin(),expected.end(),pushed.begin());
        pushes=0;
        if(!same) {
            char text[96];std::snprintf(text,sizeof text,"Native 2F36F0 %s arguments differ from the logic's",call);
            throw std::logic_error(text);
        }
    }

    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::child_agent_update::Access;
        auto &flags=port.flags;
        switch(a) {
        // body A's AABB
        case body_a_argument:port.push(0x2f3718u,reg(esi));port.push(0x2f3719u,reg(edi));break;
        case body_a_motion:reg(esi)=reg(ecx);break;  // MOV ESI,ECX
        case agent_store:  // ADD ECX,20; PUSH ECX; ADD EAX,20; PUSH EAX; LEA ECX,[ESP+398]
            reg(ecx)=flags.plus(reg(ecx),0x20u,4);pushed_a=reg(ecx);port.push(0x2f3725u,pushed_a);
            reg(eax)=flags.plus(reg(eax),0x20u,4);pushed_b=reg(eax);port.push(0x2f3729u,pushed_b);reg(ecx)=t+0x390u;
            break;
        case half_store:reg(edi)=t+0x40u;argument(0x2f374eu,reg(edi));break;  // LEA EDI,[ESP+40]; PUSH EDI
        case half_reload:port.push(0x2f3753u,reg(ecx));break;                   // PUSH ECX (the tolerance's slot)
        case margin_store:reg(eax)=t+0x390u;break;                              // LEA EAX,[ESP+398]
        case shape_aabb_function:argument(0x2f3765u,reg(eax));break;
        // the query
        case cache_minimum_store:reg(ecx)=t+0x18cu;reg(eax)=flags.logic(0u,4);break;  // LEA ECX,[ESP+18C]; XOR EAX,EAX
        case query_function:  // MOV [ESP+3DC],EAX (unwind state 0); LEA EAX,[ESP+180]; PUSH EAX; LEA ECX,[ESP+44]; PUSH ECX; MOV ECX,EDI
            port.store(0x2f3808u,t+0x3dcu,4,reg(eax));
            reg(eax)=t+0x180u;argument(0x2f3816u,reg(eax));reg(ecx)=t+0x40u;argument(0x2f381bu,reg(ecx));reg(ecx)=reg(edi);
            break;
        // the query-order matching: removal
        case manager_store:reg(esi)=flags.plus(reg(esi),0xcu,4);port.reached(u_query_order);break;
        case keys_size:reg(ecx)=reg(eax)*3u;break;
        case entries_end_store:  // LEA ECX,[EDI+ECX*4]; CMP EDI,ECX; LEA EAX,[EDX+EAX*4]
            reg(ecx)=reg(edi)+reg(ecx)*4u;flags.minus(reg(edi),reg(ecx),4);reg(eax)=reg(edx)+reg(eax)*4u;
            break;
        case keys_end_store:reg(ebx)=reg(edx);break;
        case entry_key:flags.minus(reg(ebx),reg(eax),4);break;  // CMP EBX,EAX
        case search_start_store:  // (CMP EBX,EAX;) CMP EDX,EAX; MOV EBX,EDX
            flags.minus(reg(ebx),reg(eax),4);flags.minus(reg(edx),reg(eax),4);reg(ebx)=reg(edx);
            break;
        case removal_size_store:reg(ecx)=flags.increment(reg(ecx),4,true);break;  // DEC ECX
        case removal_size_reload: {  // MOV ECX,EDI; SUB ECX,EAX; MOV EAX,2AAAAAAB; IMUL ECX (CF and OF only)
            reg(ecx)=flags.minus(reg(edi),reg(eax),4);
            const auto full=std::int64_t(std::int32_t(0x2aaaaaabu))*std::int32_t(reg(ecx));
            reg(eax)=std::uint32_t(full);reg(edx)=std::uint32_t(std::uint64_t(full)>>32u);
            flags.cf=flags.of=full!=std::int64_t(std::int32_t(reg(eax)));
            break;
        }
        case shift_offset_store:reg(ecx)=flags.shift(reg(eax)*3u,2u,4);port.reached(u_removed_shift);break;  // LEA; SHL ECX,2
        case shift_word0:reg(ecx)=flags.plus(reg(ecx),reg(edx),4);reg(edx)=reg(ecx)+0xcu;break;  // ADD ECX,EDX; LEA EDX,[ECX+C]
        case entries_end_store_again:reg(edi)=flags.minus(reg(edi),0xcu,4);reg(eax)=flags.minus(reg(eax),0xcu,4);break;
        // the query-order matching: insertion
        case insert_entries_end_store:  // LEA ECX,[EAX+EAX*2]; LEA ECX,[EDI+ECX*4]; LEA EBX,[EDX+EBX*4]; CMP EDX,EBX
            reg(ecx)=reg(edi)+reg(eax)*3u*4u;reg(ebx)=reg(edx)+reg(ebx)*4u;flags.minus(reg(edx),reg(ebx),4);
            break;
        case insert_cursor_store:reg(ecx)=reg(edx);break;
        case move_count_store:  // MOV EDI,ECX; SUB EDI,EDX; LEA EBX,[EAX+1]; SAR EDI,2; SUB EAX,EDI
            reg(edi)=flags.minus(reg(ecx),reg(edx),4);reg(ebx)=reg(eax)+1u;reg(edi)=flags.right(reg(edi),2u,4,true);
            reg(eax)=flags.minus(reg(eax),reg(edi),4);port.reached(u_inserted);
            break;
        case move_first_reload:reg(ebx)=reg(ecx)*3u;reg(edx)=reg(edx)+reg(ebx)*4u;break;  // LEA EBX,[ECX+ECX*2]; LEA EDX,[EDX+EBX*4]
        case move_step_store:reg(ecx)=flags.increment(reg(ecx),4,false);port.reached(u_insert_move);break;  // INC ECX
        case move_word0:reg(edi)=flags.plus(reg(edi),reg(edx),4);break;  // ADD EDI,EDX
        case move_word0_store:reg(ecx)=reg(edx);break;
        case container_vtable:  // LEA EDX,[ESP+80]; PUSH EDX; ADD EDI,EAX
            reg(edx)=t+0x80u;argument(0x2f3a3eu,reg(edx));reg(edi)=flags.plus(reg(edi),reg(eax),4);
            break;
        case child_shape_function:phase=0;argument(0x2f3a43u,reg(ebx));break;
        case filter_container:argument(0x2f3a5du,reg(ebx));break;
        case filter_body_a:argument(0x2f3a62u,reg(ebx));reg(ebx)=t+0x64u;argument(0x2f3a67u,reg(ebx));break;
        case filter_function:
            argument(0x2f3a6bu,reg(ebx));argument(0x2f3a6cu,reg(eax));reg(eax)=t+0x1fu;argument(0x2f3a71u,reg(eax));
            break;
        case create_function:
            argument(0x2f3a80u,reg(ecx));argument(0x2f3a81u,reg(edx));reg(eax)=t+0x64u;argument(0x2f3a86u,reg(eax));
            reg(ecx)=reg(ebx);argument(0x2f3a89u,reg(ecx));
            break;
        case insert_keys_data_reload:reg(ecx)=reg(edx)+reg(eax)*3u*4u;break;
        // the sorted merge
        case sort_keys_data:  // PUSH EDX; DEC EAX; PUSH EAX
            argument(0x2f3aecu,reg(edx));reg(eax)=flags.increment(reg(eax),4,true);argument(0x2f3aeeu,reg(eax));
            break;
        case merge_keys_data:reg(edx)=reg(esi)*3u;break;
        case merge_entries_end_store:reg(ecx)=reg(edi)+reg(edx)*4u;break;
        case merge_keys_end_store:reg(edx)=reg(esi)+reg(eax)*4u;argument(0x2f3b29u,reg(eax));reg(ecx)=t+0x2cu;break;
        case kept_word0:reg(ecx)=reg(edi);port.reached(u_kept);break;
        case kept_word0_store:reg(edx)=reg(ebx);break;
        case kept_word2_store:reg(ebx)=flags.plus(reg(ebx),0xcu,4);reg(edi)=flags.plus(reg(edi),0xcu,4);break;
        case merge_key_reload:reg(eax)=t+0x80u;argument(0x2f3bccu,reg(eax));port.reached(u_merged_new);break;
        case merge_child_shape_function:phase=1;argument(0x2f3bd1u,reg(eax));break;
        case merge_filter_container:argument(0x2f3bebu,reg(edx));break;
        case merge_filter_body_a:argument(0x2f3bf0u,reg(edx));reg(edx)=t+0x64u;argument(0x2f3bf5u,reg(edx));break;
        case merge_filter_input_again:argument(0x2f3bf9u,reg(edx));break;
        case merge_filter_function:argument(0x2f3bfdu,reg(edx));reg(edx)=t+0x1fu;argument(0x2f3c02u,reg(edx));break;
        case merge_create_body_a:argument(0x2f3c11u,reg(eax));break;
        case merge_create_function:
            argument(0x2f3c15u,reg(ecx));reg(edx)=t+0x64u;argument(0x2f3c1au,reg(edx));argument(0x2f3c1bu,reg(eax));
            break;
        case merge_entries_end_next:
            if(gone_pending){gone_pending=false;reg(edi)=flags.plus(reg(edi),0xcu,4);}
            break;
        case tail_child_shape_function:
            phase=2;reg(eax)=t+0x80u;argument(0x2f3c96u,reg(eax));argument(0x2f3c97u,reg(edi));
            port.reached(u_tail);
            break;
        case tail_filter_container:argument(0x2f3cadu,reg(edi));break;
        case tail_filter_body_a:argument(0x2f3cb2u,reg(edi));reg(edi)=t+0x64u;argument(0x2f3cb7u,reg(edi));break;
        case tail_filter_function:
            argument(0x2f3cbbu,reg(edi));argument(0x2f3cbcu,reg(eax));reg(eax)=t+0x1fu;argument(0x2f3cc1u,reg(eax));
            break;
        case tail_create_function:
            argument(0x2f3cd0u,reg(ecx));argument(0x2f3cd1u,reg(edx));reg(eax)=t+0x64u;argument(0x2f3cd6u,reg(eax));
            reg(ecx)=reg(edi);argument(0x2f3cd9u,reg(ecx));
            break;
        case install_deallocate_function:  // LEA EAX,[EAX+EAX*2]; PUSH 12; SHL EAX,2; PUSH EAX; PUSH EDX
            reg(eax)=reg(eax)*3u;argument(0x2f3d24u,0x12u);reg(eax)=flags.shift(reg(eax),2u,4);argument(0x2f3d29u,reg(eax));
            argument(0x2f3d2au,reg(edx));
            break;
        case install_allocate_function:  // LEA EAX,[EAX+EAX*2]; PUSH 12; SHL EAX,2; PUSH EAX
            reg(eax)=reg(eax)*3u;argument(0x2f3d3du,0x12u);reg(eax)=flags.shift(reg(eax),2u,4);argument(0x2f3d42u,reg(eax));
            break;
        case install_data:flags.logic(reg(edx),4);break;  // TEST EDX,EDX
        case copy_word0:reg(esi)=reg(eax);break;
        case copy_word0_store:reg(edi)=reg(ecx);break;
        case copy_word2_store:  // ADD EAX,C; ADD ECX,C; DEC EDX
            reg(eax)=flags.plus(reg(eax),0xcu,4);reg(ecx)=flags.plus(reg(ecx),0xcu,4);reg(edx)=flags.increment(reg(edx),4,true);
            break;
        // the keys' storage
        case hits_deallocate_function:  // AND EAX,7FFFFFFF; PUSH 12; SHL EAX,2; PUSH EAX; PUSH EDX
            reg(eax)=flags.logic(reg(eax)&0x7fffffffu,4);argument(0x2f3db8u,0x12u);reg(eax)=flags.shift(reg(eax),2u,4);
            argument(0x2f3dbdu,reg(eax));argument(0x2f3dbeu,reg(edx));port.reached(u_keys_freed);
            break;
        default:break;
        }
        const auto pc=child_agents_instruction(a,null_path);
        if(is_x87(a))x87.at(pc);
        if(a==half_store || a==margin_store)x87.store_check();
        return pc;
    }
    // ---- Schedule: after each load (the register each load writes, then
    // the register arithmetic up to the next access)
    void loaded(Access a,std::uint32_t value) {
        using enum logic::child_agent_update::Access;
        auto &flags=port.flags;
        switch(a) {
        case body_b_motion:case input_argument:case manager_read:case entries_size:case keys_size:case removal_data:
        case entries_end_reload:case keys_end_reload:case insert_entries_size:case container_vtable:case filter_input:case new_key:
        case insert_size_reload:case sort_keys_size:case sort_keys_data:case sort_keys_size_again:case entries_capacity_read:
        case merge_entries_end_reload:case merge_key:case kept_word0:case kept_word1:case merge_key_reload:case merge_filter_vtable:
        case merge_create_manager:case merge_create_body_a:case gone_agent:case rest_agent:case rest_vtable:case rest_end:
        case merge_entries_end_next:case tail_filter_input:case tail_keys_end_next:case install_size_again:case install_source:
        case hits_capacity_read:
            reg(eax)=value;break;
        case body_a_motion:case body_a_shape:case slack_read:case entry_key:case search_entry_key:case entry_agent:case removal_size:
        case removal_size_reload:case shift_offset_reload:case entries_end_next:case move_count_reload:case move_left_reload:
        case container_reload:case filter_read:case create_manager:case insert_cursor_next:case merge_manager_read:case merge_keys_size:
        case merge_entry_key:case kept_word2:case merge_container:case merge_key_again:case merge_filter:case merge_create_input:
        case merge_new_key:case tail_container:case tail_filter:case tail_create_manager:case install_capacity:case install_memory:
        case install_memory_again:case install_data:case hits_memory:
            reg(ecx)=value;break;
        case shape_a_vtable:case tree_vtable:case body_b_motion_again:case keys_data:case child_vtable:case shift_data:case shift_word2:
        case shift_size:case keys_data_reload:case insert_data:case insert_cursor_reload:case filter_vtable:case create_input:
        case insert_cursor_final:case insert_data_reload:case insert_keys_data_reload:case comparator_read:
        case merge_container_vtable:case merge_filter_input:case merge_filter_key:case merge_filter_container:case merge_filter_body_a:
        case merge_filter_input_again:case gone_vtable:case tail_container_vtable:case tail_filter_vtable:case tail_create_input:
        case tail_new_key:case install_size:case install_old_data:case install_memory_vtable_again:case install_size_reload:
        case sorted_keys_data_reload:
            reg(edx)=value;break;
        case body_b_argument:case shift_word0:case shift_word1:case position_reload:case insert_keys_size_again:case insert_entry_key:
        case move_word0:case move_word1:case new_size_reload:case insert_key_read:case insert_cursor_again:case filter_key:
        case filter_container:case filter_body_a:case insert_keys_end_reload:case entries_data_read:case copy_word0:case copy_word1:
            reg(ebx)=value;break;
        case merge_entries_size:case merge_keys_data:case install_agent:case hits_memory_vtable:case copy_word2:
            reg(esi)=value;break;
        case body_a_argument:case tree_read:case container_read:case entries_data:case insert_entries_data:case move_step_reload:
        case move_word2:case insert_data_again:case merge_entries_data:case tail_key:case tail_filter_key:case tail_filter_container:
        case tail_filter_body_a:case install_memory_vtable:
            reg(edi)=value;break;
        default:break;
        }
        switch(a) {
        case tolerance:case half_reload:x87.load(value);break;
        case half_factor:x87.multiply(value);break;
        case tolerance_again:x87.add(value);break;
        case cache_enabled:case query_order_read:byte_flag(value);break;
        case slack_reload:port.set_xmm(0,scalar(std::bit_cast<float>(value)));break;
        case expected_key:  // CMP ECX,[EBX]; (ADD EBX,4)
            flags.minus(reg(ecx),value,4);
            if(reg(ecx)==value){reg(ebx)=flags.plus(reg(ebx),4u,4);port.reached(u_expected);}
            break;
        case search_key:  // CMP ECX,[EBX]; (ADD EBX,4; CMP EBX,EAX)
            flags.minus(reg(ecx),value,4);
            if(reg(ecx)!=value){reg(ebx)=flags.plus(reg(ebx),4u,4);flags.minus(reg(ebx),reg(eax),4);}
            break;
        case entry_agent:port.reached(u_removed);break;
        case removal_size_reload:  // SAR EDX,1; MOV EAX,EDX; SHR EAX,1F; ADD EAX,EDX; CMP EAX,ECX
            reg(edx)=flags.right(reg(edx),1u,4,true);reg(eax)=flags.right(reg(edx),0x1fu,4,false);
            reg(eax)=flags.plus(reg(eax),reg(edx),4);flags.minus(reg(eax),value,4);
            break;
        case shift_size:  // INC EAX; ADD ECX,C; CMP EAX,EDX
            reg(eax)=flags.increment(reg(eax),4,false);reg(ecx)=flags.plus(reg(ecx),0xcu,4);flags.minus(reg(eax),value,4);
            break;
        case entries_end_next:reg(edi)=flags.plus(reg(edi),0xcu,4);flags.minus(reg(edi),value,4);break;  // ADD EDI,C; CMP EDI,ECX
        case insert_keys_size:flags.minus(value,reg(eax),4);break;  // CMP [ESP+184],EAX
        case insert_end_read:flags.minus(reg(edi),value,4);break;   // CMP EDI,[ESP+10]
        case insert_key:flags.minus(reg(ebx),value,4);break;        // CMP EBX,[ECX]
        case insert_capacity:reg(eax)=flags.logic(value&0x7fffffffu,4);flags.minus(reg(eax),reg(ebx),4);break;
        case move_count_reload:  // LEA EAX,[EDI+EDI*2]; SHL EAX,2; DEC ECX; LEA EDI,[EAX+EDX]; LEA EDX,[EDI+C]
            reg(eax)=flags.shift(reg(edi)*3u,2u,4);reg(ecx)=flags.increment(reg(ecx),4,true);
            reg(edi)=reg(eax)+reg(edx);reg(edx)=reg(edi)+0xcu;
            break;
        case move_first_reload:reg(edi)=flags.minus(reg(edi),value,4);break;  // SUB EDI,[ESP+10]
        case move_left_reload:reg(edx)=flags.minus(reg(edx),0xcu,4);reg(ecx)=flags.increment(reg(ecx),4,true);break;
        case enabled_read:case merge_enabled:case tail_enabled:  // CMP BYTE PTR [EAX],0
            flags.minus(value&0xffu,0u,1);
            null_path=(value&0xffu)==0u;
            port.reached(null_path?u_null:u_created);
            break;
        case insert_keys_end_reload:  // ADD ECX,4; ADD EDI,C; CMP ECX,EBX
            reg(ecx)=flags.plus(reg(ecx),4u,4);reg(edi)=flags.plus(reg(edi),0xcu,4);flags.minus(reg(ecx),value,4);
            break;
        case sort_keys_size:flags.minus(value,1u,4);break;  // CMP EAX,1
        case sort_keys_size_again:drop(0x10u);break;        // ADD ESP,10
        case merge_keys_size:  // AND EAX,7FFFFFFF; CMP EAX,ECX; MOV BYTE PTR [ESP+3DC],1; MOV EBX,ECX
            reg(eax)=flags.logic(reg(eax)&0x7fffffffu,4);flags.minus(reg(eax),value,4);
            port.store(0x2f3b49u,t+0x3dcu,1,1u);  // the unwind state: the merged entries live
            reg(ebx)=value;
            break;
        case merge_entries_end_reload:case merge_entries_end_next:flags.minus(reg(edi),value,4);break;  // CMP EDI,EAX
        case merge_keys_end_read:  // CMP ESI,[ESP+14]; (CMP EDI,EAX before the rest)
            flags.minus(reg(esi),value,4);
            if(reg(esi)==value)flags.minus(reg(edi),reg(eax),4);
            break;
        case merge_entry_key:flags.minus(reg(eax),value,4);break;  // CMP EAX,ECX
        case gone_agent:case rest_agent:  // TEST EAX,EAX; (MOV ECX,EAX)
            flags.logic(value,4);
            if(value){reg(ecx)=value;port.reached(u_gone);}
            if(a==gone_agent)gone_pending=true;
            break;
        case rest_end:reg(edi)=flags.plus(reg(edi),0xcu,4);flags.minus(reg(edi),value,4);break;  // ADD EDI,C; CMP EDI,EAX
        case tail_keys_end_read:flags.minus(reg(esi),value,4);break;  // CMP ESI,[ESP+14]
        case install_size:  // MOV EAX,ECX; AND EAX,7FFFFFFF; CMP EAX,EDX; (TEST ECX,ECX)
            reg(eax)=flags.logic(reg(ecx)&0x7fffffffu,4);flags.minus(reg(eax),value,4);
            if(std::int32_t(reg(eax))<std::int32_t(value)){flags.logic(reg(ecx),4);port.reached(u_install_grow);}
            break;
        case hits_capacity_read:  // TEST EAX,EAX; MOV [ESP+3DC],-1
            flags.logic(value,4);
            port.store(0x2f3d9eu,t+0x3dcu,4,0xffffffffu);  // the unwind state: no local lives
            break;
        default:break;
        }
    }
    void loaded_vector(Access a,__m128 value) {
        using enum logic::child_agent_update::Access;
        switch(a) {
        case aabb_minimum:port.set_xmm(4,value);break;
        case aabb_maximum:port.set_xmm(5,value);break;
        case cached_maximum:case cached_minimum:port.set_xmm(1,value);break;
        case absolute_mask_read:port.set_xmm(3,value);break;
        default:break;
        }
    }
    // ---- Schedule: after each store
    void stored(Access a) {
        using enum logic::child_agent_update::Access;
        auto &flags=port.flags;
        switch(a) {
        case half_store:case margin_store:x87.pop();break;
        case moved_store:if(flags.zf)port.reached(u_cache_moved);break;  // TEST AL,AL: AL is 0 when a lane moved
        case moved_store_again: {  // SETZ AL; TEST AL,AL (ZF of AND EAX,7)
            const bool kept=flags.zf;
            reg(eax)=(reg(eax)&0xffffff00u)|(kept?1u:0u);flags.logic(reg(eax)&0xffu,1);
            port.reached(kept?u_cache_kept:u_cache_moved);
            break;
        }
        case found_store:reg(ebx)=flags.plus(reg(ebx),4u,4);port.reached(u_found);break;  // ADD EBX,4
        case kept_word2_store:reg(esi)=flags.plus(reg(esi),4u,4);break;
        case merge_new_key_store:  // (ADD ESP,10;) ADD EBX,C; ADD ESI,4
            if(create_pending){create_pending=false;drop(0x10u);}
            reg(ebx)=flags.plus(reg(ebx),0xcu,4);reg(esi)=flags.plus(reg(esi),4u,4);
            break;
        case tail_new_key_store:  // ADD ESI,4; ADD EBX,C; CMP ESI,EAX
            reg(esi)=flags.plus(reg(esi),4u,4);reg(ebx)=flags.plus(reg(ebx),0xcu,4);flags.minus(reg(esi),reg(eax),4);
            break;
        default:break;
        }
    }

    // ---- Frame
    const logic::child_agent_update::Locals &locals()const{return local;}
    // 2E4550 inline (agent_store pushed its arguments: body A's, then body B's).
    void relative_transform(std::uint32_t out,std::uint32_t b,std::uint32_t a) {
        if(reg(ecx)!=out || pushed_b!=b || pushed_a!=a || reg(esp)!=t-8u)
            throw std::logic_error("Native 2F36F0 relative transform arguments differ from the pushed ones");
        port.push(0x2f3735u,0x2f373au);
        kernel_2e4550(port);
        if(reg(esp)!=t)throw std::logic_error("Native 2F36F0 frame is unbalanced after 2E4550");
    }
    // shape->getAabb(transform, tolerance, aabb): thiscall, RET C.
    void shape_aabb(std::uint32_t function,std::uint32_t shape,std::uint32_t transform,std::uint32_t aabb) {
        arguments({aabb,transform},"getAabb");  // the tolerance's slot is stored by the logic
        if(reg(ecx)!=shape || reg(esp)!=t-0xcu)throw std::logic_error("Native 2F36F0 getAabb arguments differ from the registers");
        port.push(0x2f3766u,0x2f3769u);
        genuine(t,[&]{call_dispatch(port,function,child_agents_site::shape_aabb,children.dispatch);});
    }
    // MOVMSKPS; AND reg,7 (then, for the maximum corner, SETZ AL; TEST AL,AL).
    void faces_compared(const logic::child_agent_update::FaceTest &test,bool minimum) {
        auto &flags=port.flags;
        port.set_xmm(1,test.distance);
        if(!minimum) {
            port.set_xmm(0,test.slack);port.set_xmm(2,test.beyond);
            reg(edx)=flags.logic(std::uint32_t(_mm_movemask_ps(test.beyond))&7u,4);
            reg(eax)=(reg(eax)&0xffffff00u)|(flags.zf?1u:0u);flags.logic(reg(eax)&0xffu,1);
        } else {
            port.set_xmm(0,test.beyond);
            reg(eax)=flags.logic(std::uint32_t(_mm_movemask_ps(test.beyond))&7u,4);
        }
    }
    // tree->queryAabb(aabb, hits): thiscall, RET 8.
    void query_tree(std::uint32_t function,std::uint32_t tree,std::uint32_t aabb,std::uint32_t hits) {
        arguments({hits,aabb},"queryAabb");
        if(reg(ecx)!=tree || reg(esp)!=t-8u)throw std::logic_error("Native 2F36F0 query arguments differ from the registers");
        port.push(0x2f381eu,0x2f3821u);
        genuine(t,[&]{call_dispatch(port,function,child_agents_site::query_tree,children.dispatch);});
    }
    // child->cleanup(): no argument.
    void cleanup(std::uint32_t function,std::uint32_t child) {
        if(reg(ecx)!=child)throw std::logic_error("Native 2F36F0 clean-up target differs from ECX");
        port.push(0x2f38b8u,0x2f38bbu);
        genuine(t,[&]{call_dispatch(port,function,child_agents_site::cleanup,children.dispatch);});
    }
    void cleanup_gone(std::uint32_t function,std::uint32_t child,bool rest) {
        if(reg(ecx)!=child)throw std::logic_error("Native 2F36F0 clean-up target differs from ECX");
        port.push(rest?0x2f3c73u:0x2f3c50u,rest?0x2f3c76u:0x2f3c53u);
        genuine(t,[&]{call_dispatch(port,function,rest?child_agents_site::rest:child_agents_site::gone,children.dispatch);});
    }
    // 2E31E0(array, capacity, 12): cdecl. The capacity is chosen in EAX.
    void reserve(std::uint32_t array,std::uint32_t capacity) {
        auto &flags=port.flags;
        const bool merged=array==local.entries;
        const auto needed=reg(merged?ecx:ebx);
        reg(eax)=flags.plus(reg(eax),reg(eax),4);flags.minus(needed,reg(eax),4);  // ADD EAX,EAX; CMP needed,EAX
        if(!(std::int32_t(needed)<std::int32_t(reg(eax))))reg(eax)=needed;
        if(reg(eax)!=capacity)throw std::logic_error("Native 2F36F0 reserve capacity differs from EAX");
        if(merged) {  // PUSH C; PUSH EAX; LEA EAX,[ESP+34]; PUSH EAX
            port.push(0x2f3b5du,0xcu);port.push(0x2f3b5fu,reg(eax));reg(eax)=t+0x2cu;port.push(0x2f3b64u,reg(eax));
            port.push(0x2f3b65u,0x2f3b6au);
        } else {  // PUSH C; PUSH EAX; PUSH ESI
            if(reg(esi)!=array)throw std::logic_error("Native 2F36F0 reserve array differs from ESI");
            port.push(0x2f39b8u,0xcu);port.push(0x2f39bau,reg(eax));port.push(0x2f39bbu,reg(esi));port.push(0x2f39bcu,0x2f39c1u);
            port.reached(u_insert_grow);
        }
        genuine(t-0xcu,[&]{port.call_child(0x2e31e0u,children.reserve);});
        drop(0xcu);
    }
    // container->getChildShape(key, buffer): thiscall, RET 8.
    std::uint32_t child_shape(std::uint32_t function,std::uint32_t container,std::uint32_t key,std::uint32_t buffer) {
        arguments({buffer,key},"getChildShape");
        if(reg(ecx)!=container || reg(esp)!=t-8u)
            throw std::logic_error("Native 2F36F0 child shape arguments differ from the registers");
        static constexpr std::uint32_t call[3]{0x2f3a44u,0x2f3bd2u,0x2f3c98u},next[3]{0x2f3a47u,0x2f3bd5u,0x2f3c9bu},
            site[3]{child_agents_site::child_shape,child_agents_site::merge_child_shape,child_agents_site::tail_child_shape};
        port.push(call[phase],next[phase]);
        genuine(t,[&]{call_dispatch(port,function,site[phase],children.dispatch);});
        return reg(eax);
    }
    // filter->isCollisionEnabled(result, input, body A, child body, container, key): thiscall, RET 18; EAX = the result.
    std::uint32_t collision_enabled(std::uint32_t function,std::uint32_t filter,std::uint32_t result,std::uint32_t input,
        std::uint32_t body_a,std::uint32_t child_body,std::uint32_t container,std::uint32_t key) {
        arguments({key,container,child_body,body_a,input,result},"isCollisionEnabled");
        if(reg(ecx)!=filter || reg(esp)!=t-0x18u)throw std::logic_error("Native 2F36F0 filter arguments differ from the registers");
        static constexpr std::uint32_t call[3]{0x2f3a72u,0x2f3c03u,0x2f3cc2u},next[3]{0x2f3a74u,0x2f3c05u,0x2f3cc4u},
            site[3]{child_agents_site::enabled,child_agents_site::merge_enabled,child_agents_site::tail_enabled};
        port.push(call[phase],next[phase]);
        genuine(t,[&]{call_dispatch(port,function,site[phase],children.dispatch);});
        return reg(eax);
    }
    // create(body A, child body, input, contact manager): cdecl.
    std::uint32_t create_agent(std::uint32_t function,std::uint32_t body_a,std::uint32_t child_body,std::uint32_t input,
        std::uint32_t manager) {
        arguments({manager,input,child_body,body_a},"creation");
        if(reg(esp)!=t-0x10u)throw std::logic_error("Native 2F36F0 creation arguments are not pushed");
        static constexpr std::uint32_t call[3]{0x2f3a8au,0x2f3c1cu,0x2f3cdau},next[3]{0x2f3a8du,0x2f3c1fu,0x2f3cddu},
            site[3]{child_agents_site::create,child_agents_site::merge_create,child_agents_site::tail_create};
        port.push(call[phase],next[phase]);
        genuine(t-0x10u,[&]{call_dispatch(port,function,site[phase],children.dispatch);});
        if(phase==1)create_pending=true;  // the merge drops the arguments after it stores the entry
        else drop(0x10u);
        return reg(eax);
    }
    // 30BAD0, hkpNullAgent::getNullAgent(), inline: CALL; MOV EAX,48CDC4; RET.
    std::uint32_t null_agent() {
        static constexpr std::uint32_t call[3]{0x2f3a92u,0x2f3c31u,0x2f3ce2u},next[3]{0x2f3a97u,0x2f3c36u,0x2f3ce7u};
        port.push(call[phase],next[phase]);
        reg(eax)=logic::child_agent_update::globals::null_agent;
        port.ret(0x30bad5u,0u);
        return reg(eax);
    }
    // 2F33B0(keys, 0, count - 1, comparator): cdecl (sort_keys_data pushed the last two).
    void sort_keys(std::uint32_t keys,std::uint32_t low,std::uint32_t high,std::uint32_t comparator) {
        if(reg(esp)!=t-8u)throw std::logic_error("Native 2F36F0 sort arguments differ from the registers");
        argument(0x2f3af6u,0u);argument(0x2f3af8u,reg(eax));
        arguments({comparator,high,low,keys},"sort");
        port.push(0x2f3af9u,0x2f3afeu);
        port.reached(u_sort);
        genuine(t-0x10u,[&]{port.call_child(0x2f33b0u,children.sort);});
    }
    // 2F3430, hkLocalArray<ChildEntry>(count): thiscall, RET 4.
    void construct_entries(std::uint32_t array,std::uint32_t count) {
        arguments({count},"hkLocalArray");
        if(reg(ecx)!=array || reg(esp)!=t-4u)throw std::logic_error("Native 2F36F0 local array arguments differ from the registers");
        port.push(0x2f3b32u,0x2f3b37u);
        genuine(t,[&]{port.call_child(0x2f3430u,children.construct);});
    }
    // 2F34D0, ~hkLocalArray: LEA ECX,[ESP+2C]; MOV BYTE PTR [ESP+3DC],0 (the unwind state); CALL.
    void destroy_entries(std::uint32_t array) {
        reg(ecx)=array;
        port.store(0x2f3d81u,t+0x3dcu,1,0u);
        port.push(0x2f3d89u,0x2f3d8eu);
        genuine(t,[&]{port.call_child(0x2f34d0u,children.destroy);});
    }
    // memory->deallocate(data, bytes, class): thiscall, RET C.
    void deallocate(std::uint32_t function,std::uint32_t memory,std::uint32_t data,std::uint32_t bytes,std::uint32_t type,bool keys) {
        arguments({type,bytes,data},"deallocate");
        if(reg(ecx)!=memory || reg(esp)!=t-0xcu)throw std::logic_error("Native 2F36F0 deallocate arguments differ from the registers");
        port.push(keys?0x2f3dbfu:0x2f3d2bu,keys?0x2f3dc2u:0x2f3d2eu);
        genuine(t,[&]{call_dispatch(port,function,keys?child_agents_site::keys_deallocate:child_agents_site::install_deallocate,
            children.dispatch);});
    }
    // memory->allocate(bytes, class): thiscall, RET 8.
    std::uint32_t allocate(std::uint32_t function,std::uint32_t memory,std::uint32_t bytes,std::uint32_t type) {
        arguments({type,bytes},"allocate");
        if(reg(ecx)!=memory || reg(esp)!=t-8u)throw std::logic_error("Native 2F36F0 allocate arguments differ from the registers");
        port.push(0x2f3d43u,0x2f3d46u);
        genuine(t,[&]{call_dispatch(port,function,child_agents_site::install_allocate,children.dispatch);});
        return reg(eax);
    }
};
template<class State,class Children> void kernel_2f36f0(Port<State> &port,Children &children) {
    auto &gp=port.gp;
    const auto entry=gp[esp],self=gp[ecx];
    port.push(0x2f36f0u,gp[ebp]);gp[ebp]=gp[esp];
    gp[esp]=port.flags.logic(gp[ebp]&0xfffffff0u,4);  // AND ESP,-16 (ESP is EBP here)
    open_exception_frame(port,0x2f36f6u,child_agents_handler);
    gp[esp]=port.flags.minus(gp[esp],0x3c8u,4);
    port.push(0x2f3711u,gp[ebx]);
    ChildAgentAbi<State,Children> abi(port,children,gp[ebp],gp[esp]-8u);  // ESI and EDI follow
    Memory<Port<State>,ChildAgentAbi<State,Children>> memory{port,abi};
    logic::child_agent_update::update(memory,abi,self);
    if(gp[esp]!=abi.t)throw std::logic_error("Native 2F36F0 frame is unbalanced");
    // The exit: the previous exception frame, the saved registers, RET 10.
    gp[ecx]=port.read(0x2f3dc2u,abi.t+0x3d4u,4);
    gp[edi]=port.pop(0x2f3dc9u);gp[esi]=port.pop(0x2f3dcau);
    port.store(0x2f3dcbu,port.s.fs_base,4,gp[ecx]);
    gp[ebx]=port.pop(0x2f3dd2u);
    gp[esp]=gp[ebp];gp[ebp]=port.pop(0x2f3dd5u);
    if(gp[esp]!=entry)throw std::logic_error("Native 2F36F0 frame is unbalanced at its exit");
    port.ret(0x2f3dd6u,0x10u);port.publish(0x2f3dd6u);
}

// ---- E8D80 object flag update (the vehicle update unit) ---------------------
// stdcall: [ESP+4] the object's handle; RET 4. PUSH ECX (its slot's top byte
// is the result), then EBX, EBP, ESI and EDI saved between the first reads.
// T, the ESP after them: +13 the result byte, +18 the handle. The unit
// publishes its canonical instructions (E7E60..E7EE6), so this adapter does.
// Registers: EBX = the flag block, EBP = the flag's index, ESI = its word's
// offset, EDI = its bit (inverted before a clear). Each update function is
// a genuine child through the unit's dispatch (canonical site E7EBE;
// stdcall: the handle and the index, RET 8; AL back). The exit flags are the
// last CMP EBP,3C; AL the result.
inline constexpr std::uint32_t flag_update_site=0xe7ebeu,flag_update_return=0xe8de0u;
constexpr std::uint32_t flag_update_instruction(logic::object_flag_update::Access a) {
    using enum logic::object_flag_update::Access;
    switch(a) {
    case handle_argument:return 0xe7e61u;case headers_read:return 0xe7e65u;case headers_data:return 0xe7e6bu;
    case object_read:return 0xe7e77u;case block_offset:return 0xe7e7bu;case result_clear:return 0xe7e87u;
    case flag_word:return 0xe7ea4u;case definition:return 0xe7eaau;case function:return 0xe7eb1u;case handle_reload:return 0xe7eb8u;
    case result_set:return 0xe7ec4u;case flag_word_again:return 0xe7ecbu;case flag_clear:return 0xe7ed3u;case result_read:return 0xe7eddu;
    }
    return 0;
}
// E8D80 (validated by tools/validate_native_object_flag_update_tu15.py).
enum FlagUpdateBranch : unsigned {f_flag_clear=child_agent_branch_end,f_no_update,f_kept,f_cleared,flag_update_branch_end};
static_assert(flag_update_branch_end<=coverage_bits);

template<class State,class Dispatch> struct FlagUpdateAbi {
    using Access=logic::object_flag_update::Access;
    Port<State> &port;Dispatch &dispatch;
    std::uint32_t t;  // ESP after the saves
    logic::object_flag_update::Locals local{};
    bool counting{};  // a flag's iteration ended: INC EBP; CMP EBP,3C come first
    FlagUpdateAbi(Port<State> &p,Dispatch &d,std::uint32_t entry):port(p),dispatch(d),t(entry-0x14u) {
        local={.handle=entry+4u,.result=entry-1u};
    }
    std::uint32_t &reg(unsigned r){return port.gp[r];}
    void next_flag() {
        if(counting){reg(ebp)=port.flags.increment(reg(ebp),4,false);port.flags.minus(reg(ebp),logic::object_flag_update::flag_count,4);}
        counting=true;
    }
    // ---- Schedule: before each access
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum logic::object_flag_update::Access;
        auto &flags=port.flags;
        switch(a) {
        case object_read:  // AND EAX,FFFF; PUSH EBX; LEA EAX,[EAX+EAX*2]
            reg(eax)=flags.logic(reg(eax)&logic::object_flag_update::handle_index,4);port.push(0xe7e73u,reg(ebx));reg(eax)*=3u;
            break;
        case result_clear:  // PUSH EBP; PUSH ESI; ADD EBX,EAX; PUSH EDI
            port.push(0xe7e82u,reg(ebp));port.push(0xe7e83u,reg(esi));reg(ebx)=flags.plus(reg(ebx),reg(eax),4);
            port.push(0xe7e86u,reg(edi));
            break;
        case flag_word:  // MOV ECX,EBP; AND ECX,1F; MOV ESI,EBP; MOV EDI,1; SAR ESI,5; SHL EDI,CL; SHL ESI,2
            next_flag();
            reg(ecx)=flags.logic(reg(ebp)&0x1fu,4);reg(esi)=reg(ebp);reg(edi)=1u;reg(esi)=flags.right(reg(esi),5u,4,true);
            reg(edi)=flags.shift(reg(edi),reg(ecx)&0xffu,4);reg(esi)=flags.shift(reg(esi),2u,4);
            break;
        case result_read:next_flag();break;
        default:break;
        }
        return flag_update_instruction(a);
    }
    // ---- Schedule: after each load
    void loaded(Access a,std::uint32_t value) {
        using enum logic::object_flag_update::Access;
        auto &flags=port.flags;
        switch(a) {
        case handle_argument:case definition:reg(eax)=value;break;
        case headers_read:reg(ecx)=value;break;
        case headers_data:reg(edx)=value;break;
        case object_read:reg(eax)=value;break;
        case block_offset:reg(ebx)=std::uint32_t(std::int32_t(std::int16_t(value)));break;
        case flag_word:  // TEST [ESI+EBX+4],EDI
            flags.logic(value&reg(edi),4);
            if(!(value&reg(edi)))port.reached(f_flag_clear);
            break;
        case function:  // TEST EAX,EAX
            reg(eax)=flags.logic(value,4);
            if(!value)port.reached(f_no_update);
            break;
        case handle_reload:reg(ecx)=value;break;
        case flag_word_again:  // NOT EDI; AND EAX,EDI
            reg(edi)=~reg(edi);reg(eax)=flags.logic(value&reg(edi),4);port.reached(f_cleared);
            break;
        case result_read:reg(eax)=(reg(eax)&0xffffff00u)|(value&0xffu);break;  // MOV AL,[ESP+13]
        default:break;
        }
    }
    void stored(Access a) {
        using enum logic::object_flag_update::Access;
        if(a==result_clear)reg(ebp)=port.flags.logic(0u,4);  // XOR EBP,EBP
        if(a==result_set)port.reached(f_kept);
    }
    // ---- Frame
    const logic::object_flag_update::Locals &locals()const{return local;}
    // update(handle, flag): PUSH EBP; PUSH ECX; CALL EAX; TEST AL,AL.
    bool keep_flag(std::uint32_t function,std::uint32_t handle,std::uint32_t flag) {
        if(reg(eax)!=function || reg(ecx)!=handle || reg(ebp)!=flag || reg(esp)!=t)
            throw std::logic_error("Native E8D80 update arguments differ from the registers");
        port.push(0xe7ebcu,reg(ebp));port.push(0xe7ebdu,reg(ecx));port.push(0xe7ebeu,flag_update_return);
        genuine_child(port,t,"E8D80",[&]{call_dispatch(port,function,flag_update_site,dispatch);});
        port.flags.logic(reg(eax)&0xffu,1);
        return (reg(eax)&0xffu)!=0u;
    }
};
template<class State,class Dispatch> void kernel_e8d80(Port<State> &port,Dispatch &dispatch) {
    auto &gp=port.gp;
    const auto entry=gp[esp];
    port.push(0xe7e60u,gp[ecx]);
    FlagUpdateAbi<State,Dispatch> abi(port,dispatch,entry);
    Memory<Port<State>,FlagUpdateAbi<State,Dispatch>> memory{port,abi};
    logic::object_flag_update::update(memory,abi);
    if(gp[esp]!=abi.t)throw std::logic_error("Native E8D80 frame is unbalanced");
    gp[edi]=port.pop(0xe7ee1u);gp[esi]=port.pop(0xe7ee2u);gp[ebp]=port.pop(0xe7ee3u);gp[ebx]=port.pop(0xe7ee4u);gp[ecx]=port.pop(0xe7ee5u);
    port.ret(0xe7ee6u,4u);port.publish(0xe7ee6u);
}

// ---- Admission, dispatch and verification ----------------------------------
// Serves the translated reference's reads from the native record (same order,
// address and width) and checks its stores (address, width, bytes). Its child
// calls are routed to replay_child().
template<class State> struct Replay : SourcePhysicsContactMemory {
    const Trace<State> &trace;const std::vector<Event> &events;mutable std::size_t at{};std::size_t call{};
    mutable std::uint32_t expected_pc{};mutable bool store_pending{};
    explicit Replay(const Trace<State> &t):trace(t),events(t.events){}
    [[noreturn]] void fail(const char *what,std::uint32_t address,std::size_t width)const {
        char text[200];
        const auto *e=at<events.size()?&events[at]:nullptr;
        std::snprintf(text,sizeof text,"Native physics world leaf differs from the translated body at access %zu of %zu: %s %x/%zu, native %s pc=%x %x/%u",
            at,events.size(),what,address,width,e?(e->kind?"store":"read"):"end",e?e->pc:0u,e?e->address:0u,e?e->width:0u);
        throw std::runtime_error(text);
    }
    void serve(std::uint32_t address,std::span<std::byte> output)const {
        if(at<events.size()) {
            const auto &e=events[at];
            if(e.address==address && e.width==output.size()) {
                // A store preimage read (observer present) precedes its write.
                if(e.kind)std::memset(output.data(),0,output.size());
                else {std::memcpy(output.data(),e.bytes.data(),output.size());++at;}
                return;
            }
        }
        fail("read",address,output.size());
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {serve(address,output);}
    void read_vector(std::uint32_t address,std::span<std::byte,16> output)const override {serve(address,output);}
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        if(at>=events.size())fail("store",address,input.size());
        const auto &e=events[at];
        if(!e.kind || e.address!=address || e.width!=input.size() || std::memcmp(e.bytes.data(),input.data(),input.size()))
            fail("store",address,input.size());
        expected_pc=e.pc;store_pending=true;++at;
    }
};
template<class State> inline thread_local Replay<State> *active_replay=nullptr;
// A child call of the translated reference: it must have the native call's
// target, position and entry registers; the native call's results are applied.
template<class State> void replay_child(State &s,std::uint32_t target) {
    auto *replay=active_replay<State>;
    if(!replay)throw std::runtime_error("Physics world leaf reference child outside verification");
    if(replay->call>=replay->trace.calls.size())throw std::runtime_error("Physics world leaf reference made an extra child call");
    const auto &c=replay->trace.calls[replay->call];
    if(c.target!=target || c.at!=replay->at || !c.entry.same(Registers<State>::capture(s))) {
        char text[160];
        std::snprintf(text,sizeof text,"Native physics world leaf child %zu differs: native %x at %zu, translated %x at %zu",
            replay->call,c.target,c.at,target,replay->at);
        throw std::runtime_error(text);
    }
    c.exit.apply(s);++replay->call;
}
struct VerifySummary {
    // 1F5FE0, 2E4550, 303BA0, 2E47B0, 303EE0, 303C70, 303D00, 304350, 30CF40, 304130, 2F33B0, 1F76A0,
    // 2F8630, 2E4FE0, 1F7C10, 1F6D10, 2F36F0, E8D80
    std::atomic<std::uint64_t> calls[18]{},accesses{},stores{},children{};
    ~VerifySummary() {
        std::uint64_t any=0;for(const auto &c:calls)any|=c.load();
        if(any)
            std::fprintf(stderr,"Physics world leaves verify: 1F5FE0 %llu, 2E4550 %llu, 303BA0 %llu, 2E47B0 %llu, 303EE0 %llu, "
                "303C70 %llu, 303D00 %llu, 304350 %llu, 30CF40 %llu, 304130 %llu, 2F33B0 %llu, 1F76A0 %llu, "
                "2F8630 %llu, 2E4FE0 %llu, 1F7C10 %llu, 1F6D10 %llu, 2F36F0 %llu, E8D80 %llu calls; "
                "%llu ordered accesses (%llu stores) and %llu child boundaries identical, coverage %s\n",
                (unsigned long long)calls[0].load(),(unsigned long long)calls[1].load(),(unsigned long long)calls[2].load(),
                (unsigned long long)calls[3].load(),(unsigned long long)calls[4].load(),(unsigned long long)calls[5].load(),
                (unsigned long long)calls[6].load(),(unsigned long long)calls[7].load(),(unsigned long long)calls[8].load(),
                (unsigned long long)calls[9].load(),(unsigned long long)calls[10].load(),(unsigned long long)calls[11].load(),
                (unsigned long long)calls[12].load(),(unsigned long long)calls[13].load(),(unsigned long long)calls[14].load(),
                (unsigned long long)calls[15].load(),(unsigned long long)calls[16].load(),
                (unsigned long long)calls[17].load(),(unsigned long long)accesses.load(),
                (unsigned long long)stores.load(),(unsigned long long)children.load(),coverage_text().data());
    }
    // The coverage words as one hexadecimal number, most significant first.
    static std::array<char,8u*coverage_words*2u+1u> coverage_text() {
        std::array<char,8u*coverage_words*2u+1u> text{};std::size_t used=0;
        for(unsigned w=coverage_words;w-->0;) {
            const auto word=(unsigned long long)coverage(w).load();
            if(used)used+=std::snprintf(text.data()+used,text.size()-used,"%016llx",word);
            else if(word || w==0)used+=std::snprintf(text.data()+used,text.size()-used,"%llx",word);
        }
        return text;
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// The verification reference re-enters the routine; the nested entry runs the
// translated body.
inline bool &bypass(){thread_local bool value=false;return value;}

// Admission keeps every case outside the authenticated domain translated:
// another image, canonical observation PCs, unmasked MXCSR exceptions, active
// traces, arbitrary store observers, full callbacks without a literal cursor
// (except the census diagnostic) and service sites in the leaf.
template<class State> bool admitted(const State &s,std::uint32_t first,std::uint32_t last,std::uint32_t *&cursor) {
    if(reference_mode() || bypass() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    cursor=nullptr;
    if(!s.services.instruction)return true;
    if(native_census_detail::uncursored(s,first,last))return true;
    if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return false;
    const auto *policy=s.instruction_cursor;
    if((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace) ||
        (s.observe && !policy->observation_only_stores))return false;
    for(const auto site:policy->service_sites)if(site>=first && site<=last)return false;
    cursor=policy->position;
    return true;
}
// Run a leaf; in verify mode record it and replay the translated body.
template<class State,class Reference,class Kernel> void execute(State &s,std::uint32_t *cursor,
    std::atomic<std::uint64_t> &calls,Reference reference,Kernel kernel) {
    const auto run=[&](Trace<State> *trace) {
        Port<State> port(s,cursor,trace);
        const auto record=[&]{for(unsigned w=0;w<coverage_words;++w)coverage(w).fetch_or(port.hit[w],std::memory_order_relaxed);};
        try {kernel(port);} catch(...) {port.abandon();record();throw;}
        record();
    };
    if(!verify_mode()){run(nullptr);return;}
    const auto entry=Registers<State>::capture(s);
    Trace<State> trace;
    run(&trace);
    const auto final=Registers<State>::capture(s);
    Replay<State> bank{trace};
    std::uint32_t mismatched_pc=0;bool pc_mismatch=false;
    const SourcePhysicsContactRecordObserver observer=[&](const SourcePhysicsContactRecordStore &e) {
        if(!bank.store_pending || e.instruction!=bank.expected_pc){pc_mismatch=true;mismatched_pc=e.instruction;}
        bank.store_pending=false;
    };
    auto services=s.services;services.instruction=nullptr;
    std::size_t served=0;
    if(services.timestamp_counter)services.timestamp_counter=[&]()->std::uint64_t {
        if(served>=trace.time_stamps.size())throw std::runtime_error("Native physics world leaf read fewer time stamps than the translated body");
        return trace.time_stamps[served++];
    };
    State original{bank,observer,services,s.image};entry.apply(original);
    if constexpr(requires{original.selected_observation_pcs;})original.selected_observation_pcs=s.selected_observation_pcs;
    {
        struct Scope {
            Replay<State> *previous;
            explicit Scope(Replay<State> *bank):previous(active_replay<State>){bypass()=true;active_replay<State> =bank;}
            ~Scope(){bypass()=false;active_replay<State> =previous;}
        } held{&bank};
        reference(original);
    }
    if(pc_mismatch) {
        char text[96];std::snprintf(text,sizeof text,"Native physics world leaf store pc differs (translated %x)",mismatched_pc);
        throw std::runtime_error(text);
    }
    if(bank.at!=trace.events.size())throw std::runtime_error("Native physics world leaf made accesses the translated body did not");
    if(bank.call!=trace.calls.size())throw std::runtime_error("Native physics world leaf made child calls the translated body did not");
    if(served!=trace.time_stamps.size())throw std::runtime_error("Native physics world leaf read time stamps the translated body did not");
    const bool same=final.same(Registers<State>::capture(original));
    _mm_setcsr(final.mxcsr);set_host_x87_exceptions(final.x87_exceptions);  // the live run's sticky state stays the result
    if(!same)throw std::runtime_error("Native physics world leaf registers/flags/XMM differ from the translated body");
    calls.fetch_add(1);summary().accesses.fetch_add(trace.events.size());summary().children.fetch_add(trace.calls.size());
    std::uint64_t stores=0;for(const auto &e:trace.events)stores+=e.kind;
    summary().stores.fetch_add(stores);
}

template<class State,class Reference> bool bridge_1f5fe0(State &s,Reference reference) {
    std::uint32_t *cursor;if(!admitted(s,0x1f5fe0u,0x1f61a6u,cursor))return false;
    execute(s,cursor,summary().calls[0],reference,[](Port<State> &port){kernel_1f5fe0(port);});
    return true;
}
template<class State,class Reference> bool bridge_2e4550(State &s,Reference reference) {
    std::uint32_t *cursor;if(!admitted(s,0x2e4550u,0x2e4645u,cursor))return false;
    execute(s,cursor,summary().calls[1],reference,[](Port<State> &port){kernel_2e4550(port);});
    return true;
}
template<class State,class Reference> bool bridge_303ba0(State &s,Reference reference) {
    std::uint32_t *cursor;if(!admitted(s,0x303ba0u,0x303bfeu,cursor))return false;
    execute(s,cursor,summary().calls[2],reference,[](Port<State> &port){kernel_303ba0(port);});
    return true;
}
template<class State,class Reference> bool bridge_2e47b0(State &s,Reference reference) {
    std::uint32_t *cursor;if(!admitted(s,0x2e47b0u,0x2e481cu,cursor))return false;
    execute(s,cursor,summary().calls[3],reference,[](Port<State> &port){kernel_2e47b0(port);});
    return true;
}
// reference: the translated body with its 2E3250 call routed to replay_child;
// reserve: the 2E3250 route.
template<class State,class Reference,class Reserve> bool bridge_303ee0(State &s,Reference reference,Reserve reserve) {
    std::uint32_t *cursor;if(!admitted(s,0x303ee0u,0x303f46u,cursor))return false;
    execute(s,cursor,summary().calls[4],reference,[&](Port<State> &port){kernel_303ee0(port,reserve);});
    return true;
}
// reference: the translated body with its dispatch calls routed to
// replay_child; dispatch(state,target,site): the unit's dispatch.
template<class State,class Reference,class Dispatch> bool bridge_303c70(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x303c70u,0x303cfdu,cursor))return false;
    execute(s,cursor,summary().calls[5],reference,[&](Port<State> &port){kernel_303c70(port,dispatch);});
    return true;
}
template<class State,class Reference,class Dispatch> bool bridge_303d00(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x303d00u,0x303d99u,cursor))return false;
    execute(s,cursor,summary().calls[6],reference,[&](Port<State> &port){kernel_303d00(port,dispatch);});
    return true;
}
// The translated children of 304350, each entered through the unit's route:
// update (2F36F0), weld (304130), confirm (30CD00), report (30CF40),
// dispatch(state,target,site) (the virtual and function-pointer calls, and
// those of the inline 303C70/303D00) and reserve (2E3250, the inline 303EE0's
// growth). reference: the translated body with every one of them routed to
// replay_child (see tools/native_collection_agent_replacements.py).
template<class Update,class Weld,class Confirm,class Report,class Dispatch,class Reserve> struct CollectionChildren {
    Update update;Weld weld;Confirm confirm;Report report;Dispatch dispatch;Reserve reserve;
};
template<class State,class Reference,class Update,class Weld,class Confirm,class Report,class Dispatch,class Reserve>
bool bridge_304350(State &s,Reference reference,Update update,Weld weld,Confirm confirm,Report report,Dispatch dispatch,Reserve reserve) {
    std::uint32_t *cursor;if(!admitted(s,0x304350u,0x304ab7u,cursor))return false;
    CollectionChildren<Update,Weld,Confirm,Report,Dispatch,Reserve> children{update,weld,confirm,report,dispatch,reserve};
    execute(s,cursor,summary().calls[7],reference,[&](Port<State> &port){kernel_304350(port,children);});
    return true;
}
// The translated children of 30CF40: reserve (2E31E0) and dispatch (the
// contact manager's removal, through the unit's dispatch). reference: the
// translated body with both routed to replay_child (see
// tools/native_manifold_replacements.py).
template<class Reserve,class Dispatch> struct ReportChildren {Reserve reserve;Dispatch dispatch;};
template<class State,class Reference,class Reserve,class Dispatch>
bool bridge_30cf40(State &s,Reference reference,Reserve reserve,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x30cf40u,0x30d190u,cursor))return false;
    ReportChildren<Reserve,Dispatch> children{reserve,dispatch};
    execute(s,cursor,summary().calls[8],reference,[&](Port<State> &port){kernel_30cf40(port,children);});
    return true;
}
// 304130's test (303800) is entered through the unit's dispatch. reference:
// the translated body with it routed to replay_child.
template<class State,class Reference,class Dispatch> bool bridge_304130(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x304130u,0x304341u,cursor))return false;
    execute(s,cursor,summary().calls[9],reference,[&](Port<State> &port){kernel_304130(port,dispatch);});
    return true;
}
// 2F33B0 runs its recursion inline. reference: the translated body, whose
// recursion re-enters the bound body with the native route bypassed.
template<class State,class Reference> bool bridge_2f33b0(State &s,Reference reference) {
    std::uint32_t *cursor;if(!admitted(s,0x2f33b0u,0x2f3428u,cursor))return false;
    execute(s,cursor,summary().calls[10],reference,[](Port<State> &port){kernel_2f33b0(port);});
    return true;
}
// 1F76A0's children: canonical(target) (the unit's canonical_branch(), which
// throws the unit's message for an unknown jump-table entry) and dispatch
// (185AA0). reference: the translated body with the dispatch routed to
// replay_child.
template<class Canonical,class Dispatch> struct FilterChildren {Canonical canonical;Dispatch dispatch;};
template<class State,class Reference,class Canonical,class Dispatch>
bool bridge_1f76a0(State &s,Reference reference,Canonical canonical,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x1f76a0u,0x1f77abu,cursor))return false;
    FilterChildren<Canonical,Dispatch> children{canonical,dispatch};
    execute(s,cursor,summary().calls[11],reference,[&](Port<State> &port){kernel_1f76a0(port,children);});
    return true;
}
// 2F8630's children: features (30E930) and points (30D350), each through the
// unit's route; 2E4550 and 2E4760 run inline. reference: the translated body
// with both routed to replay_child (see tools/native_convex_pair_replacements.py).
template<class Features,class Points> struct ClosestPointChildren {Features features;Points points;};
template<class State,class Reference,class Features,class Points>
bool bridge_2f8630(State &s,Reference reference,Features features,Points points) {
    std::uint32_t *cursor;if(!admitted(s,0x2f8630u,0x2f88c4u,cursor))return false;
    ClosestPointChildren<Features,Points> children{features,points};
    execute(s,cursor,summary().calls[12],reference,[&](Port<State> &port){kernel_2f8630(port,children);});
    return true;
}
// 2E4FE0's children: may_sweep (283380), set_up_cache (309060) and
// closest_point (2F8630), each through the unit's route, and dispatch (the
// shapes' getType() and 309150); 2E4550 runs inline. reference: the
// translated body with each routed to replay_child.
template<class MaySweep,class SetUpCache,class Query,class Dispatch> struct SweptPairChildren {
    MaySweep may_sweep;SetUpCache set_up_cache;Query closest_point;Dispatch dispatch;
};
template<class State,class Reference,class MaySweep,class SetUpCache,class Query,class Dispatch>
bool bridge_2e4fe0(State &s,Reference reference,MaySweep may_sweep,SetUpCache set_up_cache,Query query,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x2e4fe0u,0x2e5312u,cursor))return false;
    SweptPairChildren<MaySweep,SetUpCache,Query,Dispatch> children{may_sweep,set_up_cache,query,dispatch};
    execute(s,cursor,summary().calls[13],reference,[&](Port<State> &port){kernel_2e4fe0(port,children);});
    return true;
}
// 1F7C10's children: filter (1F76A0), polygon (1F5FE0) and is_static (283450),
// each through the unit's route (1F76A0 and 1F5FE0 are bound at their own
// bodies), and dispatch (2E88B0, 184E20, the destructor, 1F67C0, 1F7330).
// reference: the translated body with each routed to replay_child.
template<class Filter,class Polygon,class IsStatic,class Dispatch> struct ChildShapeChildren {
    Filter filter;Polygon polygon;IsStatic is_static;Dispatch dispatch;
};
template<class State,class Reference,class Filter,class Polygon,class IsStatic,class Dispatch>
bool bridge_1f7c10(State &s,Reference reference,Filter filter,Polygon polygon,IsStatic is_static,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x1f7c10u,0x1f7d79u,cursor))return false;
    ChildShapeChildren<Filter,Polygon,IsStatic,Dispatch> children{filter,polygon,is_static,dispatch};
    execute(s,cursor,summary().calls[14],reference,[&](Port<State> &port){kernel_1f7c10(port,children);});
    return true;
}
// 1F6D10's children: query_structure (2E9670), filter_keys (1F6FA0) and
// filter (1F76A0, bound at its own body), each through the unit's route, and
// dispatch (the definition's queryObb); 2E4550 runs inline. reference: the
// translated body with each routed to replay_child.
template<class QueryStructure,class FilterKeys,class Filter,class Dispatch> struct AabbQueryChildren {
    QueryStructure query_structure;FilterKeys filter_keys;Filter filter;Dispatch dispatch;
};
template<class State,class Reference,class QueryStructure,class FilterKeys,class Filter,class Dispatch>
bool bridge_1f6d10(State &s,Reference reference,QueryStructure query_structure,FilterKeys filter_keys,Filter filter,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x1f6d10u,0x1f6f98u,cursor))return false;
    AabbQueryChildren<QueryStructure,FilterKeys,Filter,Dispatch> children{query_structure,filter_keys,filter,dispatch};
    execute(s,cursor,summary().calls[15],reference,[&](Port<State> &port){kernel_1f6d10(port,children);});
    return true;
}
// 2F36F0's children: reserve (2E31E0), construct (2F3430), destroy (2F34D0)
// and sort (2F33B0, bound at its own body), each through the unit's route,
// and dispatch (the virtual calls and the creation function); 2E4550 and
// 30BAD0 run inline. reference: the translated body with each routed to
// replay_child.
template<class Reserve,class Construct,class Destroy,class Sort,class Dispatch> struct ChildAgentChildren {
    Reserve reserve;Construct construct;Destroy destroy;Sort sort;Dispatch dispatch;
};
template<class State,class Reference,class Reserve,class Construct,class Destroy,class Sort,class Dispatch>
bool bridge_2f36f0(State &s,Reference reference,Reserve reserve,Construct construct,Destroy destroy,Sort sort,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0x2f36f0u,0x2f3dd6u,cursor))return false;
    ChildAgentChildren<Reserve,Construct,Destroy,Sort,Dispatch> children{reserve,construct,destroy,sort,dispatch};
    execute(s,cursor,summary().calls[16],reference,[&](Port<State> &port){kernel_2f36f0(port,children);});
    return true;
}
// E8D80 (the vehicle update unit's VehicleState): its update functions go
// through the unit's dispatch. reference: the translated body with the
// dispatch routed to replay_child.
template<class State,class Reference,class Dispatch> bool bridge_e8d80(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;if(!admitted(s,0xe7e60u,0xe7ee6u,cursor))return false;
    execute(s,cursor,summary().calls[17],reference,[&](Port<State> &port){kernel_e8d80(port,dispatch);});
    return true;
}
} // namespace halo2::native_world_leaves_detail
