// Included by the weapon effect dispatcher with its real ABI and call helpers.
namespace authored_marker_emission {

enum class Resume { retained, completed, quaternion_return, product_return };

class Emission {
    Memory &memory;
    Abi &abi;
    const Observer &observer;
    const Services &services;
    IteratorRegisters registers{abi.gp, abi.eflags};
    std::optional<SseEnvironment> scalar_environment;
    std::uint32_t *position{};

    bool admitted() {
        if (!abi.floating_point || abi.active_mmx ||
            (abi.floating_point->mxcsr & 0x1f80u) != 0x1f80u ||
            (abi.floating_point->mxcsr & 0xffff0000u))
            return false;
        position = nullptr;
        if (native_model_marker_lookup_detail::protocol(services, observer)) {
            if (services.instruction)
                position = services.instruction_cursor.position;
            return true;
        }
        const auto *vehicle = services.instruction.target<VehicleMarkerEmissionInstruction>();
        return vehicle && !observer && vehicle->native_position(position);
    }

    void at(std::uint32_t site) const {
        if (position)
            *position = site;
        if (scalar_environment)
            abi.floating_point->mxcsr = _mm_getcsr();
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

    std::uint32_t add(std::uint32_t left, std::uint32_t right) {
        return iterator_flags(registers, left, right, 4, false);
    }

    std::uint32_t increment(std::uint32_t value) {
        const auto carry = abi.eflags & 1u;
        const auto result = add(value, 1u);
        abi.eflags = (abi.eflags & ~1u) | carry;
        return result;
    }

    void compare(std::uint32_t left, std::uint32_t right, unsigned width = 4) {
        iterator_flags(registers, left, right, width, true);
    }

    bool signed_less() const { return bool(abi.eflags & 128u) != bool(abi.eflags & 2048u); }

    void low_byte(std::uint32_t value) { abi.gp[0] = (abi.gp[0] & 0xffffff00u) | (value & 255u); }

    bool open() {
        auto &gp = abi.gp;
        gp[4] = iterator_flags(registers, gp[4], 12u, 4, true);
        save(0x1e1203u, gp[3]);
        gp[3] = 0;
        compare(gp[0], none);
        if (abi.eflags & 64u)
            return false;

        gp[2] = load(0x1e120fu, 0x4f0414u);
        gp[1] = (gp[1] & 65535u) << 4u;
        iterator_logic_flags(registers, gp[1], 4);
        gp[1] = load(0x1e121eu, gp[1] + gp[2] + 8u);
        gp[2] = gp[0] * 3u;
        gp[0] = load(0x1e1225u, gp[1] + 0x5cu);
        gp[1] = gp[0] + gp[2] * 4u;
        compare(load(0x1e122bu, gp[1] + 4u), gp[3]);
        put(0x1e122eu, gp[4] + 8u, gp[1]);
        put(0x1e1232u, gp[4] + 12u, gp[3]);
        if ((abi.eflags & 64u) || signed_less())
            return false;

        save(0x1e123cu, gp[5]);
        gp[5] = load(0x1e123du, gp[4] + 0x28u);
        save(0x1e1241u, gp[6]);
        put(0x1e1242u, gp[4] + 12u, gp[3]);
        save(0x1e1246u, gp[7]);
        return true;
    }

    bool accept_candidate() {
        auto &gp = abi.gp;
        gp[7] = load(0x1e1250u, gp[1] + 8u);
        gp[2] = load(0x1e1253u, gp[4] + 16u);
        gp[6] = load(0x1e1257u, gp[4] + 0x20u);
        gp[7] = add(gp[7], gp[2]);
        iterator_logic_flags(registers, gp[6], 4);
        if (abi.eflags & 64u)
            return true;
        low_byte(load(0x1e1261u, gp[7], 1));
        compare(gp[0] & 255u, 255u, 1);
        if (abi.eflags & 64u)
            return true;
        gp[2] = std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(gp[0]))));
        low_byte(load(0x1e126au, gp[2] + gp[6], 1));
        compare(gp[0] & 255u, load(0x1e126du, gp[7] + 1u, 1), 1);
        low_byte((abi.eflags & 64u) != 0u);
        iterator_logic_flags(registers, gp[0] & 255u, 1);
        return !(abi.eflags & 64u);
    }

    bool prepare_rotation() {
        auto &gp = abi.gp;
        compare(gp[3], load(0x1e127bu, gp[4] + 0x34u));
        if (!signed_less())
            return false;
        gp[1] = load(0x1e1285u, gp[4] + 0x24u);
        gp[0] = load(0x1e1289u, gp[7] + 2u, 1);
        gp[6] = gp[5];
        gp[3] = increment(gp[3]);
        gp[5] = add(gp[5], 0x70u);
        iterator_logic_flags(registers, gp[1], 4);
        if (!(abi.eflags & 64u))
            gp[0] = load(0x1e1297u, gp[1] + gp[0] * 4u);
        put(0x1e129au, gp[6], gp[0] & 65535u, 2);
        gp[2] = gp[6] + 4u;
        gp[1] = gp[7] + 16u;
        gp[0] = gp[2] + 4u;
        return true;
    }

    void prepare_transform() {
        auto &gp = abi.gp;
        abi.xmm[0] = {};
        constexpr std::array zero_sites{0x1e12aeu, 0x1e12b3u, 0x1e12b8u};
        for (unsigned axis = 0; axis < zero_sites.size(); ++axis)
            put(zero_sites[axis], gp[2] + 0x28u + axis * 4u, 0u);
        at(0x1e12bdu);
        scalar(memory, abi, 0, 0x46a0d0u);
        put(0x1e12c5u, gp[2], abi.xmm[0][0]);
        gp[1] = gp[7] + 4u;
        gp[0] = load(0x1e12ccu, gp[1]);
        put(0x1e12ceu, gp[2] + 0x28u, gp[0]);
        gp[0] = load(0x1e12d1u, gp[1] + 4u);
        put(0x1e12d4u, gp[2] + 0x2cu, gp[0]);
        gp[1] = load(0x1e12d7u, gp[1] + 8u);
        put(0x1e12dau, gp[2] + 0x30u, gp[1]);

        const auto node = std::bit_cast<std::int16_t>(std::uint16_t(load(0x1e12ddu, gp[6], 2)));
        gp[1] = std::uint32_t(std::int32_t(node) * 52);
        abi.eflags &= ~0x801u;
        gp[0] = gp[6] + 0x38u;
        save(0x1e12e6u, gp[0]);
        gp[1] = add(gp[1], load(0x1e12e7u, gp[4] + 0x2cu));
    }

    void reflect_position() {
        constexpr std::array reads{0x1e1308u, 0x1e1315u, 0x1e131fu};
        constexpr std::array stores{0x1e130du, 0x1e131au, 0x1e1324u};
        abi.xmm[0] = {};
        for (unsigned axis = 0; axis < reads.size(); ++axis) {
            const auto lane = axis == 2 ? 0u : 1u;
            if (lane != 0u)
                abi.xmm[1] = abi.xmm[0];
            const auto address = abi.gp[6] + 0x48u + axis * 4u;
            const auto word = load(reads[axis], address);
            if (axis == 0u)
                scalar_environment.emplace(abi);
            const auto right = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word)));
            vector(abi, lane, native_tu15_leaves::ordered_difference_ss(vector(abi, lane), right));
            put(stores[axis], address, abi.xmm[lane][0]);
        }
        scalar_environment.reset();
    }

    void publish_result() {
        auto &gp = abi.gp;
        gp[1] = load(0x1e12f0u, gp[7] + 0x20u);
        low_byte(load(0x1e12f3u, gp[4] + 0x2cu, 1));
        iterator_logic_flags(registers, gp[0] & 255u, 1);
        put(0x1e12f9u, gp[6] + 0x6cu, gp[1]);
        gp[1] = load(0x1e12fcu, gp[4] + 0x14u);
        if (!(abi.eflags & 64u))
            reflect_position();
    }

    bool advance() {
        auto &gp = abi.gp;
        gp[0] = load(0x1e1329u, gp[4] + 0x18u);
        gp[6] = load(0x1e132du, gp[4] + 0x10u);
        gp[2] = load(0x1e1331u, gp[1] + 4u);
        gp[0] = increment(gp[0]);
        gp[6] = add(gp[6], 0x24u);
        compare(gp[0], gp[2]);
        put(0x1e133au, gp[4] + 0x18u, gp[0]);
        put(0x1e133eu, gp[4] + 0x10u, gp[6]);
        return signed_less();
    }

    void close(bool opened) {
        auto &gp = abi.gp;
        if (opened) {
            gp[7] = restore(0x1e1348u);
            gp[6] = restore(0x1e1349u);
            gp[5] = restore(0x1e134au);
        }
        gp[0] = gp[3];
        gp[3] = restore(opened ? 0x1e134du : 0x1e1356u);
        gp[4] = add(gp[4], 12u);
        restore(opened ? 0x1e1351u : 0x1e135au);
        gp[4] += 24u;
    }

public:
    Emission(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services) {}

    Resume run() {
        static const bool reference = [] {
            const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_MARKER_EMISSION");
            return value && *value;
        }();
        if (reference || !admitted())
            return Resume::retained;
        const bool opened = open();
        if (opened) {
            do {
                if (!accept_candidate())
                    continue;
                if (!prepare_rotation())
                    break;
                call(memory, abi, observer, services, 0x1e12a6u, 0x144910u);
                // Reopen admission after each real child. A provider may
                // change trace policy, callback identity, or the FP packet.
                if (!admitted())
                    return Resume::quaternion_return;
                prepare_transform();
                call(memory, abi, observer, services, 0x1e12ebu, 0x145560u);
                if (!admitted())
                    return Resume::product_return;
                publish_result();
            } while (advance());
        }
        close(opened);
        return Resume::completed;
    }
};

} // namespace authored_marker_emission
