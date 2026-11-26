#include "physics/runtime/native_tu15_mopp_ray_plane_compare.hpp"

#include <iostream>

namespace {
using namespace halo2;
using namespace physics_world_runtime_detail;
namespace plane = native_mopp_ray_plane_compare_detail;
using Route = plane::Continuation;
constexpr std::uint32_t base = 0x62000000u;
constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};

std::uint32_t native_spill(long double value, std::uint16_t &status) {
    std::uint32_t bits;
    std::uint16_t actual;
    asm volatile("fldt %2; fstps %0; fnstsw %1"
        :"=m"(bits),"=m"(actual):"m"(value):"st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return bits;
}

Route retained(WorldState &s, Route route) {
    switch (route) {
    case Route::split: case Route::resume: return route;
    case Route::read_end: goto read_end;
    case Route::original: break;
    }
    s.instruction_literal(0x30ade0u); w(s.memory, s.observe, 0x30ade0u, s.gp[4]+0x18u, 4,
        native_spill(s.fp.st(0), s.fp.status)); s.fp.pop();
read_end:
    s.instruction_literal(0x30ade4u); s.fp.push(ext(f(r(s.memory, s.gp[4]+0x18u))));
    s.instruction_literal(0x30ade8u); s.fp.comparison(ext(f(r(s.memory, s.gp[4]+0x1cu))), 1);
    s.instruction_literal(0x30adecu); s.set(0, s.fp.word(), 2, 0);
    s.instruction_literal(0x30adeeu); s.flags.logic(s.reg(0, 1, 8) & 5u, 1);
    s.instruction_literal(0x30adf1u); if (s.flags.pf) return Route::split;
    s.instruction_literal(0x30adf3u); s.fp.comparison(ext(f(r(s.memory, s.gp[4]+0x1cu))), 0);
    s.instruction_literal(0x30adf7u); s.set(0, s.fp.word(), 2, 0);
    s.instruction_literal(0x30adf9u); s.flags.logic(s.reg(0, 1, 8) & 5u, 1);
    s.instruction_literal(0x30adfcu); if (s.flags.pf) return Route::split;
    s.instruction_literal(0x30adfeu); s.gp[6] = s.flags.plus(s.gp[6], s.gp[1], 4);
    s.instruction_literal(0x30ae00u); s.fp.st(0) = s.fp.st(0); s.fp.status &= ~0x200u; s.fp.pop();
    return Route::resume;
}

struct Failure {};
enum class WriteEffect { none, lower_limit, move_frame, replace_cursor, change_control };
struct Fixture {
    std::uint32_t sp = base + 0x160u, fault_pc{};
    long double end = 0.25L, origin = 0.75L;
    unsigned rounding{}, precision{3};
    bool deny{}, observer{}, observer_fault{}, trace{}, install_trace{}, ordinary_view{};
    WriteEffect after_write{};
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;
    std::array<std::byte, 8> bytes{};
    std::vector<std::uint32_t> provider, instructions;
    std::vector<SourcePhysicsContactRecordStore> observed;
    std::uint32_t pc{0x30addcu}, writes{};
    Route stage = Route::original, exit = Route::original;
    bool fault{};
};
struct Bank final : SourcePhysicsContactMemory {
    const Fixture &input;
    Result &result;
    bool &trace;
    std::array<std::byte, 8192> bytes{};
    SourceDirectPageIndex index;
    std::function<void()> after_write;

    Bank(const Fixture &fixture, Result &report, bool &tracing)
        : input(fixture), result(report), trace(tracing) {
        direct_pages_ = &index;
        auto &block = index.blocks[base >> 22u];
        block = std::make_unique<SourceDirectPageIndex::Block>();
        for (unsigned i=0; i<2; ++i)
            (*block)[((base >> 12u)&1023u)+i] = {bytes.data()+i*4096u, true};
        const auto limit = std::bit_cast<std::uint32_t>(1.0f);
        std::memcpy(bytes.data()+input.sp-base+0x1cu, &limit, 4u);
    }
    bool try_callback_free_write(std::uint32_t address, std::span<const std::byte> data) noexcept override {
        if (input.deny || input.observer || !direct_page(address, data.size())) return false;
        std::memcpy(bytes.data()+address-base, data.data(), data.size());
        ++result.writes;
        return true;
    }
    void write(std::uint32_t address, std::span<const std::byte> data) override {
        if (try_callback_free_write(address, data)) return;
        access();
        std::memcpy(bytes.data()+address-base, data.data(), data.size());
        ++result.writes;
        if (input.install_trace) {
            trace = true;
            const auto limit = std::bit_cast<std::uint32_t>(-1.0f);
            std::memcpy(bytes.data()+input.sp-base+0x1cu, &limit, 4u);
        }
        if(after_write)
            after_write();
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t address,std::size_t width)const noexcept override {
        if (!input.ordinary_view || address<base || std::uint64_t(address-base)+width>bytes.size())
            return std::nullopt;
        return std::span(bytes).subspan(address-base,width);
    }
    void read(std::uint32_t address, std::span<std::byte> data) const override {
        access();
        std::memcpy(data.data(), bytes.data()+address-base, data.size());
    }
    void access() const {
        result.provider.push_back(result.pc);
        if (input.fault_pc==result.pc) throw Failure{};
    }
};

Result run(const Fixture &fixture, bool native) {
    Result result;
    bool trace = fixture.trace;
    Bank memory{fixture, result, trace};
    SourcePhysicsContactRecordObserver observer;
    if (fixture.observer) observer = [&](const auto &store) {
        result.observed.push_back(store);
        if (fixture.observer_fault) throw Failure{};
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) { result.pc=pc; result.instructions.push_back(pc); };
    services.instruction_cursor = {&result.pc, &trace, nullptr, service_sites,
        std::this_thread::get_id(), &services.instruction.target_type(), true};
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = {0x12349016u, 9u, 1u, base, fixture.sp, 0x3541u, 0xfffffffcu, 9u};
    abi.eflags=0x2d7u;
    abi.x87_depth=3;
    abi.x87_status=0x3200u;
    abi.x87[0]=fixture.end;
    abi.x87[1]=fixture.origin;
    abi.x87[2]=-1.25L;
    const auto cw = std::uint16_t(0x7fu | fixture.precision<<8u | fixture.rounding<<10u);
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{cw, 0x1f80u};
    asm volatile("fnclex; fldcw %0"::"m"(cw));
    WorldState state{memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    memory.after_write=[&] {
        switch(fixture.after_write) {
        case WriteEffect::none: break;
        case WriteEffect::lower_limit: {
            const auto lower=std::bit_cast<std::uint32_t>(-1.0f);
            std::memcpy(memory.bytes.data()+fixture.sp-base+0x1cu,&lower,4u);
            break;
        }
        case WriteEffect::move_frame: {
            state.gp[4]+=0x40u;
            const std::array words{std::bit_cast<std::uint32_t>(0.125f),std::bit_cast<std::uint32_t>(2.0f)};
            std::memcpy(memory.bytes.data()+state.gp[4]-base+0x18u,words.data(),8u);
            break;
        }
        case WriteEffect::replace_cursor:
            services.instruction=[&](std::uint32_t pc) { result.pc=pc; result.instructions.push_back(pc); };
            break;
        case WriteEffect::change_control: {
            const auto changed=std::uint16_t(cw^0x400u);
            asm volatile("fldcw %0"::"m"(changed));
            break;
        }
        }
    };
    try {
        if (native) result.stage=plane::plane_compare(state, native_spill);
        result.exit=retained(state, result.stage);
    } catch (const Failure &) {
        result.fault=true;
    }
    result.abi=state.save();
    std::memcpy(result.bytes.data(), memory.bytes.data()+fixture.sp-base+0x18u, result.bytes.size());
    return result;
}

void compare(const Fixture &fixture, Route stage, bool fault = false) {
    if (plane::reference_plane_compare()) stage=Route::original;
    else if (plane::reference_provider_handoff() && (fixture.deny || fixture.ordinary_view))
        stage=Route::read_end;
    auto translated=run(fixture, false);
    const auto native=run(fixture, true);
    // The topology-only view authenticates these bytes as ordinary. The
    // fixture's virtual-read counters are diagnostic, with no semantic effects.
    if(fixture.ordinary_view && native.stage!=Route::read_end && native.stage!=Route::original)
        std::erase_if(translated.provider,[](auto pc){return pc==0x30ade4u || pc==0x30ade8u || pc==0x30adf3u;});
    const auto &a=translated.abi, &b=native.abi;
    if ((!native.fault && native.stage!=stage) || native.fault!=fault || native.fault!=translated.fault ||
        native.exit!=translated.exit || native.pc!=translated.pc || native.writes!=translated.writes ||
        native.provider!=translated.provider || native.instructions!=translated.instructions ||
        native.observed!=translated.observed || native.bytes!=translated.bytes ||
        a.gp!=b.gp || a.eflags!=b.eflags || a.xmm!=b.xmm || a.x87_depth!=b.x87_depth ||
        a.x87_status!=b.x87_status || a.floating_point->mxcsr!=b.floating_point->mxcsr ||
        a.floating_point->x87_control_word!=b.floating_point->x87_control_word)
        throw std::runtime_error("Plane comparison state, provider, observer or continuation differs");
    for (unsigned i=0; i<a.x87_depth; ++i) {
        if (std::memcmp(&a.x87[i], &b.x87[i], 10u))
            throw std::runtime_error("Plane comparison changed a live x87 lane");
    }
}
}

int main() try {
    unsigned compared{};
    for (const auto pc : {0u, 2u, 3u}) for (unsigned rc=0; rc<4u; ++rc) {
        Fixture f; f.precision=pc; f.rounding=rc;
        compare(f, Route::resume); ++compared;
        auto first=f; first.end=2.0L;
        compare(first, Route::split); ++compared;
        auto second=f; second.origin=2.0L;
        compare(second, Route::split); ++compared;
        auto boundary=f; boundary.sp=base+0xfe3u;
        compare(boundary, Route::read_end); ++compared;
        boundary.fault_pc=0x30ade8u;
        compare(boundary, Route::read_end, true); ++compared;
        boundary.fault_pc=0x30adf3u;
        compare(boundary, Route::read_end, true); ++compared;
        boundary.sp=base+0xfe6u; boundary.fault_pc=0x30ade4u;
        compare(boundary, Route::read_end, true); ++compared;
        auto denied=f; denied.deny=true;
        compare(denied, Route::resume); ++compared;
        denied.after_write=WriteEffect::lower_limit;
        compare(denied, Route::split); ++compared;
        denied.after_write=WriteEffect::move_frame;
        compare(denied, Route::resume); ++compared;
        denied.after_write=WriteEffect::replace_cursor;
        compare(denied, Route::read_end); ++compared;
        denied.after_write=WriteEffect::change_control;
        compare(denied, Route::read_end); ++compared;
        auto sparse=f; sparse.sp=base+0xfe3u; sparse.ordinary_view=true;
        compare(sparse, Route::resume); ++compared;
        denied.after_write=WriteEffect::none;
        denied.install_trace=true;
        compare(denied, Route::read_end); ++compared;
        denied.fault_pc=0x30ade0u;
        compare(denied, Route::read_end, true); ++compared;
        auto observed=f; observed.observer=true; observed.observer_fault=true;
        compare(observed, Route::original, true); ++compared;
        f.trace=true;
        compare(f, Route::original); ++compared;
    }
    std::cout << "30ADE0 plane: " << compared << " exact continuation and fault comparisons\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
}
