#include "physics/runtime/native_tu15_mopp_ray_traversal.hpp"

#include <algorithm>
#include <compare>
#include <iostream>
#include <string_view>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace ray = native_mopp_ray_caller_detail;

constexpr std::uint32_t store_pc = 0x30af0eu;
enum class Operation { read, write, observe };
enum class ObserverChange { none, install, remove, replace, edit_second_half };
struct Access {
    Operation operation;
    std::uint32_t address;
    std::size_t width;
    auto operator<=>(const Access &) const = default;
};
struct Failure {};
struct Fixture {
    std::uint32_t address;
    ObserverChange change{};
    Operation failure{};
    unsigned failure_ordinal{};
    bool observed{true};
    bool callback_free{};
};

void check(bool valid, std::string_view reason) {
    if (!valid) throw std::runtime_error(std::string(reason));
}

struct Memory : SourcePhysicsContactMemory {
    const Fixture &fixture;
    SourcePhysicsContactRecordObserver *observer{};
    SourcePhysicsContactRecordObserver alternate;
    std::array<std::byte, 16> bytes{};
    mutable std::vector<Access> trace;
    mutable std::array<unsigned, 3> ordinal{};
    unsigned fast_writes{};
    std::function<void(Operation)> on_access;

    explicit Memory(const Fixture &input) : fixture(input) {
        for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = std::byte(i + 1);
    }

    unsigned access(Operation kind, std::uint32_t address, std::size_t width) const {
        trace.push_back({kind, address, width});
        const auto current = ++ordinal[unsigned(kind)];
        if (on_access) on_access(kind);
        if (kind == fixture.failure && current == fixture.failure_ordinal) throw Failure{};
        return current;
    }

    void read(std::uint32_t address, std::span<std::byte> out) const override {
        access(Operation::read, address, out.size());
        const auto offset = address - fixture.address;
        check(std::uint64_t(offset) + out.size() <= bytes.size(), "Read outside fixture");
        std::copy_n(bytes.begin() + offset, out.size(), out.begin());
    }

    void write(std::uint32_t address, std::span<const std::byte> data) override {
        const auto current = access(Operation::write, address, data.size());
        const auto offset = address - fixture.address;
        check(std::uint64_t(offset) + data.size() <= bytes.size(), "Write outside fixture");
        std::copy(data.begin(), data.end(), bytes.begin() + offset);
        if (current != 1) return;
        if (fixture.change == ObserverChange::install || fixture.change == ObserverChange::replace)
            *observer = alternate;
        else if (fixture.change == ObserverChange::remove) *observer = {};
    }

    bool try_callback_free_write(std::uint32_t address, std::span<const std::byte> data) noexcept override {
        const auto offset = address - fixture.address;
        if (!fixture.callback_free || std::uint64_t(offset) + data.size() > bytes.size()) return false;
        ++fast_writes;
        std::copy(data.begin(), data.end(), bytes.begin() + offset);
        return true;
    }
};

template<class Before>
struct Frame : ray::IntegerFrame<WorldState, Before, ray::ScopeIo<WorldState>> {
    using Base = ray::IntegerFrame<WorldState, Before, ray::ScopeIo<WorldState>>;
    using Base::Base;
    using Base::at;
    using Base::push;
    using Base::spill_single;
    using Base::vector;
    using Base::vector_store;
    using Base::word;
};

struct Outcome {
    std::array<std::byte, 16> bytes;
    std::vector<Access> trace;
    std::vector<std::pair<unsigned, SourcePhysicsContactRecordStore>> observed;
    SourcePhysicsWorldRuntimeAbi abi;
    std::uint32_t cursor{};
    unsigned fast_writes{};
    bool fault{};
    bool alignment_fault{};
};

enum class Policy { retained, hosted, physical };

Outcome run(const Fixture &fixture, Policy policy, bool load = false) {
    Memory memory(fixture);
    Outcome result{};
    SourcePhysicsContactRecordObserver observer;
    const auto observe = [&](unsigned identity, const SourcePhysicsContactRecordStore &event) {
        result.observed.emplace_back(identity, event);
        if (memory.access(Operation::observe, event.address, event.written.size()) == 1 &&
            fixture.change == ObserverChange::edit_second_half) memory.bytes[8] = std::byte{0xfa};
    };
    if (fixture.observed) observer = [&](const auto &event) { observe(1, event); };
    memory.observer = &observer;
    memory.alternate = [&](const auto &event) { observe(2, event); };

    SourcePhysicsContactRuntimeServices services;
    static constexpr std::array sites{0x38846eu, 0x21f590u, 0x33b595u};
    services.instruction = [&](std::uint32_t pc) { result.cursor = pc; };
    auto &cursor = services.instruction_cursor;
    cursor.position = &result.cursor;
    cursor.service_sites = sites;
    cursor.callback_type = &services.instruction.target_type();
    cursor.executor = std::this_thread::get_id();
    cursor.literal_service_sites = true;
    cursor.observation_only_stores = true;

    SourcePhysicsWorldRuntimeAbi incoming{};
    incoming.gp = {1, 2, 3, 4, 0x62000200u, 6, 7, 8};
    incoming.eflags = 0xad7u;
    incoming.x87_depth = 1;
    incoming.x87_status = 0x1200u;
    incoming.x87[0] = 1.25L;
    incoming.xmm[2] = {0x11223344u, 0x55667788u, 0x99aabbccu, 0xddeeff00u};
    incoming.floating_point = SourcePhysicsFactoryFloatingPoint{0x27fu, 0x1f81u};
    FpScope environment(*incoming.floating_point);
    WorldState state{memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(incoming);

    try {
        if (policy == Policy::retained) {
            state.instruction_literal(store_pc);
            if (load) state.xmm[2] = v(memory, fixture.address);
            else vw(memory, observer, store_pc, fixture.address, state.xmm[2]);
        } else {
            native_mopp_ray_traversal_detail::Noop hook;
            native_raw_x87_detail::Scope scope(state);
            auto perform = [&](auto &selected_hook) {
                Frame<std::remove_reference_t<decltype(selected_hook)>> frame{state, selected_hook, {&scope}};
                if (load) state.xmm[2] = frame.vector(store_pc, fixture.address);
                else frame.vector_store(store_pc, fixture.address, state.xmm[2]);
            };
            if (policy == Policy::hosted) {
                ray::HostedMemory hosted{hook};
                perform(hosted);
            } else perform(hook);
        }
    } catch (const Failure &) { result.fault = true; }
    catch (const ray::AlignedLoadFault &fault) {
        check(fault.address == fixture.address && fault.instruction == store_pc, "Physical alignment fault location");
        check(fault.saved_eflags == (fault.execution_eflags | 0x10000u), "Physical alignment fault RF");
        result.alignment_fault = true;
    }
    result.abi = state.save();
    result.bytes = memory.bytes;
    result.trace = std::move(memory.trace);
    result.fast_writes = memory.fast_writes;
    return result;
}

bool same_abi(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    if (a.gp != b.gp || a.eflags != b.eflags || a.xmm != b.xmm ||
        a.x87_depth != b.x87_depth || a.x87_status != b.x87_status ||
        a.floating_point->mxcsr != b.floating_point->mxcsr ||
        a.floating_point->x87_control_word != b.floating_point->x87_control_word) return false;
    for (unsigned n = 0; n < a.x87_depth; ++n)
        if (std::memcmp(&a.x87[n], &b.x87[n], 10) != 0) return false;
    return true;
}

bool same_state(const Outcome &a, const Outcome &b) {
    return a.cursor == b.cursor && same_abi(a.abi, b.abi);
}

void compare(const Fixture &fixture, bool load = false) {
    const auto retained = run(fixture, Policy::retained, load), hosted = run(fixture, Policy::hosted, load);
    check(hosted.bytes == retained.bytes, "Hosted memory differs");
    check(hosted.trace == retained.trace, "Hosted provider order or widths differ");
    check(hosted.observed == retained.observed, "Hosted observer identity or payload differs");
    check(hosted.fault == retained.fault && !hosted.alignment_fault, "Hosted fault differs");
    check(same_state(hosted, retained), "Hosted cursor or ABI differs");
}

enum class Scalar { push, call, fst, fstp, key_add_byte, key_add_word };
struct ScalarOutcome : Outcome {
    std::vector<std::pair<std::uint32_t, SourcePhysicsWorldRuntimeAbi>> callback_state;
    unsigned nested_calls{};
};

// The retained TU1.5 owner runs this conversion before invoking w(), then
// performs the logical pop after w() and its optional observer return.
std::uint32_t retained_single_spill(long double value, std::uint16_t &status) {
    float result;
    std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        :"=m"(result),"=m"(actual):"m"(value):"st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return std::bit_cast<std::uint32_t>(result);
}

ScalarOutcome run_scalar(const Fixture &fixture, Policy policy, Scalar instruction,
                         unsigned rounding, long double value, std::optional<Operation> reenter = {}) {
    constexpr std::array pcs{0x30aa70u, 0x30b615u, 0x30b3cfu, 0x30af59u, 0x30b46bu, 0x30b4b4u};
    const auto pc = pcs[unsigned(instruction)];
    const bool push = instruction == Scalar::push || instruction == Scalar::call;
    const bool pop = instruction == Scalar::fstp;
    const bool add = instruction == Scalar::key_add_byte || instruction == Scalar::key_add_word;
    const auto encoded = instruction == Scalar::key_add_byte ? 0x11u : 0x1211u;
    const std::uint32_t pushed = instruction == Scalar::push ? 0x12ab34cdu : 0x30b617u;

    Memory memory(fixture);
    ScalarOutcome result{};
    SourcePhysicsContactRecordObserver observer;
    if (fixture.observed) observer = [&](const auto &event) {
        result.observed.emplace_back(1, event);
        memory.access(Operation::observe, event.address, event.written.size());
    };
    memory.observer = &observer;
    SourcePhysicsContactRuntimeServices services;
    services.instruction = [&](std::uint32_t positioned) { result.cursor = positioned; };
    auto &cursor = services.instruction_cursor;
    cursor.position = &result.cursor;
    cursor.callback_type = &services.instruction.target_type();
    cursor.executor = std::this_thread::get_id();
    cursor.literal_service_sites = true;

    SourcePhysicsWorldRuntimeAbi incoming{};
    incoming.gp = {1, 2, 3, 4, fixture.address + 4u, pushed, 7, 8};
    if (add) {
        const auto base = instruction == Scalar::key_add_byte ? 0x7ffffff0u : 0xffffedefu;
        std::memcpy(memory.bytes.data(), &base, sizeof(base));
        incoming.gp[0] = encoded;
        incoming.gp[4] = fixture.address - 0x58u;
    }
    incoming.eflags = 0xad7u;
    // TOP 7 wraps on pop. Keep the other condition bits live alongside C1.
    incoming.x87_depth = 2;
    incoming.x87_status = 0x7d04u;
    incoming.x87[0] = value;
    incoming.x87[1] = -3.25L;
    incoming.floating_point = SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x27fu | (rounding << 10u)), 0x1f81u};
    FpScope environment(*incoming.floating_point);
    WorldState state{memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(incoming);
    memory.on_access = [&](Operation current) {
        const auto before = state.save();
        result.callback_state.emplace_back(result.cursor, before);
        if (reenter != current || result.nested_calls != 0) return;
        const auto nested = run_scalar({0x61008000u}, policy, Scalar::fstp, (rounding + 1u) % 4u, 7.0L);
        check(!nested.fault && nested.abi.x87_depth == 1 && nested.observed.size() == 1,
              "Nested spill did not complete");
        ++result.nested_calls;
        check(same_abi(before, state.save()), "Nested spill changed outer callback ABI");
    };

    try {
        if (policy == Policy::retained) {
            state.instruction_literal(pc);
            if (instruction == Scalar::call) (void)r(memory, fixture.address + 4u);
            if (push) state.push(pc, pushed);
            else if (add) {
                const auto address = state.gp[4] + 0x58u;
                w(memory, observer, pc, address, 4, state.flags.plus(r(memory, address, 4), state.reg(0, 4, 0), 4));
            } else {
                w(memory, observer, pc, fixture.address, 4, retained_single_spill(state.fp.st(0), state.fp.status));
                if (pop) state.fp.pop();
            }
        } else {
            native_mopp_ray_traversal_detail::Noop hook;
            native_raw_x87_detail::Scope scope(state);
            const auto perform = [&](auto &selected_hook) {
                Frame<std::remove_reference_t<decltype(selected_hook)>> frame{state, selected_hook, {&scope}};
                if (instruction == Scalar::call) {
                    frame.at(pc);
                    (void)frame.word(fixture.address + 4u);
                    frame.push(pc, pushed, true);
                } else if (push) frame.push(pc, pushed);
                else if (add) {
                    native_mopp_integer_program_detail::Progress progress{};
                    progress.count = instruction == Scalar::key_add_byte ? 1u : 2u;
                    progress.encoded = encoded;
                    progress.base = frame.key_base(progress);
                    frame.store_key_base(progress, progress.base + progress.encoded);
                } else {
                    frame.at(pc);
                    frame.spill_single(pc, fixture.address, pop, scope);
                }
            };
            if (policy == Policy::hosted) {
                ray::HostedMemory hosted{hook};
                perform(hosted);
            } else perform(hook);
        }
    } catch (const Failure &) { result.fault = true; }
    result.abi = state.save();
    result.bytes = memory.bytes;
    result.trace = std::move(memory.trace);
    result.fast_writes = memory.fast_writes;
    return result;
}

void compare_scalar(const Fixture &fixture, Scalar instruction, unsigned rounding, long double value,
                    std::optional<Operation> reenter = {}) {
    const auto retained = run_scalar(fixture, Policy::retained, instruction, rounding, value, reenter);
    const auto hosted = run_scalar(fixture, Policy::hosted, instruction, rounding, value, reenter);
    check(hosted.bytes == retained.bytes && hosted.trace == retained.trace && hosted.observed == retained.observed,
          "Hosted scalar store changed bytes, provider order, or observer payload");
    check(hosted.fault == retained.fault && same_state(hosted, retained), "Hosted scalar store changed fault or final ABI");
    check(hosted.nested_calls == retained.nested_calls && hosted.nested_calls == unsigned(reenter.has_value()),
          "Hosted scalar reentrancy did not execute at the expected boundary");
    check(hosted.callback_state.size() == retained.callback_state.size(), "Hosted scalar callback count differs");
    for (unsigned n = 0; n < hosted.callback_state.size(); ++n) {
        const auto &[actual_pc, actual_state] = hosted.callback_state[n];
        const auto &[expected_pc, expected_state] = retained.callback_state[n];
        if (actual_pc != expected_pc || !same_abi(actual_state, expected_state)) {
            std::cerr << "Scalar callback " << n << " at " << std::hex << expected_pc << " ESP "
                      << actual_state.gp[4] << '/' << expected_state.gp[4] << " EFLAGS "
                      << actual_state.eflags << '/' << expected_state.eflags << " x87 "
                      << actual_state.x87_status << '/' << expected_state.x87_status << std::dec << '\n';
            throw std::runtime_error("Hosted scalar callback observed different cursor or ABI");
        }
    }
}

void check_add_flags(const Fixture &fixture, Scalar instruction, std::uint32_t computed_flags) {
    constexpr auto initial_flags = 0xad7u;
    const auto hosted = run_scalar(fixture, Policy::hosted, instruction, 0, 1.0L);
    const auto physical = run_scalar(fixture, Policy::physical, instruction, 0, 1.0L);
    check(hosted.trace == physical.trace && hosted.fault && physical.fault, "ADD fault boundary changed");
    check(hosted.callback_state.size() == hosted.trace.size() &&
          physical.callback_state.size() == physical.trace.size(), "ADD fault callback state missing");
    for (unsigned n = 0; n < hosted.trace.size(); ++n) {
        check(hosted.callback_state[n].second.eflags == (n == 0 ? initial_flags : computed_flags),
              "Hosted ADD committed flags at the wrong callback");
        check(physical.callback_state[n].second.eflags ==
              (hosted.trace[n].operation == Operation::observe ? computed_flags : initial_flags),
              "Physical ADD committed flags at the wrong callback");
    }
    const auto hosted_flags = fixture.failure == Operation::read && fixture.failure_ordinal == 1 ?
        initial_flags : computed_flags;
    check(hosted.abi.eflags == hosted_flags, "Hosted ADD fault published incorrect flags");
    check(physical.abi.eflags == (fixture.failure == Operation::observe ? computed_flags : initial_flags),
          "Physical ADD fault published incorrect flags");
}

struct TraversalMemory : SourcePhysicsContactMemory {
    static constexpr std::uint32_t base = 0x62000000u;
    std::array<std::byte, 4096> bytes{};
    std::function<void()> invalid_read;
    mutable unsigned provider_reads{};
    unsigned provider_writes{};

    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t address,
                                                           std::size_t width) const noexcept override {
        const auto offset = address - base;
        if (offset > bytes.size() || width > bytes.size() - offset) return std::nullopt;
        return std::span(bytes).subspan(offset,width);
    }
    void read(std::uint32_t address,std::span<std::byte> data) const override {
        ++provider_reads;
        const auto view = try_read_view(address,data.size());
        if (!view) {
            invalid_read();
            throw Failure{};
        }
        std::memcpy(data.data(),view->data(),data.size());
    }
    void write(std::uint32_t address,std::span<const std::byte> data) override {
        ++provider_writes;
        check(try_callback_free_write(address,data),"Unexpected traversal write range");
    }
    bool try_callback_free_write(std::uint32_t address,std::span<const std::byte> data) noexcept override {
        if (!try_read_view(address,data.size())) return false;
        std::memcpy(bytes.data()+(address-base),data.data(),data.size());
        return true;
    }
    void put(std::uint32_t address,std::uint32_t word) {
        check(try_callback_free_write(address,std::as_bytes(std::span{&word,1})),
              "Unexpected traversal input range");
    }
};

void check_deferred_traversal(bool fail) {
    TraversalMemory memory;
    constexpr auto base = TraversalMemory::base;
    constexpr auto esp = base+0xf00u;
    memory.put(esp,0x63000000u);
    memory.put(esp+4u,base+0x400u);
    memory.put(esp+8u,fail ? 0x6bad0000u : base+0x100u);
    memory.put(esp+12u,base+0x500u);

    SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    std::uint32_t pc{};
    static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction = [&](std::uint32_t positioned) { pc=positioned; };
    auto &cursor = services.instruction_cursor;
    cursor.position=&pc;
    cursor.service_sites=sites;
    cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;

    SourcePhysicsWorldRuntimeAbi incoming{};
    incoming.gp[4]=esp;
    incoming.gp[5]=0x55555555u;
    incoming.x87_depth=2;
    incoming.x87_status=0x7d04u;
    incoming.x87[0]=1.25L;
    incoming.x87[1]=-3.25L;
    incoming.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f81u};
    FpScope environment(*incoming.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(incoming);
    memory.invalid_read = [&] {
        const auto fault = state.save();
        check(pc==0x30aa81u && fault.x87_depth==incoming.x87_depth &&
              fault.x87_status==incoming.x87_status && fault.gp[4]==esp-0x1acu,
              "First real callback lost the retired prologue or retained x87 state");
    };

    // Distinguish the surrounding host's C1 from the logical owner's other
    // condition codes. An eager import would publish the latter back.
    std::array<std::uint32_t,7> host;
    asm volatile("fnstenv %0":"=m"(host)::"memory");
    host[1]=(host[1]&~0x4700u)|0x0200u;
    asm volatile("fldenv %0"::"m"(host):"memory");

    bool faulted{};
    try {
        check(native_mopp_ray_traversal_detail::bridge_hosted(state,[](auto &,auto,auto) {
            throw std::logic_error("Empty traversal unexpectedly dispatched a child");
        }),"Hosted traversal refused ordinary source memory");
    } catch (const Failure &) { faulted=true; }
    std::uint16_t host_status;
    asm volatile("fnstsw %0":"=am"(host_status)::"memory");
    const auto final = state.save();
    check(faulted==fail && final.x87_depth==incoming.x87_depth && final.x87_status==incoming.x87_status,
          "Deferred traversal changed the retained outcome");
    if (!fail) check(pc==0x30b6b5u && final.gp[4]==esp+16u,"Empty traversal did not return");
    if (!native_raw_x87_detail::scoped_access_reference()) {
        check(memory.provider_writes==0 && memory.provider_reads==unsigned(fail),
              "Ordinary traversal unnecessarily invoked a provider");
        if (!fail) check((host_status&0x4700u)==0x0200u,"Integer-only traversal eagerly imported x87");
    }
}
} // namespace

int main() try {
    constexpr auto aligned = 0x61001000u;
    unsigned cases{};
    for (auto address : {aligned, aligned + 4u, 0xfffffff8u}) {
        compare({address});
        compare({address, ObserverChange::none, Operation::read, 0, false});
        cases += 2;
        for (auto operation : {Operation::read, Operation::write, Operation::observe})
            for (unsigned ordinal : {1u, 2u}) {
                compare({address, ObserverChange::none, operation, ordinal});
                ++cases;
            }
        for (auto change : {ObserverChange::install, ObserverChange::remove, ObserverChange::replace,
                            ObserverChange::edit_second_half}) {
            compare({address, change, Operation::read, 0u, change != ObserverChange::install});
            ++cases;
        }
        for (unsigned ordinal = 0; ordinal <= 4; ++ordinal) {
            compare({address, ObserverChange::none, Operation::read, ordinal}, true);
            ++cases;
        }
    }
    const Fixture callback_free{aligned, ObserverChange::none, Operation::read, 0, false, true};
    const auto fast = run(callback_free, Policy::hosted), retained = run(callback_free, Policy::retained);
    check(fast.bytes == retained.bytes && same_state(fast, retained), "Callback-free hosted store differs");
    if (native_raw_x87_detail::scoped_access_reference())
        check(fast.fast_writes == 0 && fast.trace == retained.trace, "Scoped reference bypassed its provider");
    else
        check(fast.fast_writes == 2 && fast.trace.empty(), "Callback-free hosted store calls its provider");
    const auto physical = run({aligned}, Policy::physical);
    check(physical.trace == std::vector<Access>{{Operation::read, aligned, 16}, {Operation::write, aligned, 16},
              {Operation::observe, aligned, 16}}, "Physical store is no longer full width");
    const auto unaligned = run({aligned + 4u}, Policy::physical);
    check(unaligned.alignment_fault && unaligned.trace.empty(), "Physical store lost alignment fault");

    constexpr long double inexact = 0x1.0000018p0L;
    unsigned scalar_cases{};
    for (auto instruction : {Scalar::push, Scalar::call, Scalar::fst, Scalar::fstp}) {
        for (auto operation : {Operation::read, Operation::write, Operation::observe}) {
            compare_scalar({aligned, ObserverChange::none, operation, 1}, instruction, 2, inexact, operation);
            compare_scalar({aligned}, instruction, 2, inexact, operation);
            scalar_cases += 2;
        }
        compare_scalar({aligned, ObserverChange::none, Operation::write, 1, false}, instruction, 2, inexact);
        compare_scalar({aligned, ObserverChange::none, Operation::read, 0, false}, instruction, 2, inexact);
        scalar_cases += 2;
    }
    compare_scalar({aligned, ObserverChange::none, Operation::read, 2}, Scalar::call, 2, inexact, Operation::read);
    ++scalar_cases;
    for (unsigned rounding = 0; rounding < 4; ++rounding)
        for (auto value : {inexact, -inexact, -0.0L, 0x1p140L, 0x1.4p-148L}) {
            compare_scalar({aligned}, Scalar::fst, rounding, value);
            compare_scalar({aligned}, Scalar::fstp, rounding, value);
            scalar_cases += 2;
        }

    const auto push_write_fault = run_scalar({aligned, ObserverChange::none, Operation::write, 1},
                                             Policy::physical, Scalar::push, 2, inexact);
    const auto spill_write_fault = run_scalar({aligned, ObserverChange::none, Operation::write, 1},
                                              Policy::physical, Scalar::fstp, 2, inexact);
    const auto spill_observer_fault = run_scalar({aligned, ObserverChange::none, Operation::observe, 1},
                                                 Policy::physical, Scalar::fstp, 2, inexact);
    check(push_write_fault.fault && push_write_fault.abi.gp[4] == aligned + 4u, "Physical PUSH retired before write");
    check(spill_write_fault.fault && spill_write_fault.abi.x87_depth == 2 &&
          spill_write_fault.abi.x87_status == 0x7d04u, "Physical FSTP retired before write");
    check(spill_observer_fault.fault && spill_observer_fault.abi.x87_depth == 1 &&
          spill_observer_fault.abi.x87_status == 0x4724u, "Physical FSTP did not retire before observer");
    const auto scalar_fast = run_scalar(callback_free, Policy::hosted, Scalar::fstp, 2, inexact);
    const auto scalar_retained = run_scalar(callback_free, Policy::retained, Scalar::fstp, 2, inexact);
    check(scalar_fast.bytes == scalar_retained.bytes && same_state(scalar_fast, scalar_retained) &&
          scalar_fast.fast_writes == unsigned(!native_raw_x87_detail::scoped_access_reference()),
          "Callback-free FSTP differs from retained");

    for (auto [instruction, flags] : {std::pair{Scalar::key_add_byte, 0xa82u}, std::pair{Scalar::key_add_word, 0x257u}}) {
        for (auto [operation, ordinal] : {std::pair{Operation::read, 1u}, std::pair{Operation::read, 2u},
                                          std::pair{Operation::write, 1u}, std::pair{Operation::observe, 1u}}) {
            const Fixture fault{aligned, ObserverChange::none, operation, ordinal};
            compare_scalar(fault, instruction, 0, 1.0L, operation);
            check_add_flags(fault, instruction, flags);
            ++scalar_cases;
        }
        compare_scalar({aligned}, instruction, 0, 1.0L, Operation::observe);
        compare_scalar({aligned, ObserverChange::none, Operation::write, 1, false}, instruction, 0, 1.0L);
        compare_scalar({aligned, ObserverChange::none, Operation::read, 0, false}, instruction, 0, 1.0L);
        scalar_cases += 3;
        const auto fast_add = run_scalar(callback_free, Policy::hosted, instruction, 0, 1.0L);
        check(!fast_add.fault && fast_add.abi.eflags == flags &&
              fast_add.fast_writes == unsigned(!native_raw_x87_detail::scoped_access_reference()),
              "Callback-free ADD published incorrect flags");
    }
    check_deferred_traversal(false);
    check_deferred_traversal(true);
    std::cout << "PASS " << cases << " hosted vector comparisons, " << scalar_cases
              << " hosted scalar comparisons and 19 physical/callback-free checks\n";
    return 0;
} catch (const std::exception &failure) {
    std::cerr << failure.what() << '\n';
    return 1;
}
