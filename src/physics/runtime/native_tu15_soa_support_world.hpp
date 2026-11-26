#pragma once
// Observed World 3077F0 uses ECX for its shape and takes direction/output
// pointers as stack arguments. Keep the canonical observer and TU1.5 cursor
// conventions on the world leaf boundary.
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_soa_support_world_systems.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_soa_support_world_detail {
namespace world=halo2::native_world_leaves_detail;
namespace logic=halo2::native_soa_support_world;
namespace fp_math=halo2::physics_step_detail;
using logic::Access;
using logic::Compare;
using logic::Search;
using world::eax;
using world::ecx;
using world::edx;
using world::ebx;
using world::esp;
using world::ebp;
using world::esi;
using world::edi;

inline constexpr world::Function functions[]{{0x3077f0u,0x307983u,0x4000u,0x4000u}};
enum Branch : unsigned {no_groups,scanned,unchanged,continued,ended,only_x,only_y,only_z,only_w,
    without_y,without_x,all_from_table,all_direct,winner_x,winner_y,winner_z,winner_w,branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{};return value;}
inline bool reference_mode() {
    static const bool value=[]{const auto *flag=std::getenv("HALO2_REFERENCE_SOA_SUPPORT_WORLD");return flag&&*flag;}();
    return value;
}
struct Summary {
    std::atomic<std::uint64_t> calls{};
    ~Summary(){
        if(calls)std::fprintf(stderr,"World SoA support verify: %llu calls; coverage %llx\n",
            (unsigned long long)calls.load(),(unsigned long long)coverage().load());
    }
};
inline Summary &summary(){static Summary value;return value;}

inline long double load_operand(std::uint32_t bits) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return halo2::native_live_support::x87::load_single(bits);
#else
    return fp_math::ext(fp_math::f(bits));
#endif
}

inline constexpr std::uint32_t sites[]{
    0x3077fau,0x3077ffu,0x307802u,0x30780au,0x307823u,0x307826u,0x30782cu,0x307835u,
    0x307828u,0x307838u,0x307842u,0x30782fu,0x30783cu,0x307846u,0x307860u,0x30786au,0x307874u,
    0x307892u,0x3078a2u,0x3078a9u,0x30792bu,0x30792fu,0x30794du,0x307962u,0x307967u,0x30796eu,
    0x307965u,0x30796bu,0x307972u,0x30797bu,
};
static_assert(std::size(sites)==unsigned(Access::count_access));
inline constexpr std::uint32_t left_sites[]{0x3078b0u,0x3078bfu,0x3078ceu,0x3078e6u,0x3078f5u,0x30790du};
inline constexpr std::uint32_t winner_sites[]{0x3078ddu,0x307904u,0x30791cu,0x307925u};

template<class State,class Resolve>
struct Frame {
    world::Port<State> &port;
    Resolve &canonical;
    std::uint64_t hit{};

    std::uint32_t instruction(Access access,std::uint32_t)const{return sites[unsigned(access)];}
    void loaded(Access access,std::uint32_t value) {
        auto &gp=port.gp;
        using enum Access;
        switch(access) {
        case coordinates:gp[ebx]=value;break;
        case count:case vertex_x:case vertex_y:gp[edi]=value;break;
        case direction_arg:case direction_z:case vertex_z:gp[ecx]=value;break;
        case direction_x:case direction_y:case mask_code:gp[edx]=value;break;
        case output_arg:gp[esi]=value;break;
        default:break;
        }
    }
    void loaded_vector(Access,__m128 value){port.set_xmm(0u,value);}
    void reached(unsigned branch){hit|=std::uint64_t(1)<<branch;}
    std::uint32_t best_slot()const{return port.gp[esp]+0x0cu;}
    std::uint32_t dots_slot()const{return port.gp[esp]+0x10u;}
    std::uint32_t direction_slot()const{return port.gp[ebp]+8u;}
    std::uint32_t output_slot()const{return port.gp[ebp]+0x0cu;}
    void begin() {
        auto &gp=port.gp;
        auto &flags=port.flags;
        port.push(0x3077f0u,gp[ebp]);
        gp[ebp]=gp[esp];
        gp[esp]=flags.logic(gp[esp]&0xfffffff0u,4);
        gp[esp]=flags.minus(gp[esp],0x14u,4);
        port.push(0x3077f9u,gp[ebx]);
    }
    void save_scan_registers() {
        port.push(0x3077fdu,port.gp[esi]);
        port.push(0x3077feu,port.gp[edi]);
    }
    static __m128 scalar(std::uint32_t bits){return _mm_set_ss(fp_math::f(bits));}
    void seed(std::uint32_t bits){port.set_xmm(4u,scalar(bits));}
    bool begin_scan(std::uint32_t count,std::uint32_t groups) {
        auto &gp=port.gp;
        auto &flags=port.flags;
        gp[edi]=flags.shift(count,2u,4);
        gp[eax]=flags.logic(0u,4);
        flags.logic(gp[edi],4);
        broadcast_axis(4u);
        gp[esi]=groups;
        const bool scan=!flags.zf&&(flags.sf==flags.of);
        reached(scan?Branch::scanned:Branch::no_groups);
        return scan;
    }
    void load_axis(unsigned reg,std::uint32_t bits){port.set_xmm(reg,scalar(bits));}
    __m128 broadcast_axis(unsigned reg) {
        auto &xmm=port.s.xmm[reg];
        return xmm=native_packed_sse::broadcast<0>(xmm);
    }
    void begin_groups(){port.gp[ecx]=3u;}
    __m128 product(unsigned reg,__m128 direction,__m128 positions) {
        auto &xmm=port.s.xmm[reg];
        return xmm=native_packed_sse::multiply(direction,positions);
    }
    std::uint32_t improvements(__m128 dots) {
        auto &x=port.s.xmm;
        x[0]=dots;
        x[5]=_mm_cmplt_ps(x[4],dots);
        const auto mask=port.gp[edx]=std::uint32_t(_mm_movemask_ps(x[5]));
        port.flags.logic(mask,4);
        if(mask==0u)reached(Branch::unchanged);
        return mask;
    }
    bool uses_table(std::uint32_t mask) {
        port.flags.minus(mask,14u,4);
        if(port.flags.cf||port.flags.zf)return true;
        reached(Branch::all_direct);
        return false;
    }
    Search search_path(std::uint32_t target) {
        using enum Search;
        Search path;
        switch(canonical(target)) {
        case 0x3038ddu:path=only_x;break;
        case 0x303904u:path=only_y;break;
        case 0x30391cu:path=only_z;break;
        case 0x303925u:path=only_w;break;
        case 0x3038bfu:path=without_y;break;
        case 0x3038e6u:path=without_x;break;
        case 0x3038b0u:path=all;break;
        default:throw std::runtime_error("Required original solver dispatch target unavailable");
        }
        reached(unsigned(Branch::only_x)+unsigned(path));
        return path;
    }
    bool greater(Compare comparison,unsigned lhs,unsigned rhs) {
        auto &fp=port.s.fp;
        const auto slot=dots_slot(),site=left_sites[unsigned(comparison)];
        fp.push(load_operand(port.read(site,slot+4u*lhs,4u)));
        const auto right=port.read(site+4u,slot+4u*rhs,4u);
        fp.comparison(load_operand(right),1u);
        auto &accumulator=port.gp[eax];
        accumulator=(accumulator&0xffff0000u)|fp.word();
        port.flags.logic((accumulator>>8u)&0x41u,1u);
        return port.flags.zf;
    }
    std::uint32_t winner(unsigned lane,std::uint32_t vertex) {
        auto &fp=port.s.fp;
        fp.push(load_operand(port.read(winner_sites[lane],dots_slot()+4u*lane,4u)));
        port.gp[eax]=vertex;
        reached(unsigned(Branch::winner_x)+lane);
        return fp_math::u(fp_math::spill(fp.st(0)));
    }
    void pop_winner(){port.s.fp.pop();}
    void broadcast_best(std::uint32_t bits) {
        port.set_xmm(0u,scalar(bits));
        port.set_xmm(4u,broadcast_axis(0u));
    }
    bool next_group(std::uint32_t first) {
        auto &gp=port.gp;
        auto &flags=port.flags;
        gp[ecx]=flags.plus(gp[ecx],logic::lanes,4u);
        gp[edx]=first;
        gp[esi]=flags.plus(gp[esi],logic::group_stride,4u);
        flags.minus(gp[edx],gp[edi],4u);
        const bool next=flags.sf!=flags.of;
        reached(next?Branch::continued:Branch::ended);
        return next;
    }
    std::uint32_t point_address(std::uint32_t groups,std::uint32_t vertex) {
        auto &gp=port.gp;
        auto &flags=port.flags;
        const auto slab=flags.right(vertex,2u,4u,true);
        gp[ecx]=flags.plus(flags.shift(3u*slab,4u,4u),groups,4u);
        gp[edx]=flags.logic(vertex&3u,4u);
        return gp[ecx]+gp[edx]*4u;
    }
    std::uint32_t feature(std::uint32_t value) {
        port.gp[eax]=port.flags.logic(value,4u);
        port.gp[edi]=port.pop(0x30797au);
        return port.gp[eax];
    }
    void finish() {
        auto &gp=port.gp;
        gp[esi]=port.pop(0x30797eu);
        gp[ebx]=port.pop(0x30797fu);
        gp[esp]=gp[ebp];
        gp[ebp]=port.pop(0x307982u);
        port.ret(0x307983u,8u);
        port.publish(0x307983u);
    }
};

template<class State,class Reference,class Resolve>
bool bridge(State &state,Reference reference,Resolve canonical) {
    if(reference_mode())return false;
    if constexpr(requires{state.active_mmx;})if(state.active_mmx)return false;
    world::Publication publication;
    if(!world::admitted(state,functions,publication))return false;
    world::execute(state,publication,summary().calls,reference,[&](world::Port<State> &port) {
        Frame<State,Resolve> frame{port,canonical};
        world::Memory<world::Port<State>,Frame<State,Resolve>> memory{port,frame};
        frame.begin();
        logic::find(memory,frame,port.gp[ecx]);
        frame.finish();
        coverage().fetch_or(frame.hit,std::memory_order_relaxed);
    });
    return true;
}
} // namespace halo2::native_soa_support_world_detail
