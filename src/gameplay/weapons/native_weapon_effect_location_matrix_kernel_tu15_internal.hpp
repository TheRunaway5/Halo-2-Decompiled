// Included by the Effect dispatcher after its memory and ABI helpers.
namespace effect_location_matrix {

struct Resume {
    std::uint32_t pc;
};

bool admitted(const Services &services, const Observer &observer) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_EFFECT_LOCATION_COPY");
        return value && *value;
    }();
    return !reference && (!services.instruction || services.immutable_instruction_cursor.identifies(services)) &&
        native_model_marker_lookup_detail::protocol(services, observer);
}

class Frame {
    Memory &memory;
    Abi &abi;
    const Observer &observer;
    const Services &services;
    IteratorRegisters registers{abi.gp, abi.eflags};
    const Services::InstructionCursor *cursor;

    void at(std::uint32_t pc) const {
        if (cursor)
            *cursor->position = pc;
    }

    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(services.instruction))
            throw Resume{next};
    }

    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return read(memory, address, width);
    }

    void put(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        if (observer) {
            store(memory, observer, pc, address, value);
        } else {
            const std::array bytes{std::byte(value), std::byte(value >> 8u),
                                   std::byte(value >> 16u), std::byte(value >> 24u)};
            memory.write(address, bytes);
        }
    }

    void save(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        if (observer) {
            push(memory, abi, observer, pc, value);
        } else {
            const auto stack = abi.gp[4] - 4u;
            put(pc, stack, value);
            abi.gp[4] = stack;
        }
    }

    std::uint32_t restore(std::uint32_t pc) {
        at(pc);
        return pop(memory, abi);
    }

    bool zero() const {
        return (abi.eflags & 64u) != 0;
    }

    void logic(std::uint32_t value, unsigned width = 4) {
        iterator_logic_flags(registers, value, width);
    }

    void compare(std::uint32_t value, std::uint32_t other, unsigned width = 4) {
        iterator_flags(registers, value, other, width, true);
    }

    void invoke(std::uint32_t pc, std::uint32_t target) {
        call(memory, abi, observer, services, pc, target);
        recheck(pc + 5u);
    }

    static std::uint32_t signed_word(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }

    void copy_words(std::uint32_t transfer, std::uint32_t successor) {
        auto &gp = abi.gp;
        gp[1] = 13;
        do {
            // Each original MOVSD reads before writing. Its cursor advances
            // only on success, even when operands or the saved stack alias.
            const auto word = load(transfer, gp[6]);
            put(transfer, gp[7], word);
            const auto step = (abi.eflags & 1024u) ? std::uint32_t(-4) : 4u;
            gp[6] += step;
            gp[7] += step;
            --gp[1];
            recheck(gp[1] ? transfer : successor);
        } while (gp[1]);
    }

    void close(std::uint32_t edi, std::uint32_t esi, std::uint32_t ret, unsigned arguments) {
        abi.gp[7] = restore(edi);
        recheck(esi);
        abi.gp[6] = restore(esi);
        recheck(ret);
        restore(ret);
        abi.gp[4] += arguments;
    }

    bool copy_player_matrix() {
        auto &gp = abi.gp;
        logic((gp[0] >> 8u) & 255u, 1);
        if (!(abi.eflags & 128u))
            return false;
        gp[1] = load(0x17dd60u, gp[6] + 0x58u);
        recheck(0x17dd63u);
        compare(gp[1], none);
        if (zero())
            return false;
        gp[0] &= 0x7fffu;
        logic(gp[0]);
        save(0x17dd6du, gp[7]);
        recheck(0x17dd6eu);
        save(0x17dd6eu, gp[0]);
        recheck(0x17dd6fu);
        gp[0] = load(0x17dd6fu, gp[6] + 0x48u);
        recheck(0x17dd72u);
        save(0x17dd72u, gp[0]);
        recheck(0x17dd73u);
        invoke(0x17dd73u, 0x1693abu);
        close(0x17dd78u, 0x17dd79u, 0x17dd7au, 0);
        return true;
    }

    void copy_object_matrix() {
        auto &gp = abi.gp;
        gp[0] = load(0x17dd87u, gp[6] + 0x48u);
        recheck(0x17dd8au);
        gp[0] &= 65535u;
        logic(gp[0]);
        gp[1] = gp[0] * 3u;
        gp[0] = load(0x17dd92u, 0x4ecbd0u);
        recheck(0x17dd97u);
        gp[0] = load(0x17dd97u, gp[0] + 0x44u);
        recheck(0x17dd9au);
        gp[1] = load(0x17dd9au, gp[0] + gp[1] * 4u + 8u);
        recheck(0x17dd9eu);
        gp[0] = signed_word(load(0x17dd9eu, gp[1] + 0x116u, 2));
        recheck(0x17dda5u);
        gp[6] = signed_word(gp[2]) * 52u;
        gp[0] = iterator_flags(registers, gp[0], gp[1], 4, false);
        gp[6] = iterator_flags(registers, gp[6], gp[0], 4, false);
        copy_words(0x17ddb4u, 0x17ddb6u);
        close(0x17ddb6u, 0x17ddb7u, 0x17ddb8u, 0);
    }

    bool select_player_location() {
        auto &gp = abi.gp;
        if (zero())
            return false;
        compare(gp[6], 65535u, 2);
        if (zero())
            return false;
        logic(gp[6] & 0x8000u);
        if (zero())
            return false;
        gp[0] = load(0x17dddau, gp[2] + 0x58u);
        recheck(0x17ddddu);
        compare(gp[0], none);
        return !zero();
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services),
          cursor(services.instruction ? &services.instruction_cursor : nullptr) {}

    void direct() {
        auto &gp = abi.gp;
        compare(gp[0], 65535u, 2);
        save(0x17dd54u, gp[6]);
        recheck(0x17dd55u);
        save(0x17dd55u, gp[7]);
        recheck(0x17dd56u);
        gp[7] = gp[1];
        gp[6] = gp[2];
        if (zero()) {
            gp[2] = none;
            logic(gp[2]);
        } else {
            if (copy_player_matrix())
                return;
            gp[0] &= 0x7fffu;
            logic(gp[0]);
            gp[2] = gp[0];
        }
        copy_object_matrix();
    }

    void selected() {
        auto &gp = abi.gp;
        logic(gp[0] & 255u, 1);
        save(0x17ddc2u, gp[6]);
        recheck(0x17ddc3u);
        gp[6] = load(0x17ddc3u, gp[4] + 8u);
        recheck(0x17ddc7u);
        save(0x17ddc7u, gp[7]);
        recheck(0x17ddc8u);
        gp[7] = gp[1];
        if (select_player_location()) {
            gp[1] = gp[6] & 0x7fffu;
            logic(gp[1]);
            save(0x17ddeau, gp[1]);
            recheck(0x17ddebu);
            gp[1] = load(0x17ddebu, gp[2] + 0x48u);
            recheck(0x17ddeeu);
            save(0x17ddeeu, gp[1]);
            recheck(0x17ddefu);
            invoke(0x17ddefu, 0x169376u);
            gp[6] = gp[0];
            copy_words(0x17ddfbu, 0x17ddfdu);
            close(0x17ddfdu, 0x17ddfeu, 0x17ddffu, 4);
        } else {
            gp[1] = gp[7];
            gp[0] = gp[6];
            invoke(0x17de06u, 0x17dd50u);
            close(0x17de0bu, 0x17de0cu, 0x17de0du, 4);
        }
    }
};

} // namespace effect_location_matrix
