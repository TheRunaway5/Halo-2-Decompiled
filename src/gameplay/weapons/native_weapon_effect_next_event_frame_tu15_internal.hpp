// Included by the Effect dispatcher after its memory and ABI helpers.
namespace effect_next_event {

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
        if (cursor) *cursor->position = site;
    }

    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(services.instruction))
            throw Resume{next};
    }

    std::uint32_t load(std::uint32_t site, std::uint32_t address, unsigned width = 4) {
        at(site);
        return read(memory, address, width);
    }

    void put(std::uint32_t site, std::uint32_t address, std::uint32_t value) {
        at(site);
        store(memory, observer, site, address, value);
    }

    void save(std::uint32_t site, std::uint32_t value) {
        at(site);
        push(memory, abi, observer, site, value);
    }

    std::uint32_t restore(std::uint32_t site) {
        at(site);
        return pop(memory, abi);
    }

    std::uint32_t signed_word(std::uint32_t site, std::uint32_t address) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(load(site, address, 2)))));
    }

    void low_byte(unsigned reg, std::uint32_t value) {
        abi.gp[reg] = (abi.gp[reg] & 0xffffff00u) | (value & 255u);
    }

    void shift(unsigned reg, unsigned bits) {
        const auto old = abi.gp[reg];
        abi.gp[reg] = old >> bits;
        iterator_logic_flags(registers, abi.gp[reg], 4);
        abi.eflags |= (old >> (bits - 1u)) & 1u;
    }

    void compare(std::uint32_t left, std::uint32_t right) {
        iterator_flags(registers, left, right, 4, true);
    }

    bool equal() const { return (abi.eflags & 64u) != 0; }
    bool signed_less() const { return bool(abi.eflags & 128u) != bool(abi.eflags & 2048u); }

    void step_event_number(bool down) {
        const auto carry = abi.eflags & 1u;
        abi.gp[0] = iterator_flags(registers, abi.gp[0], 1u, 4, down);
        abi.eflags = (abi.eflags & ~1u) | carry;
    }

    void tag_index_to_offset(unsigned reg) {
        abi.gp[reg] = (abi.gp[reg] & 65535u) << 4u;
        iterator_logic_flags(registers, abi.gp[reg], 4);
    }

    void test_loop_byte() {
        low_byte(3, (abi.gp[1] & 255u) >> 1u);
        iterator_logic_flags(registers, abi.gp[3] & 1u, 1);
    }

    bool use_loop_slot() {
        if (!equal()) return true;
        shift(1, 9u);
        iterator_logic_flags(registers, abi.gp[1] & 1u, 1);
        return !equal();
    }

    void open() {
        auto &gp = abi.gp;
        gp[4] = iterator_flags(registers, gp[4], 8u, 4, true);
        gp[1] = load(0x17c023u, 0x4f0414u);
        recheck(0x17c029u);
        save(0x17c029u, gp[3]);
        recheck(0x17c02au);
        save(0x17c02au, gp[5]);
        recheck(0x17c02bu);
        gp[5] = load(0x17c02bu, gp[4] + 0x14u);
        recheck(0x17c02fu);
        gp[0] = load(0x17c02fu, gp[5] + 4u);
        recheck(0x17c032u);
        tag_index_to_offset(0);
        gp[2] = load(0x17c03au, gp[0] + gp[1] + 8u);
        recheck(0x17c03eu);
        gp[0] = signed_word(0x17c03eu, gp[2] + 4u);
        recheck(0x17c042u);
        gp[1] = 0;
        iterator_logic_flags(registers, 0, 4);
        gp[1] = load(0x17c044u, gp[5] + 2u, 2);
        recheck(0x17c048u);
        put(0x17c048u, gp[4] + 0x14u, gp[0]);
        recheck(0x17c04cu);
        gp[0] = load(0x17c04cu, gp[2] + 0x14u);
        recheck(0x17c04fu);
        step_event_number(true);
        test_loop_byte();
        save(0x17c057u, gp[6]);
        recheck(0x17c058u);
        put(0x17c058u, gp[4] + 12u, gp[0]);
        recheck(0x17c05cu);
        gp[0] = signed_word(0x17c05cu, gp[5] + 0x5cu);
        recheck(0x17c060u);
        save(0x17c060u, gp[7]);
        recheck(0x17c061u);

        if (use_loop_slot()) {
            const auto loop_event = load(0x17c06bu, gp[4] + 0x1cu);
            compare(loop_event, none);
            recheck(0x17c070u);
            if (equal()) {
                put(0x17c072u, gp[4] + 0x1cu, 0);
                recheck(0x17c07au);
            }
        }
        abi.xmm[0] = {};
    }

    void next_candidate() {
        auto &gp = abi.gp;
        gp[1] = 0;
        iterator_logic_flags(registers, 0, 4);
        gp[1] = load(0x17c082u, gp[5] + 2u, 2);
        recheck(0x17c086u);
        test_loop_byte();
        if (use_loop_slot()) {
            const auto last_event = load(0x17c097u, gp[4] + 0x10u);
            compare(gp[0], last_event);
            recheck(0x17c09bu);
            if (equal()) {
                gp[1] = load(0x17c09du, gp[4] + 0x1cu);
                recheck(0x17c0a1u);
                compare(gp[1], none);
                if (!equal()) {
                    gp[0] = gp[1];
                    return;
                }
            }
        }
        step_event_number(false);
    }

    bool needs_random_draw() {
        auto &gp = abi.gp;
        const auto count = load(0x17c0abu, gp[2] + 0x14u);
        compare(gp[0], count);
        recheck(0x17c0aeu);
        if (!signed_less()) return false;

        gp[1] = load(0x17c0b0u, gp[2] + 0x18u);
        recheck(0x17c0b3u);
        gp[7] = state_product(registers, gp[0], 0x38u);
        abi.xmm[1] = {load(0x17c0b8u, gp[1] + gp[7] + 4u), 0, 0, 0};
        recheck(0x17c0beu);
        state_scalar_compare(abi, vector(abi, 1), vector(abi, 0), false);
        return !(abi.eflags & 0x41u);
    }

    void update_random_stream() {
        auto &gp = abi.gp;
        gp[1] = load(0x17c0c3u, gp[5] + 4u);
        recheck(0x17c0c6u);
        gp[6] = load(0x17c0c6u, 0x4f0414u);
        recheck(0x17c0ccu);
        tag_index_to_offset(1);
        gp[1] = load(0x17c0d5u, gp[1] + gp[6] + 8u);
        recheck(0x17c0d9u);
        gp[1] = load(0x17c0d9u, gp[1]);
        recheck(0x17c0dbu);
        gp[6] = load(0x17c0dbu, 0x4f3cd8u);
        recheck(0x17c0e1u);
        shift(1, 2u);
        iterator_logic_flags(registers, gp[1] & 1u, 1);
        if (equal()) gp[6] = iterator_flags(registers, gp[6], 4u, 4, false);
        gp[1] = load(0x17c0ecu, gp[6]);
        recheck(0x17c0eeu);
        gp[1] = state_product(registers, gp[1], 1664525u);
        gp[1] = iterator_flags(registers, gp[1], 1013904223u, 4, false);
        put(0x17c0fau, gp[6], gp[1]);
        recheck(0x17c0fcu);
    }

    bool accept_draw() {
        auto &gp = abi.gp;
        shift(1, 16u);
        iterator_logic_flags(registers, gp[1], 4);
        put(0x17c101u, gp[4] + 0x14u, gp[1]);
        recheck(0x17c105u);
        effect_fp(abi, EffectFp::integer, load(0x17c105u, gp[4] + 0x14u));
        recheck(0x17c109u);
        if (signed_less()) {
            effect_fp(abi, EffectFp::add, load(0x17c10bu, 0x46a0c8u));
            recheck(0x17c111u);
        }
        effect_fp(abi, EffectFp::multiply, load(0x17c111u, 0x46a160u));
        recheck(0x17c117u);
        gp[1] = load(0x17c117u, gp[2] + 0x18u);
        recheck(0x17c11au);
        effect_fp(abi, EffectFp::single, load(0x17c11au, gp[1] + gp[7] + 4u));
        recheck(0x17c11eu);
        effect_fp(abi, EffectFp::compare);
        effect_fp(abi, EffectFp::pop);
        return (abi.eflags & 0x41u) != 0;
    }

    void close() {
        auto &gp = abi.gp;
        gp[7] = restore(0x17c128u);
        recheck(0x17c129u);
        gp[6] = restore(0x17c129u);
        recheck(0x17c12au);
        gp[5] = restore(0x17c12au);
        recheck(0x17c12bu);
        gp[3] = restore(0x17c12bu);
        recheck(0x17c12cu);
        gp[4] = iterator_flags(registers, gp[4], 8u, 4, false);
        (void)restore(0x17c12fu);
        gp[4] += 4u;
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services),
          cursor(services.instruction ? &services.instruction_cursor : nullptr) {}

    void run() {
        open();
        for (;;) {
            next_candidate();
            if (!needs_random_draw()) break;
            update_random_stream();
            if (accept_draw()) break;
        }
        close();
    }
};

} // namespace effect_next_event
