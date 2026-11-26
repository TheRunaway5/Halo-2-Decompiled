#include "core/native_tu15_authored_control_point.hpp"
#include "native_tu15_frame_misc_harness.hpp"

#include <iostream>

using namespace frame_misc_harness;
namespace authored = halo2::native_authored_control_point;

namespace {
struct Access {
    std::uint32_t pc, address, width;
    bool operator==(const Access &) const = default;
};

struct InjectedFault {};

struct ObservedMemory : Memory {
    std::uint32_t cursor{};
    unsigned fail_read{};
    bool *trace_after_first_read{};
    mutable std::vector<Access> reads;
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        reads.push_back({cursor, address, std::uint32_t(output.size())});
        if (fail_read && reads.size() == fail_read) throw InjectedFault{};
        Memory::read(address, output);
        if (trace_after_first_read) *trace_after_first_read = true;
    }
};

struct Sample {
    ObservedMemory memory;
    SourcePhysicsContactRuntimeAbi input{};
    std::array<std::uint32_t, 8> expected_registers{};
    std::uint32_t expected_flags{}, mutation_address{}, mutation_byte{}, fault_pc{};
    std::vector<std::uint32_t> instructions;
    std::vector<Access> reads;
};

bool read_fixture(Sample &sample) {
    auto &in = std::cin;
    if (!(in >> sample.input.eflags)) return false;
    for (auto &reg : sample.input.gp) in >> reg;
    in >> sample.memory.fail_read >> sample.fault_pc >> sample.mutation_address >> sample.mutation_byte;
    unsigned fields;
    in >> fields;
    for (unsigned n = 0; n < fields; ++n) {
        std::uint32_t address, width, word;
        in >> address >> width >> word;
        sample.memory.put(address, word, width);
    }
    for (auto &reg : sample.expected_registers) in >> reg;
    in >> sample.expected_flags >> fields;
    for (unsigned n = 0; n < fields; ++n) {
        auto &pc = sample.instructions.emplace_back();
        in >> pc;
    }
    in >> fields;
    for (unsigned n = 0; n < fields; ++n) {
        auto &access = sample.reads.emplace_back();
        in >> access.pc >> access.address >> access.width;
    }
    if (!in) throw std::runtime_error("Incomplete authenticated control-point fixture");
    return true;
}

void initialize_preserved_abi(SourcePhysicsContactRuntimeAbi &abi, unsigned index) {
    abi.fs_base = 0x10180000u + index * 4u;
    abi.x87_depth = index % 8u;
    abi.x87_status = std::uint16_t(((index % 8u) << 11u) | (index & 0x473fu));
    const auto cw = std::uint16_t(0x37fu | ((index & 3u) << 10u));
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{cw, 0x1f80u | (index & 0x803fu)};
    for (unsigned n = 0; n < abi.x87_depth; ++n)
        abi.x87[n] = static_cast<long double>(index * 3u + n);
    for (unsigned n = 0; n < abi.xmm.size(); ++n)
        for (unsigned lane = 0; lane < abi.xmm[n].size(); ++lane)
            abi.xmm[n][lane] = 0x7f800001u ^ (index << (lane + 1u)) ^ n;
    if (index % 17u == 0u) {
        abi.x87_depth = 0;
        abi.x87_status &= ~0x3800u;
        auto &mmx = abi.active_mmx.emplace();
        mmx.known_mask = std::uint8_t(index);
        for (unsigned n = 0; n < mmx.physical.size(); ++n) mmx.physical[n].fill(std::byte(index + n));
    }
}

struct ProbeTotals {
    unsigned attempted{}, skipped{}, faults{}, accesses{}, literal_callbacks{}, mutations{};
};

void run_owner(SourcePhysicsContactMemory &memory, SourcePhysicsContactRuntimeAbi &abi,
    const SourcePhysicsContactRuntimeServices &services) {
    const auto fp = abi.floating_point.value();
    if (!authored::owns_entry(authored::entry))
        throw std::runtime_error("Native ownership omitted the control-point target");
    authored::run_entry(memory, authored::entry, abi, fp, services);
}

void verify(Sample &sample, unsigned index, const std::string &mode, ProbeTotals &totals) {
    const bool use_cursor = mode == "cursor";
    const bool no_cursor = mode == "census";
    const bool toggle_trace = mode == "toggle";
    if ((use_cursor || no_cursor) && sample.mutation_address) {
        ++totals.skipped;
        return;
    }
    ++totals.attempted;
    if (sample.mutation_address) ++totals.mutations;
    initialize_preserved_abi(sample.input, index);
    auto observed = sample.input;
    std::vector<std::uint32_t> instructions;
    SourcePhysicsContactRuntimeServices services;
    bool trace = mode == "trace";
    if (toggle_trace) sample.memory.trace_after_first_read = &trace;
    services.instruction = [&](std::uint32_t pc) {
        if (use_cursor || no_cursor)
            throw std::runtime_error("Quiet control-point owner delivered a literal callback");
        instructions.push_back(pc);
        sample.memory.cursor = pc;
        if (sample.mutation_address && pc == 0x13e9b4u)
            sample.memory.put(sample.mutation_address, sample.mutation_byte, 1);
    };
    if (use_cursor || toggle_trace || trace) {
        static constexpr std::array<std::uint32_t, 3> service_pcs{0x38846eu, 0x21f590u, 0x33b595u};
        services.instruction_cursor = {&sample.memory.cursor, &trace, nullptr, service_pcs,
            std::this_thread::get_id(), &services.instruction.target_type(), true, true};
    }
    bool fault = false;
    const auto before_mxcsr = _mm_getcsr();
    std::uint16_t before_cw{}, after_cw{};
    asm volatile("fnstcw %0" : "=m"(before_cw));
    try { run_owner(sample.memory, observed, services); }
    catch (const InjectedFault &) { fault = true; }
    asm volatile("fnstcw %0" : "=m"(after_cw));
    if (before_mxcsr != _mm_getcsr() || before_cw != after_cw)
        throw std::runtime_error("Control-point owner changed caller host FP environment");
    if (fault) ++totals.faults;
    const auto expected = [&] {
        auto value = sample.input;
        value.gp = sample.expected_registers;
        value.eflags = sample.expected_flags;
        return value;
    }();
    const bool same_mmx = observed.active_mmx.has_value() == expected.active_mmx.has_value() &&
        (!expected.active_mmx || (observed.active_mmx->known_mask == expected.active_mmx->known_mask &&
                                 observed.active_mmx->physical == expected.active_mmx->physical));
    if (!same_abi(expected, observed) || expected.fs_base != observed.fs_base ||
        !same_mmx || !observed.floating_point || observed.floating_point->mxcsr != expected.floating_point->mxcsr) {
        std::fprintf(stderr, "case %u EFLAGS %x/%x, registers eax %x/%x, ecx %x/%x, edx %x/%x, esp %x/%x\n",
            index, observed.eflags, expected.eflags, observed.gp[0], expected.gp[0], observed.gp[1], expected.gp[1],
            observed.gp[2], expected.gp[2], observed.gp[4], expected.gp[4]);
        throw std::runtime_error("Control-point ABI differs from authenticated source");
    }
    if (fault != bool(sample.fault_pc) || (!no_cursor && sample.memory.cursor != sample.instructions.back()))
        throw std::runtime_error("Control-point fault or final cursor differs");
    const auto first_callback = toggle_trace ? sample.instructions.begin() + 1 : sample.instructions.begin();
    const auto last_callback = sample.instructions.end();
    if ((!use_cursor && !no_cursor && !std::equal(first_callback, last_callback, instructions.begin(), instructions.end())) ||
        ((use_cursor || no_cursor) && !instructions.empty()))
        throw std::runtime_error("Control-point callback order differs");
    if (no_cursor) for (auto &read : sample.reads) read.pc = 0u;
    if (sample.memory.reads != sample.reads) {
        std::fprintf(stderr, "case %u ordered read count %zu/%zu\n", index, sample.memory.reads.size(), sample.reads.size());
        throw std::runtime_error("Control-point memory reads differ");
    }
    totals.accesses += unsigned(sample.memory.reads.size());
    totals.literal_callbacks += unsigned(instructions.size());
}

void check_required_owner() {
    Memory memory;
    constexpr std::uint32_t ref = 0x10001000u, blob = 0x11000080u, sp = 0x20010020u;
    constexpr auto call_site = 0x13e2b6u;
    constexpr SourcePhysicsFactoryFloatingPoint fp{0x37fu, 0x1f80u};
    memory.put(ref + 4u, blob);
    memory.put(blob, 10u, 1);
    memory.put(0x44b884u + 10u, 16u, 1);
    memory.put(sp, 0x13e2bbu);
    memory.put(sp + 4u, 1u);
    memory.put(sp + 8u, 3u);
    SourcePhysicsContactRuntimeAbi input{};
    input.gp[0] = ref;
    input.gp[4] = sp;
    input.floating_point = fp;
    SourcePhysicsContactRuntimeServices services;
    unsigned dispatches{};
    services.required_owner = [&](auto &bank, const auto &call, auto &abi, const auto &observe) {
        if (call.instruction != call_site || call.tail)
            throw std::runtime_error("Unexpected control-point required-owner metadata");
        if (!authored::owns_entry(call.target)) throw InjectedFault{};
        ++dispatches;
        authored::run_entry(bank, call.target, abi, fp, services, observe);
    };
    const SourcePhysicsContactRecordObserver observe;
    WorldState caller{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    caller.load(input);
    caller.required(call_site, authored::entry, false);
    if (dispatches != 1u || caller.gp[0] != blob + 0x14u + 4u * 22u || caller.gp[4] != sp + 12u)
        throw std::runtime_error("Required-owner call failed to publish the control point and return stack");
    const auto before = caller.save();
    bool refused = false;
    try { caller.required(call_site, authored::entry + 1u, false); }
    catch (const InjectedFault &) { refused = true; }
    if (!refused || dispatches != 1u || !same_abi(caller.save(), before))
        throw std::runtime_error("Unrelated target changed the existing missing-owner route");
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "full";
    if (mode == "census" || mode == "counted") setenv("HALO2_CENSUS_NATIVE_BRIDGES", "1", 1);
    if (mode == "counted") setenv("HALO2_CENSUS_COUNT_NATIVE_REPORTS", "1", 1);
    std::cin >> std::hex;
    ProbeTotals totals;
    for (unsigned index = 0;; ++index) {
        Sample sample;
        if (!read_fixture(sample)) break;
        verify(sample, index, mode, totals);
    }
    check_required_owner();
    std::printf("{\"mode\":\"%s\",\"cases\":%u,\"skipped\":%u,\"faults\":%u,\"ordered_reads\":%u,"
        "\"callbacks\":%u,\"mutated_rereads\":%u}\n", mode.c_str(), totals.attempted, totals.skipped,
        totals.faults, totals.accesses, totals.literal_callbacks, totals.mutations);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
