// Included after the Effect dispatcher's integer, memory and SSE helpers.
namespace effect_origin_native {

struct Resume {
    std::uint32_t pc;
};

class Frame {
    Memory &memory;
    Abi &abi;
    const Observer &observe;
    const Services &services;
    IteratorRegisters registers{abi.gp, abi.eflags};
    const Services::InstructionCursor *cursor;
    std::uint32_t observation_delta;
    bool sse_active{};

    void at(std::uint32_t pc) const {
        if (sse_active)
            abi.floating_point->mxcsr = _mm_getcsr();
        if (cursor)
            *cursor->position = pc - observation_delta;
    }

    void recheck(std::uint32_t pc) const {
        if (cursor ? (cursor->first_trace && *cursor->first_trace) ||
                         (cursor->second_trace && *cursor->second_trace)
                   : bool(services.instruction))
            throw Resume{pc};
    }

    std::uint32_t load(std::uint32_t pc, std::uint32_t address, unsigned width = 4) const {
        at(pc);
        return read(memory, address, width);
    }

    void load_scalar(std::uint32_t pc, unsigned lane, std::uint32_t address, std::uint32_t next) {
        at(pc);
        scalar(memory, abi, lane, address);
        recheck(next);
    }

    void put(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        at(pc);
        if (observe) {
            store(memory, observe, pc - observation_delta, address, value);
        } else {
            const std::array bytes{
                std::byte(value), std::byte(value >> 8u), std::byte(value >> 16u), std::byte(value >> 24u)};
            memory.write(address, bytes);
        }
    }

    void ret(std::uint32_t pc) {
        at(pc);
        (void)pop(memory, abi);
    }

    void copy_position() {
        auto &gp = abi.gp;
        gp[0] = iterator_flags(registers, gp[0], 0x64u, 4, false);
        gp[2] = load(0xbbe20u, gp[0]);
        recheck(0xbbe22u);
        gp[1] = gp[6];
        put(0xbbe24u, gp[1], gp[2]);
        recheck(0xbbe26u);
        gp[2] = load(0xbbe26u, gp[0] + 4u);
        recheck(0xbbe29u);
        put(0xbbe29u, gp[1] + 4u, gp[2]);
        recheck(0xbbe2cu);
        gp[0] = load(0xbbe2cu, gp[0] + 8u);
        recheck(0xbbe2fu);
        put(0xbbe2fu, gp[1] + 8u, gp[0]);
        recheck(0xbbe32u);
        gp[0] = gp[6];
        ret(0xbbe34u);
    }

    void select_parent_node() {
        auto &gp = abi.gp;
        load_scalar(0xbbe35u, 0, gp[0] + 0x64u, 0xbbe3au);
        load_scalar(0xbbe3au, 1, gp[0] + 0x68u, 0xbbe3fu);
        load_scalar(0xbbe3fu, 2, gp[0] + 0x6cu, 0xbbe44u);
        at(0xbbe44u);
        push(memory, abi, observe, 0xbbe44u - observation_delta, gp[7]);
        recheck(0xbbe45u);
        gp[7] = std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(
            std::uint8_t(load(0xbbe45u, gp[0] + 0x18u, 1)))));
        recheck(0xbbe49u);
        gp[7] *= 0x34u;
        abi.eflags &= ~0x801u;
        gp[1] &= 65535u;
        iterator_logic_flags(registers, gp[1], 4);
        gp[1] *= 3u;
        gp[2] = load(0xbbe55u, gp[2] + gp[1] * 4u + 8u);
        recheck(0xbbe59u);
        gp[1] = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(
            std::uint16_t(load(0xbbe59u, gp[2] + 0x116u, 2)))));
        recheck(0xbbe60u);
        gp[7] = iterator_flags(registers, gp[7], gp[2], 4, false);
        load_scalar(0xbbe62u, 3, gp[1] + gp[7], 0xbbe67u);
        gp[1] = iterator_flags(registers, gp[1], gp[7], 4, false);
    }

    void multiply(unsigned dest, unsigned source) {
        auto value = vector(abi, dest);
        asm volatile("mulss %1,%0" : "+x"(value) : "x"(vector(abi, source)));
        vector(abi, dest, value);
    }

    void add(unsigned dest, __m128 source) {
        auto value = vector(abi, dest);
        asm volatile("addss %1,%0" : "+x"(value) : "x"(source));
        vector(abi, dest, value);
    }

    void add(unsigned dest, unsigned source) {
        add(dest, vector(abi, source));
    }

    __m128 scalar_operand(std::uint32_t pc, std::uint32_t address) const {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(load(pc, address))));
    }

    bool restore_edi_and_select_scale() {
        auto &gp = abi.gp;
        const auto one = scalar_operand(0xbbe69u, 0x46a0d0u);
        auto ax = std::uint16_t(gp[0]);
        asm volatile("ucomiss %2,%1; lahf" : "+a"(ax) : "x"(vector(abi, 3)), "x"(one) : "cc");
        abi.eflags = (abi.eflags & ~0x8d5u) | ((ax >> 8u) & 0xd5u);
        recheck(0xbbe70u);
        gp[0] = (gp[0] & 0xffff00ffu) | (ax & 0xff00u);
        iterator_logic_flags(registers, (gp[0] >> 8u) & 0x44u, 1);
        at(0xbbe74u);
        gp[7] = pop(memory, abi);
        recheck(0xbbe75u);
        return (abi.eflags & 4u) != 0;
    }

    void scale_position() {
        multiply(3, 0);
        abi.xmm[0] = abi.xmm[3];
        load_scalar(0xbbe7eu, 3, abi.gp[1], 0xbbe82u);
        multiply(3, 1);
        abi.xmm[1] = abi.xmm[3];
        load_scalar(0xbbe89u, 3, abi.gp[1], 0xbbe8du);
        multiply(3, 2);
        abi.xmm[2] = abi.xmm[3];
    }

    struct Axis {
        std::uint32_t z, y, products, x, product_x, translation, store, next;
        unsigned offset;
    };

    void project_axis(const Axis &site) {
        auto &gp = abi.gp;
        load_scalar(site.z, 3, gp[1] + 0x1cu + site.offset, site.y);
        load_scalar(site.y, 4, gp[1] + 0x10u + site.offset, site.products);
        multiply(3, 2);
        multiply(4, 1);
        add(3, 4);
        load_scalar(site.x, 4, gp[1] + 4u + site.offset, site.product_x);
        multiply(4, 0);
        add(3, 4);
        add(3, scalar_operand(site.translation, gp[1] + 0x28u + site.offset));
        recheck(site.store);
        put(site.store, gp[6] + site.offset, abi.xmm[3][0]);
        recheck(site.next);
    }

    void project_last_axis() {
        auto &gp = abi.gp;
        load_scalar(0xbbeedu, 3, gp[1] + 0x24u, 0xbbef2u);
        multiply(3, 2);
        load_scalar(0xbbef6u, 2, gp[1] + 0x18u, 0xbbefbu);
        multiply(2, 1);
        load_scalar(0xbbeffu, 1, gp[1] + 12u, 0xbbf04u);
        add(3, 2);
        multiply(1, 0);
        add(3, 1);
        add(3, scalar_operand(0xbbf10u, gp[1] + 0x30u));
        recheck(0xbbf15u);
        put(0xbbf15u, gp[6] + 8u, abi.xmm[3][0]);
        recheck(0xbbf1au);
    }

    void transform_position() {
        select_parent_node();
        SseEnvironment environment(abi);
        sse_active = true;
        if (restore_edi_and_select_scale())
            scale_position();
        // Each axis reads after the preceding store, even if the output
        // overlaps the matrix, scratch stack, or source coordinates.
        project_axis({0xbbe94u, 0xbbe99u, 0xbbe9eu, 0xbbeaau, 0xbbeafu, 0xbbeb7u, 0xbbebcu, 0xbbec0u, 0});
        project_axis({0xbbec0u, 0xbbec5u, 0xbbecau, 0xbbed6u, 0xbbedbu, 0xbbee3u, 0xbbee8u, 0xbbeedu, 4});
        project_last_axis();
        abi.gp[0] = abi.gp[6];
        ret(0xbbf1cu);
    }

public:
    Frame(Memory &memory, Abi &abi, const Observer &observe, const Services &services,
          const Services::InstructionCursor *cursor, std::uint32_t observation_delta)
        : memory(memory), abi(abi), observe(observe), services(services), cursor(cursor),
          observation_delta(observation_delta) {}

    void run() {
        auto &gp = abi.gp;
        gp[1] = load(0xbbe00u, 0x4ecbd0u);
        recheck(0xbbe06u);
        gp[2] = load(0xbbe06u, gp[1] + 0x44u);
        recheck(0xbbe09u);
        gp[0] &= 65535u;
        iterator_logic_flags(registers, gp[0], 4);
        gp[0] *= 3u;
        gp[0] = load(0xbbe11u, gp[2] + gp[0] * 4u + 8u);
        recheck(0xbbe15u);
        gp[1] = load(0xbbe15u, gp[0] + 0x14u);
        recheck(0xbbe18u);
        iterator_flags(registers, gp[1], none, 4, true);
        if (abi.eflags & 64u)
            copy_position();
        else
            transform_position();
    }
};

} // namespace effect_origin_native
