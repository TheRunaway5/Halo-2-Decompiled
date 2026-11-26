#include "halo2/source_physics_store_observers_tu15.hpp"
#include "pstencil/native_tu15_light_shape_written.hpp"
#include "rendering/native_tu15_light_shape.hpp"
#include "native_tu15_shadow_frame_harness.hpp"

namespace prepared {
using namespace physics_world_runtime_detail;
using WorldState = InstrumentedState;
#include "light_shape_written_owner.inc"
} // namespace prepared

namespace {
using namespace shadow_frame_harness;
using pstencil::LightShapeWrittenObserver;
using Written = std::array<std::uint8_t,124>;

void check_collector_scope() {
    Written written{};
    constexpr std::uint32_t output = 0x61002000u;
    SourcePhysicsContactRecordObserver expired;
    std::optional<LightShapeWrittenObserver> current;
    current.emplace(output,written);
    const auto observer = current->observer();
    const auto copied = source_physics_store_observers_tu15(observer,{});
    check(LightShapeWrittenObserver::qualified(copied),"Exact copied shape collector refused");
    const auto combined = source_physics_store_observers_tu15(observer,[](const auto &){});
    check(!LightShapeWrittenObserver::qualified(combined),"Tee with a second observer qualified");
    bool observed_elsewhere{};
    std::thread other([&] { observed_elsewhere = LightShapeWrittenObserver::qualified(copied); });
    other.join();
    check(!observed_elsewhere,"Shape collector qualified on a second executor");
    {
        Written inner_written{};
        LightShapeWrittenObserver inner(output,inner_written);
        check(LightShapeWrittenObserver::qualified(inner.observer()) &&
              !LightShapeWrittenObserver::qualified(copied),"Nested shape collector lost its scope");
    }
    check(LightShapeWrittenObserver::qualified(copied),"Outer shape collector scope was not restored");
    expired = copied;
    current.reset();
    check(!LightShapeWrittenObserver::qualified(expired),"Expired shape collector qualified");
    current.emplace(output,written);
    check(!LightShapeWrittenObserver::qualified(expired),"Reused scope address qualified an expired collector");

    const auto active = current->observer();
    active({0u,output - 2u,{},std::vector<std::byte>(4)});
    active({0u,output + 123u,{},std::vector<std::byte>(2)});
    active({0u,output + 124u,{},std::vector<std::byte>(4)});
    check(written[0] && written[1] && written[123] &&
          std::count(written.begin(),written.end(),std::uint8_t{1}) == 3,
          "Written-byte collector changed its output clipping");
}

enum class Route {qualified, plain, tee, traced};
struct Counts {
    unsigned native{}, retained{}, read_accesses{}, writes{}, packets{};
};
void compare(const J &fixture, const J &row, Route route, bool forced, Counts &counts) {
    Memory memory;
    for (const auto &bank : row.at("banks"))
        memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
    for (auto pc : fixture.at("write_sites"))
        memory.write_sites.insert(pc.get<std::uint32_t>());
    for (auto pc : fixture.at("rmw_sites"))
        memory.rmw_sites.insert(pc.get<std::uint32_t>());

    SourcePhysicsWorldRuntimeAbi input{};
    input.gp = row.at("gp");
    input.eflags = row.at("eflags");
    input.xmm = row.at("xmm");
    input.floating_point = SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    input.x87_status = row.at("before_fp")[1];
    input.x87_depth = row.at("live_x87").size();
    for (unsigned n = 0; n < input.x87_depth; ++n) {
        const auto bytes = unhex(row.at("live_x87")[n]);
        std::memcpy(&input.x87[n],bytes.data(),10u);
    }
    constexpr std::array policy_sites{0x38846eu,0x21f590u,0x33b595u};
    const bool trace = route == Route::traced;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) { memory.pc = pc; };
    // Production never labels this per-call observer as a global read-only
    // observer. The owning type grants the narrow C3AB0 permission.
    services.instruction_cursor = {&memory.pc,&trace,nullptr,policy_sites,std::this_thread::get_id(),
                                   &services.instruction.target_type(),true,false};
    J calls = J::array();
    services.required_owner = Stub{&memory,&row.at("stub_effects"),&calls};
    certify(services);

    const auto output = input.gp[2];
    Written written{}, expected_written{};
    const LightShapeWrittenObserver collector(output,written);
    const auto local_observer = collector.observer();
    unsigned forwarded{};
    SourcePhysicsContactRecordObserver observer;
    if (route == Route::plain)
        observer = [&](const auto &store) { local_observer(store); };
    else if (route == Route::tee)
        observer = source_physics_store_observers_tu15(local_observer,[&](const auto &) { ++forwarded; });
    else
        observer = source_physics_store_observers_tu15(local_observer,{});
    InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(input);
    memory.capture = [&] { return packet(state,memory.pc); };
    retained_statements = 0;
    prepared::run_c3ab0(state);
    const bool native = route == Route::qualified && !forced;
    const auto name = row.at("name").get<std::string>() + " mode " + std::to_string(unsigned(route));
    check(bool(retained_statements) != native,name + ": wrong native or retained route");
    const J expected{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},
                     {"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},
                     {"fp",row.at("after_fp")},{"live_x87",row.at("after_live_x87")}};
    check(packet(state,memory.pc) == expected,name + ": final source ABI differs");
    check(memory.reads == row.at("reads") && memory.writes == row.at("writes") && calls.empty(),
          name + ": ordered memory or child behavior differs");
    if (native) {
        check(memory.packets == row.at("calls"),name + ": native pre-access packets differ");
        counts.packets += unsigned(memory.packets.size());
    }
    for (const auto &bank : row.at("banks"))
        check(hex(memory.banks.at(bank.at("address"))) == bank.at("after").get<std::string>(),
              name + ": output bank differs");
    for (const auto &store : row.at("writes")) {
        const auto address = store.at("address").get<std::uint32_t>();
        const auto bytes = store.at("written").get<std::string>().size() / 2u;
        for (unsigned offset = 0; offset < bytes; ++offset)
            if (address + offset >= output && address + offset < output + expected_written.size())
                expected_written[address + offset - output] = 1;
    }
    check(written == expected_written,name + ": written-byte mask differs");
    if (route == Route::tee)
        check(forwarded == memory.writes.size(),name + ": registered observer delivery differs");
    counts.native += native;
    counts.retained += !native;
    counts.read_accesses += unsigned(memory.reads.size());
    counts.writes += unsigned(memory.writes.size());
}
} // namespace

int main(int argc, char **argv) try {
    check(argc == 2,"Supply the original light shape corpus");
    std::ifstream input(argv[1]);
    const auto fixture = J::parse(input);
    const bool forced = std::getenv("HALO2_REFERENCE_LIGHT_SHAPE") != nullptr;
    check_collector_scope();
    Counts counts;
    for (const auto &row : fixture.at("cases"))
        for (const auto route : {Route::qualified,Route::plain,Route::tee,Route::traced})
            compare(fixture,row,route,forced,counts);
    std::cout << J{{"native",counts.native},{"retained",counts.retained},{"ordered_reads",counts.read_accesses},
                   {"ordered_writes",counts.writes},{"native_pre_access_packets",counts.packets}}.dump() << '\n';
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
