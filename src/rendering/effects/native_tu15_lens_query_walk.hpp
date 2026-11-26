#pragma once

#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>

namespace halo2::native_lens_query_walk_detail {
namespace memory = physics_step_detail;

enum Register : unsigned { eax, ecx, edx, ebx, esp, ebp, esi, edi };
inline constexpr std::uint32_t first = 0x20b90u, last = 0x20d56u, done = 1u;

template<class State, class Commit>
void write_committed(State &s, std::uint32_t pc, std::uint32_t address, std::uint32_t value, Commit commit) {
    const std::array bytes{
        std::byte(value), std::byte(value >> 8u), std::byte(value >> 16u), std::byte(value >> 24u)};
    if (s.observe) {
        SourcePhysicsContactRecordStore event{pc, address, std::vector<std::byte>(4), {bytes.begin(), bytes.end()}};
        s.memory.read(address, event.before);
        s.memory.write(address, bytes);
        commit();
        s.observe(event);
    } else {
        s.memory.write(address, bytes);
        commit();
    }
}

template<class State>
void save_word(State &s, std::uint32_t pc, std::uint32_t word) {
    const auto stack = s.gp[esp] - 4u;
    write_committed(s, pc, stack, word, [&] { s.gp[esp] = stack; });
}

template<class State>
void mark_pending(State &s) {
    const auto address = s.gp[eax];
    auto flags = s.flags;
    const auto bits = flags.logic(memory::r(s.memory, address, 4) | s.gp[edx], 4);
    // OR commits flags only if the write succeeds. The completed-store
    // observer sees the committed flags and successful bitmap word.
    write_committed(s, 0x20d31u, address, bits, [&] { s.flags = flags; });
}

struct Resume {
    std::uint32_t pc;
};

template<class State>
class Walk {
    State &s;
    const SourcePhysicsContactRuntimeServices::InstructionCursor *cursor;
    decltype(State::gp) &gp = s.gp;
    decltype(State::flags) &flags = s.flags;

    void at(std::uint32_t pc) const {
        if (cursor)
            *cursor->position = pc;
    }

    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(s.services.instruction))
            throw Resume{next};
    }

    std::uint32_t word(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return memory::r(s.memory, address, width);
    }

    void load(std::uint32_t pc, std::uint32_t next, unsigned dest, std::uint32_t address, unsigned width = 4) {
        s.set(dest, word(pc, address, width), width);
        recheck(next);
    }

    void put(std::uint32_t pc, std::uint32_t next, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        memory::w(s.memory, s.observe, pc, address, width, value);
        recheck(next);
    }

    void save(std::uint32_t pc, std::uint32_t next, std::uint32_t value) {
        at(pc);
        save_word(s, pc, value);
        recheck(next);
    }

    void restore(std::uint32_t pc, std::uint32_t next, unsigned dest) {
        at(pc);
        gp[dest] = s.pop();
        recheck(next);
    }

    template<class Child>
    void invoke(std::uint32_t pc, std::uint32_t next, Child &child) {
        at(pc);
        save_word(s, pc, next);
        child(s);
        recheck(next);
    }

    template<class Dispatch>
    void visit_candidate(std::uint32_t pc, std::uint32_t next, Dispatch &dispatch) {
        const auto target = word(pc, gp[esp] + 0x4cu);
        save_word(s, pc, next);
        dispatch(s, target, pc, false);
        recheck(next);
    }

    bool admitted_view() {
        load(first, 0x20b96u, ecx, 0x52dfd8u, 1);
        load(0x20b96u, 0x20b9bu, eax, 0x4922c0u);
        gp[esp] = flags.minus(gp[esp], 0x1cu, 4);
        flags.logic(s.reg(ecx, 1), 1);
        if (flags.zf)
            return false;
        flags.minus(word(0x20ba6u, 0x532080u), gp[eax], 4);
        recheck(0x20bacu);
        if (!flags.zf)
            return false;
        load(0x20bb2u, 0x20bb8u, ecx, 0x532094u);
        flags.minus(gp[ecx], 0xffffffffu, 4);
        if (flags.zf)
            return false;
        flags.minus(gp[ecx], word(0x20bc1u, gp[esp] + 0x20u), 4);
        recheck(0x20bc5u);
        return flags.zf;
    }

    bool open_candidates() {
        // CDQ and IDIV 3 consume a signed frame stamp. The remainder selects
        // the original bitmap ring, and stays signed on malformed inputs.
        const auto stamp = std::bit_cast<std::int32_t>(gp[eax]);
        gp[ecx] = 3;
        gp[eax] = std::uint32_t(stamp / 3);
        gp[edx] = std::uint32_t(stamp % 3);
        load(0x20bd3u, 0x20bd7u, ecx, gp[esp] + 0x28u);
        flags.logic(gp[ecx], 4);
        gp[eax] = gp[edx];
        put(0x20bdbu, 0x20bdfu, gp[esp] + 8u, gp[eax]);
        if (flags.zf) {
            load(0x20be1u, 0x20be7u, edx, 0x531fbcu);
            put(0x20be7u, 0x20bebu, gp[esp] + 0x2cu, gp[edx]);
        }
        load(0x20bebu, 0x20befu, edx, gp[esp] + 0x2cu);
        save(0x20befu, 0x20bf0u, gp[ebp]);
        gp[ebp] = flags.logic(0, 4);
        flags.logic(gp[edx], 4);
        put(0x20bf4u, 0x20bf8u, gp[esp] + 8u, gp[ebp]);
        return !flags.zf && flags.sf == flags.of;
    }

    bool select_candidate() {
        flags.logic(gp[ecx], 4);
        if (flags.zf) {
            put(0x20c1du, 0x20c21u, gp[esp] + 0x10u, gp[ebp]);
            gp[edx] = gp[ebp];
        } else {
            load(0x20c14u, 0x20c17u, edx, gp[ecx] + gp[ebp] * 4u);
            put(0x20c17u, 0x20c1bu, gp[esp] + 0x10u, gp[edx]);
        }
        gp[esi] = flags.shift(gp[edx], 5, 4);
        gp[esi] = flags.plus(gp[esi], 0x52dfdcu, 4);
        flags.logic(word(0x20c2eu, gp[esi] + 4u, 1) & 2u, 1);
        recheck(0x20c32u);
        if (flags.zf)
            return false;
        load(0x20c38u, 0x20c3au, ecx, gp[esi]);
        load(0x20c3au, 0x20c3eu, ebx, gp[esp] + 0x30u);
        gp[edi] = flags.right(gp[ecx], 1, 4, false);
        gp[edi] = flags.logic(gp[edi] & 3u, 4);
        flags.minus(gp[ebx], gp[edi], 4);
        if (!flags.zf)
            return false;
        gp[ecx] = flags.right(gp[ecx], 3, 4, false);
        gp[ecx] = flags.logic(gp[ecx] & 7u, 4);
        gp[edi] = flags.shift(1u, gp[ecx] & 255u, 4);
        load(0x20c5au, 0x20c5eu, ecx, gp[esp] + 0x34u);
        flags.logic(gp[ecx] & gp[edi], 4);
        return !flags.zf;
    }

    template<class Dispatch, class Release>
    void enqueue_request(Dispatch &dispatch, Release &release) {
        load(0x20cb0u, 0x20cb6u, ebx, 0x411568u);
        load(0x20cb6u, 0x20cb8u, eax, gp[ebx]);
        flags.minus(gp[eax], word(0x20cb8u, gp[ebx] + 4u), 4);
        recheck(0x20cbbu);
        if (!flags.cf) {
            load(0x20cbdu, 0x20cc2u, eax, 0x412728u);
            gp[ecx] = flags.right(gp[eax], 1, 4, false);
            save(0x20cc6u, 0x20cc7u, gp[ecx]);
            auto grow = [&](State &state) { dispatch(state, 0x404bc0u, 0x20cc7u, false); };
            invoke(0x20cc7u, 0x20cccu, grow);
        }
        put(0x20cccu, 0x20cd2u, gp[eax], 0x817c8u);
        gp[ecx] = 1;
        put(0x20cd7u, 0x20cdau, gp[eax] + 4u, gp[ecx]);
        gp[edx] = gp[esi] + 8u;
        put(0x20cddu, 0x20ce0u, gp[eax] + 8u, gp[ecx]);
        save(0x20ce0u, 0x20ce1u, gp[edx]);
        gp[eax] = flags.plus(gp[eax], 12u, 4);
        save(0x20ce4u, 0x20ce5u, gp[esi]);
        put(0x20ce5u, 0x20ce7u, gp[ebx], gp[eax]);
        visit_candidate(0x20ce7u, 0x20cebu, dispatch);
        gp[ebp] = gp[eax];
        gp[eax] = gp[edi];
        invoke(0x20cefu, 0x20cf4u, release);
        flags.logic(gp[ebp], 4);
        gp[ebx] = gp[eax];
        if (flags.sf != flags.of) {
            gp[ebp] = flags.logic(0, 4);
        } else {
            flags.minus(gp[ebp], 0xffffu, 4);
            if (!flags.zf && flags.sf == flags.of)
                gp[ebp] = 0xffffu;
        }
    }

    template<class Dispatch, class Classify, class Poll, class Release>
    void issue_candidate(Dispatch &dispatch, Classify &classify, Poll &poll, Release &release) {
        gp[edi] = gp[eax] + gp[edx] * 2u;
        gp[eax] = flags.shift(gp[eax], 6, 4) + 0x531fc0u;
        save(0x20c7eu, 0x20c7fu, gp[eax]);
        gp[edi] = flags.plus(gp[edi], gp[edx], 4);
        gp[ebx] = flags.logic(0, 4);
        put(0x20c83u, 0x20c87u, gp[esp] + 0x20u, gp[eax]);
        invoke(0x20c87u, 0x20c8cu, classify);
        flags.logic(s.reg(eax, 1), 1);
        if (!flags.zf) {
            gp[edx] = gp[esp] + 0x20u;
            save(0x20c94u, 0x20c95u, gp[edx]);
            gp[edx] = gp[esp] + 0x28u;
            gp[eax] = gp[edi];
            invoke(0x20c9bu, 0x20ca0u, poll);
            gp[ebx] = gp[eax];
            flags.logic(gp[ebx], 4);
            if (!flags.zf)
                return;
        }
        gp[ebp] = flags.logic(0, 4);
        flags.logic(gp[ebx], 4);
        if (flags.zf)
            enqueue_request(dispatch, release);
        flags.logic(gp[ebx], 4);
        put(0x20d0du, 0x20d11u, gp[esi] + 6u, gp[ebp] & 65535u, 2);
        load(0x20d11u, 0x20d15u, ebp, gp[esp] + 0x14u);
        if (!flags.zf)
            return;
        load(0x20d17u, 0x20d1bu, ecx, gp[esp] + 0x10u);
        load(0x20d1bu, 0x20d1fu, edx, gp[esp] + 0x1cu);
        gp[eax] = flags.right(gp[ecx], 5, 4, true);
        gp[eax] = gp[edx] + gp[eax] * 4u;
        gp[ecx] = flags.logic(gp[ecx] & 31u, 4);
        gp[edx] = flags.shift(1u, gp[ecx] & 255u, 4);
        at(0x20d31u);
        mark_pending(s);
        recheck(0x20d33u);
    }

    template<class Dispatch>
    void visit_without_query(Dispatch &dispatch) {
        gp[eax] = gp[esi] + 8u;
        save(0x20d38u, 0x20d39u, gp[eax]);
        save(0x20d39u, 0x20d3au, gp[esi]);
        visit_candidate(0x20d3au, 0x20d3eu, dispatch);
    }

    void return_frame() {
        gp[esp] = flags.plus(gp[esp], 0x1cu, 4);
        at(last);
        (void)s.pop();
        gp[esp] += 24u;
    }

public:
    Walk(State &state, const SourcePhysicsContactRuntimeServices::InstructionCursor *cursor)
        : s(state), cursor(cursor) {}

    template<class Dispatch, class Classify, class Poll, class Release>
    void run(Dispatch &dispatch, Classify &classify, Poll &poll, Release &release) {
        if (!admitted_view()) {
            return_frame();
            return;
        }
        if (open_candidates()) {
            save(0x20bfeu, 0x20bffu, gp[ebx]);
            save(0x20bffu, 0x20c00u, gp[esi]);
            save(0x20c00u, 0x20c01u, gp[edi]);
            for (;;) {
                if (select_candidate()) {
                    load(0x20c66u, 0x20c6au, ecx, gp[esp] + 0x40u, 1);
                    flags.logic(s.reg(ecx, 1), 1);
                    if (flags.zf)
                        visit_without_query(dispatch);
                    else
                        issue_candidate(dispatch, classify, poll, release);
                }
                load(0x20d3eu, 0x20d42u, eax, gp[esp] + 0x3cu);
                gp[ebp] = flags.increment(gp[ebp], 4, false);
                flags.minus(gp[ebp], gp[eax], 4);
                put(0x20d45u, 0x20d49u, gp[esp] + 0x14u, gp[ebp]);
                if (flags.sf == flags.of)
                    break;
                load(0x20c03u, 0x20c07u, ecx, gp[esp] + 0x38u);
                load(0x20c07u, 0x20c0bu, eax, gp[esp] + 0x18u);
            }
            restore(0x20d4fu, 0x20d50u, edi);
            restore(0x20d50u, 0x20d51u, esi);
            restore(0x20d51u, 0x20d52u, ebx);
        }
        restore(0x20d52u, 0x20d53u, ebp);
        return_frame();
    }
};

template<class State, class Dispatch, class Classify, class Poll, class Release>
std::uint32_t bridge(State &s, Dispatch dispatch, Classify classify, Poll poll, Release release) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_LENS_QUERY_WALK");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15)
        return 0;
    const auto *cursor = s.immutable_cursor.cursor;
    if (s.services.instruction) {
        if (!cursor || (cursor->first_trace && *cursor->first_trace) ||
            (cursor->second_trace && *cursor->second_trace) ||
            (s.observe && !cursor->observation_only_stores))
            return 0;
        for (const auto pc : cursor->service_sites)
            if (pc >= first && pc <= last)
                return 0;
    }
    try {
        Walk<State>(s, cursor).run(dispatch, classify, poll, release);
        return done;
    } catch (Resume next) {
        return next.pc;
    }
}
} // namespace halo2::native_lens_query_walk_detail
