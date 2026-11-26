// Included by the Effect dispatcher after its memory and ABI helpers.
namespace effect_systems_update {

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

    void recheck(std::uint32_t next) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(services.instruction))
            throw Resume{next};
    }

    std::uint32_t read_at(std::uint32_t site, std::uint32_t address, unsigned width = 4) {
        at(site);
        return read(memory, address, width);
    }

    std::uint32_t local(std::uint32_t site, unsigned offset, unsigned width = 4) {
        return read_at(site, abi.gp[4] + offset, width);
    }

    void save(std::uint32_t site, std::uint32_t value) {
        at(site);
        push(memory, abi, observer, site, value);
    }

    void write_local(std::uint32_t site, unsigned offset, std::uint32_t value, unsigned width = 4) {
        at(site);
        store(memory, observer, site, abi.gp[4] + offset, value, width);
    }

    std::uint32_t add(std::uint32_t left, std::uint32_t right) {
        return iterator_flags(registers, left, right, 4, false);
    }

    void compare(std::uint32_t left, std::uint32_t right, unsigned width = 4) {
        iterator_flags(registers, left, right, width, true);
    }

    void test(std::uint32_t value, unsigned width = 4) {
        iterator_logic_flags(registers, value, width);
    }

    bool zero() const {
        return (abi.eflags & 64u) != 0;
    }

    void low(unsigned reg, std::uint32_t value, unsigned width) {
        const auto mask = width == 1u ? 255u : 65535u;
        abi.gp[reg] = (abi.gp[reg] & ~mask) | (value & mask);
    }

    void invoke(std::uint32_t site, std::uint32_t target) {
        call(memory, abi, observer, services, site, target);
        recheck(site + 5u);
    }

    void invoke_virtual(std::uint32_t site, unsigned offset) {
        if (services.instruction)
            services.instruction(site);
        const auto target = read(memory, abi.gp[2] + offset);
        push(memory, abi, observer, site, site + 3u);
        if (!services.required_owner)
            throw std::runtime_error("Native effect system requires its actual virtual owner");
        services.required_owner(memory, {site, target, false}, abi, observer);
        recheck(site + 3u);
    }

    bool open_systems() {
        auto &gp = abi.gp;
        gp[4] = iterator_flags(registers, gp[4], 0x54u, 4, true);
        gp[2] = read_at(0x179ed3u, 0x51df74u);
        recheck(0x179ed9u);
        gp[1] = read_at(0x179ed9u, 0x4f720cu);
        recheck(0x179edfu);
        save(0x179edfu, gp[3]);
        recheck(0x179ee0u);
        gp[3] = read_at(0x179ee0u, gp[1] + 0x44u);
        recheck(0x179ee3u);
        save(0x179ee3u, gp[5]);
        recheck(0x179ee4u);
        gp[5] = local(0x179ee4u, 0x60u);
        recheck(0x179ee8u);
        gp[0] &= 65535u;
        test(gp[0]);
        gp[0] *= 0x54u;
        gp[5] &= 65535u;
        test(gp[5]);
        save(0x179ef6u, gp[6]);
        recheck(0x179ef7u);
        gp[5] *= 0x190u;
        const auto component_table = read_at(0x179efdu, gp[2] + 0x44u);
        gp[0] = add(gp[0], component_table);
        recheck(0x179f00u);
        gp[6] = gp[0];
        save(0x179f02u, gp[7]);
        recheck(0x179f03u);
        gp[7] = read_at(0x179f03u, gp[6] + 0x38u);
        recheck(0x179f06u);
        gp[1] = gp[6];
        gp[5] = add(gp[5], gp[3]);
        write_local(0x179f0au, 0x20u, gp[6]);
        recheck(0x179f0eu);
        write_local(0x179f0eu, 0x18u, gp[7]);
        recheck(0x179f12u);
        invoke(0x179f12u, 0x178070u);
        gp[1] = read_at(0x179f17u, gp[0] + 8u);
        recheck(0x179f1au);
        gp[2] = read_at(0x179f1au, gp[5] + gp[1] * 4u + 0x7cu);
        recheck(0x179f1eu);
        write_local(0x179f1eu, 0x14u, gp[2]);
        recheck(0x179f22u);
        const auto systems = read_at(0x179f22u, gp[0] + 0xcu, 2);
        compare(systems, 0, 2);
        recheck(0x179f27u);
        write_local(0x179f27u, 0x10u, gp[0]);
        recheck(0x179f2bu);
        return !zero();
    }

    void initialize_systems() {
        auto &gp = abi.gp;
        gp[1] = read_at(0x179f31u, gp[6] + 0x30u);
        recheck(0x179f34u);
        gp[0] = read_at(0x179f34u, gp[5] + 0x104u);
        recheck(0x179f3au);
        gp[7] = read_at(0x179f3au, gp[5] + 0x108u);
        recheck(0x179f40u);
        write_local(0x179f40u, 0x2cu, gp[1]);
        recheck(0x179f44u);
        gp[1] = gp[6];
        write_local(0x179f46u, 0x1cu, gp[0]);
        recheck(0x179f4au);
        invoke(0x179f4au, 0x178070u);
        gp[2] = read_at(0x179f4fu, gp[0] + 4u);
        recheck(0x179f52u);
        invoke(0x179f52u, 0x13a6b0u);
        gp[3] = gp[0];
        gp[2] = read_at(0x179f59u, gp[3]);
        recheck(0x179f5bu);
        gp[1] = gp[3];
        invoke_virtual(0x179f5du, 0x48u);
        gp[2] = read_at(0x179f60u, gp[3]);
        recheck(0x179f62u);
        gp[1] = gp[3];
        write_local(0x179f64u, 0x68u, gp[0], 1);
        recheck(0x179f68u);
        invoke_virtual(0x179f68u, 0x44u);
        gp[1] = local(0x179f6bu, 0x1cu);
        recheck(0x179f6fu);
        save(0x179f6fu, gp[0]);
        recheck(0x179f70u);
        low(0, local(0x179f70u, 0x6cu, 1), 1);
        recheck(0x179f74u);
        invoke(0x179f74u, 0x178920u);
    }

    struct LocationSites {
        std::uint32_t descriptor;
        std::uint32_t mode;
        std::uint32_t push_mode;
        std::uint32_t cursor_address;
        std::uint32_t push_cursor;
        std::uint32_t push_effect;
        std::uint32_t call;
    };

    static constexpr LocationSites first_location{
        0x179f79u, 0x179f7du, 0x179f81u, 0x179f82u, 0x179f86u, 0x179f87u, 0x179f88u};
    static constexpr LocationSites following_location{
        0x179fedu, 0x179ff1u, 0x179ff5u, 0x179ff6u, 0x179ffau, 0x179ffbu, 0x179ffcu};

    bool next_location(const LocationSites &sites) {
        auto &gp = abi.gp;
        gp[0] = local(sites.descriptor, 0x10u);
        recheck(sites.mode);
        gp[0] = read_at(sites.mode, gp[0] + 0x12u, 2);
        recheck(sites.push_mode);
        save(sites.push_mode, gp[0]);
        recheck(sites.cursor_address);
        gp[1] = gp[4] + 0x18u;
        save(sites.push_cursor, gp[1]);
        recheck(sites.push_effect);
        save(sites.push_effect, gp[5]);
        recheck(sites.call);
        invoke(sites.call, 0x17dce0u);
        gp[3] = gp[0];
        test(gp[3]);
        return !zero();
    }

    void advance_particle() {
        auto &gp = abi.gp;
        save(0x179f93u, 1u);
        recheck(0x179f95u);
        gp[7] = gp[4] + 0x34u;
        gp[6] = gp[5];
        invoke(0x179f9bu, 0x17ba50u);
        low(3, read_at(0x179fa0u, gp[3] + 2u, 2), 2);
        recheck(0x179fa4u);
        compare(gp[3], none, 2);
        bool player_node{};
        if (!zero()) {
            test((gp[3] >> 8u) & 255u, 1);
            write_local(0x179facu, 0x68u, 1u, 1);
            recheck(0x179fb1u);
            player_node = (abi.eflags & 128u) != 0;
        }
        if (!player_node) {
            write_local(0x179fb3u, 0x68u, 0u, 1);
            recheck(0x179fb8u);
        }

        gp[6] = local(0x179fb8u, 0x2cu);
        recheck(0x179fbcu);
        compare(gp[6], none);
        if (zero())
            return;
        gp[2] = read_at(0x179fc1u, 0x52d71cu);
        recheck(0x179fc7u);
        gp[1] = local(0x179fc7u, 0x68u);
        recheck(0x179fcbu);
        gp[7] = read_at(0x179fcbu, gp[2] + 0x44u);
        recheck(0x179fceu);
        gp[6] &= 65535u;
        test(gp[6]);
        gp[6] *= 0x34u;
        save(0x179fd7u, gp[1]);
        recheck(0x179fd8u);
        gp[1] = local(0x179fd8u, 0x24u);
        recheck(0x179fdcu);
        gp[6] = add(gp[6], gp[7]);
        save(0x179fdeu, gp[0]);
        recheck(0x179fdfu);
        gp[0] = gp[6];
        invoke(0x179fe1u, 0x251f70u);
        gp[2] = read_at(0x179fe6u, gp[6] + 0xcu);
        recheck(0x179fe9u);
        write_local(0x179fe9u, 0x2cu, gp[2]);
        recheck(0x179fedu);
    }

    void close(std::uint32_t first_pop, std::uint32_t ret) {
        auto &gp = abi.gp;
        for (unsigned n = 0; n < 4u; ++n) {
            at(first_pop + n);
            constexpr std::array destinations{7u, 6u, 5u, 3u};
            gp[destinations[n]] = pop(memory, abi);
            recheck(first_pop + n + 1u);
        }
        gp[4] = add(gp[4], 0x54u);
        at(ret);
        pop(memory, abi);
        gp[4] += 4u;
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services),
          cursor(services.instruction ? &services.instruction_cursor : nullptr) {}

    void run() {
        if (!open_systems()) {
            abi.gp[0] = abi.gp[7];
            close(0x17a017u, 0x17a01eu);
            return;
        }
        initialize_systems();
        if (next_location(first_location)) {
            do {
                advance_particle();
            } while (next_location(following_location));
        }
        abi.gp[0] = local(0x17a007u, 0x18u);
        recheck(0x17a00bu);
        close(0x17a00bu, 0x17a012u);
    }
};

} // namespace effect_systems_update
