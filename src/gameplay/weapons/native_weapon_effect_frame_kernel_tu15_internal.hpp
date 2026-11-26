// Included after the shared effect frame flag and memory helpers.
namespace effect_frame_update {

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

    std::uint32_t signed_word(std::uint32_t site, std::uint32_t address) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(load(site, address, 2)))));
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

    void tail(std::uint32_t site, std::uint32_t target) {
        if (services.instruction) services.instruction(site);
        if (!services.required_owner)
            throw std::runtime_error("Native effect frame requires its actual particle tail owner");
        services.required_owner(memory, {site, target, true}, abi, observer);
    }

    void test(std::uint32_t value) {
        iterator_logic_flags(registers, value, 4);
    }

    void compare(std::uint32_t left, std::uint32_t right) {
        iterator_flags(registers, left, right, 4, true);
    }

    bool equal() const { return (abi.eflags & 64u) != 0; }
    bool signed_less() const { return bool(abi.eflags & 128u) != bool(abi.eflags & 2048u); }

    bool first_effect() {
        auto &gp = abi.gp;
        save(0x179df0u, gp[3]);
        recheck(0x179df1u);
        gp[3] = load(0x179df1u, 0x4f720cu);
        recheck(0x179df7u);
        save(0x179df7u, gp[6]);
        recheck(0x179df8u);
        gp[2] = 0;
        test(0);
        gp[1] = gp[3];
        invoke(0x179dfcu, 0x16ea60u);
        gp[6] = none;
        test(none);
        compare(gp[0], none);
        if (!equal()) {
            gp[1] = load(0x179e09u, gp[3] + 0x24u);
            recheck(0x179e0cu);
            gp[2] = load(0x179e0cu, gp[3] + 0x44u);
            recheck(0x179e0fu);
            gp[1] = state_product(registers, gp[1], gp[0]);
            gp[6] = signed_word(0x179e12u, gp[1] + gp[2]);
            recheck(0x179e16u);
            gp[6] = effect_frame_shift_left(registers, gp[6], 16);
            gp[6] |= gp[0];
            test(gp[6]);
        }
        compare(gp[6], none);
        return !equal();
    }

    void advance_components() {
        auto &gp = abi.gp;
        gp[1] = load(0x179e30u, gp[3] + 0x44u);
        recheck(0x179e33u);
        gp[7] = gp[6] & 65535u;
        test(gp[7]);
        gp[0] = state_product(registers, gp[7], 0x190u);
        gp[0] = load(0x179e43u, gp[0] + gp[1] + 0xfcu);
        recheck(0x179e4au);
        compare(gp[0], none);
        if (equal()) return;

        do {
            save(0x179e50u, gp[6]);
            recheck(0x179e51u);
            invoke(0x179e51u, 0x179ed0u);
            compare(gp[0], none);
        } while (!equal());
        gp[3] = load(0x179e5bu, 0x4f720cu);
        recheck(0x179e61u);
    }

    void scan_following_index() {
        auto &gp = abi.gp;
        compare(gp[6], none);
        gp[0] = equal() ? 0u : gp[7] + 1u;
        gp[6] = none;
        test(gp[0]);
        if (signed_less()) return;
        gp[7] = load(0x179e74u, gp[3] + 0x38u);
        recheck(0x179e77u);
        compare(gp[0], gp[7]);
        if (!signed_less()) return;
        gp[2] = load(0x179e7bu, gp[3] + 0x48u);
        recheck(0x179e7eu);

        do {
            gp[1] = gp[0] & 31u;
            test(gp[1]);
            gp[5] = effect_frame_shift_left(registers, 1u, gp[1]);
            gp[1] = effect_frame_signed_right_five(registers, gp[0]);
            const auto occupied = load(0x179e91u, gp[2] + gp[1] * 4u);
            test(occupied & gp[5]);
            recheck(0x179e94u);
            if (!equal()) {
                gp[6] = gp[0];
                return;
            }
            gp[0] = effect_frame_increment(registers, gp[0]);
            compare(gp[0], gp[7]);
        } while (signed_less());
    }

    bool following_effect() {
        auto &gp = abi.gp;
        scan_following_index();
        gp[0] = none;
        test(none);
        compare(gp[6], none);
        if (!equal()) {
            gp[2] = load(0x179ea7u, gp[3] + 0x24u);
            recheck(0x179eaau);
            gp[0] = load(0x179eaau, gp[3] + 0x44u);
            recheck(0x179eadu);
            gp[2] = state_product(registers, gp[2], gp[6]);
            gp[0] = signed_word(0x179eb0u, gp[2] + gp[0]);
            recheck(0x179eb4u);
            gp[0] = effect_frame_shift_left(registers, gp[0], 16);
            gp[0] |= gp[6];
            test(gp[0]);
        }
        compare(gp[0], none);
        gp[6] = gp[0];
        return !equal();
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services),
          cursor(services.instruction ? &services.instruction_cursor : nullptr) {}

    void run() {
        auto &gp = abi.gp;
        if (first_effect()) {
            save(0x179e24u, gp[5]);
            recheck(0x179e25u);
            save(0x179e25u, gp[7]);
            recheck(0x179e26u);
            do {
                advance_components();
            } while (following_effect());
            gp[7] = restore(0x179ec4u);
            recheck(0x179ec5u);
            gp[5] = restore(0x179ec5u);
            recheck(0x179ec6u);
        }
        gp[6] = restore(0x179ec6u);
        recheck(0x179ec7u);
        gp[3] = restore(0x179ec7u);
        recheck(0x179ec8u);
        tail(0x179ec8u, 0x177830u);
    }
};

} // namespace effect_frame_update
