#include "halo2/object_schedule.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value, const std::string &context) {
  if (!value) throw std::runtime_error("Original object schedule differs: " + context);
}
std::uint32_t number(const std::string &value) { return std::stoul(value, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &text) {
  check(text.size() % 2 == 0, "hex extent");
  std::vector<std::byte> result;
  for (std::size_t i = 0; i < text.size(); i += 2)
    result.push_back(std::byte(number(text.substr(i, 2))));
  return result;
}
struct State {
  std::vector<std::byte> pool, headers, bitmap, queue, context;
  std::map<std::uint32_t, std::vector<std::byte>> bodies;
  friend bool operator==(const State &, const State &) = default;
};
struct Callback {
  std::uint32_t entry;
  std::array<std::uint32_t, 2> arguments;
  unsigned before, after;
};
struct Phase {
  std::string label;
  std::uint32_t entry;
  unsigned before, after;
  std::vector<Callback> callbacks;
  std::vector<halo2::SourceObjectScheduleStore> stores;
};
void replace(State &target, const State &source, const std::string &label) {
  auto copy = [&](auto &to, const auto &from) {
    check(to.size() == from.size(), label + " explicit leaf storage extent");
    std::copy(from.begin(), from.end(), to.begin());
  };
  copy(target.pool, source.pool); copy(target.headers, source.headers);
  copy(target.bitmap, source.bitmap); copy(target.queue, source.queue);
  copy(target.context, source.context);
  check(target.bodies.size() == source.bodies.size(), label + " explicit leaf body roster");
  for (auto &[datum, body] : target.bodies) {
    const auto found = source.bodies.find(datum);
    check(found != source.bodies.end(), label + " explicit leaf full body identity");
    copy(body, found->second);
  }
}
void run(const Phase &phase, const std::map<unsigned, State> &states,
         unsigned &callback_count, unsigned &store_count) {
  auto state = states.at(phase.before);
  const halo2::SourceObjectScheduleStorage storage{
      state.pool, state.headers, state.bitmap, state.queue, state.context};
  std::size_t cursor = 0;
  auto callback = [&](std::uint32_t entry,
                      std::array<std::uint32_t, 2> arguments) {
    check(cursor < phase.callbacks.size(), phase.label + " extra leaf");
    const auto &expected = phase.callbacks[cursor++];
    check(entry == expected.entry && arguments == expected.arguments,
          phase.label + " full leaf arguments/order");
    check(state == states.at(expected.before), phase.label + " full state at leaf entry");
    // The literal original outputs remain explicit leaf boundaries. They do
    // not implement the ordinary/dirty producer, cleanup or Havok world step.
    replace(state, states.at(expected.after), phase.label); ++callback_count;
  };
  constexpr std::uint32_t none = 0xffffffffu;
  halo2::SourceObjectScheduleServices services;
  services.object_body = [&](std::uint32_t datum) -> std::span<const std::byte> {
    check(state.bodies.contains(datum), phase.label + " full body lookup identity");
    return state.bodies.at(datum);
  };
  services.ordinary_update = [&](auto datum) { callback(0xbd360, {datum, none}); };
  services.dirty_update = [&](auto datum) { callback(0xbd710, {datum, none}); };
  services.queue_dirty_update = [&](auto datum) { callback(0x10d710, {datum, none}); };
  services.finish_queued_dirty_update = [&](auto datum, auto index) {
    callback(0x10d7f0, {datum, std::uint32_t(index)});
  };
  services.finish_dirty_updates = [&] { callback(0x1019f0, {none, none}); };
  services.finish_new_updates = [&] { callback(0xc0270, {none, none}); };
  services.pending_delete_first = [&](auto datum, auto mode) { callback(0xb92a0, {datum, mode}); };
  services.pending_delete_second = [&](auto datum, auto mode) { callback(0xb9350, {datum, mode}); };
  std::vector<halo2::SourceObjectScheduleStore> stores;
  const halo2::SourceObjectScheduleObserver observe = [&](const auto &store) { stores.push_back(store); };
  if (phase.entry == 0xb7e00) halo2::source_ordinary_object_schedule(storage, services, observe);
  else if (phase.entry == 0xb7f50) halo2::source_dirty_object_schedule(storage, services, observe);
  else {
    check(phase.entry == 0xb8040, phase.label + " measured dispatcher entry");
    halo2::source_new_object_schedule(storage, services, observe);
  }
  check(cursor == phase.callbacks.size(), phase.label + " complete leaf consumption");
  check(state == states.at(phase.after), phase.label + " complete final state");
  check(stores == phase.stores, phase.label + " ordered owning stores, including unchanged writes");
  store_count += unsigned(stores.size());
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original object-schedule fixture export");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream input(argv[1]); std::map<unsigned, State> states;
    std::vector<Phase> phases;
    unsigned expected_callbacks = 0, expected_stores = 0, expected_phases = 0;
    for (std::string line; std::getline(input, line);) {
      std::istringstream stream(line); std::vector<std::string> row;
      for (std::string part; stream >> part;) row.push_back(part);
      if (row.empty()) continue;
      if (row[0] == "Q") {
        check(row.size() == 7 && !states.contains(number(row[1])), "state row extent/identity");
        states.emplace(number(row[1]), State{bytes(row[2]), bytes(row[3]), bytes(row[4]),
            bytes(row[5]), bytes(row[6]), {}});
      } else if (row[0] == "B") {
        check(row.size() == 4, "body row extent");
        auto &bodies = states.at(number(row[1])).bodies;
        check(bodies.emplace(number(row[2]), bytes(row[3])).second, "unique full body identity");
      } else if (row[0] == "P") {
        check(row.size() == 5, "phase row extent");
        phases.push_back({row[1], number(row[2]), number(row[3]), number(row[4]), {}, {}});
      } else if (row[0] == "K") {
        check(row.size() == 6 && !phases.empty(), "leaf row extent");
        phases.back().callbacks.push_back({number(row[1]), {number(row[2]), number(row[3])},
            number(row[4]), number(row[5])});
      } else if (row[0] == "W") {
        check(row.size() == 6 && !phases.empty() && (row[2] == "H" || row[2] == "C"),
              "owning store row extent/area");
        phases.back().stores.push_back({number(row[1]), row[2] == "H" ?
            halo2::SourceObjectScheduleArea::header : halo2::SourceObjectScheduleArea::update_context,
            number(row[3]), std::uint8_t(number(row[4])), std::uint8_t(number(row[5]))});
      } else if (row[0] == "N") {
        check(row.size() == 4, "recorded suite extent");
        expected_phases = number(row[1]); expected_callbacks = number(row[2]); expected_stores = number(row[3]);
      } else throw std::runtime_error("Unknown object-schedule fixture row");
    }
    check(expected_phases == 3 && phases.size() == expected_phases,
          "complete three-dispatcher original suite");
    unsigned callbacks = 0, stores = 0;
    for (const auto &phase : phases) run(phase, states, callbacks, stores);
    check(callbacks == expected_callbacks && stores == expected_stores,
          "complete original callback/store suite");
    std::cout << phases.size() << " original object dispatchers matched complete header/bitmap/queue/context/body state; "
              << callbacks << " explicit literal-output leaves and " << stores << " ordered owning stores\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
