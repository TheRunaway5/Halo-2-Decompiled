#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/source_object_loop_audio_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace halo2;
using J = nlohmann::json;
using Bytes = std::vector<std::byte>;
using Abi = SourcePhysicsWorldRuntimeAbi;
constexpr std::string_view original_sha =
    "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd";
constexpr std::uint32_t private_base = 0x02400000u, private_size = 262144u;
constexpr std::uint32_t stack_base = 0x60000000u, stack_size = 0x200000u;
constexpr std::uint32_t fs_base = 0x60200000u, fs_size = 4096u;
constexpr std::size_t trace_limit = 262144u, writes_limit = 2u * 1024u * 1024u;

void check(bool value, const std::string &message) {
    if (!value) throw std::runtime_error(message);
}
std::string hex_address(std::uint32_t value) {
    std::ostringstream out; out << std::hex << value; return out.str();
}
unsigned nibble(char c) {
    if (c >= '0' && c <= '9') return unsigned(c - '0');
    if (c >= 'a' && c <= 'f') return unsigned(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return unsigned(c - 'A' + 10);
    throw std::runtime_error("Invalid fixture hexadecimal digit");
}
Bytes decode(const J &value, std::size_t limit) {
    const auto &s = value.get_ref<const std::string &>();
    check(s.size() % 2 == 0 && s.size() / 2 <= limit, "Fixture byte extent exceeds bound");
    Bytes out(s.size() / 2);
    for (std::size_t n = 0; n < out.size(); ++n)
        out[n] = std::byte((nibble(s[n * 2]) << 4u) | nibble(s[n * 2 + 1]));
    return out;
}
std::uint32_t word(std::span<const std::byte> bytes, std::size_t offset) {
    check(offset <= bytes.size() && bytes.size() - offset >= 4, "Truncated caller word");
    std::uint32_t result{};
    for (unsigned n = 0; n < 4; ++n) result |= std::uint32_t(std::to_integer<unsigned>(bytes[offset + n])) << (n * 8u);
    return result;
}
void append(Bytes &out, std::uint32_t value) {
    for (unsigned n = 0; n < 4; ++n) out.push_back(std::byte(value >> (n * 8u)));
}

// Only the declared image cells exist. Unlisted image bytes cannot silently
// become zero operands. The three explicitly declared writable banks do begin
// zero-filled, exactly as supplied to the independent original-call fixture.
struct Memory final : SourcePhysicsContactMemory {
    struct Bank { Bytes bytes; bool writable; };
    std::map<std::uint32_t, Bank> banks;
    void mount(std::uint32_t address, Bytes bytes, bool writable) {
        check(!bytes.empty() && std::uint64_t(address) + bytes.size() <= 0x100000000ull,
              "Invalid fixture bank extent");
        const auto next = banks.lower_bound(address);
        check(next == banks.end() || std::uint64_t(address) + bytes.size() <= next->first,
              "Overlapping fixture bank");
        if (next != banks.begin()) {
            const auto previous = std::prev(next);
            check(std::uint64_t(previous->first) + previous->second.bytes.size() <= address,
                  "Overlapping fixture bank");
        }
        check(banks.emplace(address, Bank{std::move(bytes), writable}).second, "Duplicate fixture bank");
    }
    auto containing(std::uint32_t address) const {
        auto found = banks.upper_bound(address);
        check(found != banks.begin(), "Unmapped original operand @ " + hex_address(address));
        --found;
        check(std::uint64_t(address - found->first) < found->second.bytes.size(),
              "Unmapped original operand @ " + hex_address(address));
        return found;
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        check(std::uint64_t(address) + out.size() <= 0x100000000ull, "Read operand wraps");
        for (auto &value : out) {
            const auto bank = containing(address);
            value = bank->second.bytes[address++ - bank->first];
        }
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        check(std::uint64_t(address) + in.size() <= 0x100000000ull, "Write operand wraps");
        // Validate the complete write first, including read-only boundaries.
        for (std::size_t n = 0; n < in.size(); ++n)
            check(containing(address + static_cast<std::uint32_t>(n))->second.writable,
                  "Source wrote a read-only original image operand");
        for (auto value : in) {
            const auto bank = containing(address);
            banks.at(bank->first).bytes[address++ - bank->first] = value;
        }
    }
    Bytes writable_state() const {
        Bytes result;
        for (const auto &[address, bank] : banks)
            if (bank.writable) result.insert(result.end(), bank.bytes.begin(), bank.bytes.end());
        return result;
    }
};

// This snapshot intentionally copies only integer representations. Observer
// callbacks neither calculate floating-point values nor call source owners.
using Snapshot = std::array<std::byte, 180>;
Snapshot snapshot(const Abi &a) {
    Snapshot result{}; std::size_t at{};
    const auto emit = [&](std::uint32_t value, unsigned size) {
        for (unsigned n = 0; n < size; ++n) result[at++] = std::byte(value >> (n * 8u));
    };
    for (const auto value : a.gp) emit(value, 4);
    for (const auto &reg : a.xmm) for (const auto value : reg) emit(value, 4);
    emit(a.eflags, 4); emit(a.fs_base, 4); emit(a.x87_depth, 4); emit(a.x87_status, 2);
    emit(a.floating_point ? a.floating_point->x87_control_word : 0u, 2);
    emit(a.floating_point ? a.floating_point->mxcsr : 0u, 4);
    return result;
}
Abi caller(const J &c) {
    const auto bytes = decode(c.at("caller_hex"), 168);
    check(bytes.size() == 168, "Original caller must contain 168 bytes");
    Abi a{}; std::size_t at{};
    for (auto &value : a.gp) { value = word(bytes, at); at += 4; }
    for (auto &reg : a.xmm) for (auto &value : reg) { value = word(bytes, at); at += 4; }
    a.eflags = word(bytes, at); a.fs_base = word(bytes, at + 4);
    check(c.at("initial_fp").at("x87_control_word") == 639 &&
          c.at("initial_fp").at("mxcsr") == 8064, "Unexpected original floating-point controls");
    a.floating_point = SourcePhysicsFactoryFloatingPoint{639, 8064};
    check(a.fs_base == fs_base && a.gp[4] >= stack_base + 4096u &&
          a.gp[4] <= stack_base + stack_size - 64u, "Original caller stack/FS extent differs");
    return a;
}
Memory memory(const J &c, const Abi &a, std::uint32_t entry) {
    Memory m;
    const auto &image = c.at("image_data");
    check(image.is_array() && !image.empty() && image.size() <= 4096, "Invalid original image operand count");
    std::size_t image_bytes{};
    for (const auto &cell : image) {
        auto bytes = decode(cell.at("hex"), 1024u * 1024u);
        image_bytes += bytes.size(); check(image_bytes <= 1024u * 1024u, "Original image operands exceed bound");
        m.mount(cell.at("address").get<std::uint32_t>(), std::move(bytes), false);
    }
    const auto &bank = c.at("private_bank");
    check(bank.at("address") == private_base && bank.at("bytes") == private_size &&
          bank.at("zero_fill") == true, "Private original input bank differs");
    Bytes data(private_size); std::vector<bool> supplied(private_size);
    check(bank.at("nonzero_blocks").is_array() && bank.at("nonzero_blocks").size() <= 4096,
          "Private original block count exceeds bound");
    for (const auto &block : bank.at("nonzero_blocks")) {
        check(block.is_array() && block.size() == 2, "Invalid private original block");
        const auto address = block.at(0).get<std::uint32_t>();
        const auto bytes = decode(block.at(1), private_size);
        check(address >= private_base && std::uint64_t(address - private_base) + bytes.size() <= private_size,
              "Private original block escapes bank");
        for (std::size_t n = 0; n < bytes.size(); ++n) {
            const auto offset = address - private_base + n;
            check(!supplied[offset], "Overlapping private original input blocks");
            supplied[offset] = true; data[offset] = bytes[n];
        }
    }
    m.mount(private_base, std::move(data), true);
    m.mount(stack_base, Bytes(stack_size), true); m.mount(fs_base, Bytes(fs_size), true);
    const auto &arguments = c.at("stack_arguments");
    check(arguments.is_array() && arguments.size() == (entry == 0x17e160u ? 3u : 4u) &&
          arguments.at(0) == 0x60300000u, "Original caller stack arguments differ");
    Bytes stack; for (const auto &argument : arguments) append(stack, argument.get<std::uint32_t>());
    m.write(a.gp[4], stack); return m;
}

struct Result {
    Bytes trace, writes, retained;
    Snapshot exit{};
    std::size_t entering{}, returning{};
    Snapshot observed_entry{}, observed_exit{};
    std::uint32_t observed_entry_id{}, observed_exit_id{};
};
Bytes native_copy_trace(const J &c, const J &physical) {
    check(c.at("case") == physical.at("case") &&
          c.at("original_trace_hex") == physical.at("legacy_trace_hex") &&
          physical.at("original_writes_equal") == true &&
          physical.at("original_full_hardware_exit_equal_except_IF_TF_RF") == true,
          "Physical loop-location input/trace provenance differs");
    const auto expected = decode(physical.at("physical_trace_hex"), trace_limit * 4);
    check(expected == decode(physical.at("native_copy_trace_hex"), trace_limit * 4),
          "Complete physical case trace differs from native-owner reference");
    return expected;
}
Result run(const J &c, bool bound, const J &physical) {
    const auto entry = c.at("entry").get<std::uint32_t>();
    auto a = caller(c); const auto incoming = snapshot(a); auto m = memory(c, a, entry);
    Result result;
    result.trace.reserve(decode(c.at("original_trace_hex"), trace_limit * 4).size());
    result.writes.reserve(decode(c.at("original_ordered_nonstack_writes_hex"), writes_limit).size());
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](auto instruction) {
        check(result.trace.size() / 4 < trace_limit, "Original instruction execution exceeds bound");
        append(result.trace, instruction);
    };
    services.required_owner = [&](auto &bank, const auto &call, auto &state, const auto &observe) {
        const auto fp = state.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{639, 8064});
        if (source_effect_particle_frame_tu15_owns_entry(call.target))
            source_run_effect_particle_frame_tu15(bank, call.target, state, fp, services, observe);
        else if (source_combat_action_tu15_owns_entry(call.target))
            source_run_combat_action_tu15(bank, call.target, state, fp, services, observe);
        else if (source_physics_live_support_tu15_owns_entry(call.target))
            source_run_physics_live_support_tu15(bank, call.target, state, fp, services, observe);
        else if (call.target == 0x144910u) {
            auto control = fp;
            source_run_physics_phantom_world_tu15_runtime(bank, call.target, state, control, services, observe);
            state.floating_point = control;
        } else throw std::runtime_error("Unbound real loop-location callee " + hex_address(call.instruction) +
                                        " -> " + hex_address(call.target));
    };
    std::optional<SourcePhysicsEntryObserverBinding> observation;
    if (bound) observation.emplace(m, [&](bool entering, std::uint32_t id, const Abi &state) {
        if (entering) {
            ++result.entering; result.observed_entry_id = id; result.observed_entry = snapshot(state);
        } else {
            ++result.returning; result.observed_exit_id = id; result.observed_exit = snapshot(state);
        }
    });
    source_run_effect_particle_frame_tu15(m, entry, a, *a.floating_point, services, [&](const auto &store) {
        if (store.address >= stack_base && store.address < stack_base + stack_size) return;
        check(store.before.size() == store.written.size() && store.written.size() <= 4096,
              "Invalid original ordered write extent");
        check(result.writes.size() + 12 + store.before.size() * 2 <= writes_limit,
              "Original ordered writes exceed bound");
        append(result.writes, store.instruction); append(result.writes, store.address);
        append(result.writes, static_cast<std::uint32_t>(store.written.size()));
        result.writes.insert(result.writes.end(), store.before.begin(), store.before.end());
        result.writes.insert(result.writes.end(), store.written.begin(), store.written.end());
    });
    check(a.x87_depth == 0 && !a.active_mmx && a.floating_point.has_value(), "Unexpected loop-location exit register class");
    result.exit = snapshot(a); result.retained = m.writable_state();
    const auto expected_trace=native_copy_trace(c,physical);
    if(result.trace!=expected_trace) {
        std::size_t offset{};const auto common=std::min(result.trace.size(),expected_trace.size());
        while(offset+4<=common&&word(result.trace,offset)==word(expected_trace,offset))offset+=4;
        throw std::runtime_error("Original instruction trace differs at index "+std::to_string(offset/4)+
            ": native="+(offset+4<=result.trace.size()?hex_address(word(result.trace,offset)):"end")+
            " original="+(offset+4<=expected_trace.size()?hex_address(word(expected_trace,offset)):"end")+
            " native_count="+std::to_string(result.trace.size()/4)+
            " original_count="+std::to_string(expected_trace.size()/4));
    }
    check(result.writes == decode(c.at("original_ordered_nonstack_writes_hex"), writes_limit), "Original ordered nonstack writes differ");
    auto expected = decode(c.at("expected_hardware_exit_hex"), 180);
    check(expected.size() == 180, "Original hardware exit must contain 180 bytes");
    expected[161] &= std::byte{0xfd}; // Host hardware alone supplies interrupt-enable.
    check(std::ranges::equal(result.exit, expected), "Original full GP/XMM/flags/FS/x87/CW/MXCSR exit differs");
    if (bound) {
        check(result.entering == 1 && result.returning == 1 && result.observed_entry_id == entry &&
              result.observed_exit_id == entry, "Passive entry/return observation count differs");
        check(result.observed_entry == incoming && result.observed_exit == result.exit,
              "Passive observation incoming/outgoing ABI differs");
    }
    return result;
}

void owner_coverage() {
    // The complete direct 18F9B0 callee set, including the formerly missing two
    // location owners. Declaration coverage is separate from fixture execution.
    constexpr std::array targets{0x17e160u, 0x18bbc0u, 0x11e600u, 0x230b10u,
        0x18b580u, 0xbcb70u, 0xbac70u, 0xbad60u, 0xbc200u, 0xbc330u, 0x1902e0u};
    for (const auto target : targets)
        check(source_effect_particle_frame_tu15_owns_entry(target) ||
              source_physics_live_support_tu15_owns_entry(target) ||
              source_combat_action_tu15_owns_entry(target) ||
              source_object_loop_audio_tu15_owns_entry(target),
              "Direct 18F9B0 callee lacks a source owner: " + hex_address(target));
    check(source_effect_particle_frame_tu15_owns_entry(0x17e160u) &&
          source_effect_particle_frame_tu15_owns_entry(0x230b10u), "Loop-location frame owners absent");
}
void report(const std::filesystem::path &path, const J &value) {
    if (path.empty()) return;
    const auto text = value.dump(2) + "\n"; check(text.size() < 65536, "Loop-location report exceeds bound");
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc); out << text;
    check(bool(out), "Cannot retain loop-location report");
}
} // namespace

int main(int argc, char **argv) {
    std::filesystem::path output;
    J result{{"passed", false}, {"cases", 0}, {"comparisons", 0}, {"mode3_cases", 0},
             {"mode2_cases", 0}, {"original_xbe_sha256", original_sha},
             {"scope", "Original TU1.5 loop-location data/exit and complete physical case traces; no all-path, lifecycle, game ELF or GPU claim"},
             {"physical_native_copy_rep_sites", {0xbaeb0u, 0x17ddb4u, 0x17ddfbu}},
             {"physical_complete_case_traces", true}, {"all_paths_or_lifecycles_proven", false},
             {"case_results", J::array()}};
    try {
        check(argc == 3 || (argc == 5 && std::string_view(argv[3]) == "--report-output"),
              "Supply FIXTURE PHYSICAL_TRACE [--report-output REPORT.json]");
        if (argc == 5) output = argv[4];
        const std::filesystem::path fixture_path = argv[1];
        check(std::filesystem::is_regular_file(fixture_path) && std::filesystem::file_size(fixture_path) <= 8u * 1024u * 1024u,
              "Original loop-location fixture absent or exceeds bound");
        std::ifstream in(fixture_path); const auto fixture = J::parse(in);
        const std::filesystem::path physical_path = argv[2];
        check(std::filesystem::is_regular_file(physical_path) &&
              std::filesystem::file_size(physical_path) <= 1024u * 1024u,
              "Physical loop-location trace absent or exceeds bound");
        std::ifstream physical_in(physical_path); const auto physical = J::parse(physical_in);
        check(physical.at("schema") == "HALO2_LOOP_AUDIO_MARKER_MATRIX_PHYSICAL_TRACE_V1" &&
              physical.at("original_xbe_sha256") == original_sha &&
              physical.at("native_copy_rep_sites") == J::array({0xbaeb0u, 0x17ddb4u, 0x17ddfbu}) &&
              physical.at("source_instruction_patches").empty() &&
              physical.at("supplied_game_services").empty() &&
              physical.at("cases").size() == 22,
              "Physical loop-location provenance/scope differs");
        check(fixture.at("schema") == "HALO2_LOOP_AUDIO_LOCATION_ORIGINAL_V1" &&
              fixture.at("original_xbe_sha256") == original_sha, "Original loop-location fixture provenance differs");
        check(fixture.at("source_instruction_patches").is_array() &&
              fixture.at("source_instruction_patches").empty() &&
              fixture.at("supplied_game_services").is_array() &&
              fixture.at("supplied_game_services").empty(), "Original loop-location fixture replaces source execution");
        const auto &cases = fixture.at("cases");
        check(cases.is_array() && cases.size() == 22, "Expected all 22 original loop-location cases");
        owner_coverage(); std::set<std::string> names; std::size_t mode3{}, mode2{};
        for (const auto &c : cases) {
            const auto name = c.at("case").get<std::string>();
            check(!name.empty() && name.size() <= 128 && names.insert(name).second, "Invalid/duplicate original case name");
            const auto entry = c.at("entry").get<std::uint32_t>();
            check(entry == 0x17e160u || entry == 0x230b10u, "Unexpected original loop-location entry");
            if (c.contains("source_image_sha256"))
                check(c.at("source_image_sha256") == original_sha, "Original case image provenance differs");
            try {
                const auto &physical_cases = physical.at("cases");
                const auto first = std::find_if(physical_cases.begin(), physical_cases.end(),
                    [&](const auto &value) { return value.at("case") == name; });
                check(first != physical_cases.end() && std::count_if(physical_cases.begin(), physical_cases.end(),
                    [&](const auto &value) { return value.at("case") == name; }) == 1,
                    "Missing or duplicate physical loop-location case");
                const auto unbound = run(c, false, *first), bound = run(c, true, *first);
                check(unbound.trace == bound.trace && unbound.writes == bound.writes &&
                      unbound.retained == bound.retained && unbound.exit == bound.exit,
                      "Passive observer changed source execution or retained banks");
                if (entry == 0x17e160u) ++mode3; else ++mode2;
                result["case_results"].push_back({{"case", name}, {"entry", entry}, {"passed", true},
                    {"comparisons", 2}, {"instructions", unbound.trace.size() / 4},
                    {"ordered_nonstack_write_bytes", unbound.writes.size()},
                    {"observer_entries", bound.entering}, {"observer_returns", bound.returning}});
                result["cases"] = result["case_results"].size();
                result["comparisons"] = result["case_results"].size() * 2;
                result["mode3_cases"] = mode3; result["mode2_cases"] = mode2;
            } catch (const std::exception &error) {
                result["case_results"].push_back({{"case", name}, {"passed", false}, {"error", error.what()}});
                throw std::runtime_error(name + ": " + error.what());
            }
        }
        check(mode3 == 6 && mode2 == 16, "Original loop-location mode coverage differs");
        result["passed"] = true; report(output, result);
        std::cout << "PASS original TU1.5 loop-location CPU owners: 22 cases, 44 bound/unbound comparisons; "
                     "complete physical traces, ordered nonstack writes and full hardware ABI except IF\n";
        return 0;
    } catch (const std::exception &error) {
        result["passed"] = false;
        result["error"] = error.what();
        try { report(output, result); } catch (const std::exception &failure) { std::cerr << failure.what() << '\n'; }
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
