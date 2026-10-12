#include "halo2/source_main_heap_storage.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/physics_body_state.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/object_preparation_pressure.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using J = nlohmann::json;
void check(bool ok, const std::string &why) {
  if (!ok) throw std::runtime_error(why);
}
std::vector<std::byte> bytes(const std::string &text) {
  check(!(text.size() & 1u), "Invalid heap fixture bytes");
  std::vector<std::byte> out;
  for (std::size_t i = 0; i < text.size(); i += 2)
    out.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
  return out;
}
std::vector<std::byte> file(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary | std::ios::ate);
  check(bool(in), "Missing original retained heap bytes: " + p.string());
  const auto n = in.tellg();
  check(n >= 0, "Invalid original heap file extent");
  std::vector<std::byte> out(static_cast<std::size_t>(n));
  in.seekg(0);
  check(bool(in.read(reinterpret_cast<char *>(out.data()), out.size())),
        "Truncated original retained heap bytes");
  return out;
}
std::uint32_t word(const halo2::SourceRuntimeMemory &m, std::uint32_t p) {
  const auto b = m.view(p, 4);
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8*i);
  return v;
}
void put(halo2::SourceRuntimeMemory &m, std::uint32_t p, std::uint32_t v) {
  std::array<std::byte, 4> b;
  for (unsigned i = 0; i < 4; ++i) b[i] = std::byte(v >> (8*i));
  m.write(p, b);
}
void same(std::span<const std::byte> a, std::span<const std::byte> b,
          const std::string &why) {
  check(a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin()), why);
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 4, "Supply original main-heap fixture, TU1.1 image and original hardware pressure fixture");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]) ||
        !std::filesystem::exists(argv[3]))
      return 77;
#if !defined(__linux__)
    return 77;
#endif
    std::ifstream input(argv[1]);
    const auto fixture = J::parse(input);
    std::ifstream hardware_input(argv[3]);
    const auto hardware = J::parse(hardware_input);
    const auto directory = std::filesystem::path(argv[1]).parent_path();
    const auto raw = fixture.at("raw_address").get<std::uint32_t>();
    const auto global = fixture.at("global_address").get<std::uint32_t>();
    std::size_t steps = 0, stores_count = 0, retained_bytes = 0;
    unsigned exhaustion = 0;
    std::size_t hardware_index = 0, sequence_index = 0;
    for (const auto &sequence : fixture.at("sequences")) {
      const auto name = sequence.at("name").get<std::string>();
      auto m = halo2::load_source_runtime_image_1_1(argv[2]);
      same(m.view(global, 0x34), bytes(sequence.at("initial_globals")),
           name + " image global bytes differ");
      const auto stack = fixture.at("stack_address").get<std::uint32_t>();
      m.mount(stack, std::vector<std::byte>(fixture.at("stack_bytes").get<std::size_t>()),
              halo2::SourceRuntimeMemory::Access::ReadWrite,
              "explicit standalone experiment stack controls");
      halo2::SourceRuntimeHostStorage host(m, raw);
      halo2::SourceStaticAllocationServices platform;
      unsigned allocations = 0;
      platform.allocate = [&](auto request) {
        check(allocations < sequence.at("resources").size(),
              name + " unexpected resource request");
        const auto &q = sequence.at("resources")[allocations++];
        check(std::array{request.base, request.zero_bits, request.bytes,
                         request.allocation_type, request.protection} ==
                  q.at("request").template get<std::array<std::uint32_t, 5>>(),
              name + " original kernel arguments differ");
        const auto result = host.allocate(request);
        check(std::array{result.status, result.base, result.bytes} ==
                  q.at("result").template get<std::array<std::uint32_t, 3>>(),
              name + " actual host storage adaptation differs");
        same(m.view(raw, result.bytes), file(directory / q.at("initial_storage").get<std::string>()),
             name + " actual host initial raw pages differ");
        return result;
      };
      halo2::SourceObjectPreparationPressureServices pressure;
      pressure.read = [&](auto p, auto n) { return m.view(p, n); };
      const halo2::SourceHeapPressureState state{0x480da0, 0x480da4, 0x480da8};
      const halo2::SourceObjectPressureOperands operands{
          word(m, 0x4650a4), word(m, 0x451ff8), word(m, 0x4650a0),
          word(m, 0x46509c), word(m, 0x465098), word(m, 0x465094)};
      std::size_t step_index = 0;
      for (const auto &row : sequence.at("steps")) {
        const auto kind = row.at("kind").get<std::string>();
        const auto label = name + " step " + std::to_string(steps) + " " + kind;
        std::vector<halo2::SourcePhysicsContactRecordStore> stores;
        const halo2::SourcePhysicsContactRecordObserver observe = [&](const auto &s) {
          if (s.address >= stack && s.address < stack + 0x10000) return;
          stores.push_back(s);
        };
        std::uint32_t result = 0;
        if (kind == "raw") {
          result = halo2::source_initialize_main_heap_storage_1_1(m, platform, observe);
        } else if (kind == "construct") {
          halo2::SourcePhysicsWorldRuntimeAbi abi{};
          abi.gp = row.at("entry_abi").at("gp").get<decltype(abi.gp)>();
          abi.eflags = row.at("entry_abi").at("eflags");
          abi.floating_point = halo2::SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
          put(m, abi.gp[4], fixture.at("stop"));
          halo2::source_run_physics_world_runtime(m, 0x148190, abi,
              halo2::SourcePhysicsContactImage::MapPack11, {0x27f, 0x1f80}, {}, observe);
          result = abi.gp[0];
          check(abi.gp == row.at("exit_abi").at("gp").get<decltype(abi.gp)>(),
                label + " constructor GP differs");
          check((abi.eflags & 0xcd5u) ==
                    (row.at("exit_abi").at("eflags").get<std::uint32_t>() & 0xcd5u),
                label + " constructor defined flags differ");
          check(abi.floating_point->mxcsr == row.at("exit_abi").at("mxcsr"),
                label + " constructor SSE status differs");
        } else if (kind == "allocate") {
          result = halo2::source_allocate_physics_body_small_block_1_1(
              m, word(m, 0x480da0), row.at("size"), observe);
          if (!result) ++exhaustion;
        } else {
          check(kind == "free", label + " unknown operation");
          halo2::source_free_physics_body_small_block_1_1(
              m, word(m, 0x480da0), row.at("block"), row.at("size"), observe);
        }
        if (kind != "free")
          check(result == row.at("exit_abi").at("gp")[0], label + " EAX differs");
        const auto &expected = row.at("stores");
        check(stores.size() == expected.size(), label + " store count differs");
        for (std::size_t i = 0; i < stores.size(); ++i) {
          const auto &s = stores[i];
          const auto &e = expected[i];
          check(s.instruction == e.at("instruction") && s.address == e.at("address") &&
                    s.before == bytes(e.at("before")) && s.written == bytes(e.at("written")),
                label + " ordered store differs at " + std::to_string(i));
        }
        const auto expected_raw = file(directory / row.at("raw_storage").get<std::string>());
        same(m.view(raw, expected_raw.size()), expected_raw, label + " complete raw bytes differ");
        same(m.view(global, 0x34), bytes(row.at("globals")), label + " complete globals differ");
        if (kind != "raw") {
          check(hardware_index < hardware.at("rows").size(), "Missing original CPU pressure row");
          const auto &hw = hardware.at("rows")[hardware_index++];
          check(hw.at("sequence") == sequence_index && hw.at("step") == step_index &&
                    hw.at("statistics_bits") == row.at("statistics_bits") &&
                    hw.at("pressure") == row.at("pressure"),
                label + " hardware/original scalar provenance differs");
          halo2::ObjectConstructionFloatingPointContext fp{0x1f80};
          const auto statistic = halo2::source_object_pressure_statistics(
              word(m, 0x480da0), operands, pressure, fp);
          check(statistic == row.at("statistics_bits"),
                label + " actual retained pressure statistic bits differ: " +
                std::to_string(statistic) + "/" + std::to_string(row.at("statistics_bits").get<std::uint32_t>()));
          check(fp.mxcsr == hw.at("statistics_mxcsr"),
                label + " statistic SSE status differs: " + std::to_string(fp.mxcsr) +
                "/" + std::to_string(hw.at("statistics_mxcsr").get<std::uint32_t>()));
          fp.mxcsr = 0x1f80;
          check(halo2::source_object_heap_pressure(state, operands, pressure, fp) ==
                    row.at("pressure") && fp.mxcsr == hw.at("pressure_mxcsr"),
                label + " actual retained heap pressure differs");
        }
        ++steps; stores_count += stores.size(); retained_bytes += expected_raw.size()+0x34;
        ++step_index;
      }
      check(allocations == 1, name + " unused resource captures");
      ++sequence_index;
    }
    // A real host token collision must leave the source global untouched. The
    // resource owner throws; the caller must not turn it into quiet pressure.
    auto blocked = halo2::load_source_runtime_image_1_1(argv[2]);
    const auto before_span = blocked.view(global, 0x34);
    const std::vector before(before_span.begin(), before_span.end());
    halo2::SourceRuntimeHostStorage collision(blocked, 0x480000);
    halo2::SourceStaticAllocationServices absent;
    bool failed = false;
    try { halo2::source_initialize_main_heap_storage_1_1(blocked, absent); }
    catch (const std::runtime_error &e) {
      failed = std::string(e.what()) == "Original static virtual-memory owner unavailable";
    }
    check(failed, "Missing main raw allocator fabricated completion");
    absent.allocate = [&](auto request) { return collision.allocate(request); };
    failed = false;
    try { halo2::source_initialize_main_heap_storage_1_1(blocked, absent); }
    catch (const std::invalid_argument &e) {
      failed = std::string(e.what()) == "Original runtime regions overlap";
    }
    check(failed, "Host storage overlap fabricated completion");
    same(blocked.view(global, 0x34), before, "Unavailable raw owner altered source globals");
    check(steps == 326 && stores_count == 5254 && exhaustion == 3 &&
              hardware_index == hardware.at("rows").size() && hardware_index == 324,
          "Original complete heap operation coverage differs");
    std::cout << steps << " original heap steps, " << stores_count
              << " ordered stores and " << retained_bytes
              << " retained bytes match with actual host storage; "
              << exhaustion << " actual source page-exhaustion returns\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
