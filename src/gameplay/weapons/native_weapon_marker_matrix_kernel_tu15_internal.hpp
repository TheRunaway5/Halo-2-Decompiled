// Included by the weapon dispatcher after its ABI and memory helpers.
namespace authored_marker_matrix {

enum class Resume { retained, completed, parent_return, lookup_return, emission_return };

class MatrixLocations {
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
        const auto *vehicle = services.instruction.target<VehicleMarkerMatrixInstruction>();
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

    std::uint32_t load_signed_word(std::uint32_t site, std::uint32_t address) const {
        const auto word = std::uint16_t(load(site, address, 2));
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(word)));
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

    void compare(std::uint32_t left, std::uint32_t right) {
        iterator_flags(registers, left, right, 4, true);
    }

    std::uint32_t right_shift(std::uint32_t value, unsigned count) {
        const auto result = value >> count;
        iterator_logic_flags(registers, result, 4);
        abi.eflags = (abi.eflags & ~1u) | ((value >> (count - 1u)) & 1u);
        return result;
    }

    bool open() {
        auto &gp = abi.gp;
        gp[4] = iterator_flags(registers, gp[4], 8u, 4, true);
        save(0xbad63u, gp[5]);
        save(0xbad64u, gp[6]);
        gp[6] = load(0xbad65u, gp[4] + 0x14u);
        gp[5] = 0;
        iterator_logic_flags(registers, gp[1] & 255u, 1);
        save(0xbad6du, gp[7]);
        gp[0] = gp[6];
        return abi.eflags & 64u;
    }

    bool prepare_group_lookup() {
        auto &gp = abi.gp;
        compare(gp[0], none);
        if (abi.eflags & 64u)
            return false;

        gp[1] = load(0xbad81u, 0x4ecbd0u);
        gp[2] = load(0xbad87u, gp[1] + 0x44u);
        gp[0] &= 65535u;
        iterator_logic_flags(registers, gp[0], 4);
        gp[0] *= 3u;
        gp[0] = load(0xbad92u, gp[2] + gp[0] * 4u + 8u);
        gp[1] = load(0xbad96u, gp[0]);
        gp[2] = load(0xbad98u, 0x4f0414u);
        gp[1] = (gp[1] & 65535u) << 4u;
        iterator_logic_flags(registers, gp[1], 4);
        gp[1] = load(0xbada7u, gp[1] + gp[2] + 8u);
        gp[1] = load(0xbadabu, gp[1] + 0x38u);
        compare(gp[1], none);
        if (abi.eflags & 64u)
            return false;

        gp[6] = load_signed_word(0xbadb3u, gp[0] + 0x11au);
        gp[5] = load_signed_word(0xbadbau, gp[0] + 0x116u);
        gp[1] = (gp[1] & 65535u) << 4u;
        iterator_logic_flags(registers, gp[1], 4);
        gp[7] = load(0xbadcau, gp[1] + gp[2] + 8u);
        gp[2] = load(0xbadceu, gp[0] + 4u);
        gp[1] = load_signed_word(0xbadd1u, gp[0] + 0x114u);
        const auto object_flags = gp[2] >> 10u;
        gp[2] = (object_flags & 0xffffff00u) | (object_flags & 1u);
        gp[6] = add(gp[6], gp[0]);
        gp[5] = add(gp[5], gp[0]);
        put(0xbade2u, gp[4] + 0xcu, gp[2] & 255u, 1);

        const auto product = std::uint64_t(0x4ec4ec4fu) * gp[1];
        gp[0] = std::uint32_t(product);
        gp[2] = std::uint32_t(product >> 32u);
        abi.eflags = (abi.eflags & ~0x801u) | (gp[2] ? 0x801u : 0u);
        gp[1] = load(0xbadedu, gp[7] + 4u);
        gp[7] = load(0xbadf0u, gp[4] + 0x1cu);
        gp[2] = right_shift(gp[2], 4);
        // Both writes are real. The render-model handle replaces the extent
        // quotient only after that earlier provider boundary has completed.
        put(0xbadf7u, gp[4] + 0x10u, gp[2]);
        put(0xbadfbu, gp[4] + 0x10u, gp[1]);
        return true;
    }

    void prepare_emission() {
        auto &gp = abi.gp;
        gp[2] = load_signed_word(0xbae04u, gp[4] + 0x20u);
        gp[1] = load(0xbae09u, gp[4] + 0xcu);
        save(0xbae0du, gp[2]);
        save(0xbae0eu, gp[3]);
        save(0xbae0fu, gp[1]);
        gp[1] = load(0xbae10u, gp[4] + 0x1cu);
        save(0xbae14u, gp[5]);
        save(0xbae15u, 0);
        save(0xbae17u, gp[6]);
    }

    void reflect_world_translation() {
        constexpr std::array reads{0xbaec5u, 0xbaed2u, 0xbaedcu};
        constexpr std::array writes{0xbaecau, 0xbaed7u, 0xbaee1u};
        for (unsigned axis = 0; axis < 3u; ++axis) {
            const auto lane = axis == 2u ? 0u : 1u;
            if (lane)
                abi.xmm[1] = abi.xmm[0];
            const auto address = abi.gp[3] + 0x48u + axis * 4u;
            const auto word = load(reads[axis], address);
            if (axis == 0u)
                scalar_environment.emplace(abi);
            const auto offset = _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word)));
            vector(abi, lane, native_tu15_leaves::ordered_difference_ss(vector(abi, lane), offset));
            put(writes[axis], address, abi.xmm[lane][0]);
        }
    }

    bool fallback_matrix() {
        auto &gp = abi.gp;
        abi.xmm[0] = {};
        gp[1] = load(0xbae2fu, 0x4ecbd0u);
        gp[2] = load(0xbae35u, gp[1] + 0x44u);
        abi.xmm[1] = {load(0xbae38u, 0x46a0d0u), 0, 0, 0};
        gp[6] &= 65535u;
        gp[0] = gp[6] * 12u;
        iterator_logic_flags(registers, gp[0], 4);
        gp[2] = load(0xbae4eu, gp[0] + gp[2] + 8u);
        put(0xbae52u, gp[3], 0, 2);
        for (unsigned field = 0; field < 13u; ++field) {
            const auto lane = field == 0u || field == 1u || field == 5u || field == 9u;
            put(0xbae57u + field * 5u, gp[3] + 4u + field * 4u, abi.xmm[lane][0]);
        }

        gp[1] = load(0xbae98u, gp[1] + 0x44u);
        gp[0] = load(0xbae9bu, gp[0] + gp[1] + 8u);
        gp[6] = load_signed_word(0xbae9fu, gp[0] + 0x116u);
        gp[6] = add(gp[6], gp[0]);
        gp[7] = gp[3] + 0x38u;
        gp[1] = 13;
        // REP MOVSD exposes each overlapping transfer and both live pointers.
        do {
            const auto value = load(0xbaeb0u, gp[6]);
            put(0xbaeb0u, gp[7], value);
            const auto step = abi.eflags & 1024u ? std::uint32_t(-4) : 4u;
            gp[6] += step;
            gp[7] += step;
            --gp[1];
        } while (gp[1]);
        put(0xbaeb2u, gp[3] + 0x6cu, abi.xmm[0][0]);
        gp[2] = load(0xbaeb7u, gp[2] + 4u) >> 10u;
        iterator_logic_flags(registers, gp[2] & 1u, 1);
        if (!(abi.eflags & 64u))
            reflect_world_translation();
        gp[0] = load(0xbaee6u, gp[4] + 0x1cu);
        iterator_logic_flags(registers, gp[0], 4);
        return abi.eflags & 64u;
    }

    void close(bool unnamed_fallback) {
        auto &gp = abi.gp;
        gp[7] = restore(unnamed_fallback ? 0xbaeeeu : 0xbaefcu);
        gp[6] = restore(unnamed_fallback ? 0xbaeefu : 0xbaefdu);
        gp[0] = unnamed_fallback ? 1u : (gp[0] & 0xffff0000u) | (gp[5] & 65535u);
        gp[5] = restore(unnamed_fallback ? 0xbaef5u : 0xbaf01u);
        gp[4] = add(gp[4], 8u);
        restore(unnamed_fallback ? 0xbaef9u : 0xbaf05u);
        gp[4] += 12u;
    }

public:
    MatrixLocations(Memory &memory, Abi &abi, const Observer &observer, const Services &services)
        : memory(memory), abi(abi), observer(observer), services(services) {}

    Resume run() {
        static const bool reference = [] {
            const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_MARKER_MATRIX");
            return value && *value;
        }();
        if (reference || !admitted())
            return Resume::retained;

        auto &gp = abi.gp;
        if (open()) {
            save(0xbad72u, gp[6]);
            call(memory, abi, observer, services, 0xbad73u, 0xbacd0u);
            if (!admitted())
                return Resume::parent_return;
        }
        if (prepare_group_lookup()) {
            call(memory, abi, observer, services, 0xbadffu, 0x1e11b0u);
            if (!admitted())
                return Resume::lookup_return;
            prepare_emission();
            call(memory, abi, observer, services, 0xbae18u, 0x1e1200u);
            if (!admitted())
                return Resume::emission_return;
            gp[5] = gp[0];
            iterator_logic_flags(registers, gp[5] & 65535u, 2);
            if (!(abi.eflags & 64u)) {
                close(false);
                return Resume::completed;
            }
            gp[6] = load(0xbae28u, gp[4] + 0x18u);
        }
        close(fallback_matrix());
        return Resume::completed;
    }
};

} // namespace authored_marker_matrix
