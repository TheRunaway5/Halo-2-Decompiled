#pragma once
#include "native_tu15_object_function_scalar_kernel.hpp"
#include "native_tu15_object_function_scalar_ordered.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_function_scalar_detail {
using namespace physics_world_runtime_detail;

enum Register : unsigned { ax, cx, dx, bx, sp, bp, si, di };

template<class State, class Transport, class Children>
class Frame {
    State& state;
    Transport& io;
    Children& children;
    std::uint32_t& g(Register index) { return state.gp[index]; }
    void at(std::uint32_t pc) { io.at(state, pc); }
    std::uint32_t read(std::uint32_t address, unsigned width = 4) { return io.read(state, address, width); }
    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        io.write(state, pc, address, value, width);
    }
    void push(std::uint32_t pc, Register index) {
        at(pc);
        io.push(state, pc, g(index));
    }
    void pop(std::uint32_t pc, Register index) {
        at(pc);
        g(index) = io.pop(state);
    }
    void finish(std::uint32_t pc) {
        at(pc);
        (void)io.pop(state);
        g(sp) += 12u;
    }
    void load_al(std::uint32_t value) { state.set(ax, value, 1, 0); }
    std::uint32_t al() { return state.reg(ax, 1, 0); }
    void test_al(std::uint32_t pc) {
        at(pc);
        state.flags.logic(al(), 1);
    }
    bool zero_at(std::uint32_t pc) {
        at(pc);
        return state.flags.zf;
    }
    void output(std::uint32_t pc, unsigned xmm) {
        at(pc);
        write(pc, g(bp), u(_mm_cvtss_f32(state.xmm[xmm])));
    }
    void call(std::uint32_t pc, std::uint32_t returned, auto& child) {
        at(pc);
        io.push(state, pc, returned);
        child(state);
    }
    void clear_al(std::uint32_t pc) {
        at(pc);
        load_al(state.flags.logic(0u, 1));
    }
    void set_true(std::uint32_t pc) {
        at(pc);
        load_al(1u);
    }
    void compare_symbol(std::uint32_t pc, std::uint32_t key) {
        at(pc);
        state.flags.minus(g(di), source_object_function_compiled_symbol_tu15(state.memory, pc, key), 4);
    }
public:
    Frame(State& state_, Transport& io_, Children& children_) : state(state_), io(io_), children(children_) {}

    void open() {
        push(0xbcb70u, cx);
        at(0xbcb71u); state.xmm[4] = _mm_xor_ps(state.xmm[4], state.xmm[4]);
        push(0xbcb74u, bx);
        at(0xbcb75u); g(bx) = read(g(sp) + 12u);
        push(0xbcb79u, bp);
        at(0xbcb7au); g(bp) = read(g(sp) + 24u);
        at(0xbcb7eu); g(ax) = g(bx);
        at(0xbcb80u); g(ax) = state.flags.logic(g(ax) & 0xffffu, 4);
        push(0xbcb85u, si);
        at(0xbcb86u); g(si) = g(ax) * 3u;
        at(0xbcb89u); g(ax) = read(0x4ecbd0u);
        output(0xbcb8eu, 4);
        at(0xbcb93u); g(cx) = read(g(ax) + 0x44u);
        at(0xbcb96u); g(si) = io.template shift<2,true>(state, g(si));
        at(0xbcb99u); write(0xbcb99u, g(sp) + 15u, 0u, 1);
        at(0xbcb9eu); g(dx) = read(g(cx) + g(si) + 8u);
        at(0xbcba2u); g(ax) = state.flags.logic(0u, 4);
        at(0xbcba4u); state.set(ax, read(g(dx) + 0xc0u, 2), 2, 0);
        push(0xbcbabu, di);
        at(0xbcbacu); g(di) = read(g(sp) + 28u);
    }
    bool has_cache() {
        at(0xbcbb0u); g(ax) = io.template shift<9,false>(state, g(ax));
        at(0xbcbb3u); state.flags.logic(al() & 1u, 1);
        return !zero_at(0xbcbb5u);
    }
    bool lookup_cache() {
        at(0xbcbb7u); g(cx) = g(sp) + 32u;
        push(0xbcbbbu, cx); push(0xbcbbcu, bp); push(0xbcbbdu, di);
        call(0xbcbbeu, 0xbcbc3u, children.cache);
        test_al(0xbcbc3u);
        return !zero_at(0xbcbc5u);
    }
    KeyRoute select_key() {
        at(0xbcbd1u); state.flags.logic(g(di), 4);
        if (zero_at(0xbcbd3u)) return KeyRoute::constant_one;
        compare_symbol(0xbcbd9u, 0x30005a6u);
        if (zero_at(0xbcbdfu)) return KeyRoute::constant_one;
        compare_symbol(0xbcbe5u, 0x40005a7u);
        return zero_at(0xbcbebu) ? KeyRoute::constant_zero : KeyRoute::lookup;
    }
    bool has_datum() {
        at(0xbcbfcu); state.flags.minus(g(bx), 0xffffffffu, 4);
        return !zero_at(0xbcbffu);
    }
    bool lookup_type() {
        push(0xbcc05u, di); push(0xbcc06u, bx);
        at(0xbcc07u); g(di) = g(sp) + 27u;
        at(0xbcc0bu); g(bx) = g(bp);
        call(0xbcc0du, 0xbcc12u, children.type);
        test_al(0xbcc12u);
        return !zero_at(0xbcc14u);
    }
    void open_rows() {
        at(0xbcc1au); g(dx) = read(0x4ecbd0u);
        at(0xbcc20u); g(cx) = read(g(dx) + 0x44u);
        at(0xbcc23u); g(cx) = read(g(cx) + g(si) + 8u);
        at(0xbcc27u); g(dx) = read(g(cx));
        at(0xbcc29u); g(si) = read(0x4f0414u);
        at(0xbcc2fu); g(dx) = state.flags.logic(g(dx) & 0xffffu, 4);
        at(0xbcc35u); g(dx) = io.template shift<4,true>(state, g(dx));
        at(0xbcc38u); g(di) = read(g(dx) + g(si) + 8u);
        at(0xbcc3cu); g(dx) = read(g(di) + 0x64u);
        at(0xbcc3fu); g(si) = state.flags.logic(0u, 4);
        at(0xbcc41u); state.flags.logic(g(dx), 4);
        at(0xbcc43u); write(0xbcc43u, g(sp) + 32u, g(cx));
    }
    bool has_rows() {
        at(0xbcc47u);
        return !state.flags.zf && state.flags.sf == state.flags.of;
    }
    void begin_rows() {
        at(0xbcc49u); g(bx) = state.flags.logic(0u, 4);
        at(0xbcc4bu);
    }
    bool row_matches() {
        at(0xbcc50u); g(cx) = read(g(di) + 0x68u);
        at(0xbcc53u); g(dx) = read(g(cx) + g(bx) + 8u);
        at(0xbcc57u); g(cx) = state.flags.plus(g(cx), g(bx), 4);
        at(0xbcc59u); state.flags.minus(g(dx), read(g(sp) + 28u), 4);
        return zero_at(0xbcc5du);
    }
    void lookup_row() {
        at(0xbcc5fu); g(ax) = read(g(sp) + 24u);
        push(0xbcc63u, bp); push(0xbcc64u, ax);
        at(0xbcc65u); g(ax) = g(cx);
        call(0xbcc67u, 0xbcc6cu, children.row);
        at(0xbcc6cu); write(0xbcc6cu, g(sp) + 19u, al(), 1);
        set_true(0xbcc70u);
    }
    bool advance_row() {
        at(0xbcc72u); g(cx) = read(g(di) + 0x64u);
        at(0xbcc75u); g(si) = state.flags.increment(g(si), 4, false);
        at(0xbcc76u); g(bx) = state.flags.plus(g(bx), 32u, 4);
        at(0xbcc79u); state.flags.minus(g(si), g(cx), 4);
        at(0xbcc7bu); return state.flags.sf != state.flags.of;
    }
    bool row_handled() {
        test_al(0xbcc7du);
        return !zero_at(0xbcc7fu);
    }
    void reload_body() { at(0xbcc81u); g(cx) = read(g(sp) + 32u); }
    bool find_inherited_datum() {
        at(0xbcc85u); g(dx) = read(g(cx) + 4u);
        at(0xbcc88u); g(dx) = io.template shift<26,false>(state, g(dx));
        at(0xbcc8bu); state.flags.logic(state.reg(dx, 1, 0) & 1u, 1);
        if (zero_at(0xbcc8eu)) return false;
        at(0xbcc90u); g(ax) = read(g(cx) + 20u);
        at(0xbcc93u); state.flags.minus(g(ax), 0xffffffffu, 4);
        return !zero_at(0xbcc96u);
    }
    void lookup_inherited() {
        at(0xbcc98u); g(cx) = read(g(sp) + 28u);
        push(0xbcc9cu, bp); push(0xbcc9du, cx); push(0xbcc9eu, ax);
        call(0xbcc9fu, 0xbcca4u, children.inherited);
    }
    void finish_cache() {
        pop(0xbcbc7u, di); pop(0xbcbc8u, si); pop(0xbcbc9u, bp);
        set_true(0xbcbcau);
        pop(0xbcbccu, bx); pop(0xbcbcdu, cx); finish(0xbcbceu);
    }
    void finish_constant_one() {
        at(0xbccbeu); state.xmm[0] = _mm_set_ss(f(read(0x46a0d0u)));
        pop(0xbccc6u, di); pop(0xbccc7u, si); output(0xbccc8u, 0); pop(0xbcccdu, bp);
        set_true(0xbccceu);
        pop(0xbccd0u, bx); pop(0xbccd1u, cx); finish(0xbccd2u);
    }
    void finish_constant_zero() {
        pop(0xbcbedu, di); pop(0xbcbeeu, si); output(0xbcbefu, 4); pop(0xbcbf4u, bp);
        clear_al(0xbcbf5u);
        pop(0xbcbf7u, bx); pop(0xbcbf8u, cx); finish(0xbcbf9u);
    }
    void finish_handled() {
        at(0xbccd5u); load_al(read(g(sp) + 19u, 1));
        pop(0xbccd9u, di); pop(0xbccdau, si); pop(0xbccdbu, bp);
        pop(0xbccdcu, bx); pop(0xbccddu, cx); finish(0xbccdeu);
    }
    void finish_inherited() {
        pop(0xbcca4u, di); pop(0xbcca5u, si); pop(0xbcca6u, bp);
        pop(0xbcca7u, bx); pop(0xbcca8u, cx); finish(0xbcca9u);
    }
    void finish_absent() {
        at(0xbccacu); state.xmm[0] = _mm_xor_ps(state.xmm[0], state.xmm[0]);
        pop(0xbccafu, di); pop(0xbccb0u, si); output(0xbccb1u, 0); pop(0xbccb6u, bp);
        clear_al(0xbccb7u);
        pop(0xbccb9u, bx); pop(0xbccbau, cx); finish(0xbccbbu);
    }
};

template<class Cache, class Type, class Row, class Inherited> struct Children {
    Cache cache;
    Type type;
    Row row;
    Inherited inherited;
};

template<bool Literal,bool Observed> struct SourceIo {
    template<class State> void at(State& state, std::uint32_t pc) {
        // Census diagnostic: this complete native is not translated work.
        if (native_census_detail::skip_report(state.services, pc)) return;
        if constexpr (Literal) state.instruction_literal(pc);
        else if (state.services.instruction) state.services.instruction(pc);
    }
    template<class State> std::uint32_t read(State& state, std::uint32_t address, unsigned width) {
        return r(state.memory, address, width);
    }
    template<class State> void write(State& state, std::uint32_t pc, std::uint32_t address,
                                     std::uint32_t value, unsigned width) {
        w(state.memory, state.observe, pc, address, width, value);
    }
    template<class State> void push(State& state, std::uint32_t pc, std::uint32_t value) {
        state.push(pc, value);
    }
    template<class State> std::uint32_t pop(State& state) { return state.pop(); }
    template<unsigned Count,bool Left,class State> std::uint32_t shift(State& state, std::uint32_t value) {
        if constexpr (Observed) return native_object_function_scalar_ordered_detail::shift_for_image<Count,Left>(state,value);
        else if constexpr (Left) return state.flags.shift(value,Count,4);
        else return state.flags.right(value,Count,4,false);
    }
};

template<bool Literal,bool Observed=false,class State,class Routes> void run_source(State& state, Routes routes) {
    SourceIo<Literal,Observed> io;
    Frame frame{state, io, routes};
    evaluate(frame);
}
} // namespace halo2::native_object_function_scalar_detail
#endif
