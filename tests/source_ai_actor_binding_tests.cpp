#include "halo2/source_ai_actor.hpp"
#include "halo2/source_ai_actor_lifecycle.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_unit_control.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool result, const char *message) {
  if (!result)
    throw std::runtime_error(message);
}
std::vector<std::uint8_t> decode(const std::string &hex) {
  check(hex.size()%2 == 0, "Actor binding fixture has incomplete bytes");
  std::vector<std::uint8_t> bytes(hex.size()/2);
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = std::uint8_t(std::stoul(hex.substr(i*2, 2), nullptr, 16));
  return bytes;
}
std::vector<std::uint8_t> apply(std::vector<std::uint8_t> bytes,
                               const nlohmann::json &deltas) {
  for (const auto &delta : deltas) {
    const auto offset = delta.at(0).get<unsigned>();
    const auto data = decode(delta.at(1).get<std::string>());
    check(offset <= bytes.size() && data.size() <= bytes.size()-offset,
          "Actor binding delta exceeds its actual body");
    std::copy(data.begin(), data.end(), bytes.begin()+offset);
  }
  return bytes;
}
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i)
    value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
void write(std::vector<std::uint8_t> &bytes, unsigned offset, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
struct ReleaseBoundary {};
}
int main(int argc, char **argv) try {
  check(argc == 2 || (argc == 3 && std::string(argv[2]) == "source-owned"),
        "Supply the retained actor unit-binding fixture and optional source-owned mode");
  const bool source_owned = argc == 3;
  std::ifstream file(argv[1]);
  std::string line;
  bool found = false;
  while (std::getline(file, line))
    if (line == "```json") { found = true; break; }
  check(found, "Actor binding fixture has no JSON block");
  nlohmann::json fixture;
  file >> fixture;
  check(fixture.at("passed").get<bool>(), "Original actor binding oracle failed");
  const auto baseline = decode(fixture.at("actor_baseline_hex").get<std::string>());
  const auto sizes = fixture.at("record_sizes").get<std::vector<unsigned>>();
  const auto pattern = fixture.at("baseline_byte").get<std::uint8_t>();
  const auto full_lifecycle = fixture.value("full_lifecycle",false);
  unsigned cases = 0, completed = 0, boundaries = 0;
  for (const auto &row : fixture.at("cases")) {
    SourceRuntimeMemory memory;
    std::unique_ptr<SourceAiActorBank> bank_owner;
    if (source_owned) {
      // Construct one actual source bank, then borrow it through the engine
      // API. Allocations, links and retirement stay in this same backing.
      std::vector<std::uint8_t> header(0x4c),records(256*0x888),bitmap(8*4);
      const std::string label = "actor";
      std::copy(label.begin(),label.end(),header.begin());
      write(header,0x20,256);write(header,0x24,0x888);header[0x29]=1;
      write(header,0x44,0x101000);write(header,0x48,0x1ff000);
      const auto mount = [&](std::uint32_t at, const auto &bytes, const char *part) {
        memory.mount(at,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,part);
      };
      mount(0x100000,header,"actor header");mount(0x101000,records,"actor records");mount(0x1ff000,bitmap,"actor bits");
      DatumPool actual_owner(memory,0x100000u);
      actual_owner.reset();
      bank_owner = std::make_unique<SourceAiActorBank>(memory,actual_owner.address());
      check(bank_owner->pool().source_memory() == &memory,"Binding bank copied its actual source owner");
    } else bank_owner = std::make_unique<SourceAiActorBank>();
    auto &bank = *bank_owner;
    std::uint16_t counter = 0;
    const auto constants = source_retail_ai_actor_constructor_constants();
    const SourceAiActorDefinition definition{0xe1000000u, 0, 1};
    const auto actor = bank.allocate(definition, constants, {417,30,0}, counter);
    const auto other = bank.allocate(definition, constants, {417,30,0}, counter);
    check(actor == 0xe3610000u && other == 0xe3620001u,
          "Actor unit binding lost its actual actor-pool salt");
    std::vector<std::vector<std::uint8_t>> before;
    for (unsigned i = 0; i < sizes.size(); ++i)
      before.push_back(apply(std::vector<std::uint8_t>(sizes[i], pattern), row.at("before").at(i)));
    auto bodies = before;
    nlohmann::json events = nlohmann::json::array();
    NativeUnitInteractionDefinition unit_definition;
    SourceUnitControl units({
      [&](std::uint32_t id) -> std::span<std::uint8_t> {
        const auto slot = id&0xffffu;
        check(slot < bodies.size() && id == 0xe26f0000u+slot,
              "Actor binding received a foreign unit/object datum");
        return bodies[slot];
      },
      [](std::uint32_t id) { return std::uint8_t((id&0xffffu) < 2 ? (id&1u) : 2); },
      [&](std::uint32_t id) -> const NativeUnitInteractionDefinition & {
        check(id == 0xe1000001u, "Actor binding lost its canonical unit definition");
        return unit_definition;
      },
      [](std::uint32_t) { throw std::runtime_error("Binding fixture reached an unprovided item-effect owner"); },
      [&](std::uint32_t id) {
        nlohmann::json owners = nlohmann::json::array();
        for (unsigned i = 0; i < 2; ++i)
          owners.push_back({read(bodies[i],0x12c),read(bodies[i],0x134),
                            read(bodies[i],0x248),read(bodies[i],0x24c)});
        events.push_back({{"kind","driver"},{"unit",id},
                          {"actor_unit",read(bank.record(actor),0x18)},
                          {"owners",owners}});
        // Actual original1BBDF0 returns here with the declared inactive AI.
      }});
    DatumPool squads("squad",335,0x98,0), joints("joint state",20,0xbc,0),
      props("prop",256,0xc4,0), references("prop_ref",1024,0x3c,0),
      tracking("tracking",100,0x124,0), clumps("clump",20,0x50,0),
      locations("dynamic firing points",15,0x484,0), perceptions("swarm",32,0x34,0),
      commands("command scripts",40,0xd4,0), joint_commands("joint command scripts",10,0x8c,0),
      script_threads("hs thread",320,0x418,0);
    std::vector<std::uint8_t> globals(0x400), statistics(0x30);
    SourceAiActorLifecycleServices lifecycle{units,squads,joints,props,references,tracking,
      clumps,locations,perceptions,commands,joint_commands,script_threads,
      globals,statistics,source_retail_ai_actor_slot_callbacks(),
      constants.empty_reference,[] { return 417; },
      [&](std::uint32_t id) -> std::optional<SourceAiActorDefinition> {
        check(id == definition.datum,"Binding retirement lost its canonical character");
        return definition;
      }};
    SourceAiActorBindingServices services{units,
      [&](std::uint32_t id, bool argument) {
        events.push_back({{"kind","release"},{"actor",id},{"argument",unsigned(argument)}});
        if (full_lifecycle) {
          bank.release_actor(id,argument,lifecycle);
          return;
        }
        throw ReleaseBoundary{}; // Required owner is unavailable; never succeed.
      }};
    if (full_lifecycle) {
      const auto expected_other = apply(baseline,row.at("other_before"));
      const auto other_unit = read(expected_other,0x18);
      if (other_unit != SourceAiActorBank::none) {
        for (unsigned i = 0; i < 2; ++i) {
          write(bodies[i],0x12c,SourceAiActorBank::none);
          write(bodies[i],0x248,SourceAiActorBank::none);
        }
        bank.bind_unit(other,other_unit,services);
      }
      check(std::equal(expected_other.begin(),expected_other.end(),bank.record(other).begin()),
            "Other binding actor differs from original cold constructor/link");
      bodies = before;
      events.clear();
    }
    const auto expected_before = apply(baseline, row.at("actor_before"));
    const auto prior_unit = read(expected_before, 0x18);
    if (prior_unit != SourceAiActorBank::none) {
      // Establish the actor's prior link through its actual native API; then
      // supply the fixture's independently declared object-owner state.
      for (unsigned i = 0; i < 2; ++i) {
        write(bodies[i],0x12c,SourceAiActorBank::none);
        write(bodies[i],0x248,SourceAiActorBank::none);
      }
      bank.bind_unit(actor, prior_unit, services);
      bodies = before;
      events.clear();
    }
    check(std::equal(expected_before.begin(),expected_before.end(),bank.record(actor).begin()),
          "Actor binding setup differs from the original cold constructor/link");
    bool complete = true;
    try {
      if (row.at("mode").get<std::string>() == "bind")
        bank.bind_unit(actor,0xe26f0000u,services);
      else
        bank.unbind_unit(actor,services);
    } catch (const ReleaseBoundary &) { complete = false; }
    if (complete != row.at("complete").get<bool>() || events != row.at("events")) {
      std::cerr << "Actor binding case=" << cases << " complete=" << complete
                << " events=" << events.dump() << " original=" << row.at("events").dump() << '\n';
      throw std::runtime_error("Native actor binding owner calls differ from the original");
    }
    const auto expected_actor = apply(baseline,row.at("actor_after"));
    check(std::equal(expected_actor.begin(),expected_actor.end(),bank.record(actor).begin()),
          "Native actor binding bytes differ from original1E0D50/1E1250");
    for (unsigned i = 0; i < sizes.size(); ++i)
      check(bodies[i] == apply(std::vector<std::uint8_t>(sizes[i], pattern),row.at("after").at(i)),
            "Native unit/item binding bytes differ from the original");
    if (full_lifecycle) {
      const auto expected_other = apply(baseline,row.at("other_after"));
      check(std::equal(expected_other.begin(),expected_other.end(),
                       bank.pool().elements().begin()+0x888),
            "Binding retirement actor bytes differ from original1E1A00");
      const auto &state = row.at("pool_after");
      const auto bitmap = state.at("bitmap").get<std::vector<std::uint32_t>>();
      check(bank.pool().cursor() == state.at("cursor") &&
            bank.pool().high_water() == state.at("high_water") &&
            bank.pool().count() == state.at("count") &&
            bank.pool().next_salt() == state.at("next_salt") &&
            std::equal(bitmap.begin(),bitmap.end(),bank.pool().bitmap().begin()),
            "Binding retirement allocation metadata differs from original");
    } else
      check(bank.pool().count() == 2,"Binding retired storage without its lifecycle owner");
    check(counter == 0,"Binding unexpectedly changed the flag3 constructor counter");
    ++cases;
    complete ? ++completed : ++boundaries;
  }
  std::cout << "PASS " << completed << " complete original actor binding/unbinding cases"
            << (full_lifecycle ? " through cold retirement" : "") << " and "
            << boundaries << " required-release boundaries (no supplied success)"
            << (source_owned ? " in the actual source-backed actor bank" : "") << '\n';
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
