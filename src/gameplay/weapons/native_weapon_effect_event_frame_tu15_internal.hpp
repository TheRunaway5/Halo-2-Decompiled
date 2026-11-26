// Included by the Effect dispatcher after its memory and ABI helpers.
namespace effect_event_advance {

struct Resume {
    std::uint32_t pc;
};

class Frame {
    Memory &memory;
    Abi &abi;
    const Observer &observer;
    const Services &services;
    IteratorRegisters registers{abi.gp, abi.eflags};
    const Services::InstructionCursor *cursor;

    void at(std::uint32_t site) const {
        if (cursor)
            *cursor->position = site;
    }

    // Reads, writes and genuine children can change the live trace policy.
    // Publish their completed state before resuming at the next retained PC.
    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(services.instruction))
            throw Resume{next};
    }

    std::uint32_t load(std::uint32_t site, std::uint32_t address, unsigned width = 4) const {
        at(site);
        return read(memory, address, width);
    }

    void put(std::uint32_t site, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(site);
        store(memory, observer, site, address, value, width);
    }

    void save(std::uint32_t site, std::uint32_t value) {
        at(site);
        push(memory, abi, observer, site, value);
    }

    std::uint32_t restore(std::uint32_t site) {
        at(site);
        return pop(memory, abi);
    }

    void invoke(std::uint32_t site, std::uint32_t target) {
        call(memory, abi, observer, services, site, target);
        recheck(site + 5u);
    }

    void low(unsigned reg, std::uint32_t value, unsigned width = 1) {
        const auto mask = width == 1 ? 255u : 65535u;
        abi.gp[reg] = (abi.gp[reg] & ~mask) | (value & mask);
    }

    std::uint32_t add(std::uint32_t left, std::uint32_t right) {
        return iterator_flags(registers, left, right, 4, false);
    }

    void test_byte(std::uint32_t value) {
        iterator_logic_flags(registers, value, 1);
    }

    void compare(std::uint32_t left, std::uint32_t right) {
        iterator_flags(registers, left, right, 4, true);
    }

    bool signed_less() const {
        return bool(abi.eflags & 128u) != bool(abi.eflags & 2048u);
    }

    void load_scalar(std::uint32_t site, unsigned lane, std::uint32_t address) {
        abi.xmm[lane] = {load(site, address), 0, 0, 0};
    }

    static __m128 scalar_bits(std::uint32_t value) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));
    }

    void subtract_scalar(unsigned lane, __m128 value) {
        SseEnvironment environment(abi);
        vector(abi, lane, native_tu15_leaves::ordered_difference_ss(vector(abi, lane), value));
    }

    bool open() {
        auto &gp = abi.gp;
        gp[0] = load(0x17c140u, 0x4f720cu);
        recheck(0x17c145u);
        gp[1] = load(0x17c145u, gp[0] + 0x44u);
        recheck(0x17c148u);
        gp[4] = iterator_flags(registers, gp[4], 12u, 4, true);
        gp[2] = load(0x17c14bu, 0x4f0414u);
        recheck(0x17c151u);
        save(0x17c151u, gp[5]);
        recheck(0x17c152u);
        save(0x17c152u, gp[6]);
        recheck(0x17c153u);
        save(0x17c153u, gp[7]);
        recheck(0x17c154u);
        gp[7] = load(0x17c154u, gp[4] + 0x1cu);
        recheck(0x17c158u);
        gp[6] = gp[7] & 65535u;
        iterator_logic_flags(registers, gp[6], 4);
        gp[6] *= 400u;
        low(0, load(0x17c166u, gp[6] + gp[1] + 2u, 1));
        recheck(0x17c16au);
        gp[6] = add(gp[6], gp[1]);
        gp[1] = load(0x17c16cu, gp[6] + 4u);
        recheck(0x17c16fu);
        gp[1] = (gp[1] & 65535u) << 4u;
        iterator_logic_flags(registers, gp[1], 4);
        gp[5] = load(0x17c178u, gp[1] + gp[2] + 8u);
        recheck(0x17c17cu);
        const auto pause_bits = (gp[0] & 255u) >> 6u;
        low(0, pause_bits);
        test_byte(pause_bits & 1u);
        return (abi.eflags & 64u) != 0;
    }

    bool open_interval_budget() {
        auto &gp = abi.gp;
        load_scalar(0x17c183u, 0, gp[4] + 0x20u);
        recheck(0x17c189u);
        abi.xmm[2] = {};
        state_scalar_compare(abi, vector(abi, 0), vector(abi, 2), false);
        save(0x17c18fu, gp[3]);
        recheck(0x17c190u);
        put(0x17c190u, gp[4] + 0x10u, 0);
        recheck(0x17c198u);
        return (abi.eflags & 1u) == 0;
    }

    bool accept_iteration() {
        auto &gp = abi.gp;
        const auto flags = load(0x17c1b5u, gp[6] + 2u, 2);
        low(0, flags, 2);
        recheck(0x17c1b9u);
        low(1, (flags & 255u) >> 2u);
        test_byte((flags >> 2u) & 1u);
        if (!(abi.eflags & 64u))
            return false;
        const auto iteration = load(0x17c1c3u, gp[4] + 0x10u);
        compare(iteration, 8u);
        recheck(0x17c1c8u);
        return signed_less();
    }

    bool schedule_interval() {
        auto &gp = abi.gp;
        load_scalar(0x17c1cau, 1u, gp[6] + 0x64u);
        recheck(0x17c1cfu);
        const auto elapsed = load(0x17c1cfu, gp[6] + 0x60u);
        subtract_scalar(1u, scalar_bits(elapsed));
        recheck(0x17c1d4u);
        state_scalar_compare(abi, vector(abi, 1), vector(abi, 0), false);
        load_scalar(0x17c1d7u, 3u, gp[6] + 0x60u);
        recheck(0x17c1dcu);
        put(0x17c1dcu, gp[4] + 0x18u, abi.xmm[3][0]);
        recheck(0x17c1e2u);
        put(0x17c1e2u, gp[4] + 0x14u, abi.xmm[0][0]);
        recheck(0x17c1e8u);
        if (abi.eflags & 0x41u) {
            put(0x17c1eau, gp[4] + 0x14u, abi.xmm[1][0]);
            recheck(0x17c1f0u);
        }

        state_scalar_compare(abi, vector(abi, 0), vector(abi, 1), false);
        if (!(abi.eflags & 1u)) {
            gp[2] = load(0x17c1f5u, gp[6] + 0x64u);
            recheck(0x17c1f8u);
            low(3, 1u);
            put(0x17c1fau, gp[6] + 0x60u, gp[2]);
            recheck(0x17c1fdu);
            subtract_scalar(0u, vector(abi, 1));
        } else {
            load_scalar(0x17c203u, 1u, gp[6] + 0x60u);
            recheck(0x17c208u);
            {
                SseEnvironment environment(abi);
                vector(abi, 1, native_tu15_leaves::ordered_sum_ss(vector(abi, 1), vector(abi, 0)));
            }
            load_scalar(0x17c20cu, 0u, 0x46a0dcu);
            recheck(0x17c214u);
            low(3, 0);
            test_byte(0);
            put(0x17c216u, gp[6] + 0x60u, abi.xmm[1][0]);
            recheck(0x17c21bu);
        }

        const auto flags = load(0x17c21bu, gp[6] + 2u, 1);
        test_byte(flags & 1u);
        recheck(0x17c21fu);
        put(0x17c21fu, gp[4] + 0x24u, abi.xmm[0][0]);
        recheck(0x17c225u);
        return (abi.eflags & 64u) == 0;
    }

    void dispatch_active_components() {
        auto &gp = abi.gp;
        low(0, (gp[0] & 255u) >> 3u);
        test_byte(gp[0] & 1u);
        if (abi.eflags & 64u) {
            gp[0] = load(0x17c22eu, gp[4] + 0x14u);
            recheck(0x17c232u);
            gp[1] = load(0x17c232u, gp[4] + 0x18u);
            recheck(0x17c236u);
            save(0x17c236u, gp[0]);
            recheck(0x17c237u);
            save(0x17c237u, gp[1]);
            recheck(0x17c238u);
            save(0x17c238u, gp[7]);
            recheck(0x17c239u);
            invoke(0x17c239u, 0x17beb0u);
        }
        low(2, load(0x17c23eu, gp[6] + 2u, 1));
        recheck(0x17c241u);
        low(2, (gp[2] & 255u) >> 3u);
        test_byte(gp[2] & 1u);
        if (abi.eflags & 64u)
            invoke(0x17c249u, 0x17ce40u);
    }

    bool advance_active_event() {
        auto &gp = abi.gp;
        test_byte(gp[3] & 255u);
        if (abi.eflags & 64u)
            return true;
        save(0x17c256u, gp[6]);
        recheck(0x17c257u);
        invoke(0x17c257u, 0x17c020u);
        const auto event_count = load(0x17c25cu, gp[5] + 0x14u);
        compare(gp[0], event_count);
        recheck(0x17c25fu);
        if (!signed_less())
            return false;
        gp[2] = load(0x17c265u, gp[4] + 0x20u);
        recheck(0x17c269u);
        gp[7] = gp[0];
        invoke(0x17c26bu, 0x17b130u);
        gp[7] = load(0x17c270u, gp[4] + 0x20u);
        recheck(0x17c274u);
        return true;
    }

    void start_interval() {
        auto &gp = abi.gp;
        test_byte(gp[3] & 255u);
        if (abi.eflags & 64u)
            return;
        gp[1] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(
            std::uint16_t(load(0x17c27au, gp[6] + 0x5cu, 2)))));
        recheck(0x17c27eu);
        gp[0] = load(0x17c27eu, gp[5] + 0x18u);
        recheck(0x17c281u);
        gp[1] = state_product(registers, gp[1], 0x38u);
        at(0x17c284u);
        modify_effect_byte(memory, abi, observer, 0x17c284u, gp[6] + 2u,
            [](auto value) { return value | 1u; });
        recheck(0x17c288u);
        load_scalar(0x17c288u, 0u, 0x46a0dcu);
        recheck(0x17c290u);
        gp[1] = add(gp[1], gp[0]);
        put(0x17c292u, gp[6] + 0x60u, abi.xmm[2][0]);
        recheck(0x17c297u);
        put(0x17c297u, gp[6] + 0x68u, abi.xmm[0][0]);
        recheck(0x17c29cu);
        load_scalar(0x17c29cu, 0u, gp[1] + 0x10u);
        recheck(0x17c2a1u);
        const auto upper_duration = load(0x17c2a1u, gp[1] + 0x14u);
        state_scalar_compare(abi, vector(abi, 0), scalar_bits(upper_duration), true);
        recheck(0x17c2a5u);
        gp[0] = (gp[0] & 0xffff00ffu) | (((abi.eflags & 0xd5u) | 2u) << 8u);
        test_byte((gp[0] >> 8u) & 0x44u);
        gp[0] = load(0x17c2a9u, gp[1] + 0x10u);
        recheck(0x17c2acu);
        if (!(abi.eflags & 4u)) {
            put(0x17c2aeu, gp[6] + 0x64u, gp[0]);
            recheck(0x17c2b1u);
        } else {
            gp[2] = load(0x17c2b3u, gp[1] + 0x14u);
            recheck(0x17c2b6u);
            save(0x17c2b6u, gp[2]);
            recheck(0x17c2b7u);
            save(0x17c2b7u, gp[0]);
            recheck(0x17c2b8u);
            gp[0] = load(0x17c2b8u, gp[6] + 4u);
            recheck(0x17c2bbu);
            invoke(0x17c2bbu, 0x17aab0u);
            gp[1] = gp[0];
            invoke(0x17c2c2u, 0x259b0u);
            at(0x17c2c7u);
            state_store_single(memory, abi, observer, 0x17c2c7u);
            recheck(0x17c2cau);
        }

        low(1, load(0x17c2cau, gp[6] + 2u, 1));
        recheck(0x17c2cdu);
        low(1, (gp[1] & 255u) >> 3u);
        test_byte(gp[1] & 1u);
        if (abi.eflags & 64u) {
            save(0x17c2d5u, gp[6]);
            recheck(0x17c2d6u);
            invoke(0x17c2d6u, 0x17d210u);
        }
        gp[2] = gp[6];
        invoke(0x17c2ddu, 0x17dc60u);
        low(2, load(0x17c2e2u, gp[6] + 0x5cu, 2), 2);
        recheck(0x17c2e6u);
        put(0x17c2e6u, gp[6] + 0x5eu, gp[2], 2);
        recheck(0x17c2eau);
    }

    bool more_intervals() {
        auto &gp = abi.gp;
        gp[0] = load(0x17c2eau, gp[4] + 0x10u);
        recheck(0x17c2eeu);
        load_scalar(0x17c2eeu, 0, gp[4] + 0x24u);
        recheck(0x17c2f4u);
        abi.xmm[2] = {};
        const auto carry = abi.eflags & 1u;
        gp[0] = add(gp[0], 1u);
        abi.eflags = (abi.eflags & ~1u) | carry;
        state_scalar_compare(abi, vector(abi, 0), vector(abi, 2), false);
        put(0x17c2fbu, gp[4] + 0x10u, gp[0]);
        recheck(0x17c2ffu);
        return (abi.eflags & 1u) == 0;
    }

    void close_live() {
        auto &gp = abi.gp;
        gp[0] = 0;
        iterator_logic_flags(registers, 0, 4);
        gp[7] = restore(0x17c1a1u);
        recheck(0x17c1a2u);
        iterator_logic_flags(registers, gp[6], 4);
        gp[6] = restore(0x17c1a4u);
        recheck(0x17c1a5u);
        low(0, bool(abi.eflags & 64u));
        gp[5] = restore(0x17c1a8u);
        recheck(0x17c1a9u);
        gp[4] = add(gp[4], 12u);
        restore(0x17c1acu);
        gp[4] += 8u;
    }

    void clear_active() {
        at(0x17c19au);
        and_effect_byte(memory, abi, observer, 0x17c19au, abi.gp[6] + 3u, 0xfdu);
        recheck(0x17c19eu);
        abi.gp[3] = restore(0x17c19eu);
        recheck(0x17c19fu);
        close_live();
    }

    void retire() {
        auto &gp = abi.gp;
        gp[0] = gp[7];
        invoke(0x17c30cu, 0x17a4a0u);
        test_byte(gp[0] & 255u);
        if (abi.eflags & 64u) {
            clear_active();
            return;
        }
        gp[3] = restore(0x17c319u);
        recheck(0x17c31au);
        gp[6] = 0;
        gp[0] = 0;
        iterator_logic_flags(registers, 0, 4);
        gp[7] = restore(0x17c31eu);
        recheck(0x17c31fu);
        iterator_logic_flags(registers, gp[6], 4);
        gp[6] = restore(0x17c321u);
        recheck(0x17c322u);
        low(0, bool(abi.eflags & 64u));
        gp[5] = restore(0x17c325u);
        recheck(0x17c326u);
        gp[4] = add(gp[4], 12u);
        restore(0x17c329u);
        gp[4] += 8u;
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services),
          cursor(services.instruction ? &services.instruction_cursor : nullptr) {}

    void run() {
        if (!open()) {
            close_live();
            return;
        }
        if (open_interval_budget()) {
            while (accept_iteration()) {
                if (schedule_interval()) {
                    dispatch_active_components();
                    if (!advance_active_event()) {
                        retire();
                        return;
                    }
                } else {
                    start_interval();
                }
                if (!more_intervals())
                    break;
                load_scalar(0x17c1afu, 0, abi.gp[4] + 0x24u);
                recheck(0x17c1b5u);
            }
        }
        clear_active();
    }
};

} // namespace effect_event_advance
