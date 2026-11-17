#pragma once
// Boundary adapters for the device update systems
// (native_tu15_device_update_systems.hpp): 1179C0 machine update and 10B840
// device update, the device type callbacks of the scene device unit. Their
// caller (10DD10 through dispatch and the required owner) and their children
// are still translated TU1.5 code, so each adapter owns that ABI and nothing
// else, with the boundary framework of native_tu15_object_tick.hpp:
//   - the argument from its stack slot, the result in AL over the original's
//     EAX, the exit flags of ADD ESP, and every register as the original
//     leaves it at each child call and at exit (each access role's register
//     effects, including partial-register writes, LAHF's AH and the values
//     the original leaves in scratch registers);
//   - the "kept" values the logic hands to a child call (loaded into their
//     callee-saved registers before it, read back after it), and the flags of
//     the instruction that decides them;
//   - the original's stack frame: register saves and restores, the frame
//     slots (the definition, the changed and animated bytes, spilled floats,
//     the door query's unit buffer, marker and loop slots), return addresses
//     and the stack pointer as the children leave it;
//   - XMM0-5, all lanes, as the original leaves them: loads put a scalar in
//     lane 0; the arithmetic the original keeps only in a register is
//     replayed on the register copy with the MXCSR restored afterwards (the
//     logic's own instruction already raised the sticky flags), and the
//     compares that decide flags are replayed the same way;
//   - the x87 register stack: its sequences are balanced and contain no
//     child call, so the stack (count, TOP, the live lanes) is the caller's at
//     every boundary. The adapter keeps C1 as the translated neighbours see it
//     (each sequence ends with C1 clear, the portal loop's FISTP leaves its
//     rounding bit) and performs the two spills to the frame (FST m32 of the
//     turning rate, FLD/FISTP of a portal's openness). Lanes above the top are
//     empty registers that no instruction reads;
//   - the diagnostic cursor (each access publishes its original instruction)
//     and the store stream (every store goes through the unit's helper).
// Delete an adapter when its callers and children go native.
//
// Every child stays genuine and is entered through the unit's own original
// route with the exact register, flag, XMM, x87 and stack ABI: B7E50, BBFF0
// and BF050 (1179C0), B8760 and BF050 (10B840) through the unit's dispatch,
// the others as direct bodies.
// HALO2_REFERENCE_DEVICE_UPDATE=1 restores the translated bodies.
// HALO2_VERIFY_DEVICE_UPDATE=1 records each admitted native call (children
// genuine, run once) and replays the unmodified translated body against it,
// as in native_tu15_object_tick.hpp. A summary is printed at exit.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include "physics/runtime/native_tu15_device_update_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_device_update_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_device_update;

inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_DEVICE_UPDATE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_DEVICE_UPDATE");return p && *p;}();
    return value;
}

// ---- Shared boundary helpers --------------------------------------------------
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
// The register copy's SSE arithmetic, with the MXCSR as the logic left it.
template<class F> __m128 quiet(F f){const auto csr=_mm_getcsr();const auto x=f();_mm_setcsr(csr);return x;}
inline __m128 add(__m128 a,__m128 b){return quiet([&]{return _mm_add_ss(a,b);});}
inline __m128 subtract(__m128 a,__m128 b){return quiet([&]{return _mm_sub_ss(a,b);});}
inline __m128 multiply(__m128 a,__m128 b){return quiet([&]{return _mm_mul_ss(a,b);});}
inline __m128 divide(__m128 a,__m128 b){return quiet([&]{return _mm_div_ss(a,b);});}
inline __m128 convert(__m128 a,std::uint32_t integer){return quiet([&]{return _mm_cvtsi32_ss(a,int(integer));});}
// FST m32 of an extended value: the single and C1 (the store's rounding).
inline std::uint32_t single_spill(long double x,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
#else
    (void)x;(void)status;throw std::runtime_error("Native device update x87 backend unavailable");
#endif
}
inline constexpr std::uint16_t c1=0x200u;

// Branch coverage: the outcome reached when the original instruction `to`
// follows `from` (any instruction when from is 0), in two 64-bit words.
struct WideEdge {std::uint32_t from,to;unsigned branch;};
template<class State> struct DevicePort : Port<State> {
    std::span<const WideEdge> wide;std::array<std::uint64_t,2> covered{};
    DevicePort(State &s,std::uint32_t *c,Trace<State> *t,std::span<const WideEdge> e):Port<State>(s,c,t,{}),wide(e){}
    void mark(unsigned branch){covered[branch/64u]|=std::uint64_t(1)<<(branch%64u);}
    void edge(std::uint32_t pc){for(const auto &e:wide)if(e.to==pc && (!e.from || e.from==this->previous))mark(e.branch);}
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width){edge(pc);return Port<State>::read(pc,address,width);}
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value){edge(pc);Port<State>::store(pc,address,width,value);}
    void push(std::uint32_t pc,std::uint32_t value){this->gp[esp]-=4u;store(pc,this->gp[esp],4,value);}
    std::uint32_t pop(std::uint32_t pc){const auto value=read(pc,this->gp[esp],4);this->gp[esp]+=4u;return value;}
    void ret(std::uint32_t pc,std::uint32_t bytes){read(pc,this->gp[esp],4);this->gp[esp]+=4u+bytes;}
    template<class Child> void call(std::uint32_t pc,std::uint32_t next,std::uint32_t target,Child &&child) {
        push(pc,next);this->enter(target,child);
    }
    template<class Route> void call_routed(Route &route,std::uint32_t pc,std::uint32_t next,std::uint32_t target) {
        push(pc,next);this->enter(target,[&](State &v){route(v,target,pc);});
    }
    __m128 *x(){return this->s.xmm;}
    // The flags of COMISS/UCOMISS on the register copy.
    void compare(__m128 a,__m128 b,bool unordered){const auto csr=_mm_getcsr();this->flags.scalar_compare(a,b,unordered);_mm_setcsr(csr);}
    bool above()const{return !this->flags.cf && !this->flags.zf;}
    // LAHF; TEST AH,44: JP is taken when the compare was not equal.
    void lahf_test() {
        auto &gp=this->gp;
        gp[eax]=(gp[eax]&0xffff00ffu)|(((this->flags.bits()|2u)&0xffu)<<8);
        this->flags.logic((gp[eax]>>8)&0x44u,1);
    }
    void clear_c1(){this->s.fp.status&=std::uint16_t(~c1);}
};

// ---- 1179C0 machine update -----------------------------------------------------
enum MachineBranch : unsigned {
    a_no_cluster_power,a_power_tiny,a_cluster_query,a_cluster_none,a_cluster,a_floor_one,a_floor_zero,a_power_above,a_power_floor,
    b_no_offset,b_no_graph,b_no_animation,b_animated,
    c_gear,c_not_gear,c_follow,c_no_follow,c_turning,c_still,c_wrap,c_no_wrap,c_wrap_animated,c_wrap_plain,
    c_entity,c_no_entity,c_turn_animated,c_turn_plain,c_group,c_no_group,
    d_locked,d_unlocked,d_not_door,d_door,d_off_phase,d_phase,d_mode_four,d_query,d_object_radius,d_activation_radius,
    d_no_units,d_units,d_none_open,d_opened,d_open_group,d_open_no_group,
    e_vehicle,e_not_vehicle,e_vehicle_closed,e_vehicle_opens,e_ignored,e_attentive,e_refused,e_considered,
    e_one_sided,e_two_sided,e_player_check,e_player,e_not_player,e_door_ajar,e_door_closed,e_marker,e_no_marker,
    e_front,e_behind,e_more,e_last,
    f_not_door,f_door,f_no_group,f_group,f_still,f_moving,f_group_open,f_group_closed,f_position_open,f_position_closed,
    f_no_portals,f_portals,f_bit_set,f_bit_clear,f_clamp_low,f_clamp_high,f_clamp_keep,f_more_portals,f_last_portal,
    g_open,g_not_open,g_stays,g_counts,g_mode_four,g_timer,g_close,g_wait,g_close_group,g_close_none,
    h_moved_flag,h_not_moved_flag,h_moved,h_walk,h_alone,h_walk_more,h_parent_moved,h_walk_end,
    machine_branch_count
};
static_assert(machine_branch_count<=128u);
inline constexpr WideEdge machine_edges[]{
    {0x117a11u,0x117a96u,a_no_cluster_power},{0x117a28u,0x117a96u,a_power_tiny},{0x117a28u,0x117a3eu,a_cluster_query},
    {0x117a3eu,0x117a96u,a_cluster_none},{0x117a3eu,0x117a49u,a_cluster},
    {0x117aa6u,0x117aceu,b_no_offset},{0x117aafu,0x117aceu,b_no_graph},{0x117ab7u,0x117aceu,b_no_animation},
    {0x117ab7u,0x117ac0u,b_animated},
    {0x117aceu,0x117adcu,c_gear},{0x117aceu,0x117c49u,c_not_gear},{0x117adcu,0x117ae4u,c_follow},{0x117adcu,0x117afau,c_no_follow},
    {0x117b2bu,0x117b3du,c_turning},{0x117b2bu,0x117c49u,c_still},{0x117b60u,0x117b6au,c_wrap},{0x117b60u,0x117bb2u,c_no_wrap},
    {0x117b74u,0x117b7eu,c_wrap_animated},{0x117b74u,0x117bb2u,c_wrap_plain},{0x117bbfu,0x117bcau,c_entity},
    {0x117bbfu,0x117bd5u,c_no_entity},{0x117be2u,0x117beau,c_turn_animated},{0x117be2u,0x117c19u,c_turn_plain},
    {0x117c19u,0x117c24u,c_group},{0x117c19u,0x117c3fu,c_no_group},
    {0x117c51u,0x117ec4u,d_locked},{0x117c51u,0x117c5eu,d_unlocked},{0x117c5eu,0x11807bu,d_not_door},{0x117c5eu,0x117c6cu,d_door},
    {0x117c7eu,0x117ec4u,d_off_phase},{0x117c7eu,0x117c89u,d_phase},{0x117c8fu,0x117ec4u,d_mode_four},{0x117c8fu,0x117c99u,d_query},
    {0x117ca8u,0x117cafu,d_object_radius},{0x117ca8u,0x117cb6u,d_activation_radius},
    {0x117cefu,0x117eb8u,d_no_units},{0x117cefu,0x117d00u,d_units},{0x117e8du,0x117eb8u,d_none_open},{0x117e8du,0x117e95u,d_opened},
    {0x117e95u,0x117ea0u,d_open_group},{0x117e95u,0x117eabu,d_open_no_group},
    {0x117d46u,0x117d50u,e_vehicle},{0x117d46u,0x117d61u,e_not_vehicle},{0x117d50u,0x117d73u,e_vehicle_closed},
    {0x117d50u,0x117d61u,e_vehicle_opens},{0x117d61u,0x117e7cu,e_ignored},{0x117d61u,0x117d73u,e_attentive},
    {0x117d73u,0x117e7cu,e_refused},{0x117d73u,0x117d8du,e_considered},{0x117d8du,0x117dacu,e_one_sided},
    {0x117d8du,0x117e77u,e_two_sided},{0x117d8du,0x117d9fu,e_player_check},{0x117d9fu,0x117dacu,e_player},
    {0x117d9fu,0x117e77u,e_not_player},{0x117db4u,0x117e77u,e_door_ajar},{0x117db4u,0x117dc5u,e_door_closed},
    {0x117ddau,0x117e1eu,e_marker},{0x117ddau,0x117de7u,e_no_marker},{0x117e6eu,0x117e7cu,e_front},{0x117e6eu,0x117e77u,e_behind},
    {0x117e83u,0x117d00u,e_more},{0x117e83u,0x117e8du,e_last},
    {0x117ec4u,0x11807bu,f_not_door},{0x117ec4u,0x117ed2u,f_door},{0x117ed2u,0x117f21u,f_no_group},{0x117ed2u,0x117edfu,f_group},
    {0x117edfu,0x117f0au,f_still},{0x117edfu,0x117eeau,f_moving},{0x117efbu,0x117f21u,f_group_open},
    {0x117efbu,0x117f0au,f_group_closed},{0x117f28u,0x11800du,f_no_portals},{0x117f28u,0x117f39u,f_portals},
    {0x117ffbu,0x117f60u,f_more_portals},{0x117ffbu,0x11800du,f_last_portal},
    {0x11800du,0x11801eu,g_open},{0x11800du,0x118071u,g_not_open},{0x11801eu,0x118071u,g_stays},{0x11801eu,0x118027u,g_counts},
    {0x11802du,0x118071u,g_mode_four},{0x11802du,0x118033u,g_timer},{0x11804eu,0x118057u,g_close},{0x11804eu,0x11806au,g_wait},
    {0x118057u,0x118062u,g_close_group},{0x118057u,0x11806au,g_close_none},
    {0x118081u,0x118090u,h_moved_flag},{0x118081u,0x1180a4u,h_not_moved_flag},{0x1180a4u,0x1180e6u,h_moved},
    {0x1180a4u,0x1180b5u,h_walk},{0x1180a4u,0x118109u,h_alone},{0x1180dbu,0x1180b5u,h_walk_more},
    {0x1180dbu,0x1180e6u,h_parent_moved},{0x1180dbu,0x118109u,h_walk_end},
};
// The machine update's children, in the binder's argument order.
template<class Dispatch,class Cluster,class Frame,class Advance,class Report,class Query,class Marker,class Group,
         class Attach,class Moved,class Unmoved>
struct MachineChildren {
    Dispatch dispatch;Cluster cluster;Frame frame;Advance advance;Report report;Query query;Marker marker;Group group;
    Attach attach;Moved moved;Unmoved unmoved;
};
template<class State,class Children> struct MachineBoundary : DevicePort<State> {
    using Access=logic::machine_update::Access;
    using Kept=logic::machine_update::Machine;
    using Field=std::uint32_t Kept::*;
    Children &children;
    MachineBoundary(State &s,std::uint32_t *c,Trace<State> *t,Children &k):DevicePort<State>(s,c,t,machine_edges),children(k){}
    // The kept values in their callee-saved registers around a child call.
    struct Seats {Field ebx{},ebp{},esi{},edi{};bool bl{};};
    void keep(const Kept &m,Seats w) {
        auto &gp=this->gp;
        if(w.ebx)gp[ebx]=m.*w.ebx;
        if(w.ebp)gp[ebp]=m.*w.ebp;
        if(w.esi)gp[esi]=m.*w.esi;
        if(w.edi)gp[edi]=m.*w.edi;
        if(w.bl)set_low_byte(gp[ebx],m.moved);
    }
    void take(Kept &m,Seats w)const {
        const auto &gp=this->gp;
        if(w.ebx)m.*w.ebx=gp[ebx];
        if(w.ebp)m.*w.ebp=gp[ebp];
        if(w.esi)m.*w.esi=gp[esi];
        if(w.edi)m.*w.edi=gp[edi];
        if(w.bl)m.moved=std::uint8_t(gp[ebx]);
    }
    template<class Child> void call(Kept &m,Seats w,std::uint32_t pc,std::uint32_t next,std::uint32_t target,Child &child) {
        keep(m,w);DevicePort<State>::call(pc,next,target,child);take(m,w);
    }
    void routed(Kept &m,Seats w,std::uint32_t pc,std::uint32_t next,std::uint32_t target) {
        keep(m,w);DevicePort<State>::call_routed(children.dispatch,pc,next,target);take(m,w);
    }
    static constexpr Seats machine{&Kept::row,&Kept::object,&Kept::definition,&Kept::animation};
    static constexpr Seats channel{&Kept::row,&Kept::object,nullptr,&Kept::animation};
    static constexpr Seats turned{nullptr,&Kept::object,&Kept::definition,&Kept::animation};
    static constexpr Seats scan{nullptr,&Kept::object,&Kept::unit,&Kept::index};
    static constexpr Seats ending{nullptr,&Kept::object,&Kept::handle,nullptr,true};
    static constexpr Seats walked{nullptr,&Kept::parent,&Kept::handle,nullptr,true};

    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto *x=this->x();
        using enum Access;
        switch(a) {
        case table:return 0x1179d6u;
        case headers:return 0x1179dbu;
        case tags:return 0x1179deu;
        // PUSH EBP; SHL EBX,2 (the row).
        case object:this->push(0x1179e3u,gp[ebp]);gp[ebx]=this->flags.shift(gp[ebx],2,4);return 0x1179e7u;
        case definition_index:return 0x1179ebu;
        case definition:this->push(0x1179f7u,gp[esi]);return 0x1179f8u;
        case object_flags:return 0x1179feu;
        case power_magnitude:return 0x117a20u;
        case power_epsilon:return 0x117a28u;
        // After the epsilon test the original may test bit 10 into EDX; EDX
        // and the flags are written again before any boundary.
        case cluster:return 0x117a3eu;
        case cluster_table:return 0x117a49u;
        case cluster_one:return 0x117a4fu;
        // AND ECX,FFFF; LEA ECX,[ECX+ECX*4]; SHL ECX,5 (the record offset).
        case cluster_data:
            gp[ecx]=this->flags.logic(gp[ecx]&0xffffu,4);gp[ecx]*=5u;gp[ecx]=this->flags.shift(gp[ecx],5,4);return 0x117a63u;
        case cluster_power:return 0x117a70u;
        case table_again:return 0x117a96u;
        case headers_again:return 0x117a9cu;
        case object_again:return 0x117a9fu;
        case animation_offset:return 0x117aa6u;
        case channel_graph:return 0x117aafu;
        case channel_animation:return 0x117ab7u;
        case animation_offset_again:return 0x117ac0u;
        case machine_type:return 0x117aceu;
        case follow_position:return 0x117ae4u;
        case rate_one:return 0x117afau;
        case rate_power:return 0x117b03u;
        case velocity_clear:return 0x117b09u;
        case unpowered_rate:return 0x117b11u;
        case powered_rate:return 0x117b17u;
        case rate_power_again:return 0x117b1du;
        case rate_epsilon:return 0x117b2bu;
        case time:return 0x117b3du;
        case tick_length:return 0x117b42u;
        case turn_position:return 0x117b4du;
        case turn_one:return 0x117b55u;
        case turn_store:return 0x117b60u;
        case wrap_store:x[0]=subtract(x[0],x[2]);return 0x117b74u;   // SUBSS XMM0,XMM2
        case wrap_one:x[3]=_mm_setzero_ps();return 0x117ba6u;       // XORPS XMM3
        case entity_table:return 0x117bb2u;
        case entity_headers:return 0x117bb8u;
        case entity_object:return 0x117bbbu;
        case network_entity:return 0x117bbfu;
        // MOV EDX,[ESP+DC] (the handle) precedes the position.
        case channel_position:gp[edx]=this->read(0x117beau,gp[esp]+0xdcu,4);return 0x117bf1u;
        case turned_one:x[3]=_mm_setzero_ps();return 0x117c0du;
        case group_position:return 0x117c19u;
        case group_table:return 0x117c24u;
        case group_data:return 0x117c32u;
        case published_position:return 0x117c35u;
        case group_store:return 0x117c3bu;
        // XORPS XMM3 (a gear that turned keeps its earlier zero).
        case door_one:x[3]=_mm_setzero_ps();return 0x117c49u;
        case door_flags:return 0x117c51u;
        case door_type:return 0x117c5eu;
        case door_time:return 0x117c6cu;
        case door_tick:return 0x117c71u;
        case game:return 0x117c89u;
        case mode:return 0x117c8fu;
        case query_epsilon:return 0x117c99u;
        case activation_radius:return 0x117ca1u;
        case object_radius:return 0x117cafu;
        case activation_radius_again:return 0x117cb6u;
        case unit_table:return 0x117d07u;
        case unit_headers:return 0x117d15u;
        case unit_object:return 0x117d18u;
        case unit_tags:return 0x117d1cu;
        case unit_definition_index:return 0x117d25u;
        case unit_definition:return 0x117d30u;
        // MOV [ESP+24],ECX (the constant 1 the type shifts).
        case unit_type:this->store(0x117d39u,gp[esp]+0x24u,4,gp[ecx]);return 0x117d3du;
        case vehicle_flags:return 0x117d50u;
        case unit_state:return 0x117d61u;
        case unit_flags:return 0x117d73u;
        case one_sided_flags:return 0x117d8du;
        case unit_player:return 0x117d9fu;
        case door_position:return 0x117dacu;
        case door_zero:return 0x117db4u;
        case center_x:gp[eax]=gp[ebp]+0x30u;return 0x117de7u;    // LEA EAX,[EBP+30]
        case center_y:return 0x117de9u;
        case center_z:return 0x117decu;
        case unit_x:return 0x117e1eu;
        case unit_z:return 0x117e32u;
        case unit_y:return 0x117e40u;
        // MULSS XMM0,XMM1; ADDSS XMM3,XMM0 (the side's last term).
        case side_zero:x[0]=multiply(x[0],x[1]);x[3]=add(x[3],x[0]);return 0x117e6eu;
        case open_group:return 0x117e95u;
        case open_ticks_open:return 0x117eabu;
        case queried_one:x[3]=_mm_setzero_ps();return 0x117eb8u;
        case occlusion_type:return 0x117ec4u;
        case occlusion_group:return 0x117ed2u;
        case occlusion_flags:return 0x117edfu;
        case occlusion_group_table:return 0x117eeau;
        case occlusion_group_data:return 0x117ef8u;
        case occlusion_group_value:return 0x117efbu;
        case occlusion_position:return 0x117f0au;
        case first_portal:return 0x117f21u;
        case portal_count:return 0x117f28u;
        case openness_table:return 0x117f39u;
        case open_bits:return 0x117f3eu;
        case openness_scale:return 0x117f44u;
        case structure:return 0x117f60u;
        case portal_list:return 0x117f66u;
        case portal_indices:return 0x117f6cu;
        case closed_openness:return 0x117f6fu;
        case open_openness:return 0x117f77u;
        case portal_index:return 0x117f82u;
        case openness_position:return 0x117f97u;
        case open_word:return 0x117fa3u;
        case open_word_store:return 0x117fbau;
        case openness_store:return 0x117feau;
        case first_portal_again:return 0x117fedu;
        case portal_count_again:return 0x117ffbu;
        case timer_position:return 0x11800du;
        case timer_flags:return 0x11801eu;
        case timer_game:return 0x118027u;
        case timer_mode:return 0x11802du;
        case open_ticks:return 0x118033u;
        case open_ticks_store:return 0x11803au;
        // CVTSI2SS XMM0,EAX (the ticks open).
        case timer_time:x[0]=convert(x[0],gp[eax]);return 0x118044u;
        case timer_tick_length:return 0x118049u;
        case door_open_time:return 0x11804eu;
        case close_group:return 0x118057u;
        case open_ticks_clear:return 0x118071u;
        case finish_flags:return 0x11807bu;
        case moved_clear:return 0x11809bu;
        case parent:this->test8(gp[ebx]);return 0x1180a4u;            // TEST BL,BL
        case parent_table:this->compare32(gp[ebp],logic::none);return 0x1180b5u;   // CMP EBP,-1
        case parent_headers:return 0x1180bbu;
        case parent_flags:return 0x1180c8u;
        case parent_object:return 0x1180d2u;
        case parent_parent:this->test8(gp[ebx]);return 0x1180dbu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto *x=this->x();auto &f=this->flags;
        using enum Access;
        switch(a) {
        case table:case tags:case time:case door_time:case entity_headers:case network_entity:case group_position:
        case group_data:case unit_headers:case openness_table:case open_group:case close_group:case timer_time:
        case occlusion_group_data:case parent_headers:case channel_position:
            gp[eax]=v;break;
        case headers:case cluster:case table_again:case object_again:case door_tick:case structure:case portal_list:
        case portal_indices:case parent_object:
            gp[ecx]=v;break;
        case headers_again:case cluster_table:case follow_position:case entity_table:case group_table:case published_position:
        case game:case unit_table:case unit_tags:case unit_definition:case occlusion_group_table:case timer_game:case parent_table:
        case center_y:
            gp[edx]=v;break;
        case object:gp[ebp]=v;break;
        case definition:gp[esi]=v;gp[eax]=f.logic(0,4);break;               // XOR EAX,EAX
        case unit_object:gp[esi]=v;break;
        case finish_flags:gp[ebx]=v;break;
        case entity_object:gp[ebx]=v;break;
        case open_bits:gp[edi]=v;break;
        case parent:case parent_parent:gp[ebp]=v;break;
        default:break;
        }
        switch(a) {
        // AND EDX,FFFF; SHL EDX,4.
        case definition_index:gp[edx]=v;gp[edx]=f.logic(gp[edx]&0xffffu,4);gp[edx]=f.shift(gp[edx],4,4);break;
        case object_flags:gp[eax]=(gp[eax]&0xffff0000u)|v;break;              // MOV AX
        case power_magnitude:break;                                            // FLD; FABS (C1 clear at the end)
        case power_epsilon:this->clear_c1();break;                             // FLD; FCOMIP; FSTP
        case cluster:case group_position:case open_group:case close_group:case network_entity:f.minus(v,logic::none,4);break;
        case cluster_one:x[0]=scalar(v);break;
        // ADD ECX; SHR EAX,A; TEST AL,1; XORPS XMM0 without the floor.
        case cluster_data:
            gp[ecx]=f.plus(gp[ecx],v,4);gp[eax]=f.right(gp[eax],10,4,false);this->test8(gp[eax]&1u);
            if(gp[eax]&1u)this->mark(a_floor_one);else {x[0]=_mm_setzero_ps();this->mark(a_floor_zero);}
            break;
        // COMISS XMM1,XMM0; MOVAPS XMM0,XMM1 when above.
        case cluster_power:
            x[1]=scalar(v);this->compare(x[1],x[0],false);
            if(this->above()){x[0]=x[1];this->mark(a_power_above);}else this->mark(a_power_floor);
            break;
        case object_again:gp[eax]=f.logic(0xffffffffu,4);break;                // OR EAX,-1
        case animation_offset:case channel_animation:f.minus(v,0xffffu,2);break;
        case channel_graph:f.minus(v,gp[eax],4);break;
        case animation_offset_again:gp[edi]=f.plus(sx16(v),gp[ebp],4);break;    // MOVSX EDI; ADD EDI,EBP
        case machine_type:f.minus(v,2,2);break;
        case rate_one:x[3]=_mm_setzero_ps();break;                             // FLD; XORPS XMM3
        case powered_rate:break;
        case rate_epsilon:this->clear_c1();break;                              // FLD; FXCH; FCOMIP; FSTP
        case tick_length:x[0]=scalar(v);break;
        case turn_position:x[0]=add(x[0],scalar(v));break;
        case turn_one:case wrap_one:case turned_one:case door_one:case queried_one:
            x[2]=scalar(v);if(a==turn_one)this->compare(x[0],x[2],false);break;
        // AND EAX,FFFF; LEA ECX,[EAX+EAX*2].
        case group_table:case unit_table:case occlusion_group_table:gp[eax]=f.logic(gp[eax]&0xffffu,4);gp[ecx]=gp[eax]*3u;break;
        case door_flags:this->test8(v&1u);break;
        case door_type:case occlusion_type:f.minus(v,0,2);break;
        case mode:case timer_mode:f.minus(v,4,1);break;
        case query_epsilon:case object_radius:case activation_radius_again:case door_position:case unit_x:
        case occlusion_position:case timer_position:
            x[0]=scalar(v);break;
        case activation_radius:this->compare(x[0],scalar(v),false);break;
        case unit_tags:gp[eax]+=gp[ecx]*4u;break;                              // LEA EAX,[EAX+ECX*4] (the header)
        case unit_definition_index:gp[ecx]=v;gp[ecx]=f.logic(gp[ecx]&0xffffu,4);gp[ecx]=f.shift(gp[ecx],4,4);break;
        case unit_definition:gp[ecx]=1;break;                                  // MOV ECX,1
        // MOV CL; MOV EAX,[ESP+24]; SHL EAX,CL; MOV [ESP+20],ESI; MOV BL,1; TEST AL,2.
        case unit_type:
            set_low_byte(gp[ecx],v);gp[eax]=this->read(0x117d40u,gp[esp]+0x24u,4);gp[eax]=f.shift(gp[eax],gp[ecx]&0xffu,4);
            this->store(0x117d46u,gp[esp]+0x20u,4,gp[esi]);set_low_byte(gp[ebx],1);this->test8(gp[eax]&2u);
            break;
        // SHR EAX,14; TEST BL,AL; XOR BL,BL when clear.
        case vehicle_flags: {
            gp[eax]=f.right(v,0x14,4,false);const auto both=gp[ebx]&gp[eax]&0xffu;this->test8(both);
            if(!both)set_low_byte(gp[ebx],f.logic(0,1));
            break;
        }
        case unit_state:set_low_byte(gp[ecx],f.right(v&0xffu,2,1,false));this->test8(gp[ecx]&1u);break;
        // SHR EDX,E; TEST DL,1; then TEST BL,BL.
        case unit_flags:gp[edx]=f.right(v,0xe,4,false);this->test8(gp[edx]&1u);if(!(gp[edx]&1u))this->test8(gp[ebx]);break;
        case one_sided_flags:gp[eax]=v;this->test8(v&4u);if(!(v&4u))this->test8(v&8u);break;
        case unit_player:f.minus(v,logic::none,4);break;
        case door_zero:this->compare(x[0],scalar(v),true);this->lahf_test();break;
        case center_x:gp[ecx]=v;break;
        case center_z:gp[eax]=v;break;
        case unit_z:x[2]=scalar(v);break;
        case unit_y:x[1]=scalar(v);break;
        case side_zero:this->compare(x[3],scalar(v),false);break;
        // XOR BL,BL; CMP EAX,-1.
        case occlusion_group:gp[eax]=v;set_low_byte(gp[ebx],f.logic(0,1));f.minus(v,logic::none,4);break;
        case occlusion_flags:gp[ecx]=v;this->test8((v>>8)&1u);break;
        // UCOMISS XMM0,XMM3; LAHF; TEST AH,44; JP to MOV BL,1.
        case occlusion_group_value:
            x[0]=scalar(v);this->compare(x[0],x[3],true);this->lahf_test();
            if(f.pf)set_low_byte(gp[ebx],1);
            break;
        case first_portal:gp[edx]=sx16(v);break;
        // ADD ECX,EDX; CMP EDX,ECX.
        case portal_count:gp[ecx]=f.plus(sx16(v),gp[edx],4);f.minus(gp[edx],gp[ecx],4);break;
        case openness_scale:x[4]=scalar(v);gp[ecx]=gp[eax]+gp[edx]*2u+0x28u;break;
        case closed_openness:x[1]=scalar(v);break;
        case open_openness:x[0]=scalar(v);gp[eax]=gp[edx]&0xffffu;break;    // MOVZX EAX,DX
        // MOV EAX,ECX; AND ECX,1F; SAR EAX,5; MOV ESI,1; SUBSS XMM0,XMM1.
        case portal_index:
            gp[ecx]=sx16(v);gp[eax]=gp[ecx];gp[ecx]=f.logic(gp[ecx]&0x1fu,4);gp[eax]=f.right(gp[eax],5,4,true);gp[esi]=1;
            x[0]=subtract(x[0],x[1]);
            break;
        // MULSS; SHL ESI,CL; TEST BL,BL.
        case openness_position:x[0]=multiply(x[0],scalar(v));gp[esi]=f.shift(gp[esi],gp[ecx]&0xffu,4);this->test8(gp[ebx]);break;
        // LEA EAX (the word); ADDSS XMM0,XMM1; OR or NOT/AND the bit; COMISS XMM3,XMM0.
        case open_word:
            gp[ecx]=v;gp[eax]=gp[edi]+gp[eax]*4u;x[0]=add(x[0],x[1]);
            if(gp[ebx]&0xffu){gp[ecx]=f.logic(gp[ecx]|gp[esi],4);this->mark(f_bit_set);}
            else {gp[esi]=~gp[esi];gp[ecx]=f.logic(gp[ecx]&gp[esi],4);this->mark(f_bit_clear);}
            this->compare(x[3],x[0],false);
            break;
        case first_portal_again:gp[ecx]=sx16(v);break;
        // INC EDX; ADD EAX,ECX; CMP EDX,EAX.
        case portal_count_again:
            gp[edx]=f.increment(gp[edx],4,false);gp[eax]=f.plus(sx16(v),gp[ecx],4);f.minus(gp[edx],gp[eax],4);break;
        case timer_flags:this->test8(v&2u);break;
        case open_ticks:gp[eax]=f.increment(v,4,false);break;
        case timer_tick_length:x[0]=multiply(x[0],scalar(v));break;
        case door_open_time:this->compare(x[0],scalar(v),false);break;
        case parent_headers:gp[ebp]=f.logic(gp[ebp]&0xffffu,4);gp[ecx]=gp[ebp]*3u;break;
        // TEST DL,4; LEA EAX (the header).
        case parent_flags:set_low_byte(gp[edx],v);this->test8(v&4u);gp[eax]+=gp[ecx]*4u;break;
        case parent_object:if(gp[edx]&4u)set_low_byte(gp[ebx],1);break;     // MOV BL,1
        default:break;
        }
        if(a==occlusion_position) {
            this->compare(x[0],x[3],true);this->lahf_test();
            if(f.pf){set_low_byte(gp[ebx],1);this->mark(f_position_open);}
            else {set_low_byte(gp[ebx],f.logic(0,1));this->mark(f_position_closed);}
        } else if(a==timer_position){this->compare(x[0],x[2],true);this->lahf_test();}
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto *x=this->x();
        using enum Access;
        switch(a) {
        // The clamp: MOVAPS XMM0,XMM3 below 0, else COMISS XMM0,XMM2 and
        // MOVAPS XMM0,XMM2 above 1; MULSS XMM0,XMM4.
        case open_word_store:
            if(this->above()){x[0]=x[3];this->mark(f_clamp_low);}
            else {
                this->compare(x[0],x[2],false);
                if(this->above()){x[0]=x[2];this->mark(f_clamp_high);}else this->mark(f_clamp_keep);
            }
            x[0]=multiply(x[0],x[4]);
            break;
        case moved_clear:this->flags.logic(v,4);break;                       // AND m32's flags
        default:break;
        }
    }
    // ---- Frame
    // SUB ESP,C8 (in run); EAX = [ESP+CC]; AND EAX,FFFF; PUSH EBX; LEA EBX,[EAX+EAX*2].
    std::uint32_t handle() {
        auto &gp=this->gp;
        const auto value=this->read(0x1179c6u,gp[esp]+0xccu,4);
        gp[eax]=this->flags.logic(value&0xffffu,4);this->push(0x1179d2u,gp[ebx]);gp[ebx]=gp[eax]*3u;
        return value;
    }
    // ADD ECX,[ESP+DC]; TEST CL,3.
    std::uint32_t phase_handle() {
        auto &gp=this->gp;const auto value=this->read(0x117c74u,gp[esp]+0xdcu,4);
        gp[ecx]=this->flags.plus(gp[ecx],value,4);this->test8(gp[ecx]&3u);
        return value;
    }
    // MOV ESI,[ESP+DC]; SHR EBX,2; AND BL,1.
    std::uint32_t handle_again() {
        auto &gp=this->gp;gp[esi]=this->read(0x118081u,gp[esp]+0xdcu,4);
        gp[ebx]=this->flags.right(gp[ebx],2,4,false);set_low_byte(gp[ebx],this->flags.logic(gp[ebx]&1u,1));
        return gp[esi];
    }
    // PUSH EDI; XOR EDI,EDI; the definition and the two bytes; MOV ECX,EAX; SHR ECX,C; TEST CL,1.
    void begin(Kept &m) {
        auto &gp=this->gp;
        this->push(0x117a05u,gp[edi]);gp[edi]=this->flags.logic(0,4);m.animation=0;
        this->store(0x117a08u,gp[esp]+0x18u,4,gp[esi]);
        this->store(0x117a0cu,gp[esp]+0x12u,1,0);this->store(0x117a11u,gp[esp]+0x13u,1,0);
        gp[ecx]=this->flags.right(gp[eax],0xc,4,false);this->test8(gp[ecx]&1u);
    }
    void mark_changed() {
        std::uint32_t pc;
        switch(this->previous) {
        case 0x117a8cu:pc=0x117a91u;break;
        case 0x117c19u:case 0x117c3bu:pc=0x117c3fu;break;
        case 0x117c74u:pc=0x117c7eu;break;
        default:pc=0x11806au;break;
        }
        this->store(pc,this->gp[esp]+0x13u,1,1);
    }
    std::uint8_t changed(){auto &gp=this->gp;set_low_byte(gp[eax],this->read(0x11810eu,gp[esp]+0x13u,1));return std::uint8_t(gp[eax]);}
    void mark_animated(){this->store(0x117ac9u,this->gp[esp]+0x12u,1,1);}
    // MOV AL,[ESP+12]; TEST AL,AL.
    std::uint8_t animated() {
        auto &gp=this->gp;
        const auto pc=this->previous==0x117aceu?0x117adcu:this->previous==0x117b60u?0x117b6au:0x117be2u;
        set_low_byte(gp[eax],this->read(pc,gp[esp]+0x12u,1));this->test8(gp[eax]);
        return std::uint8_t(gp[eax]);
    }
    void clear_open(){this->store(0x117ca8u,this->gp[esp]+0x12u,1,0);}
    void mark_open(){this->store(0x117e77u,this->gp[esp]+0x12u,1,1);}
    std::uint8_t open() {
        auto &gp=this->gp;set_low_byte(gp[eax],this->read(0x117e8du,gp[esp]+0x12u,1));this->test8(gp[eax]);
        return std::uint8_t(gp[eax]);
    }
    // MOV ESI,[ESP+18].
    std::uint32_t definition() {
        std::uint32_t pc;
        switch(this->previous) {
        case 0x117af1u:pc=0x117af6u;break;
        case 0x117ba6u:pc=0x117baeu;break;
        case 0x117c0du:pc=0x117c15u;break;
        case 0x117eb8u:pc=0x117ec0u;break;
        default:pc=0x117fe6u;break;
        }
        auto &gp=this->gp;gp[esi]=this->read(pc,gp[esp]+0x18u,4);return gp[esi];
    }
    // MOVSS [ESP+14],XMM0; MOV EAX,[ESP+14]; PUSH EAX; PUSH ECX (the record); 1DA750.
    void power_cluster(Kept &m,std::uint32_t record,std::uint32_t power) {
        auto &gp=this->gp;
        this->store(0x117a80u,gp[esp]+0x14u,4,power);gp[eax]=this->read(0x117a86u,gp[esp]+0x14u,4);
        gp[ecx]=record;this->push(0x117a8au,gp[eax]);this->push(0x117a8bu,gp[ecx]);
        call(m,machine,0x117a8cu,0x117a91u,0x1da750u,children.cluster);
    }
    // PUSH EDX (the position); LEA ESI,[EBP+18C]; 1CE8D0.
    void follow_position(Kept &m,std::uint32_t position) {
        auto &gp=this->gp;keep(m,channel);
        gp[edx]=position;this->push(0x117aeau,gp[edx]);gp[esi]=gp[ebp]+0x18cu;
        call(m,channel,0x117af1u,0x117af6u,0x1ce8d0u,children.frame);
    }
    // 1CEBD0(value, animation, C1630, handle) with ESI = the channel: to 1.0
    // after a wrap (the handle read into ECX), or to the position (EDX).
    void advance_channel(Kept &m,std::uint32_t value) {
        auto &gp=this->gp;
        if(this->previous==0x117b74u) {
            keep(m,channel);
            gp[ecx]=this->read(0x117b7eu,gp[esp]+0xdcu,4);this->push(0x117b85u,gp[ecx]);this->push(0x117b86u,0xc1630u);
            this->push(0x117b8bu,gp[edi]);gp[esi]=gp[ebp]+0x18cu;this->push(0x117b92u,value);
            call(m,channel,0x117b97u,0x117b9cu,0x1cebd0u,children.advance);
        } else {
            constexpr Seats seats{nullptr,&Kept::object,nullptr,&Kept::animation};
            keep(m,seats);
            gp[eax]=value;
            this->push(0x117bf7u,gp[edx]);this->push(0x117bf8u,0xc1630u);this->push(0x117bfdu,gp[edi]);
            this->push(0x117bfeu,gp[eax]);gp[esi]=gp[ebp]+0x18cu;
            call(m,seats,0x117c05u,0x117c0au,0x1cebd0u,children.advance);
        }
    }
    // PUSH 0; 1CE8D0 (ESI as 1CEBD0 left it).
    void reset_channel(Kept &m){this->push(0x117b9cu,0);call(m,channel,0x117b9eu,0x117ba3u,0x1ce8d0u,children.frame);}
    // FADDP; FST [ESP+14] (C1 from the store's rounding); FABS clears C1.
    void keep_rate(long double rate) {
        this->store(0x117b25u,this->gp[esp]+0x14u,4,single_spill(rate,this->s.fp.status));this->clear_c1();
    }
    // MULSS XMM0,[ESP+14].
    std::uint32_t rate() {
        const auto value=this->read(0x117b47u,this->gp[esp]+0x14u,4);this->x()[0]=multiply(this->x()[0],scalar(value));
        return value;
    }
    // PUSH 400; PUSH EAX (the entity); B7E50 (dispatch).
    void mark_entity(Kept &m,std::uint32_t entity) {
        this->gp[eax]=entity;this->push(0x117bcau,0x400u);this->push(0x117bcfu,this->gp[eax]);routed(m,turned,0x117bd0u,0x117bd5u,0xb7e50u);
    }
    // MOV ECX,[ESP+DC]; PUSH ECX; 10C5D0.
    void report_position(Kept &m) {
        auto &gp=this->gp;gp[ecx]=this->read(0x117bd5u,gp[esp]+0xdcu,4);this->push(0x117bdcu,gp[ecx]);
        call(m,turned,0x117bddu,0x117be2u,0x10c5d0u,children.report);
    }
    // PUSH value; PUSH EAX (the group); 10C290: 1.0 when a unit opens the
    // door, 0 when it closes.
    void set_group(Kept &m,std::uint32_t group,std::uint32_t value) {
        this->gp[eax]=group;
        if(this->previous==0x117e95u) {
            this->push(0x117ea0u,value);this->push(0x117ea5u,this->gp[eax]);
            call(m,scan,0x117ea6u,0x117eabu,0x10c290u,children.group);
        } else {
            constexpr Seats seats{nullptr,&Kept::object,&Kept::definition,nullptr};
            this->push(0x118062u,value);this->push(0x118064u,this->gp[eax]);
            call(m,seats,0x118065u,0x11806au,0x10c290u,children.group);
        }
    }
    // PUSH 10; MOVSS [ESP+18],XMM0; MOV EDX,[ESP+18]; BD080(1, 3, &location,
    // &center, radius, units [ESP+98], 16); AX = the count.
    std::uint32_t query_units(Kept &m,std::uint32_t radius) {
        auto &gp=this->gp;
        this->push(0x117cbeu,0x10u);this->store(0x117cc0u,gp[esp]+0x18u,4,radius);
        gp[edx]=this->read(0x117cc6u,gp[esp]+0x18u,4);gp[ecx]=gp[esp]+0x9cu;
        this->push(0x117cd1u,gp[ecx]);this->push(0x117cd2u,gp[edx]);gp[eax]=gp[ebp]+0x30u;this->push(0x117cd6u,gp[eax]);
        gp[eax]=gp[ebp]+0x28u;this->push(0x117cdau,gp[eax]);this->push(0x117cdbu,3);this->push(0x117cddu,1);
        call(m,turned,0x117cdfu,0x117ce4u,0xbd080u,children.query);
        return gp[eax]&0xffffu;
    }
    // MOVSX EAX,AX; XOR EDI,EDI; TEST EAX,EAX; the count and index slots.
    void begin_units(std::uint32_t count) {
        auto &gp=this->gp;
        gp[eax]=count;gp[edi]=this->flags.logic(0,4);this->test32(gp[eax]);
        this->store(0x117cebu,gp[esp]+0x1cu,4,gp[eax]);this->store(0x117cefu,gp[esp]+0x14u,4,gp[edi]);
    }
    std::uint32_t unit(std::uint32_t index) {auto &gp=this->gp;gp[eax]=this->read(0x117d00u,gp[esp]+index*4u+0x98u,4);return gp[eax];}
    std::uint32_t unit_count(){auto &gp=this->gp;gp[eax]=this->read(0x117e7cu,gp[esp]+0x1cu,4);return gp[eax];}
    // INC EDI; CMP EDI,EAX; MOV [ESP+14],EDI.
    void next_unit(std::uint32_t index) {
        auto &gp=this->gp;this->flags.increment(index-1u,4,false);gp[edi]=index;this->compare32(gp[edi],gp[eax]);
        this->store(0x117e83u,gp[esp]+0x14u,4,gp[edi]);
    }
    std::uint32_t unit_index(){auto &gp=this->gp;gp[edi]=this->read(0x117e16u,gp[esp]+0x14u,4);return gp[edi];}
    std::uint32_t unit_object(){auto &gp=this->gp;gp[esi]=this->read(0x117e1au,gp[esp]+0x20u,4);return gp[esi];}
    // BAD60(handle, 7000686, 1) with CL = 0 and EBX = the marker [ESP+28];
    // TEST AX,AX.
    std::uint32_t find_marker(Kept &m) {
        auto &gp=this->gp;
        gp[eax]=this->read(0x117dc5u,gp[esp]+0xdcu,4);this->push(0x117dccu,1);this->push(0x117dceu,0x7000686u);
        this->push(0x117dd3u,gp[eax]);set_low_byte(gp[ecx],this->flags.logic(0,1));
        keep(m,scan);gp[ebx]=gp[esp]+0x34u;
        DevicePort<State>::call(0x117ddau,0x117ddfu,0xbad60u,children.marker);take(m,scan);
        this->flags.logic(gp[eax]&0xffffu,2);
        return gp[eax]&0xffffu;
    }
    // The center as the marker's position ([ESP+88]); BBFF0 (dispatch) with
    // EAX = the handle, ESI = the forward vector [ESP+64], EDI = 0.
    void query_forward(Kept &m,std::uint32_t x,std::uint32_t y,std::uint32_t z) {
        auto &gp=this->gp;
        gp[ecx]=x;gp[edx]=y;gp[eax]=z;
        this->store(0x117defu,gp[esp]+0x90u,4,gp[eax]);gp[eax]=this->read(0x117df6u,gp[esp]+0xdcu,4);
        gp[edi]=this->flags.logic(0,4);gp[esi]=gp[esp]+0x64u;
        this->store(0x117e03u,gp[esp]+0x88u,4,gp[ecx]);this->store(0x117e0au,gp[esp]+0x8cu,4,gp[edx]);
        constexpr Seats seats{nullptr,&Kept::object,nullptr,nullptr};
        routed(m,seats,0x117e11u,0x117e16u,0xbbff0u);
    }
    // SUBSS of the marker position (x, z, y) from the unit's center.
    std::uint32_t marker_position(unsigned axis) {
        auto &gp=this->gp;auto *x=this->x();
        static constexpr std::uint32_t pcs[]{0x117e23u,0x117e45u,0x117e37u};
        static constexpr unsigned registers[]{0,1,2};
        const auto value=this->read(pcs[axis],gp[esp]+0x88u+axis*4u,4);
        x[registers[axis]]=subtract(x[registers[axis]],scalar(value));
        return value;
    }
    // The forward vector: x into XMM3; z and y into XMM0 after the previous
    // term's MULSS/ADDSS.
    std::uint32_t marker_forward(unsigned axis) {
        auto &gp=this->gp;auto *x=this->x();
        if(axis==0){const auto value=this->read(0x117e2cu,gp[esp]+0x64u,4);x[3]=scalar(value);return value;}
        if(axis==2) {
            x[3]=multiply(x[3],x[0]);
            const auto value=this->read(0x117e52u,gp[esp]+0x6cu,4);x[0]=scalar(value);return value;
        }
        x[0]=multiply(x[0],x[2]);x[3]=add(x[3],x[0]);
        const auto value=this->read(0x117e60u,gp[esp]+0x68u,4);x[0]=scalar(value);return value;
    }
    // MOV [ESP+14],ECX first, then ADD EAX,2; MOV [ESP+14],EAX each portal.
    void keep_portal_cursor(std::uint32_t cursor) {
        auto &gp=this->gp;
        if(this->previous==0x117f44u){gp[ecx]=cursor;this->store(0x117f50u,gp[esp]+0x14u,4,gp[ecx]);}
        else {this->flags.plus(gp[eax],2,4);gp[eax]=cursor;this->store(0x117ff7u,gp[esp]+0x14u,4,gp[eax]);}
    }
    std::uint32_t portal_cursor(){auto &gp=this->gp;gp[eax]=this->read(0x117fddu,gp[esp]+0x14u,4);return gp[eax];}
    // MOVSS [ESP+1C],XMM0; FLD [ESP+1C]; FISTP [ESP+20] (C1 from the rounding).
    void round_openness(std::uint32_t value) {
        auto &gp=this->gp;
        this->store(0x117fcfu,gp[esp]+0x1cu,4,value);
        const auto loaded=this->read(0x117fd5u,gp[esp]+0x1cu,4);this->clear_c1();
        const auto rounded=physics_contact_runtime_detail::integer_spill(logic::x87::load(loaded),this->s.fp.status);
        this->store(0x117fd9u,gp[esp]+0x20u,4,rounded);
    }
    std::uint32_t openness() {
        auto &gp=this->gp;const auto value=this->read(0x117fe1u,gp[esp]+0x20u,2);gp[ecx]=(gp[ecx]&0xffff0000u)|value;
        return value;
    }
    // PUSH ESI; MOV byte [ESP+17],1 (changed); BF050 (dispatch).
    void notify_moved(Kept &m) {
        auto &gp=this->gp;this->push(0x118090u,gp[esi]);this->store(0x118091u,gp[esp]+0x17u,1,1);
        routed(m,ending,0x118096u,0x11809bu,0xbf050u);
    }
    // PUSH 0; MOV EAX,ESI; 1181F0.
    void update_attachments(Kept &m) {
        auto &gp=this->gp;keep(m,walked);this->push(0x1180e6u,0);gp[eax]=gp[esi];
        call(m,walked,0x1180eau,0x1180efu,0x1181f0u,children.attach);
    }
    // MOV EAX,ESI; MOV BL,1; BDA50.
    void moved(Kept &m) {
        auto &gp=this->gp;keep(m,walked);gp[eax]=gp[esi];set_low_byte(gp[ebx],1);
        call(m,walked,0x1180f3u,0x1180f8u,0xbda50u,children.moved);
    }
    // CMP EBP,-1; TEST BL,BL; MOV EAX,ESI; 118350.
    void unmoved(Kept &m) {
        auto &gp=this->gp;keep(m,walked);this->compare32(gp[ebp],logic::none);this->test8(gp[ebx]);gp[eax]=gp[esi];
        call(m,walked,0x118109u,0x11810eu,0x118350u,children.unmoved);
    }
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0xc8u,4);
        Memory<MachineBoundary> memory{*this};
        const auto result=logic::machine_update::run(memory,*this);
        std::uint32_t last;
        if(result.exit==logic::machine_update::Exit::moved) {
            // MOV AL,BL: the logic's result is BL as BDA50 left it.
            gp[edi]=this->pop(0x1180f8u);gp[esi]=this->pop(0x1180f9u);gp[ebp]=this->pop(0x1180fau);
            set_low_byte(gp[eax],result.value);gp[ebx]=this->pop(0x1180fdu);last=0x118104u;
        } else {
            gp[edi]=this->pop(0x118112u);gp[esi]=this->pop(0x118113u);gp[ebp]=this->pop(0x118114u);gp[ebx]=this->pop(0x118115u);
            last=0x11811cu;
        }
        gp[esp]=this->flags.plus(gp[esp],0xc8u,4);
        this->ret(last,4);this->publish(last);
    }
};

// ---- 10B840 device update ------------------------------------------------------
enum DeviceBranch : unsigned {
    p_block,p_no_block,p_unscripted,p_script_done,p_script_run,p_script_finished,p_script_continues,
    p_no_group,p_group,p_away,p_at_target,p_moving,p_initializing,p_at_rest,
    p_no_follow,p_no_graph,p_no_animation,p_follow,p_up,p_down,p_accelerating,p_no_acceleration,p_no_speed,
    p_waited,p_waiting,p_open,p_closed,p_target_below,p_first_wait,p_later_wait,
    p_fast,p_slow,p_clamp_up,p_clamp_down,p_arrived,p_travelling,p_opened,p_closed_effect,
    p_stopped,p_turned,p_same_direction,p_reversed,p_opening,p_closing,
    p_moved,p_unmoved,p_reported_first,p_quiet,p_moved_no_block,p_moved_block,p_moved_no_graph,p_moved_no_animation,p_advanced,
    q_script_run,q_script_done,q_position_scripted,q_unscripted,q_script_finished,q_script_continues,q_no_group,q_group,
    q_away,q_at_target,q_moving,q_at_rest,q_no_block,q_block,q_invalid,q_follow,q_unmoved,q_moved,
    q_moved_no_block,q_moved_block,q_moved_invalid,q_advanced,
    r_check_group,r_scripted_initializing,r_scripted_settled,r_group_initializing,r_settled,r_machine,r_not_machine,
    r_none_handle,r_handle,r_counting,r_cleared,
    device_branch_count
};
static_assert(device_branch_count<=128u);
template<class Dispatch,class Script,class Report,class Frame,class Effect,class Step,class Advance,class Attach>
struct DeviceChildren {
    Dispatch dispatch;Script script;Report report;Frame frame;Effect effect;Step step;Advance advance;Attach attach;
};
inline constexpr WideEdge device_branch_edges[]{
    {0x10b88du,0x10b897u,p_block},{0x10b88du,0x10b8a4u,p_no_block},
    {0x10b8b4u,0x10b954u,p_unscripted},{0x10b8b4u,0x10bc25u,p_script_done},{0x10b8b4u,0x10b8c9u,p_script_run},
    {0x10b954u,0x10bc25u,p_no_group},{0x10b954u,0x10b963u,p_group},{0x10b98eu,0x10b9bbu,p_away},{0x10b98eu,0x10b994u,p_at_target},
    {0x10b9bbu,0x10b9edu,p_no_follow},{0x10b9c3u,0x10b9edu,p_no_graph},{0x10b9d4u,0x10b9edu,p_no_animation},
    {0x10b9d4u,0x10b9dbu,p_follow},
    {0x10baaeu,0x10baf6u,p_waited},{0x10baaeu,0x10bab7u,p_waiting},{0x10bab7u,0x10baf6u,p_open},{0x10bab7u,0x10bac4u,p_closed},
    {0x10bac4u,0x10baf6u,p_target_below},{0x10bad1u,0x10badeu,p_first_wait},{0x10bad1u,0x10bc1cu,p_later_wait},
    {0x10bb0cu,0x10bb18u,p_fast},{0x10bb0cu,0x10bb30u,p_slow},{0x10bb57u,0x10bb60u,p_arrived},{0x10bb57u,0x10bb80u,p_travelling},
    {0x10bb60u,0x10bb68u,p_opened},{0x10bb60u,0x10bb74u,p_closed_effect},
    {0x10bb80u,0x10bbcau,p_stopped},{0x10bb80u,0x10bb93u,p_turned},{0x10bb93u,0x10bbcau,p_same_direction},
    {0x10bb93u,0x10bba2u,p_reversed},{0x10bba2u,0x10bbabu,p_opening},{0x10bba2u,0x10bbb7u,p_closing},
    {0x10bbd0u,0x10bbe3u,p_moved},{0x10bbd0u,0x10bbd9u,p_unmoved},{0x10bbd9u,0x10bbe3u,p_reported_first},
    {0x10bbd9u,0x10bc1cu,p_quiet},{0x10bbedu,0x10bc1cu,p_moved_no_block},{0x10bbedu,0x10bbf5u,p_moved_block},
    {0x10bbf5u,0x10bc1cu,p_moved_no_graph},{0x10bc06u,0x10bc1cu,p_moved_no_animation},{0x10bc06u,0x10bc0du,p_advanced},
    {0x10bc8bu,0x10bd92u,q_no_group},{0x10bc8bu,0x10bc9au,q_group},{0x10bcb1u,0x10bce0u,q_away},{0x10bcb1u,0x10bcc7u,q_at_target},
    {0x10bccfu,0x10bce0u,q_moving},{0x10bccfu,0x10bd8eu,q_at_rest},{0x10bce0u,0x10bd01u,q_no_block},{0x10bce0u,0x10bcf0u,q_block},
    {0x10bcf0u,0x10bd01u,q_invalid},{0x10bcf0u,0x10bcf9u,q_follow},{0x10bd4au,0x10bd89u,q_unmoved},{0x10bd4au,0x10bd53u,q_moved},
    {0x10bd5du,0x10bd89u,q_moved_no_block},{0x10bd5du,0x10bd6du,q_moved_block},{0x10bd6du,0x10bd89u,q_moved_invalid},
    {0x10bd6du,0x10bd76u,q_advanced},
    {0x10bd9au,0x10bd9du,r_check_group},{0x10bd9au,0x10bdaau,r_scripted_initializing},{0x10bd9au,0x10be0au,r_scripted_settled},
    {0x10bd9du,0x10bdaau,r_group_initializing},{0x10bd9du,0x10be0au,r_settled},
    {0x10bdb3u,0x10bdc2u,r_machine},{0x10bdb3u,0x10bdd7u,r_not_machine},{0x10bdd7u,0x10bdf5u,r_none_handle},
    {0x10bdd7u,0x10bddeu,r_handle},{0x10bdebu,0x10bdffu,r_counting},{0x10bdebu,0x10bdf5u,r_cleared},
};
template<class State,class Children> struct DeviceBoundary : DevicePort<State> {
    using Access=logic::device_update::Access;
    using Kept=logic::device_update::Device;
    using Field=std::uint32_t Kept::*;
    Children &children;
    DeviceBoundary(State &s,std::uint32_t *c,Trace<State> *t,Children &k):DevicePort<State>(s,c,t,device_branch_edges),children(k){}
    struct Seats {Field ebx{},ebp{},esi{},edi{};};
    void keep(const Kept &d,Seats w) {
        auto &gp=this->gp;
        if(w.ebx)gp[ebx]=d.*w.ebx;
        if(w.ebp)gp[ebp]=d.*w.ebp;
        if(w.esi)gp[esi]=d.*w.esi;
        if(w.edi)gp[edi]=d.*w.edi;
    }
    void take(Kept &d,Seats w)const {
        const auto &gp=this->gp;
        if(w.ebx)d.*w.ebx=gp[ebx];
        if(w.ebp)d.*w.ebp=gp[ebp];
        if(w.esi)d.*w.esi=gp[esi];
        if(w.edi)d.*w.edi=gp[edi];
    }
    template<class Child> void call(Kept &d,Seats w,std::uint32_t pc,std::uint32_t next,std::uint32_t target,Child &child) {
        keep(d,w);DevicePort<State>::call(pc,next,target,child);take(d,w);
    }
    void routed(Kept &d,Seats w,std::uint32_t pc,std::uint32_t next,std::uint32_t target) {
        keep(d,w);DevicePort<State>::call_routed(children.dispatch,pc,next,target);take(d,w);
    }
    static constexpr Seats scripted{&Kept::row,&Kept::object,&Kept::handle,nullptr};
    static constexpr Seats rowed{&Kept::row,&Kept::object,nullptr,nullptr};
    static constexpr Seats following{&Kept::row,&Kept::object,nullptr,&Kept::value};
    static constexpr Seats waiting{&Kept::velocity,&Kept::object,&Kept::group,&Kept::value};
    static constexpr Seats stepping{&Kept::velocity,&Kept::object,nullptr,&Kept::value};
    static constexpr Seats valued{nullptr,&Kept::object,nullptr,&Kept::value};
    static constexpr Seats powered{&Kept::power_group,&Kept::object,nullptr,&Kept::value};
    bool script_done{};

    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto *x=this->x();
        using enum Access;
        switch(a) {
        case table:return 0x10b854u;
        case headers:return 0x10b859u;
        case tags:return 0x10b85cu;
        case object:gp[ebx]=this->flags.shift(gp[ebx],2,4);return 0x10b864u;   // SHL EBX,2 (the row)
        case definition_index:return 0x10b868u;
        case animation_offset:return 0x10b874u;
        case definition:return 0x10b87cu;
        case animation_offset_again:return 0x10b897u;
        case flags_load:return 0x10b8a4u;
        case flags_store:return 0x10b8b4u;
        // TEST CL,20; MOV EDX,[ESP+18] (the block).
        case script_old_position:this->test8(gp[ecx]&0x20u);gp[edx]=this->read(0x10b8c9u,gp[esp]+0x18u,4);return 0x10b8cdu;
        case clock:return 0x10b8f9u;
        case script_store:return 0x10b8ffu;
        case tick_length:return 0x10b90du;
        case script_flags:return 0x10b914u;
        case script_velocity:return 0x10b91au;
        // MOV EDX,[ESP+34]; OR or AND EAX,20; PUSH EDX.
        case script_flags_store:
            script_done=!this->flags.zf;
            gp[edx]=this->read(script_done?0x10b924u:0x10b93cu,gp[esp]+0x34u,4);
            gp[eax]=this->flags.logic(script_done?gp[eax]|0x20u:gp[eax]&~0x20u,4);
            this->push(script_done?0x10b92bu:0x10b943u,gp[edx]);
            this->mark(script_done?p_script_finished:p_script_continues);
            return script_done?0x10b92cu:0x10b944u;
        case position_group:return 0x10b954u;
        case group_table:return 0x10b963u;
        case group_data:return 0x10b969u;
        case group_value:return 0x10b974u;
        case position_compare:return 0x10b97au;
        case velocity_compare:return 0x10b994u;
        case velocity_zero:return 0x10b99cu;
        case delay_clear:return 0x10b9adu;
        case channel_graph:return 0x10b9c3u;
        case channel_animation:return 0x10b9d4u;
        case follow_position:return 0x10b9dbu;
        case one:return 0x10b9edu;
        case inverse_power:return 0x10b9f5u;
        case position_acceleration:return 0x10b9fdu;
        case unpowered_acceleration:return 0x10ba05u;
        case acceleration_power:return 0x10ba0du;
        case position_rate:return 0x10ba15u;
        case speed_power:return 0x10ba1du;
        case unpowered_rate:return 0x10ba2du;
        case direction_velocity:return 0x10ba40u;
        case moving_set:return 0x10ba73u;
        case moving_clear:return 0x10ba80u;
        case delay:return 0x10ba8bu;
        case moving_store:return 0x10ba95u;
        case clock_again:x[0]=convert(x[0],gp[edx]);return 0x10baa3u;     // CVTSI2SS XMM0,EDX
        case delay_tick_length:return 0x10baa9u;
        case delay_time:return 0x10baaeu;
        case closed_position:return 0x10bab7u;
        case closed_target:return 0x10bac4u;
        // LEA EAX,[ECX+1]; CMP AX,1.
        case delay_store:gp[eax]=gp[ecx]+1u;this->flags.minus(gp[eax]&0xffffu,1,2);return 0x10bad1u;
        case delay_effect:return 0x10bae2u;
        case old_velocity:return 0x10baf6u;
        case old_position:return 0x10baf8u;
        // MOVAPS XMM0,XMM5 up; MOVAPS XMM0,XMM1; SUBSS XMM0,XMM5 down.
        case clamp_store:
            if(gp[eax]&0xffu){x[0]=x[5];this->mark(p_clamp_up);}
            else {x[0]=subtract(x[1],x[5]);this->mark(p_clamp_down);}
            return 0x10bb2cu;
        case step_tick_length:return 0x10bb30u;
        case loop_flags:return 0x10bb3bu;
        // PUSH 1.0; PUSH 0; PUSH EAX (the acceleration).
        case step_target:
            this->push(0x10bb41u,0x3f800000u);this->push(0x10bb46u,0);this->push(0x10bb48u,gp[eax]);return 0x10bb49u;
        case opened_effect:return 0x10bb6cu;
        case closed_effect:return 0x10bb78u;
        case new_velocity:return 0x10bb80u;
        case new_velocity_again:return 0x10bba2u;
        case opening_effect:return 0x10bbafu;
        case closing_effect:return 0x10bbbbu;
        case new_position:return 0x10bbd0u;
        case initializing_flags:return 0x10bbd9u;
        case moved_graph:return 0x10bbf5u;
        case moved_animation:return 0x10bc06u;
        case moved_position:return 0x10bc0du;
        case power_flags:return 0x10bc25u;
        case power_script_flags:return 0x10bc59u;
        case power_script_store:return 0x10bc69u;
        case power_group:return 0x10bc8bu;
        case power_group_table:return 0x10bc9au;
        case power_group_data:return 0x10bca8u;
        case power_group_value:return 0x10bcabu;
        case power_compare:return 0x10bcb1u;
        case power_velocity_compare:return 0x10bcc7u;
        case power_velocity_zero:return 0x10bccfu;
        case follow_power:return 0x10bcf9u;
        case power_acceleration:return 0x10bd05u;
        case power_clock:return 0x10bd0bu;
        case power_tick_length:return 0x10bd11u;
        case old_power:return 0x10bd18u;
        case power_rate:return 0x10bd1cu;
        case power_target:
            this->push(0x10bd24u,0x3f800000u);this->push(0x10bd29u,0);this->push(0x10bd2bu,gp[eax]);return 0x10bd2cu;
        case new_power:return 0x10bd4au;
        // MOV EAX,[ESP+34] (the handle) precedes the power.
        case moved_power:gp[eax]=this->read(0x10bd76u,gp[esp]+0x34u,4);return 0x10bd7au;
        case final_flags:return 0x10bd92u;
        case final_group:return 0x10bd9du;
        case type_table:this->test8(gp[eax]);return 0x10bdaau;              // TEST AL,AL
        case type_headers:return 0x10bdb0u;
        case object_type:return 0x10bdb3u;
        case countdown_table:return 0x10bddeu;
        case countdown_headers:return 0x10bde4u;
        case countdown_object:return 0x10bde7u;
        case countdown:return 0x10bdebu;
        case initializing_clear:return 0x10bdf5u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto *x=this->x();auto &f=this->flags;
        using enum Access;
        switch(a) {
        case table:case tags:gp[eax]=v;break;
        case headers:gp[ecx]=v;break;
        case object:gp[ebp]=v;break;
        case definition_index:gp[edx]=v;gp[edx]=f.logic(gp[edx]&0xffffu,4);gp[edx]=f.shift(gp[edx],4,4);break;
        case animation_offset:f.minus(v,0xffffu,2);break;
        case definition:gp[edx]=v;break;
        case animation_offset_again:gp[eax]=f.plus(sx16(v),gp[ebp],4);break;
        // AND ECX,FFFFFEFF; TEST CL,8; PUSH EDI.
        case flags_load:
            gp[ecx]=f.logic(v&0xfffffeffu,4);this->test8(gp[ecx]&8u);this->push(0x10b8b3u,gp[edi]);
            break;
        case script_old_position:x[0]=scalar(v);break;
        case clock:case power_clock:case step_tick_length:case countdown_table:gp[ecx]=v;break;
        case tick_length:x[0]=divide(x[0],scalar(v));this->test8(gp[eax]);break;   // DIVSS; TEST AL,AL
        case script_flags:gp[eax]=v;break;
        case position_group:case power_group:gp[eax]=v;f.minus(v,logic::none,4);break;
        case group_table:case group_data:gp[esi]=v;if(a==group_data){gp[eax]=f.logic(gp[eax]&0xffffu,4);gp[eax]*=3u;}break;
        case group_value:case velocity_compare:case power_group_value:case power_velocity_compare:case old_power:case old_position:
            x[0]=scalar(v);break;
        // UCOMISS XMM0,XMM1 (0); LAHF; TEST AH,44.
        case closed_position:x[0]=scalar(v);this->compare(x[0],x[1],true);this->lahf_test();break;
        // UCOMISS; LEA ESI (the group); LEA EDI (the position); LAHF; TEST AH,44.
        case position_compare:
            this->compare(x[0],scalar(v),true);gp[esi]+=gp[eax]*4u;gp[edi]=gp[ebp]+0x140u;this->lahf_test();break;
        // UCOMISS; LAHF; TEST AH,44; TEST CL,CL when equal.
        case velocity_zero:
            this->compare(x[0],scalar(v),true);this->lahf_test();
            if(f.pf)this->mark(p_moving);
            else {this->test8(gp[ecx]);this->mark(gp[ecx]&0x80u?p_initializing:p_at_rest);}
            break;
        // CMP ECX,-1; LEA EAX,[EBP+18C].
        case channel_graph:gp[ecx]=v;f.minus(v,logic::none,4);gp[eax]=gp[ebp]+0x18cu;break;
        case channel_animation:case moved_animation:f.minus(v,0xffffu,2);break;
        case follow_position:case follow_power:gp[ecx]=v;break;
        case one:x[1]=scalar(v);break;
        case inverse_power:x[1]=subtract(x[1],scalar(v));break;
        case position_acceleration:x[0]=scalar(v);break;
        case unpowered_acceleration:x[2]=scalar(v);break;
        case acceleration_power:x[0]=multiply(x[0],scalar(v));break;
        case position_rate:case power_rate:x[5]=scalar(v);break;
        // MULSS XMM5,m; MULSS XMM2,XMM1; ADDSS XMM0,XMM2.
        case speed_power:x[5]=multiply(x[5],scalar(v));x[2]=multiply(x[2],x[1]);x[0]=add(x[0],x[2]);break;
        // MOVSS XMM2; MULSS XMM2,XMM1; XORPS XMM1; ADDSS XMM5,XMM2.
        case unpowered_rate:x[2]=multiply(scalar(v),x[1]);x[1]=_mm_setzero_ps();x[5]=add(x[5],x[2]);break;
        // COMISS XMM2,XMM1; LEA EBX,[EBP+144].
        case direction_velocity:
            x[2]=scalar(v);this->compare(x[2],x[1],false);gp[ebx]=gp[ebp]+0x144u;this->mark(this->above()?p_up:p_down);break;
        case moving_set:gp[eax]=f.logic(v|0x100u,4);break;
        case moving_clear:gp[eax]=f.logic(v&~0x100u,4);break;
        case delay:gp[ecx]=(gp[ecx]&0xffff0000u)|v;gp[edx]=sx16(v);break;      // MOV CX; MOVSX EDX,CX
        case clock_again:case power_tick_length:case countdown_headers:gp[edx]=v;break;
        case delay_tick_length:x[0]=multiply(x[0],scalar(v));break;
        case delay_time:case closed_target:this->compare(x[0],scalar(v),false);break;
        case delay_effect:case old_velocity:case step_target:case opened_effect:case closed_effect:case opening_effect:
        case closing_effect:case power_acceleration:case power_target:case countdown_object:case type_headers:
            gp[eax]=v;break;
        case loop_flags:set_low_byte(gp[edx],v);break;
        // XORPS XMM1; UCOMISS XMM0,XMM1; LAHF; TEST AH,44.
        case new_velocity:x[0]=scalar(v);x[1]=_mm_setzero_ps();this->compare(x[0],x[1],true);this->lahf_test();break;
        case new_velocity_again:x[1]=scalar(v);this->compare(x[1],x[0],false);break;
        case new_position:case new_power:this->compare(x[0],scalar(v),true);this->lahf_test();break;
        case initializing_flags:set_low_byte(gp[eax],v);this->test8(v);break;
        // CMP ECX,-1; LEA ESI,[EBP+18C].
        case moved_graph:gp[ecx]=v;f.minus(v,logic::none,4);gp[esi]=gp[ebp]+0x18cu;break;
        case moved_position:gp[edx]=v;break;
        // TEST AL,10; then TEST AL,40 or TEST AL,8.
        case power_flags:
            gp[eax]=v;this->test8(v&0x10u);this->test8(v&(v&0x10u?0x40u:8u));
            this->mark(v&0x10u?(v&0x40u?q_script_done:q_script_run):(v&8u?q_position_scripted:q_unscripted));
            break;
        case power_script_flags:gp[eax]=f.logic(script_done?v|0x40u:v&~0x40u,4);break;
        case power_group_table:gp[edx]=v;gp[eax]=f.logic(gp[eax]&0xffffu,4);gp[ecx]=gp[eax]*3u;break;
        case power_group_data:gp[eax]=v;break;
        // UCOMISS; LEA EBX (the group); LEA EDI (the power); LAHF; TEST AH,44.
        case power_compare:
            this->compare(x[0],scalar(v),true);gp[ebx]=gp[eax]+gp[ecx]*4u;gp[edi]=gp[ebp]+0x134u;this->lahf_test();break;
        case power_velocity_zero:this->compare(x[0],scalar(v),true);this->lahf_test();break;
        case moved_power:gp[ecx]=v;break;
        // TEST AL,8; POP EDI.
        case final_flags:gp[eax]=v;this->test8(v&8u);gp[edi]=this->pop(0x10bd9au);break;
        case final_group:f.minus(v,logic::none,4);break;
        case type_table:gp[edx]=v;break;
        // MOV CL; MOV EDX,1; SHL EDX,CL; TEST DL,DL.
        case object_type:set_low_byte(gp[ecx],v);gp[edx]=f.shift(1,v&0xffu,4);this->test8(gp[edx]);break;
        case countdown:set_low_byte(gp[ecx],v);this->test8(v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::initializing_clear)this->flags.logic(v,4);}
    // ---- Frame
    // SUB ESP,20 (in run); PUSH EBX/EBP/ESI; ESI = [ESP+30]; EBX = index * 3.
    std::uint32_t handle() {
        auto &gp=this->gp;
        this->push(0x10b843u,gp[ebx]);this->push(0x10b844u,gp[ebp]);this->push(0x10b845u,gp[esi]);
        gp[esi]=this->read(0x10b846u,gp[esp]+0x30u,4);gp[eax]=this->flags.logic(gp[esi]&0xffffu,4);gp[ebx]=gp[eax]*3u;
        return gp[esi];
    }
    // SHL EBX,2 happened before the object; the row, definition, changed and block slots.
    void begin(Kept &,std::uint32_t) {
        auto &gp=this->gp;
        this->store(0x10b880u,gp[esp]+0x18u,4,gp[ebx]);this->store(0x10b884u,gp[esp]+0x10u,4,gp[edx]);
        this->store(0x10b888u,gp[esp]+0xfu,1,0);this->store(0x10b88du,gp[esp]+0x14u,4,0);
    }
    void keep_block(std::uint32_t block){this->gp[eax]=block;this->store(0x10b8a0u,this->gp[esp]+0x14u,4,block);}
    // MOV r,[ESP+18]; TEST r,r (EBX in the power's report, else EAX).
    std::uint32_t block() {
        auto &gp=this->gp;
        std::uint32_t pc;unsigned reg=eax;
        switch(this->previous) {
        case 0x10bbe8u:pc=0x10bbedu;break;
        case 0x10bcb1u:case 0x10bccfu:pc=0x10bce0u;break;
        case 0x10bd58u:pc=0x10bd5du;reg=ebx;break;
        default:pc=0x10b9bbu;break;
        }
        gp[reg]=this->read(pc,gp[esp]+0x18u,4);this->test32(gp[reg]);
        return gp[reg];
    }
    // MOV r,[ESP+14] at each site: the register is the original's.
    std::uint32_t definition() {
        auto &gp=this->gp;
        std::uint32_t pc;unsigned reg;
        switch(this->previous) {
        case 0x10b9e5u:pc=0x10b9e9u;reg=edx;break;
        case 0x10ba95u:pc=0x10ba9bu;reg=eax;break;
        case 0x10bad1u:pc=0x10badeu;reg=ecx;break;
        case 0x10bb33u:pc=0x10bb37u;reg=edx;break;
        case 0x10bb60u:if(gp[eax]&0xffu){pc=0x10bb68u;reg=ecx;}else {pc=0x10bb74u;reg=edx;}break;
        case 0x10bba2u:if(this->above()){pc=0x10bbabu;reg=eax;}else {pc=0x10bbb7u;reg=ecx;}break;
        case 0x10bd11u:pc=0x10bd14u;reg=ecx;break;
        default:pc=0x10bd01u;reg=edx;break;
        }
        gp[reg]=this->read(pc,gp[esp]+0x14u,4);
        return gp[reg];
    }
    std::uint32_t group(){auto &gp=this->gp;gp[esi]=this->read(0x10b9e5u,gp[esp]+0x24u,4);return gp[esi];}
    std::uint32_t row() {
        auto &gp=this->gp;
        // MOV EBX,[ESP+1C] after the position (whose instructions all precede
        // the power's) or after the power.
        const auto pc=this->previous<0x10bc25u?0x10bc1cu:0x10bd8eu;
        gp[ebx]=this->read(pc,gp[esp]+0x1cu,4);
        return gp[ebx];
    }
    void mark_changed() {
        std::uint32_t pc;
        switch(this->previous) {
        case 0x10b932u:case 0x10b94au:case 0x10bc1cu:pc=0x10bc20u;break;
        case 0x10bc74u:pc=0x10bc79u;break;
        default:pc=0x10bd89u;break;
        }
        this->store(pc,this->gp[esp]+0x13u,1,1);
    }
    // Before the exit pops: TEST AL,AL ran unless the position group was NONE.
    std::uint8_t changed() {
        auto &gp=this->gp;
        if(this->previous==0x10bd9au || (this->previous==0x10bd9du && !this->flags.zf))this->test8(gp[eax]);
        set_low_byte(gp[eax],this->read(0x10be0au,gp[esp]+0xfu,1));
        return std::uint8_t(gp[eax]);
    }
    // 10CD70(handle, block, [ESP+20]) with EAX/ECX = the position's channel
    // and script state; the old position kept at [ESP+24].
    bool script_position(Kept &d,std::uint32_t before) {
        auto &gp=this->gp;
        gp[ecx]=gp[esp]+0x20u;this->push(0x10b8d9u,gp[ecx]);this->push(0x10b8dau,gp[edx]);
        gp[eax]=gp[ebp]+0x18cu;gp[ecx]=gp[ebp]+0x14cu;keep(d,scripted);this->push(0x10b8e7u,gp[esi]);
        this->store(0x10b8e8u,gp[esp]+0x30u,4,before);
        call(d,scripted,0x10b8eeu,0x10b8f3u,0x10cd70u,children.script);
        return (gp[eax]&0xffu)!=0;
    }
    std::uint32_t scripted_position(){const auto v=this->read(0x10b8f3u,this->gp[esp]+0x20u,4);this->x()[0]=scalar(v);return v;}
    std::uint32_t script_start() {
        const auto v=this->read(0x10b907u,this->gp[esp]+0x24u,4);this->x()[0]=subtract(this->x()[0],scalar(v));return v;
    }
    // 10C5D0(handle): the handle pushed by the flag store.
    void report_script_position(Kept &d) {
        const bool done=this->previous==0x10b92cu;
        call(d,rowed,done?0x10b932u:0x10b94au,done?0x10b937u:0x10b94fu,0x10c5d0u,children.report);
    }
    void keep_group(std::uint32_t group){this->gp[esi]=group;this->store(0x10b98eu,this->gp[esp]+0x24u,4,group);}
    // PUSH ECX (the position); MOV ESI,EAX (the channel); 1CE8D0.
    void follow_position(Kept &d,std::uint32_t position) {
        auto &gp=this->gp;keep(d,following);gp[ecx]=position;this->push(0x10b9ddu,gp[ecx]);gp[esi]=gp[eax];
        call(d,following,0x10b9e0u,0x10b9e5u,0x1ce8d0u,children.frame);
    }
    void keep_motion(std::uint32_t acceleration,std::uint32_t speed) {
        auto &gp=this->gp;
        this->store(0x10ba51u,gp[esp]+0x28u,4,acceleration);this->store(0x10ba57u,gp[esp]+0x24u,4,speed);
    }
    // MOV byte [ESP+13],1, then 0 unless up; COMISS XMM0,XMM1 and XMM5,XMM1.
    void keep_direction(bool up) {
        auto &gp=this->gp;auto *x=this->x();
        this->store(0x10ba5du,gp[esp]+0x13u,1,1);
        if(!up)this->store(0x10ba64u,gp[esp]+0x13u,1,0);
        this->compare(x[0],x[1],false);
        if(this->above()) {
            this->compare(x[5],x[1],false);
            this->mark(this->above()?p_accelerating:p_no_speed);
        } else this->mark(p_no_acceleration);
    }
    std::uint8_t direction() {
        auto &gp=this->gp;
        const auto pc=this->previous==0x10bb0cu?0x10bb18u:0x10bb60u;
        set_low_byte(gp[eax],this->read(pc,gp[esp]+0x13u,1));this->test8(gp[eax]);
        return std::uint8_t(gp[eax]);
    }
    std::uint32_t acceleration(){auto &gp=this->gp;gp[eax]=this->read(0x10bb33u,gp[esp]+0x28u,4);return gp[eax];}
    // MOV [ESP+20],EAX (the old velocity).
    void keep_old_velocity(std::uint32_t velocity){this->gp[eax]=velocity;this->store(0x10bafcu,this->gp[esp]+0x20u,4,velocity);}
    // FLD [ESP+20] (with FABS), or, after the step, MOVAPS XMM2,XMM0; MOVSS
    // XMM0,[ESP+20]; MULSS XMM2,XMM0; COMISS XMM1,XMM2.
    std::uint32_t old_velocity() {
        auto &gp=this->gp;auto *x=this->x();
        if(this->previous==0x10bafcu){this->clear_c1();return this->read(0x10bb00u,gp[esp]+0x20u,4);}
        x[2]=x[0];const auto v=this->read(0x10bb93u,gp[esp]+0x20u,4);x[0]=scalar(v);
        x[2]=multiply(x[2],x[0]);this->compare(x[1],x[2],false);
        return v;
    }
    void keep_old_position(std::uint32_t position){this->store(0x10bb06u,this->gp[esp]+0x2cu,4,position);}
    std::uint32_t old_position(){const auto v=this->read(0x10bbcau,this->gp[esp]+0x2cu,4);this->x()[0]=scalar(v);return v;}
    // FLD [ESP+24]; FXCH; FCOMIP; FSTP (C1 clear).
    std::uint32_t speed(){this->clear_c1();return this->read(0x10bb0cu,this->gp[esp]+0x24u,4);}
    // PUSH ECX (tick); AND DL,1; PUSH EAX (target); MOV ESI,EBX; MOV [ESP+38],ECX;
    // 121890 with EDI = the position, ESI = the velocity, XMM5 = the speed.
    bool step_position(Kept &d,std::uint32_t target,std::uint32_t tick,std::uint32_t,bool loops) {
        auto &gp=this->gp;
        gp[ecx]=tick;gp[eax]=target;
        this->push(0x10bb4cu,gp[ecx]);set_low_byte(gp[edx],this->flags.logic(loops?1u:0u,1));this->push(0x10bb50u,gp[eax]);
        keep(d,stepping);gp[esi]=gp[ebx];this->store(0x10bb53u,gp[esp]+0x38u,4,gp[ecx]);
        call(d,stepping,0x10bb57u,0x10bb5cu,0x121890u,children.step);
        this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    // PUSH 1.0; PUSH 0; PUSH EAX (acceleration) happened at the target; PUSH
    // EDX (tick); PUSH EAX (target); LEA ESI,[EBP+138]; XOR DL,DL; MOV
    // [ESP+40],XMM0 (the old power); 121890.
    void step_power(Kept &d,std::uint32_t target,std::uint32_t tick,std::uint32_t,std::uint32_t before,std::uint32_t) {
        auto &gp=this->gp;
        gp[edx]=tick;gp[eax]=target;
        this->push(0x10bd2fu,gp[edx]);this->push(0x10bd30u,gp[eax]);keep(d,powered);gp[esi]=gp[ebp]+0x138u;
        set_low_byte(gp[edx],this->flags.logic(0,1));this->store(0x10bd39u,gp[esp]+0x40u,4,before);
        call(d,powered,0x10bd3fu,0x10bd44u,0x121890u,children.step);
    }
    std::uint32_t old_power(){const auto v=this->read(0x10bd44u,this->gp[esp]+0x2cu,4);this->x()[0]=scalar(v);return v;}
    // MOV EDX,[ESP+34]; 10CA30 with EAX = the effect, ECX/EDX as the original leaves them.
    void play_effect(Kept &d,std::uint32_t effect) {
        auto &gp=this->gp;gp[eax]=effect;
        if(this->previous==0x10bae2u) {
            gp[edx]=this->read(0x10bae8u,gp[esp]+0x34u,4);
            call(d,waiting,0x10baecu,0x10baf1u,0x10ca30u,children.effect);
        } else {
            gp[edx]=this->read(0x10bbc1u,gp[esp]+0x34u,4);
            call(d,stepping,0x10bbc5u,0x10bbcau,0x10ca30u,children.effect);
        }
    }
    // MOV EBX,[ESP+34]; PUSH EBX; 10C5D0.
    void report_position(Kept &d) {
        auto &gp=this->gp;gp[ebx]=this->read(0x10bbe3u,gp[esp]+0x34u,4);this->push(0x10bbe7u,gp[ebx]);
        call(d,valued,0x10bbe8u,0x10bbedu,0x10c5d0u,children.report);
    }
    // PUSH EBX (handle); PUSH C1630; PUSH EAX (block); PUSH EDX (position); 1CEBD0.
    void advance_position(Kept &d,std::uint32_t position,std::uint32_t block) {
        auto &gp=this->gp;gp[edx]=position;gp[eax]=block;
        this->push(0x10bc0fu,gp[ebx]);this->push(0x10bc10u,0xc1630u);this->push(0x10bc15u,gp[eax]);this->push(0x10bc16u,gp[edx]);
        call(d,valued,0x10bc17u,0x10bc1cu,0x1cebd0u,children.advance);
    }
    // 10CD70(handle, block, 0) with XMM0 = 0 and EAX/ECX = the power's channel and script state.
    bool script_power(Kept &d) {
        auto &gp=this->gp;
        gp[ecx]=this->read(0x10bc37u,gp[esp]+0x18u,4);gp[edx]=this->read(0x10bc3bu,gp[esp]+0x34u,4);this->x()[0]=_mm_setzero_ps();
        this->push(0x10bc42u,0);this->push(0x10bc44u,gp[ecx]);gp[eax]=gp[ebp]+0x1acu;gp[ecx]=gp[ebp]+0x16cu;this->push(0x10bc51u,gp[edx]);
        call(d,rowed,0x10bc52u,0x10bc57u,0x10cd70u,children.script);
        this->test8(gp[eax]);script_done=(gp[eax]&0xffu)!=0;
        this->mark(script_done?q_script_finished:q_script_continues);
        return script_done;
    }
    void report_script_power(Kept &d) {
        auto &gp=this->gp;gp[eax]=this->read(0x10bc6fu,gp[esp]+0x34u,4);this->push(0x10bc73u,gp[eax]);
        call(d,rowed,0x10bc74u,0x10bc79u,0x10c5d0u,children.report);
    }
    // LEA ESI,[EBP+1AC]; MOV EAX,ESI; B8760 (dispatch); TEST AL,AL.
    bool channel_valid(Kept &d) {
        auto &gp=this->gp;
        const bool first=this->previous==0x10bce0u;
        const Seats seats=first?powered:valued;
        keep(d,seats);gp[esi]=gp[ebp]+0x1acu;gp[eax]=gp[esi];
        routed(d,seats,first?0x10bcf0u:0x10bd6du,first?0x10bcf5u:0x10bd72u,0xb8760u);
        this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    // PUSH ECX (the power); 1CE8D0 (ESI as B8760 left it).
    void follow_power(Kept &d,std::uint32_t power) {
        this->gp[ecx]=power;this->push(0x10bcfbu,this->gp[ecx]);call(d,powered,0x10bcfcu,0x10bd01u,0x1ce8d0u,children.frame);
    }
    // MOV EDX,[ESP+34]; PUSH EDX; 10C5D0.
    void report_power(Kept &d) {
        auto &gp=this->gp;gp[edx]=this->read(0x10bd53u,gp[esp]+0x34u,4);this->push(0x10bd57u,gp[edx]);
        call(d,powered,0x10bd58u,0x10bd5du,0x10c5d0u,children.report);
    }
    // PUSH EAX (handle); PUSH C1630; PUSH EBX (block); PUSH ECX (power); 1CEBD0.
    // The block pushed is EBX as B8760 left it.
    void advance_power(Kept &d,std::uint32_t power,std::uint32_t) {
        auto &gp=this->gp;gp[ecx]=power;
        this->push(0x10bd7cu,gp[eax]);this->push(0x10bd7du,0xc1630u);this->push(0x10bd82u,gp[ebx]);this->push(0x10bd83u,gp[ecx]);
        call(d,valued,0x10bd84u,0x10bd89u,0x1cebd0u,children.advance);
    }
    // BF050(handle) (dispatch), then 1181F0(1) with EAX = the handle.
    void notify_machine(Kept &d) {
        auto &gp=this->gp;
        gp[eax]=this->read(0x10bdc2u,gp[esp]+0x30u,4);this->push(0x10bdc6u,gp[eax]);
        routed(d,rowed,0x10bdc7u,0x10bdccu,0xbf050u);
        gp[eax]=this->read(0x10bdccu,gp[esp]+0x30u,4);this->push(0x10bdd0u,1);
        call(d,rowed,0x10bdd2u,0x10bdd7u,0x1181f0u,children.attach);
    }
    // CMP dword [ESP+30],-1.
    std::uint32_t handle_argument() {
        const auto v=this->read(0x10bdd7u,this->gp[esp]+0x30u,4);this->compare32(v,logic::none);return v;
    }
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0x20u,4);
        Memory<DeviceBoundary> memory{*this};
        const auto result=logic::device_update::run(memory,*this);
        std::uint32_t last;
        if(result.exit==logic::device_update::Exit::initialized) {
            gp[esi]=this->pop(0x10bdffu);gp[ebp]=this->pop(0x10be00u);set_low_byte(gp[eax],result.value);gp[ebx]=this->pop(0x10be03u);
            last=0x10be07u;
        } else {
            gp[esi]=this->pop(0x10be0eu);gp[ebp]=this->pop(0x10be0fu);gp[ebx]=this->pop(0x10be10u);last=0x10be14u;
        }
        gp[esp]=this->flags.plus(gp[esp],0x20u,4);
        this->ret(last,4);this->publish(last);
    }
};

// ---- Verification summary, admission and bridges -------------------------------
inline std::array<std::atomic<std::uint64_t>,2> &machine_coverage(){static std::array<std::atomic<std::uint64_t>,2> value{};return value;}
inline std::array<std::atomic<std::uint64_t>,2> &device_coverage(){static std::array<std::atomic<std::uint64_t>,2> value{};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls[2]{};
    ~VerifySummary() {
        if(calls[0] || calls[1])
            std::fprintf(stderr,"Device update verify: 1179C0 %llu, 10B840 %llu calls; %llu ordered accesses (%llu stores) and "
                "%llu child boundaries identical, coverage %llx:%llx/%llx:%llx\n",
                (unsigned long long)calls[0].load(),(unsigned long long)calls[1].load(),(unsigned long long)accesses.load(),
                (unsigned long long)stores.load(),(unsigned long long)children.load(),
                (unsigned long long)machine_coverage()[1].load(),(unsigned long long)machine_coverage()[0].load(),
                (unsigned long long)device_coverage()[1].load(),(unsigned long long)device_coverage()[0].load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// Admission: the x87 work needs two free logical slots, x87 (not MMX)
// ownership and masked x87 exceptions; the SSE work masked MXCSR exceptions.
template<class State> bool floating_point_ready(const State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count+2u>8u)return false;
    std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
    return (control&0x3fu)==0x3fu && (_mm_getcsr()&0x1f80u)==0x1f80u;
#else
    (void)s;return false;
#endif
}
template<class State> bool admit(const State &s,std::uint32_t first,std::uint32_t last,std::uint32_t *&cursor) {
    return floating_point_ready(s) && native_object_tick_detail::admitted(s,first,last,cursor,reference_mode());
}
// Publish a closure's coverage words.
template<class Boundary> struct Covered : Boundary {
    using Boundary::Boundary;
    std::array<std::atomic<std::uint64_t>,2> *sink{};
    ~Covered(){if(sink)for(unsigned i=0;i<2;++i)(*sink)[i].fetch_or(this->covered[i],std::memory_order_relaxed);}
};
template<class State,class Reference,class Dispatch,class Cluster,class Frame,class Advance,class Report,class Query,class Marker,
         class Group,class Attach,class Moved,class Unmoved>
bool bridge_1179c0(State &s,Reference reference,Dispatch dispatch,Cluster cluster,Frame frame,Advance advance,Report report,
                   Query query,Marker marker,Group group,Attach attach,Moved moved,Unmoved unmoved) {
    std::uint32_t *cursor;if(!admit(s,0x1179c0u,0x11811cu,cursor))return false;
    using Children=MachineChildren<Dispatch,Cluster,Frame,Advance,Report,Query,Marker,Group,Attach,Moved,Unmoved>;
    Children children{dispatch,cluster,frame,advance,report,query,marker,group,attach,moved,unmoved};
    std::atomic<std::uint64_t> unused{0};
    native_object_tick_detail::execute(s,cursor,verify_mode(),unused,summary().calls[0],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            Covered<MachineBoundary<State,Children>> closure(v,c,t,children);closure.sink=&machine_coverage();return closure;});
    return true;
}
template<class State,class Reference,class Dispatch,class Script,class Report,class Frame,class Effect,class Step,class Advance,
         class Attach>
bool bridge_10b840(State &s,Reference reference,Dispatch dispatch,Script script,Report report,Frame frame,Effect effect,Step step,
                   Advance advance,Attach attach) {
    std::uint32_t *cursor;if(!admit(s,0x10b840u,0x10be14u,cursor))return false;
    using Children=DeviceChildren<Dispatch,Script,Report,Frame,Effect,Step,Advance,Attach>;
    Children children{dispatch,script,report,frame,effect,step,advance,attach};
    std::atomic<std::uint64_t> unused{0};
    native_object_tick_detail::execute(s,cursor,verify_mode(),unused,summary().calls[1],summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            Covered<DeviceBoundary<State,Children>> closure(v,c,t,children);closure.sink=&device_coverage();return closure;});
    return true;
}
} // namespace halo2::native_device_update_detail
