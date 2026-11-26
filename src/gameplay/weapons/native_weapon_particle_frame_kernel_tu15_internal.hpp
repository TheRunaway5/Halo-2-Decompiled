// Included after the shared effect frame flag and memory helpers.
namespace particle_frame_update {

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

    void invoke(std::uint32_t site, std::uint32_t target) {
        call(memory, abi, observer, services, site, target);
        recheck(site + 5u);
    }

    void test(std::uint32_t value, unsigned width = 4) {
        iterator_logic_flags(registers, value, width);
    }

    void compare(std::uint32_t left, std::uint32_t right) {
        iterator_flags(registers, left, right, 4, true);
    }

    bool equal() const { return (abi.eflags & 64u) != 0; }
    bool signed_less() const { return bool(abi.eflags & 128u) != bool(abi.eflags & 2048u); }

    void open() {
        auto &gp = abi.gp;
        gp[4] = iterator_flags(registers, gp[4], 0x10u, 4, true);
        save(0x177833u, gp[3]);
        recheck(0x177834u);
        save(0x177834u, gp[5]);
        recheck(0x177835u);
        save(0x177835u, gp[6]);
        recheck(0x177836u);
        save(0x177836u, gp[7]);
        recheck(0x177837u);
        gp[7] = load(0x177837u, 0x51df74u);
        recheck(0x17783du);
        put(0x17783du, gp[4] + 0x14u, gp[7]);
        recheck(0x177841u);
        gp[5] = none;
        test(none);
    }

    bool find_occupied_slot() {
        auto &gp = abi.gp;
        gp[0] = gp[5] + 1u;
        test(gp[0]);
        if (signed_less()) return false;
        gp[6] = load(0x17784bu, gp[7] + 0x38u);
        recheck(0x17784eu);
        compare(gp[0], gp[6]);
        if (!signed_less()) return false;
        gp[2] = load(0x177852u, gp[7] + 0x48u);
        recheck(0x177855u);

        do {
            gp[1] = gp[0] & 31u;
            test(gp[1]);
            gp[3] = effect_frame_shift_left(registers, 1u, gp[1]);
            gp[1] = effect_frame_signed_right_five(registers, gp[0]);
            const auto occupied = load(0x177866u, gp[2] + gp[1] * 4u);
            test(occupied & gp[3]);
            recheck(0x177869u);
            if (!equal()) {
                compare(gp[0], none);
                return !equal();
            }
            gp[0] = effect_frame_increment(registers, gp[0]);
            compare(gp[0], gp[6]);
        } while (signed_less());
        return false;
    }

    void advance_component() {
        auto &gp = abi.gp;
        gp[3] = load(0x17787fu, gp[7] + 0x24u);
        recheck(0x177882u);
        gp[6] = load(0x177882u, gp[7] + 0x44u);
        recheck(0x177885u);
        gp[3] = state_product(registers, gp[3], gp[0]);
        gp[3] = iterator_flags(registers, gp[3], gp[6], 4, false);
        const auto generation = load(0x17788au, gp[3], 2);
        gp[6] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(generation))));
        recheck(0x17788du);
        gp[6] = effect_frame_shift_left(registers, gp[6], 16);
        gp[6] |= gp[0];
        test(gp[6]);
        gp[5] = gp[0];
        const auto parent = load(0x177894u, gp[3] + 0x4cu);
        compare(parent, none);
        recheck(0x177898u);
        if (!equal()) return;

        gp[2] = load(0x17789au, gp[4] + 0x24u);
        recheck(0x17789eu);
        save(0x17789eu, gp[2]);
        recheck(0x17789fu);
        gp[1] = gp[3];
        invoke(0x1778a1u, 0x1778d0u);
        test(gp[0] & 255u, 1);
        if (!equal()) return;

        gp[7] = load(0x1778aau, gp[3] + 0x18u);
        recheck(0x1778adu);
        compare(gp[7], none);
        if (!equal()) {
            invoke(0x1778b2u, 0x17de10u);
            put(0x1778b7u, gp[3] + 0x18u, none);
            recheck(0x1778beu);
        }
        save(0x1778beu, gp[6]);
        recheck(0x1778bfu);
        invoke(0x1778bfu, 0x177020u);
        gp[7] = load(0x1778c4u, gp[4] + 0x14u);
        recheck(0x1778c8u);
    }

    void close() {
        auto &gp = abi.gp;
        gp[7] = restore(0x177870u);
        recheck(0x177871u);
        gp[6] = restore(0x177871u);
        recheck(0x177872u);
        gp[5] = restore(0x177872u);
        recheck(0x177873u);
        gp[3] = restore(0x177873u);
        recheck(0x177874u);
        gp[4] = iterator_flags(registers, gp[4], 0x10u, 4, false);
        (void)restore(0x177877u);
        gp[4] += 4u;
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services),
          cursor(services.instruction ? &services.instruction_cursor : nullptr) {}

    void run() {
        open();
        while (find_occupied_slot()) advance_component();
        close();
    }
};

} // namespace particle_frame_update
