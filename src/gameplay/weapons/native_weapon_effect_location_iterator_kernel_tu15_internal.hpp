// Included after the weapon dispatcher's integer ABI and memory helpers.
namespace effect_location_iterator {

enum class Resume {
    cursor_argument, cursor_head, clear_result, slot_index, save_edi, record_table,
    record_address, publish_cursor, node, node_flags, filter, push_mode,
    push_cursor, push_forwarded, recurse, restore_edi, restore_ebx, restore_esi, return_address,
};

class Frame {
    Memory &memory;
    IteratorRegisters &registers;
    const Observer &observer;
    const Services &services;
    std::uint32_t *position;

    // A synchronous memory policy can open a trace after an operand has
    // completed. Continue at its successor with the original frame intact.
    void recheck(Resume next) const {
        if (!position) {
            if (services.instruction)
                throw next;
            return;
        }
        const auto &cursor = services.instruction_cursor;
        if ((cursor.first_trace && *cursor.first_trace) ||
            (cursor.second_trace && *cursor.second_trace))
            throw next;
    }

    void at(std::uint32_t pc) const {
        if (position)
            *position = pc;
    }

    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) const {
        at(pc);
        return read(memory, address, width);
    }

    void save(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        push(memory, registers, observer, pc, value);
    }

    std::uint32_t restore(std::uint32_t pc) {
        at(pc);
        return pop(memory, registers);
    }

    bool open() {
        auto &gp = registers.gp;
        save(0x17dce0u, gp[6]);
        recheck(Resume::cursor_argument);
        gp[6] = load(0x17dce1u, gp[4] + 12u);
        recheck(Resume::cursor_head);
        gp[1] = load(0x17dce5u, gp[6]);
        recheck(Resume::clear_result);
        gp[0] = 0;
        iterator_flags(registers, gp[1], none, 4, true);
        return gp[1] != none;
    }

    bool select_node() {
        auto &gp = registers.gp;
        gp[0] = load(0x17dceeu, 0x4f7208u);
        recheck(Resume::slot_index);
        const auto index = gp[1] & 65535u;
        iterator_logic_flags(registers, index, 4);
        gp[1] = index * 60u;
        save(0x17dcfcu, gp[3]);
        recheck(Resume::save_edi);
        save(0x17dcfdu, gp[7]);
        recheck(Resume::record_table);
        const auto records = load(0x17dcfeu, gp[0] + 0x44u);
        gp[1] = iterator_flags(registers, gp[1], records, 4, false);
        recheck(Resume::record_address);
        gp[0] = gp[1];

        gp[1] = load(0x17dd03u, gp[0] + 4u);
        recheck(Resume::publish_cursor);
        at(0x17dd06u);
        store(memory, observer, 0x17dd06u, gp[6], gp[1]);
        recheck(Resume::node);
        // The cursor may alias the record, including its node word.
        const auto node = load(0x17dd08u, gp[0] + 2u, 2);
        gp[1] = (gp[1] & 0xffff0000u) | node;
        recheck(Resume::node_flags);
        const bool first_person = node != 65535u && (node & 0x8000u);
        gp[3] = (gp[3] & 0xffffff00u) | unsigned(first_person);
        iterator_logic_flags(registers, first_person ? node >> 8u : 0u, 1);

        gp[7] = load(0x17dd1cu, gp[4] + 24u);
        recheck(Resume::filter);
        const auto mode = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(gp[7]))));
        gp[2] &= 0xffffff00u;
        gp[1] = iterator_flags(registers, mode, 1u, 4, true);
        registers.eflags &= ~1u;
        if (mode == 1u) {
            gp[2] |= 1u;
        } else {
            gp[1] = iterator_flags(registers, gp[1], 2u, 4, true);
            if (mode == 3u)
                return true;
        }
        return iterator_flags(registers, gp[3], gp[2], 1, true) == 0;
    }

public:
    Frame(Memory &memory, IteratorRegisters &registers, const Observer &observer, const Services &services)
        : memory(memory), registers(registers), observer(observer), services(services),
          position(services.instruction ? services.instruction_cursor.position : nullptr) {}

    template <class Continue>
    void run(Continue next) {
        auto &gp = registers.gp;
        if (open()) {
            if (!select_node()) {
                gp[2] = load(0x17dd35u, gp[4] + 16u);
                recheck(Resume::push_mode);
                save(0x17dd39u, gp[7]);
                recheck(Resume::push_cursor);
                save(0x17dd3au, gp[6]);
                recheck(Resume::push_forwarded);
                save(0x17dd3bu, gp[2]);
                recheck(Resume::recurse);
                save(0x17dd3cu, 0x17dd41u);
                next();
                recheck(Resume::restore_edi);
            }
            gp[7] = restore(0x17dd41u);
            recheck(Resume::restore_ebx);
            gp[3] = restore(0x17dd42u);
            recheck(Resume::restore_esi);
        }
        gp[6] = restore(0x17dd43u);
        recheck(Resume::return_address);
        restore(0x17dd44u);
        gp[4] += 12u;
    }
};

} // namespace effect_location_iterator
