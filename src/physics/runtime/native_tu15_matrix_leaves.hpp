#pragma once
// Native kernels for TU1.5 transform leaves of the literal live-support
// unit (installed into the build-local specialized copy by
// tools/specialize_native_operands.py, bound to the exact original bodies):
//   145560  compose two real_matrix4x3 (scale, forward/left/up, position)
//   144CF0  real_orientation (quaternion, translation, scale) -> real_matrix4x3
//   2E44D0  compose two Havok transforms (four vectors, ordered in-place rows)
//   2E46E0  inverse-transform a Havok point through the transposed rotation
// Each kernel performs the original's reads and stores in the same order, at
// the same addresses and widths, through the same store helper (so guards,
// observers and the store observer see the identical store sequence), makes
// the same stack pushes/pops, and leaves the same general, vector and flag
// registers. SSE arithmetic uses the same host instructions in the same
// order, so every rounding and MXCSR sticky flag is identical. Only the
// per-instruction delivery is elided: the kernel is admitted when that
// delivery is a literal cursor store with no active trace (or absent), and it
// publishes each memory instruction's address to the cursor before the access
// (so a faulting access reports the original instruction) and the final
// instruction's address at exit.
// HALO2_REFERENCE_MATRIX_LEAVES=1 runs the original bodies.
// HALO2_VERIFY_MATRIX_LEAVES=1 computes each admitted call in a pure overlay,
// re-enters the same routine with the native route bypassed so the original
// body runs, then requires identical stored bytes, general registers, vector
// registers and flags.
#include "physics/runtime/physics_step_internal.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <span>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>

namespace halo2::native_tu15_leaves {
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_MATRIX_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_MATRIX_LEAVES");return p && *p;}();
    return value;
}
inline float as_float(std::uint32_t x){return std::bit_cast<float>(x);}
inline std::uint32_t as_bits(float x){return std::bit_cast<std::uint32_t>(x);}
inline std::uint32_t low_bits(__m128 x){return as_bits(_mm_cvtss_f32(x));}
inline std::uint64_t half_bits(__m128 x,unsigned offset) {
    std::uint64_t bits;std::memcpy(&bits,reinterpret_cast<const std::byte*>(&x)+offset,8);return bits;
}
inline void require_vector_alignment(std::uint32_t address) {
    if(address&15u)throw std::runtime_error("Original aligned-vector operand is unaligned");
}
// Authenticated MOVAPS/XORPS memory operands require alignment even when the
// retained bank happens to permit byte accesses. Check before any bank effect.
inline __m128 aligned_v(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    require_vector_alignment(address);return physics_step_detail::v(memory,address);
}
inline void aligned_vw(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t pc,std::uint32_t address,__m128 value) {
    require_vector_alignment(address);physics_step_detail::vw(memory,observe,pc,address,value);
}

// Architectural state the kernels read and write. Flags is the unit's flag type.
template<class Flags> struct Machine {
    std::array<std::uint32_t,8> gp;
    __m128 x[8];
    Flags flags;
    std::uint32_t other_flags{};
    bool reset_x87{};
    bool reset_x87_top{};
    bool clear_x87_c1{};
    std::uint16_t x87_c1{};
    // Integer work can defer flag publication until an exception boundary.
    // Native successful paths keep ordinary arithmetic; their final flags
    // remain explicit kernel outputs. No source instructions are replayed.
    enum class FaultFlagKind { none, logic, plus, minus, shift, decrement } fault_kind{};
    std::uint32_t fault_a{},fault_b{};
    unsigned fault_width=4;
    bool fault_overflow{};
    bool fault_carry{};
    void defer_logic(std::uint32_t value,unsigned width=4) {
        fault_kind=FaultFlagKind::logic;fault_a=value;fault_width=width;
    }
    void defer_plus(std::uint32_t a,std::uint32_t b,unsigned width=4) {
        fault_kind=FaultFlagKind::plus;fault_a=a;fault_b=b;fault_width=width;
    }
    void defer_minus(std::uint32_t a,std::uint32_t b,unsigned width=4) {
        fault_kind=FaultFlagKind::minus;fault_a=a;fault_b=b;fault_width=width;
    }
    void defer_shift(std::uint32_t a,unsigned count,bool overflow,unsigned width=4) {
        fault_kind=FaultFlagKind::shift;fault_a=a;fault_b=count;fault_overflow=overflow;fault_width=width;
    }
    void defer_decrement(std::uint32_t value,bool carry,unsigned width=4) {
        fault_kind=FaultFlagKind::decrement;fault_a=value;fault_width=width;fault_carry=carry;
    }
    void clear_fault_flags(){fault_kind=FaultFlagKind::none;}
    void materialize_fault_flags() {
        switch(fault_kind) {
        case FaultFlagKind::logic:flags.logic(fault_a,fault_width);break;
        case FaultFlagKind::plus:flags.plus(fault_a,fault_b,fault_width);break;
        case FaultFlagKind::minus:flags.minus(fault_a,fault_b,fault_width);break;
        case FaultFlagKind::shift:flags.of=fault_overflow;flags.shift(fault_a,fault_b,fault_width);break;
        case FaultFlagKind::decrement:flags.minus(fault_a,1,fault_width);flags.cf=fault_carry;break;
        default:break;
        }
        clear_fault_flags();
    }
};

// The original store/load primitives on the live bank.
template<class State> struct RealIo {
    State &s;
    std::uint32_t *pc{};
    void at(std::uint32_t p){if(pc)*pc=p;}
    std::uint32_t rd(std::uint32_t p,std::uint32_t a,unsigned n){at(p);return physics_step_detail::r(s.memory,a,n);}
    __m128 half(std::uint32_t p,std::uint32_t a,__m128 x,unsigned offset) {
        // As the unit's native_load_half: an 8-byte read into one half.
        at(p);std::array<std::byte,16> bytes;std::memcpy(bytes.data(),&x,16);
        s.memory.read(a,std::span(bytes).subspan(offset,8));
        std::memcpy(&x,bytes.data(),16);return x;
    }
    __m128 vec(std::uint32_t p,std::uint32_t a){at(p);return aligned_v(s.memory,a);}
    void st(std::uint32_t p,std::uint32_t a,unsigned n,std::uint64_t x){at(p);physics_step_detail::w(s.memory,s.observe,p,a,n,x);}
};

// Pure overlay for verification: reads see earlier overlay stores, stores
// are recorded and never reach the bank.
template<class Memory> struct PureIo {
    const Memory &memory;
    std::map<std::uint32_t,std::byte> overlay;
    std::vector<std::pair<std::uint32_t,unsigned>> stores;
    void at(std::uint32_t){}
    std::byte byte(std::uint32_t a) {
        if(const auto at=overlay.find(a);at!=overlay.end())return at->second;
        return std::byte(physics_step_detail::r(memory,a,1));
    }
    std::uint32_t rd(std::uint32_t,std::uint32_t a,unsigned n) {
        std::uint32_t x=0;for(unsigned i=0;i<n;++i)x|=std::to_integer<std::uint32_t>(byte(a+i))<<(8*i);return x;
    }
    __m128 half(std::uint32_t,std::uint32_t a,__m128 x,unsigned offset) {
        std::array<std::byte,16> bytes;std::memcpy(bytes.data(),&x,16);
        for(unsigned i=0;i<8;++i)bytes[offset+i]=byte(a+i);
        std::memcpy(&x,bytes.data(),16);return x;
    }
    __m128 vec(std::uint32_t,std::uint32_t a) {
        require_vector_alignment(a);
        std::array<std::byte,16> bytes;for(unsigned i=0;i<16;++i)bytes[i]=byte(a+i);
        return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(bytes.data())));
    }
    void st(std::uint32_t,std::uint32_t a,unsigned n,std::uint64_t x) {
        for(unsigned i=0;i<n;++i)overlay[a+i]=std::byte((x>>(8*i))&255u);
        stores.emplace_back(a,n);
    }
};

template<class Io,class Flags> void push(Io &io,Machine<Flags> &m,std::uint32_t p,std::uint32_t value) {
    m.gp[4]-=4;io.st(p,m.gp[4],4,value);
}
template<class Io,class Flags> std::uint32_t pop(Io &io,Machine<Flags> &m,std::uint32_t p) {
    const auto x=io.rd(p,m.gp[4],4);m.gp[4]+=4;return x;
}
template<class Io> void store_vec(Io &io,std::uint32_t pc,std::uint32_t address,__m128 value) {
    io.at(pc);require_vector_alignment(address);
    io.st(pc,address,8,half_bits(value,0));io.st(pc,address+8u,8,half_bits(value,8));
}

// SSE's first operand owns the NaN payload when both operands are NaNs.
// Explicit destinations prevent commutative register allocation from swapping
// those payloads when the native expression no longer has instruction callbacks.
inline __m128 ordered_add_ps(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addps %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_add_ps(a,b);
#endif
}
inline __m128 ordered_mul_ps(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulps %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_mul_ps(a,b);
#endif
}

inline __m128 ordered_mul_ss(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_mul_ss(a,b);
#endif
}

// 145560: composes real_matrix4x3 B (EDX) with A (ECX) into [ESP+4]; RET 4.
template<class Io,class Flags> void kernel_145560(Io &io,Machine<Flags> &m) {
    auto &x=m.x;auto &gp=m.gp;
    const auto bcast=[](__m128 v){return _mm_shuffle_ps(v,v,0);};
    gp[0]=io.rd(0x145560u,0x4u+gp[4],4);
    const auto a=gp[1],b=gp[2];
    x[7]=_mm_set_ss(as_float(io.rd(0x145564u,a,4)));
    x[7]=bcast(x[7]);
    x[3]=_mm_set_ss(as_float(io.rd(0x14556cu,b,4)));
    x[0]=_mm_set_ss(as_float(io.rd(0x145570u,0x28u+b,4)));
    x[1]=_mm_set_ss(as_float(io.rd(0x145575u,0x2cu+b,4)));
    x[2]=_mm_set_ss(as_float(io.rd(0x14557au,0x30u+b,4)));
    x[0]=bcast(x[0]);x[1]=bcast(x[1]);x[2]=bcast(x[2]);
    // Some owners activate caller SSE controls lazily at the first arithmetic
    // operation. Their earlier memory faults must preserve the entry FP state.
    if constexpr(requires{io.begin_arithmetic(0x14558bu);})io.begin_arithmetic(0x14558bu);
    x[3]=ordered_mul_ss(x[3],x[7]);
    x[4]=_mm_set_ss(as_float(io.rd(0x14558fu,0x4u+a,4)));
    x[4]=io.half(0x145594u,0x8u+a,x[4],8);
    x[5]=_mm_set_ss(as_float(io.rd(0x145598u,0x10u+a,4)));
    x[5]=io.half(0x14559du,0x14u+a,x[5],8);
    x[6]=_mm_set_ss(as_float(io.rd(0x1455a1u,0x1cu+a,4)));
    x[6]=io.half(0x1455a6u,0x20u+a,x[6],8);
    x[0]=ordered_mul_ps(x[0],x[4]);x[1]=ordered_mul_ps(x[1],x[5]);x[2]=ordered_mul_ps(x[2],x[6]);
    x[0]=ordered_add_ps(x[0],x[1]);x[0]=ordered_add_ps(x[0],x[2]);x[0]=ordered_mul_ps(x[0],x[7]);
    x[1]=_mm_set_ss(as_float(io.rd(0x1455bcu,0x28u+a,4)));
    x[1]=io.half(0x1455c1u,0x2cu+a,x[1],8);
    x[0]=ordered_add_ps(x[0],x[1]);
    x[2]=_mm_set_ss(as_float(io.rd(0x1455c8u,0x24u+b,4)));
    const auto out=gp[0];
    io.st(0x1455cdu,0x28u+out,4,low_bits(x[0]));io.st(0x1455d2u,0x2cu+out,8,half_bits(x[0],8));
    // Each remaining row: three broadcast B coefficients times A's rows.
    const auto row=[&](std::uint32_t p1,std::uint32_t o1,std::uint32_t p0,std::uint32_t o0) {
        x[1]=_mm_set_ss(as_float(io.rd(p1,o1+b,4)));
        x[0]=_mm_set_ss(as_float(io.rd(p0,o0+b,4)));
        x[2]=bcast(x[2]);x[1]=bcast(x[1]);x[0]=bcast(x[0]);
        x[2]=ordered_mul_ps(x[2],x[6]);x[1]=ordered_mul_ps(x[1],x[5]);x[0]=ordered_mul_ps(x[0],x[4]);
        x[1]=ordered_add_ps(x[1],x[2]);x[0]=ordered_add_ps(x[0],x[1]);
    };
    row(0x1455d6u,0x20u,0x1455dbu,0x1cu);
    x[2]=_mm_set_ss(as_float(io.rd(0x1455fbu,0x18u+b,4)));
    io.st(0x145600u,0x1cu+out,4,low_bits(x[0]));io.st(0x145605u,0x20u+out,8,half_bits(x[0],8));
    row(0x145609u,0x14u,0x14560eu,0x10u);
    x[2]=_mm_set_ss(as_float(io.rd(0x14562eu,0xcu+b,4)));
    io.st(0x145633u,0x10u+out,4,low_bits(x[0]));io.st(0x145638u,0x14u+out,8,half_bits(x[0],8));
    row(0x14563cu,0x8u,0x145641u,0x4u);
    io.st(0x145661u,0x4u+out,4,low_bits(x[0]));io.st(0x145666u,0x8u+out,8,half_bits(x[0],8));
    io.st(0x14566au,out,4,low_bits(x[3]));
    pop(io,m,0x14566eu);gp[4]+=4u;
}

// 144CF0: real_orientation [EBP+0C] -> real_matrix4x3 [EBP+8]; RET 8. The
// epsilon global is initialized once, as in the original.
template<class Io,class Flags> void kernel_144cf0(Io &io,Machine<Flags> &m) {
    auto &x=m.x;auto &gp=m.gp;auto &flags=m.flags;
    const auto set=[&](unsigned i,std::uint32_t v,unsigned n) {
        const auto mask=Flags::mask(n);gp[i]=(gp[i]&~mask)|(v&mask);
    };
    push(io,m,0x144cf0u,gp[5]);
    gp[5]=gp[4];
    set(1,io.rd(0x144cf3u,0x56e684u,1),1);
    gp[0]=0x1u;
    flags.logic(gp[0]&gp[1]&255u,1);
    if(flags.zf) {
        gp[1]=io.rd(0x144d02u,0x56e684u,4);
        x[0]=_mm_set_ss(as_float(io.rd(0x144d08u,0x46a0ecu,4)));
        gp[1]=flags.logic(gp[1]|gp[0],4);
        io.st(0x144d12u,0x56e684u,4,gp[1]);
        io.st(0x144d18u,0x56e680u,4,low_bits(x[0]));
    }
    gp[0]=io.rd(0x144d20u,0xcu+gp[5],4);
    gp[1]=io.rd(0x144d23u,0x8u+gp[5],4);
    const auto q=gp[0],out=gp[1];
    x[0]=io.half(0x144d26u,q,x[0],0);
    x[0]=io.half(0x144d29u,0x8u+q,x[0],8);
    x[7]=x[0];
    x[0]=_mm_mul_ps(x[0],x[7]);
    x[1]=_mm_movehl_ps(x[1],x[0]);
    x[0]=_mm_add_ps(x[0],x[1]);
    x[1]=x[0];
    x[1]=_mm_shuffle_ps(x[1],x[1],85);
    x[0]=_mm_add_ss(x[0],x[1]);
    flags.scalar_compare(x[0],_mm_set_ss(as_float(io.rd(0x144d44u,0x56e680u,4))),false);
    if(!(!flags.cf && !flags.zf)) {
        x[0]=_mm_set_ss(as_float(io.rd(0x144d4du,0x46a0d0u,4)));
        io.st(0x144d55u,0x4u+out,8,half_bits(x[0],0));
        io.st(0x144d59u,0xcu+out,8,half_bits(x[0],8));
        io.st(0x144d5du,0x14u+out,8,half_bits(x[0],0));
        io.st(0x144d61u,0x1cu+out,8,half_bits(x[0],8));
        io.st(0x144d65u,0x24u+out,4,low_bits(x[0]));
    } else {
        x[1]=_mm_move_ss(x[1],_mm_rcp_ss(x[0]));
        x[0]=_mm_mul_ss(x[0],x[1]);
        x[0]=_mm_mul_ss(x[0],x[1]);
        x[1]=_mm_add_ss(x[1],x[1]);
        x[1]=_mm_sub_ss(x[1],x[0]);
        x[1]=_mm_add_ss(x[1],x[1]);
        x[1]=_mm_shuffle_ps(x[1],x[1],0);
        x[7]=_mm_mul_ps(x[7],x[1]);
        x[4]=_mm_set_ss(as_float(io.rd(0x144d8eu,q,4)));
        x[4]=_mm_shuffle_ps(x[4],x[4],0);
        x[4]=_mm_mul_ps(x[4],x[7]);
        x[5]=_mm_set_ss(as_float(io.rd(0x144d99u,0xcu+q,4)));
        x[5]=_mm_shuffle_ps(x[5],x[5],0);
        x[5]=_mm_mul_ps(x[5],x[7]);
        x[6]=_mm_set_ss(as_float(io.rd(0x144da5u,0x4u+q,4)));
        x[6]=_mm_shuffle_ps(x[6],x[6],0);
        x[6]=_mm_mul_ps(x[6],x[7]);
        x[7]=_mm_movehl_ps(x[7],x[7]);
        x[7]=_mm_mul_ss(x[7],_mm_set_ss(as_float(io.rd(0x144db4u,0x8u+q,4))));
        x[0]=x[4];
        x[0]=_mm_shuffle_ps(x[0],x[0],153);
        x[1]=x[5];
        x[1]=_mm_shuffle_ps(x[1],x[1],102);
        x[1]=_mm_xor_ps(x[1],io.vec(0x144dc7u,0x45f490u));
        x[0]=_mm_add_ps(x[0],x[1]);
        io.st(0x144dd1u,0x8u+out,8,half_bits(x[0],8));
        io.st(0x144dd5u,0x10u+out,4,low_bits(x[0]));
        io.st(0x144ddau,0x18u+out,8,half_bits(x[0],0));
        x[2]=_mm_movehl_ps(x[2],x[6]);
        x[6]=_mm_shuffle_ps(x[6],x[6],85);
        x[0]=_mm_set_ss(as_float(io.rd(0x144de5u,0x46a0d0u,4)));
        x[0]=_mm_sub_ss(x[0],x[7]);
        x[1]=_mm_move_ss(x[1],x[0]);
        x[0]=_mm_sub_ss(x[0],x[6]);
        io.st(0x144df9u,0x4u+out,4,low_bits(x[0]));
        x[1]=_mm_sub_ss(x[1],x[4]);
        io.st(0x144e02u,0x14u+out,4,low_bits(x[1]));
        x[3]=_mm_set_ss(as_float(io.rd(0x144e07u,0x46a0d0u,4)));
        x[3]=_mm_sub_ss(x[3],x[4]);
        x[3]=_mm_sub_ss(x[3],x[6]);
        io.st(0x144e17u,0x24u+out,4,low_bits(x[3]));
        x[7]=_mm_move_ss(x[7],x[2]);
        x[7]=_mm_sub_ss(x[7],x[5]);
        io.st(0x144e24u,0x20u+out,4,low_bits(x[7]));
        x[2]=_mm_add_ss(x[2],x[5]);
        io.st(0x144e2du,0x18u+out,4,low_bits(x[2]));
    }
    x[0]=_mm_set_ss(as_float(io.rd(0x144e32u,0x1cu+q,4)));
    io.st(0x144e37u,out,4,low_bits(x[0]));
    x[0]=io.half(0x144e3bu,0x10u+q,x[0],0);
    io.st(0x144e3fu,0x28u+out,8,half_bits(x[0],0));
    x[0]=_mm_set_ss(as_float(io.rd(0x144e43u,0x18u+q,4)));
    io.st(0x144e48u,0x30u+out,4,low_bits(x[0]));
    gp[5]=pop(io,m,0x144e4du);
    pop(io,m,0x144e4eu);gp[4]+=8u;
}

// 2E44D0: compose four-vector transforms [ESP+4], [ESP+8] into ECX;
// RET 8. Keep row delivery ordered: source and destination may overlap.
template<class Io,class Flags> void kernel_2e44d0(Io &io,Machine<Flags> &m) {
    auto &x=m.x;auto &gp=m.gp;
    push(io,m,0x2e44d0u,gp[5]);gp[5]=gp[4];
    m.defer_minus(gp[4]&~15u,8u);gp[4]=(gp[4]&~15u)-8u;
    gp[0]=io.rd(0x2e44d9u,gp[5]+12u,4);
    gp[2]=io.rd(0x2e44dcu,gp[5]+8u,4);
    push(io,m,0x2e44dfu,gp[6]);
    gp[6]=gp[1];push(io,m,0x2e44e2u,gp[7]);
    m.defer_minus(gp[6],gp[0]);gp[6]-=gp[0];gp[7]=4;
    for(unsigned row=0;row<4;++row) {
        x[0]=io.vec(0x2e44f0u,gp[0]);
        x[2]=io.vec(0x2e44f3u,gp[2]+32u);
        x[3]=io.vec(0x2e44f7u,gp[2]+16u);
        x[2]=ordered_mul_ps(x[2],_mm_shuffle_ps(x[0],x[0],170));
        x[3]=ordered_mul_ps(x[3],_mm_shuffle_ps(x[0],x[0],85));
        x[1]=_mm_shuffle_ps(x[0],x[0],0);
        x[0]=io.vec(0x2e4516u,gp[2]);
        x[0]=ordered_mul_ps(x[0],x[1]);
        x[0]=ordered_add_ps(ordered_add_ps(x[0],x[3]),x[2]);
        store_vec(io,0x2e4522u,gp[6]+gp[0],x[0]);
        gp[0]=m.flags.plus(gp[0],16u,4);
        m.defer_decrement(gp[7],m.flags.cf);--gp[7];
    }
    // DEC's final zero result preserves the carry from the last input step.
    m.flags.increment(1u,4,true);
    m.clear_fault_flags();
    x[0]=io.vec(0x2e452cu,gp[2]+48u);
    x[1]=io.vec(0x2e4530u,gp[1]+48u);
    gp[7]=pop(io,m,0x2e4534u);
    x[1]=ordered_add_ps(x[1],x[0]);
    store_vec(io,0x2e4538u,gp[1]+48u,x[1]);
    gp[6]=pop(io,m,0x2e453cu);gp[4]=gp[5];
    gp[5]=pop(io,m,0x2e453fu);pop(io,m,0x2e4540u);gp[4]+=8u;
}

// 2E46E0: inverse-transform point [ESP+8] by transform [ESP+4] into ECX;
// RET 8. The SSE grouping preserves signed zeros, NaN payloads and rounding.
template<class Io,class Flags> void kernel_2e46e0(Io &io,Machine<Flags> &m) {
    auto &x=m.x;auto &gp=m.gp;
    push(io,m,0x2e46e0u,gp[5]);gp[5]=gp[4];
    gp[4]=m.flags.logic(gp[4]&~15u,4);
    gp[0]=io.rd(0x2e46e6u,gp[5]+8u,4);
    x[3]=io.vec(0x2e46e9u,gp[0]+32u);
    x[1]=io.vec(0x2e46edu,gp[0]+48u);
    gp[2]=io.rd(0x2e46f1u,gp[5]+12u,4);
    x[0]=io.vec(0x2e46f4u,gp[2]);
    x[2]=io.vec(0x2e46f7u,gp[0]);
    x[5]=io.vec(0x2e46fau,gp[0]+16u);
    x[0]=_mm_sub_ps(x[0],x[1]);
    x[4]=_mm_shuffle_ps(x[3],x[3],68);
    x[7]=_mm_shuffle_ps(x[3],x[3],238);
    x[1]=_mm_shuffle_ps(x[2],x[5],68);
    x[2]=_mm_shuffle_ps(x[2],x[5],238);
    x[3]=_mm_shuffle_ps(x[0],x[0],85);
    x[5]=_mm_shuffle_ps(x[1],x[4],221);
    x[5]=ordered_mul_ps(x[5],x[3]);
    x[1]=_mm_shuffle_ps(x[1],x[4],136);
    x[3]=_mm_shuffle_ps(x[0],x[0],0);
    x[6]=_mm_shuffle_ps(x[0],x[0],170);
    x[1]=ordered_mul_ps(x[1],x[3]);
    x[2]=_mm_shuffle_ps(x[2],x[7],136);
    x[2]=ordered_mul_ps(x[2],x[6]);
    x[1]=ordered_add_ps(ordered_add_ps(x[1],x[5]),x[2]);
    store_vec(io,0x2e474du,gp[1],x[1]);
    gp[4]=gp[5];gp[5]=pop(io,m,0x2e4752u);
    pop(io,m,0x2e4753u);gp[4]+=8u;
}

inline bool same_bits(__m128 a,__m128 b){return !std::memcmp(&a,&b,16);}

// Verification re-enters the routine; the nested entry runs the original body.
inline bool &bypass(){thread_local bool value=false;return value;}
template<class State,class Flags> void publish_machine(State &s,const Machine<Flags> &m) {
    s.gp=m.gp;s.flags=m.flags;
    if(m.reset_x87) {
        if constexpr(requires{s.emms();})s.emms();
        else throw std::runtime_error("Native packed-edge leaf requires its MMX lifetime owner");
    }
    if(m.reset_x87_top)s.fp.top=0;
    if(m.clear_x87_c1)s.fp.status=(s.fp.status&~0x200u)|m.x87_c1;
    for(unsigned i=0;i<8;++i)s.xmm[i]=m.x[i];
}
// Admission and dispatch. Reference re-enters the routine (verify mode only).
template<class State,class Kernel,class Reference>
bool run(State &s,Kernel kernel,Reference reference,std::uint32_t final_pc) {
    if(reference_mode() || bypass() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    std::uint32_t *pc=nullptr;
    if(s.services.instruction) {
        if constexpr(requires{s.literal_service_sites;s.instruction_cursor;}) {
            if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return false;
            const auto *cursor=s.instruction_cursor;
            if(s.observe && !cursor->observation_only_stores)return false;
            if((cursor->first_trace && *cursor->first_trace) || (cursor->second_trace && *cursor->second_trace))return false;
            pc=cursor->position;
        } else return false;
    }
    using Flags=std::remove_cvref_t<decltype(s.flags)>;
    Machine<Flags> m{s.gp,{},s.flags};
    if constexpr(requires{s.other_flags;})m.other_flags=s.other_flags;
    for(unsigned i=0;i<8;++i)m.x[i]=s.xmm[i];
    if(verify_mode()) {
        PureIo<std::remove_cvref_t<decltype(s.memory)>> io{s.memory,{},{}};
        kernel(io,m);
        struct Bypass{Bypass(){bypass()=true;}~Bypass(){bypass()=false;}};
        {Bypass held;reference(s);}
        bool same=m.gp==s.gp && m.flags.cf==s.flags.cf && m.flags.zf==s.flags.zf && m.flags.sf==s.flags.sf &&
            m.flags.of==s.flags.of && m.flags.pf==s.flags.pf;
        if constexpr(requires{m.flags.af;})same=same && m.flags.af==s.flags.af;
        if(m.reset_x87) {
            same=same && s.fp.count==0;
            if(m.reset_x87_top)same=same && s.fp.top==0;
            if constexpr(requires{s.active_mmx;})same=same && !s.active_mmx;
        }
        if(m.clear_x87_c1)same=same && (s.fp.status&0x200u)==m.x87_c1;
        for(unsigned i=0;i<8;++i)same=same && same_bits(m.x[i],s.xmm[i]);
        for(const auto &[address,bytes]:io.stores)for(unsigned i=0;i<bytes;++i)
            same=same && io.overlay.at(address+i)==std::byte(physics_step_detail::r(s.memory,address+i,1));
        if(!same)throw std::runtime_error("Native TU1.5 matrix leaf differs from its original body");
        return true;
    }
    RealIo<State> io{s,pc};
    try {kernel(io,m);}
    catch(...) {
        // Retained-memory guards publish the partial caller ABI on failure.
        // Preserve completed native work instead of leaving the entry state.
        // FP/MMX kernels still require their own partial-lifetime qualification.
        m.materialize_fault_flags();publish_machine(s,m);throw;
    }
    publish_machine(s,m);
    if(pc && final_pc)*pc=final_pc;
    return true;
}
} // namespace halo2::native_tu15_leaves
