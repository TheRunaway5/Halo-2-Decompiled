#include "halo2/physics_world_step.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace {
using J = nlohmann::json;
void check(bool b, const std::string &s) { if (!b) throw std::runtime_error(s); }
std::vector<std::byte> bytes(const std::string &s) {
    check(s.size() % 2 == 0, "Invalid physics source bytes");
    std::vector<std::byte> b; b.reserve(s.size() / 2);
    for (std::size_t i = 0; i < s.size(); i += 2)
        b.push_back(std::byte(std::stoul(s.substr(i, 2), nullptr, 16)));
    return b;
}
struct Memory : halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::vector<std::byte>> banks;
    auto at(std::uint32_t p, std::size_t n) const {
        auto i = banks.upper_bound(p);
        if (i == banks.begin()) throw std::runtime_error("Missing original physics step storage");
        --i;
        if (std::uint64_t(p - i->first) + n > i->second.size())
            throw std::runtime_error("Missing original physics step storage extent");
        return i;
    }
    void read(std::uint32_t p, std::span<std::byte> b) const override {
        const auto i = at(p, b.size()); std::copy_n(i->second.begin() + p - i->first, b.size(), b.begin());
    }
    void write(std::uint32_t p, std::span<const std::byte> b) override {
        const auto i = at(p, b.size()); std::copy(b.begin(), b.end(), banks.at(i->first).begin() + p - i->first);
    }
};
struct Stop { halo2::SourcePhysicsContactBatchOwnerCall call; };
void run(const J &c, Memory &m, halo2::SourcePhysicsWorldStepServices &s,
         const halo2::SourcePhysicsContactRecordObserver &observe) {
    const auto p = c.at("receiver").get<std::uint32_t>(), esp = c.at("entry_esp").get<std::uint32_t>();
    const auto args = c.at("args").get<std::vector<std::uint32_t>>();
    const halo2::SourcePhysicsFactoryFloatingPoint fp{c.at("fp")[0].get<std::uint16_t>(), c.at("fp")[1].get<std::uint32_t>()};
    const auto image = c.value("image", std::string("1.1")) == "1.5" ?
        halo2::SourcePhysicsContactImage::TitleUpdate15 : halo2::SourcePhysicsContactImage::MapPack11;
    for (unsigned i = 0; i < c.at("iterations").get<unsigned>(); ++i) {
        switch (c.at("entry").get<std::uint32_t>()) {
        case 0x321530: halo2::source_apply_physics_angular_impulse_1_1(m, p, args.at(0), fp, observe); break;
        case 0x321760: halo2::source_apply_physics_motion_forces(m, p, args.at(0), args.at(1), esp, image, fp, observe); break;
        case 0x321600: halo2::source_integrate_physics_motion(m, p, args.at(0), esp, image, fp, s.reciprocal_sqrt, observe); break;
        case 0x3218a0: check(halo2::source_prepare_physics_solver_motion(m, p, args.at(0), args.at(1), args.at(2), esp, image, fp, observe) ==
            c.at("returns").at(i).get<std::uint32_t>(), "Original solver preparation output identity"); break;
        case 0x321a60: check(halo2::source_readback_physics_solver_motion(m, p, args.at(0), args.at(1), args.at(2), esp, image, fp, s.reciprocal_sqrt, observe) ==
            c.at("returns").at(i).get<std::uint32_t>(), "Original solver readback output identity"); break;
        case 0x322690: check(args.size() == 9 && bool(s.constrained_islands), "Original constraint arguments/services");
            halo2::source_step_physics_constraints_1_1(m,
                {args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]},
                esp, fp, *s.constrained_islands, observe); break;
        case 0x31cfe0: halo2::source_integrate_physics_island_1_1(m, p, args.at(0), esp, fp, s, observe); break;
        case 0x31d0f0: check(halo2::source_check_physics_island_deactivation_1_1(m, p, args.at(0), args.at(1), esp, fp, s, observe) == args.at(0), "Deactivation output identity"); break;
        case 0x316200: halo2::source_collide_physics_world_1_1(m, p, args.at(0), esp, fp, s, observe); break;
        case 0x316440: halo2::source_integrate_physics_world_1_1(m, p, args.at(0), esp, fp, s, observe); break;
        case 0x317650: halo2::source_step_physics_world_1_1(m, p, args.at(0), esp, fp, s, observe); break;
        default: throw std::runtime_error("Unverified physics world step entry");
        }
    }
}
}
int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Supply original world/motion step fixture");
        if (!std::filesystem::exists(argv[1])) return 77;
        std::ifstream input(argv[1]); const auto fixture = J::parse(input);
        const auto base = fixture.at("base").get<std::uint32_t>(), size = fixture.at("size").get<std::uint32_t>();
        std::size_t count = 0, writes = 0, total_bytes = 0, boundaries = 0;
        for (const auto &c : fixture.at("cases")) {
            const auto name = c.at("name").get<std::string>();
            const bool natural = c.contains("banks");
            Memory m;
            if (natural) for (const auto &k : c.at("banks")) m.banks.emplace(k[0].get<std::uint32_t>(), bytes(k[1]));
            else {
                m.banks.emplace(base, bytes(c.at("before")));
                for (const auto &k : c.at("constants")) m.banks.emplace(k[0].get<std::uint32_t>(), bytes(k[1]));
            }
            m.banks.emplace(c.at("entry_esp").get<std::uint32_t>() & ~0xffffu, std::vector<std::byte>(0x10000));
            if (c.contains("stack_inputs")) for (const auto &k : c.at("stack_inputs"))
                m.write(k[0].get<std::uint32_t>(), bytes(k[1]));
            const auto initial = c.at("boundary").is_null() ? std::optional<Memory>{} : std::optional(m);
            std::vector<halo2::SourcePhysicsContactRecordStore> stores;
            const auto observe = [&](const auto &a) {
                if (natural ? !(a.address >= 0x2000000 && a.address < 0x2200000) :
                    (a.address >= base && std::uint64_t(a.address) + a.written.size() <= std::uint64_t(base) + size))
                    stores.push_back(a);
            };
            halo2::SourcePhysicsWorldStepServices services;
            services.required_owner = [](auto &, const auto &call, const auto &) -> std::uint32_t { throw Stop{call}; };
            services.agents.required_owner = [](auto &, const auto &call, const auto &) -> std::uint32_t {
                throw Stop{{call.instruction, call.entry, call.entry_esp,
                    call.cdecl ? std::nullopt : std::optional(call.receiver), call.arguments}};
            };
            if (c.value("connect_current_broadphase", false)) {
                services.current_broadphase.emplace();
                services.current_broadphase->required_owner = services.agents.required_owner;
                services.current_broadphase->pairs.required_owner = [](auto &, const auto &, const auto &) -> std::uint32_t {
                    throw std::runtime_error("Unexpected lower pair-manager owner");
                };
            }
            if (c.value("connect_contact_dispatch", false)) {
                services.contact_dispatch.emplace();
                services.contact_dispatch->packet_frame_owner = [](auto &, const auto &q, const auto &) {
                    throw Stop{{q.instruction, q.entry, q.entry_esp, q.descriptor, {}}};
                };
            }
            std::size_t estimate = 0;
            std::vector<std::uint32_t> constraint_trace;
            services.reciprocal_sqrt = [&](std::uint32_t word) {
                check(estimate < c.at("estimates").size(), name + " unexpected RSQRTSS");
                const auto &q = c.at("estimates")[estimate++];
                check(word == q[0].get<std::uint32_t>(), name + " RSQRTSS input");
                return q[1].get<std::uint32_t>(); // Explicit emulator-only comparison service.
            };
            if (c.value("connect_constrained_islands", false)) {
                services.constrained_islands.emplace();
                services.constrained_islands->required_owner = services.required_owner;
                services.constrained_islands->reciprocal_sqrt = services.reciprocal_sqrt;
                if (c.value("connect_full_solver", false)) {
                    services.constrained_islands->solver.emplace();
                    services.constrained_islands->solver->required_owner = services.required_owner;
                    services.constrained_islands->solver->reciprocal_sqrt = services.reciprocal_sqrt;
                    services.constrained_islands->solver->instruction = [&](auto p) { constraint_trace.push_back(p); };
                }
                // Prefix controls also exercise an explicitly connected module
                // with an unknown virtual entry: it must retain the boundary.
                const auto &boundary = c.at("boundary");
                if (c.value("connect_jacobians", false) ||
                    (!boundary.is_null() && boundary.value("instruction", 0u) == 0x322921)) {
                    services.constrained_islands->jacobians.emplace();
                    services.constrained_islands->jacobians->required_owner = services.required_owner;
                    services.constrained_islands->jacobians->reciprocal_sqrt = services.reciprocal_sqrt;
                    services.constrained_islands->jacobians->instruction = [&](auto p) { constraint_trace.push_back(p); };
                }
            }
            bool stopped = false;
            try { run(c, m, services, observe); }
            catch (const Stop &e) {
                stopped = true; const auto &b = c.at("boundary");
                check(!b.is_null(), name + " unexpected required owner");
                const auto ecx = b.at("ecx").is_null() ? std::nullopt : std::optional(b.at("ecx").get<std::uint32_t>());
                check(e.call.entry == b.at("entry").get<std::uint32_t>() && e.call.ecx == ecx &&
                    e.call.entry_esp == b.at("entry_esp").get<std::uint32_t>() &&
                    e.call.arguments == b.at("arguments").get<std::vector<std::uint32_t>>(), name + " required owner ABI");
                if (b.contains("instruction")) check(e.call.instruction == b.at("instruction").get<std::uint32_t>(), name + " required owner call site");
                ++boundaries;
            }
            check(stopped == !c.at("boundary").is_null(), name + " completion boundary");
            check(estimate == c.at("estimates").size(), name + " estimate count");
            if (c.contains("constraint_runtime_trace"))
                check(constraint_trace == c.at("constraint_runtime_trace").get<std::vector<std::uint32_t>>(),
                    name + " producer/solver/callback instruction sequence differs");
            if (initial) {
                estimate = 0;
                auto absent_memory = *initial;
                auto absent_services = services;
                absent_services.required_owner = {};
                absent_services.agents.required_owner = {};
                if (absent_services.current_broadphase) {
                    absent_services.current_broadphase->required_owner = {};
                    absent_services.current_broadphase->pairs.required_owner = {};
                }
                if (absent_services.contact_dispatch) absent_services.contact_dispatch->packet_frame_owner = {};
                if (absent_services.constrained_islands) {
                    absent_services.constrained_islands->required_owner = {};
                    if (absent_services.constrained_islands->solver)
                        absent_services.constrained_islands->solver->required_owner = {};
                }
                std::vector<halo2::SourcePhysicsContactRecordStore> absent_stores;
                bool rejected = false;
                try {
                    run(c, absent_memory, absent_services, [&](const auto &a) {
                        if (natural ? !(a.address >= 0x2000000 && a.address < 0x2200000) :
                            (a.address >= base && std::uint64_t(a.address) + a.written.size() <= std::uint64_t(base) + size))
                            absent_stores.push_back(a);
                    });
                } catch (const std::runtime_error &e) {
                    rejected = std::string(e.what()).starts_with("Required original");
                }
                check(rejected && absent_stores == stores, name + " absent provider fabricated completion or changed prefix");
            }
            const auto &expected = c.at("writes");
            std::size_t i = 0;
            while (i < stores.size() && i < expected.size()) {
                const auto &s = stores[i]; const auto &e = expected[i];
                if (s.instruction != e.at("instruction").get<std::uint32_t>() || s.address != e.at("address").get<std::uint32_t>() ||
                    s.before != bytes(e.at("before")) || s.written != bytes(e.at("written"))) break;
                ++i;
            }
            if (i != stores.size() || i != expected.size()) {
                std::ostringstream out; out << name << " store " << i << " counts " << stores.size() << '/' << expected.size();
                if (i < stores.size()) out << " native " << std::hex << stores[i].instruction << " @ " << stores[i].address;
                if (i < expected.size()) out << " original " << std::hex << expected[i].at("instruction").get<std::uint32_t>() << " @ " << expected[i].at("address").get<std::uint32_t>();
                if (i < stores.size() && i < expected.size()) {
                    out << " native bytes "; for (auto b : stores[i].written) out << unsigned(std::to_integer<unsigned char>(b)) << ' ';
                    out << " expected " << expected[i].at("written").get<std::string>();
                    out << " prior "; for (auto b : stores[i].before) out << unsigned(std::to_integer<unsigned char>(b)) << ' ';
                    out << " expected " << expected[i].at("before").get<std::string>();
                }
                throw std::runtime_error(out.str());
            }
            if (natural) {
                for (const auto &k : c.at("after_banks")) {
                    const auto after = bytes(k[1]); std::vector<std::byte> actual(after.size());
                    m.read(k[0].get<std::uint32_t>(), actual);
                    check(actual == after, name + " retained source bank differs"); total_bytes += after.size();
                }
            } else {
                const auto after = bytes(c.at("after"));
                if (m.banks.at(base) != after) {
                    auto pos = std::mismatch(after.begin(), after.end(), m.banks.at(base).begin()).first - after.begin();
                    throw std::runtime_error(name + " full source state differs at byte " + std::to_string(pos));
                }
                total_bytes += after.size();
            }
            writes += stores.size(); ++count;
        }
        if (fixture.value("fixture_kind", std::string{}) == "solver_motion")
            check(count == 80 && boundaries == 0, "Missing original solver preparation/readback coverage");
        else if (fixture.value("fixture_kind", std::string{}) == "constraint_step")
            check(count == 60 && boundaries == count, "Missing original constraint step prefix coverage");
        else if (fixture.value("fixture_kind", std::string{}) == "constraint_completed")
            check(count == 16 && boundaries == 0, "Missing completed constraint orchestration coverage");
        else if (fixture.value("fixture_kind", std::string{}) == "constraint_produced")
            check(count == 144 && boundaries == 0, "Missing actual producer/solver/readback coverage");
        else if (fixture.at("original_sha256").get<std::string>() == "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
            check(count == 48 && boundaries == 0, "Missing TU1.5 motion coverage");
        else if (fixture.value("connected_current_broadphase", false)) {
            const auto expected = fixture.value("connected_contact_dispatch", false) ? 2u : 1u;
            check(count == expected && boundaries == expected, "Missing connected original world collision coverage");
        }
        else check(count >= 104 && boundaries >= 4, "Missing original world/motion coverage");
        std::cout << count << " original motion/island/world cases; " << writes << " ordered persistent writes; "
                  << total_bytes << " full state bytes; " << boundaries << " required stops match\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
