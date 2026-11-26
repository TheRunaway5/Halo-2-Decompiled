// Included by the weapon effect dispatcher after its memory and ABI helpers.
namespace marker_quaternion {

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Matrix {
    Memory &memory;
    Abi &abi;
    const Observer &observe;
    std::uint32_t *position;
    std::optional<SseEnvironment> floating_point;

    void at(std::uint32_t pc) {
        if (position)
            *position = pc;
        if (floating_point)
            abi.floating_point->mxcsr = _mm_getcsr();
    }

    __m128 load(std::uint32_t pc, std::uint32_t address) {
        at(pc);
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(memory, address))));
    }

    void load(unsigned lane, std::uint32_t pc, std::uint32_t address) {
        vector(abi, lane, load(pc, address));
    }

    void put(std::uint32_t pc, std::uint32_t address, unsigned lane) {
        at(pc);
        store(memory, observe, pc, address, abi.xmm[lane][0]);
    }

    __m128 lane(unsigned index) const { return vector(abi, index); }
    void copy(unsigned out, unsigned in) { abi.xmm[out] = abi.xmm[in]; }

    void multiply(unsigned index, __m128 right) {
        vector(abi, index, native_tu15_leaves::ordered_mul_ss(lane(index), right));
    }

    void add(unsigned index, __m128 right) {
        vector(abi, index, native_tu15_leaves::ordered_sum_ss(lane(index), right));
    }

    void subtract(unsigned index, __m128 right) {
        vector(abi, index, native_tu15_leaves::ordered_difference_ss(lane(index), right));
    }

    void scale() {
        const auto q = abi.gp[1];
        load(3, 0x144910, q + 4);
        load(2, 0x144915, q + 8);
        load(0, 0x14491a, q + 12);
        load(1, 0x14491f, q);
        copy(4, 3);
        floating_point.emplace(abi);
        multiply(4, lane(3));
        copy(3, 2);
        multiply(1, lane(1));
        multiply(3, lane(2));
        add(1, lane(4));
        copy(2, 0);
        add(1, lane(3));
        multiply(2, lane(0));
        IteratorRegisters regs{abi.gp, abi.eflags};
        abi.gp[4] = iterator_flags(regs, abi.gp[4], 12, 4, true);
        add(1, lane(2));

        const auto least = load(0x14494b, 0x46a0ec);
        unsigned char carry, parity, zero;
        asm volatile("comiss %4,%3; setb %0; setp %1; sete %2"
            : "=qm"(carry), "=qm"(parity), "=qm"(zero) : "x"(lane(1)), "x"(least) : "cc");
        abi.eflags = (abi.eflags & ~0x8d5u) | carry | (parity ? 4u : 0u) | (zero ? 64u : 0u);
        if (carry || zero) {
            abi.xmm[0] = {};
        } else {
            load(0, 0x144954, 0x46a11c);
            vector(abi, 0, _mm_div_ss(lane(0), lane(1)));
        }
    }

    void products() {
        const auto q = abi.gp[1];
        const auto stack = abi.gp[4];
        load(1, 0x144965, q);
        load(4, 0x144969, q + 4);
        load(3, 0x14496e, q + 4);
        load(7, 0x144973, 0x46a0d0);
        multiply(1, lane(0));
        multiply(4, lane(0));
        multiply(0, load(0x144983, q + 8));
        copy(2, 0);
        multiply(2, load(0x14498b, q + 12));
        copy(5, 1);
        multiply(5, load(0x144993, q + 12));
        put(0x144998, stack + 4, 2);

        load(2, 0x14499e, q);
        multiply(2, lane(1));
        load(1, 0x1449a6, q);
        multiply(1, lane(4));
        put(0x1449ae, stack, 2);
        load(2, 0x1449b3, q);
        multiply(2, lane(0));
        multiply(3, lane(4));
        copy(6, 4);
        load(4, 0x1449c2, q + 4);
        multiply(6, load(0x1449c7, q + 12));
        multiply(4, lane(0));
        multiply(0, load(0x1449d0, q + 8));
        put(0x1449d5, stack + 8, 0);
    }

    void elements() {
        const auto out = abi.gp[0];
        const auto stack = abi.gp[4];
        add(0, lane(3));
        add(3, load(0x1449df, stack));
        subtract(7, lane(0));
        load(0, 0x1449e8, stack + 4);
        put(0x1449ee, out, 7);
        copy(7, 1);
        subtract(7, lane(0));
        add(1, lane(0));
        load(0, 0x1449fd, 0x46a0d0);
        put(0x144a05, out + 12, 7);
        put(0x144a0a, out + 4, 1);
        load(1, 0x144a0f, stack + 8);
        add(1, load(0x144a15, stack));
        copy(7, 2);
        add(7, lane(6));
        put(0x144a21, out + 24, 7);

        copy(7, 0);
        subtract(7, lane(1));
        copy(1, 4);
        subtract(1, lane(5));
        subtract(2, lane(6));
        add(4, lane(5));
        subtract(0, lane(3));
        put(0x144a40, out + 16, 7);
        put(0x144a45, out + 28, 1);
        put(0x144a4a, out + 8, 2);
        put(0x144a4f, out + 20, 4);
        put(0x144a54, out + 32, 0);
    }

public:
    Matrix(Memory &memory, Abi &abi, const Observer &observe, std::uint32_t *position)
        : memory(memory), abi(abi), observe(observe), position(position) {}

    void run() {
        scale();
        products();
        elements();
        IteratorRegisters regs{abi.gp, abi.eflags};
        abi.gp[4] = iterator_flags(regs, abi.gp[4], 12, 4, false);
        at(0x144a5c);
        (void)pop(memory, abi);
    }
};
#endif

bool try_build(Memory &memory, Abi &abi, const Observer &observe, const Services &services) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_WEAPON_MARKER_QUATERNION");
        return value && *value;
    }();
    if (reference || abi.active_mmx || !abi.floating_point ||
        (abi.floating_point->mxcsr & 0x1f80u) != 0x1f80u || (abi.floating_point->mxcsr & ~0xffffu) ||
        !native_effect_slot_scan_detail::admitted_protocol(services, observe))
        return false;
    Matrix(memory, abi, observe, services.instruction ? services.instruction_cursor.position : nullptr).run();
    return true;
#else
    return false;
#endif
}

} // namespace marker_quaternion
