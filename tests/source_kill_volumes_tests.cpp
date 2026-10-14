#include "halo2/source_kill_volumes.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using Json = nlohmann::json;
namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &text) {
  std::vector<std::byte> result;
  for (std::size_t i = 0; i < text.size(); i += 2)
    result.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
  return result;
}
void run(const Json &fixture, bool source_backed) {
  for (const auto &row : fixture.at("cases")) {
    auto owned = bytes(row.at("initial"));
    halo2::SourceRuntimeMemory bank;
    if (source_backed)
      bank.mount(0x1000, owned, halo2::SourceRuntimeMemory::Access::ReadWrite, "actual kill-volume state");
    const auto state = source_backed ? bank.mutable_view(0x1000, 0x24) : std::span(owned);
    const auto mapping = row.at("trigger_kill_indices").get<std::vector<std::int16_t>>();
    const auto killers = row.at("kill_triggers").get<std::vector<std::int16_t>>();
    halo2::SourceKillVolumeView owner(state, mapping, killers);
    for (const auto &op : row.at("operations")) {
      check(std::ranges::equal(state, bytes(op.at("before"))), "Initial original state differs");
      Json calls = Json::array();
      const auto contains = [&](std::uint32_t object, std::int16_t trigger) {
        calls.push_back(Json{{"kind", "contains"}, {"object", object}, {"volume", trigger}});
        return op.at("containment").get<bool>();
      };
      const auto name = op.at("name").get<std::string>();
      if (name == "new_map") owner.initialize_map();
      else if (name == "dispose_map") owner.dispose_map();
      else if (name == "query") {
        const bool actual = owner.test_object(op.at("object"), op.at("tick"), contains);
        check(actual == op.at("result").get<int>(), "Original scheduled query result differs");
        check(calls == op.at("calls"), "Original containment calls differ");
      } else {
        check(name == "enable" || name == "disable", "Unknown original operation");
        // The interpreter delivers arguments only after its actual admission.
        if (op.at("arguments_ready").get<bool>())
          owner.set_enabled(op.at("trigger"), name == "enable");
      }
      check(std::ranges::equal(state, bytes(op.at("after"))), "Complete original state differs");
    }
    // Whole-bank checkpoint publication must rebind the view, not retain it.
    if (source_backed) {
      const auto saved = bank.checkpoint();
      halo2::SourceRuntimeMemory restored;
      restored.restore(saved);
      halo2::SourceKillVolumeView rebound(restored.mutable_view(0x1000, 0x24), mapping, killers);
      rebound.initialize_storage();
      auto expected = bytes(row.at("operations").back().at("after"));
      expected[0x20] = std::byte{};
      check(std::ranges::equal(restored.view(0x1000, 0x24), expected), "Rebound storage initialization differs");
      check(bank.checkpoint() == saved, "Rebound state changed the prior bank");
    }
  }
}
void required_owners() {
  std::array<std::byte, 0x24> state{};
  const std::array<std::int16_t, 1> mapping{0}, killers{0}, absent{-1};
  halo2::SourceKillVolumeView owner(state, mapping, killers);
  owner.initialize_map();
  bool rejected = false;
  try { (void)owner.test_object(0x12340000, 0, {}); }
  catch (const std::invalid_argument &) { rejected = true; }
  check(rejected, "Missing containment owner was replaced by a result");
  owner.set_enabled(0, false);
  check(!owner.test_object(0x12340000, 0, {}), "Disabled volume called containment");
  owner.initialize_map();
  halo2::SourceKillVolumeView no_volume(state, mapping, absent);
  check(!no_volume.test_object(0x12340000, 0, {}), "NONE volume called containment");
  const auto before = state;
  rejected = false;
  try { owner.set_enabled(1, true); }
  catch (const std::invalid_argument &) { rejected = true; }
  check(rejected && state == before, "Invalid trigger changed actual state");
}
}
int main(int argc, char **argv) try {
  check(argc == 2, "Expected original kill-volume fixture");
  std::ifstream file(argv[1]); check(bool(file), "Original fixture unavailable");
  const auto fixture = Json::parse(file);
  check(fixture.at("base_tu15_cases_equal"), "Original images disagree");
  check(fixture.at("original_instruction_patches").empty(), "Original instructions were patched");
  check(fixture.at("cases").size() == 256, "Incomplete original case set");
  run(fixture, false); run(fixture, true); required_owners();
  std::cout << "PASS 256 base/TU1.5 state/scheduling cases in owned and source-backed views, including complete bytes, required callee calls and checkpoint rebinding\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n'; return 1;
}
