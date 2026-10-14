#include "halo2/source_ai_actor_membership.hpp"
#include "halo2/source_unit_control.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
namespace {
using Json = nlohmann::json;
using RecordBytes = std::vector<std::uint8_t>;
void check(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}
RecordBytes decode(const std::string &hex) {
  check(hex.size()%2 == 0,"Incomplete actor membership bytes");
  RecordBytes bytes(hex.size()/2);
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return bytes;
}
RecordBytes apply(RecordBytes bytes, const Json &changes) {
  for (const auto &change : changes) {
    const auto offset = change.at(0).get<unsigned>();
    const auto value = decode(change.at(1).get<std::string>());
    check(offset <= bytes.size() && value.size() <= bytes.size()-offset,"Actor membership delta exceeds record");
    std::copy(value.begin(),value.end(),bytes.begin()+offset);
  }
  return bytes;
}
void write(std::span<std::uint8_t> bytes, unsigned offset, std::uint32_t value, unsigned width = 4) {
  check(offset+width <= bytes.size(),"Actor membership fixture store exceeds backing");
  for (unsigned i = 0; i < width; ++i) bytes[offset+i] = std::uint8_t(value >> (8*i));
}
void compare(const DatumPool &pool, const Json &state, const RecordBytes &baseline,
             const std::string &name) {
  check(pool.cursor() == state.at("cursor") && pool.high_water() == state.at("high_water") &&
        pool.count() == state.at("count") && pool.next_salt() == state.at("next_salt"),
        name+" metadata differs");
  const auto bitmap = state.at("bitmap").get<std::vector<std::uint32_t>>();
  check(std::equal(bitmap.begin(),bitmap.end(),pool.bitmap().begin()),name+" bitmap differs");
  for (unsigned i = 0; i < state.at("records").size(); ++i) {
    const auto expected = apply(baseline,state.at("records").at(i));
    const auto actual = pool.elements().subspan(i*pool.stride(),pool.stride());
    if (!std::equal(expected.begin(),expected.end(),actual.begin())) {
      const auto at = std::mismatch(expected.begin(),expected.end(),actual.begin()).first-expected.begin();
      throw std::runtime_error(name+" record="+std::to_string(i)+" byte="+std::to_string(at)+" differs");
    }
  }
}
}
int main(int argc, char **argv) try {
  check(argc == 2 || (argc == 3 && std::string(argv[2]) == "source-owned"),
        "Supply original actor membership fixture and optional source-owned mode");
  const bool source_owned = argc == 3;
  std::ifstream file(argv[1]);std::string line;
  while (std::getline(file,line) && line != "```json") {}
  check(bool(file),"Actor membership fixture has no JSON block");
  Json fixture;file >> fixture;
  check(fixture.at("passed").get<bool>(),"Original actor membership oracle failed");
  unsigned count = 0;
  for (const auto &row : fixture.at("cases")) {
    SourceRuntimeMemory memory;
    std::map<std::string,std::unique_ptr<DatumPool>> pools;
    const auto baseline = decode(row.at("actor_baseline_hex").get<std::string>());
    for (const auto &[name,spec] : fixture.at("pools").items()) {
      const auto capacity = spec.at("capacity").get<unsigned>(),stride = spec.at("stride").get<unsigned>();
      const auto &before = row.at("before").at(name);
      const auto records = spec.at("records").get<unsigned>();
      std::unique_ptr<DatumPool> pool;
      if (source_owned) {
        const auto address = 0x100000u+unsigned(pools.size())*0x100000u;
        RecordBytes header(0x4c),storage(capacity*stride),bits((capacity+31)/32*4);
        const auto label = spec.at("name").get<std::string>();
        std::copy(label.begin(),label.end(),header.begin());
        write(header,0x20,capacity);write(header,0x24,stride);write(header,0x29,1,1);
        write(header,0x34,before.at("cursor"));write(header,0x38,before.at("high_water"));
        write(header,0x3c,before.at("count"));write(header,0x40,before.at("next_salt"),2);
        write(header,0x44,address+0x1000);write(header,0x48,address+0xff000);
        for (unsigned i = 0; i < bits.size()/4; ++i) write(bits,i*4,before.at("bitmap").at(i));
        for (unsigned i = 0; i < records; ++i) {
          const auto bytes = apply(name == "actors" ? baseline : RecordBytes(stride,0xa5),before.at("records").at(i));
          std::copy(bytes.begin(),bytes.end(),storage.begin()+i*stride);
        }
        const auto mount = [&](std::uint32_t at, const RecordBytes &bytes, const char *part) {
          memory.mount(at,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,name+part);
        };
        mount(address,header," header");mount(address+0x1000,storage," records");mount(address+0xff000,bits," bits");
        pool = std::make_unique<DatumPool>(memory,address);
      } else {
        pool = std::make_unique<DatumPool>(spec.at("name").get<std::string>(),capacity,stride,0);
        for (unsigned i = 0; i < records; ++i) pool->allocate();
        for (unsigned i = 0; i < records; ++i) {
          const auto bytes = apply(name == "actors" ? baseline : RecordBytes(stride,0xa5),before.at("records").at(i));
          std::copy(bytes.begin(),bytes.end(),pool->element_storage().begin()+i*stride);
        }
      }
      compare(*pool,before,name == "actors" ? baseline : RecordBytes(stride,0xa5),name);
      pools.emplace(name,std::move(pool));
    }
    auto &actors = *pools.at("actors");
    std::unique_ptr<SourceAiActorBank> bank;
    if (source_owned) {
      if (count%2) bank = std::make_unique<SourceAiActorBank>(memory,actors.address());
      else bank = std::make_unique<SourceAiActorBank>(memory,0x100000u);
      check(bank->pool().source_memory() == &memory,"Actor bank copied its actual source owner");
    }
    std::vector<RecordBytes> bodies;
    for (unsigned i = 0; i < fixture.at("body_sizes").size(); ++i)
      bodies.push_back(apply(RecordBytes(fixture.at("body_sizes").at(i).get<unsigned>(),0xa5),row.at("before").at("objects").at(i)));
    auto globals = decode(row.at("before").at("globals").get<std::string>());
    SourceUnitControl objects({
      [&](std::uint32_t id) -> std::span<std::uint8_t> {
        const auto index = id&0xffffu;
        check(index < bodies.size() && id == 0xe26f0000u+index,"Foreign actor membership object datum");
        return bodies[index];
      },
      [](std::uint32_t) -> std::uint8_t { throw std::runtime_error("Unrequired object-type owner entered"); },
      [](std::uint32_t) -> const NativeUnitInteractionDefinition & { throw std::runtime_error("Unrequired unit tag decoder entered"); },
      [](std::uint32_t) { throw std::runtime_error("Unrequired item-effect owner entered"); },
      [](std::uint32_t) { throw std::runtime_error("Unrequired driver owner entered"); }});
    SourceAiActorControlServices control{objects,*pools.at("perceptions"),*pools.at("clumps"),globals,
      [&] { return std::bit_cast<std::int32_t>(row.at("tick").get<std::uint32_t>()); }};
    SourceAiFollowReferenceRelease registry = [](std::uint32_t) {
      throw std::runtime_error("Membership fixture reached an unprovided nonempty registry owner");
    };
    SourceAiSlotLocationNotify slot_notify = [](std::uint32_t,std::uint32_t,
        std::span<std::uint8_t>,bool,DatumPool &) {
      throw std::runtime_error("Membership fixture reached an unprovided slot owner");
    };
    SourceAiActorLocationReferenceServices references{source_retail_ai_actor_slot_callbacks(),
      source_retail_ai_actor_constructor_constants().empty_reference,registry,slot_notify};
    SourceAiActorMembershipServices services{control,*pools.at("squads"),references,
      row.at("bsp").get<std::int16_t>()};
    const auto id = row.at("actor").get<std::uint32_t>();
    const auto squad = row.at("squad").get<std::int16_t>();
    const auto keep = row.at("keep").get<bool>();
    const auto mode = row.at("mode").get<std::string>();
    try {
      if (mode == "join") {
        if (bank) bank->join_squad(id,squad,keep,services);
        else SourceAiActorMembership::join_squad(actors,id,squad,keep,services);
      } else if (mode == "activate_squad") {
        const auto result = bank ? bank->activate_squad(squad,services) :
            SourceAiActorMembership::activate_squad(actors,squad,services);
        check(result == row.at("result").get<bool>(),"Original squad activation result differs");
      } else if (mode == "add_global") {
        if (bank) bank->add_to_global(id,services);
        else SourceAiActorMembership::add_to_global(actors,id,services);
      } else if (mode == "remove_squad") {
        if (bank) bank->remove_from_squad(id,keep,services);
        else SourceAiActorMembership::remove_from_squad(actors,id,keep,services.squads,globals);
      } else if (mode == "remove_global") {
        if (bank) bank->remove_from_global(id,services);
        else SourceAiActorMembership::remove_from_global(actors,id,globals);
      } else {
        check(mode == "clear_references","Unknown original membership mode");
        const auto conditional = row.at("conditional").get<bool>();
        if (bank) bank->reset_location_references(id,conditional,references);
        else SourceAiActorLifecycle::reset_location_references(actors,id,conditional,references);
      }
      const auto &state = row.at("after");
      for (const auto &[name,pool] : pools)
        compare(*pool,state.at(name),name == "actors" ? baseline : RecordBytes(pool->stride(),0xa5),name);
      check(globals == decode(state.at("globals").get<std::string>()),"Membership AI globals differ");
      for (unsigned i = 0; i < bodies.size(); ++i)
        check(bodies[i] == apply(RecordBytes(bodies[i].size(),0xa5),state.at("objects").at(i)),"Membership object bytes differ");
      if (bank)
        check(std::equal(bank->record(id).begin(),bank->record(id).end(),actors.lookup(id).begin()),
              "Membership actor API diverged from its shared source pool");
    } catch (const std::exception &error) {
      throw std::runtime_error("Membership case="+std::to_string(count)+" mode="+mode+": "+error.what());
    }
    ++count;
  }
  std::cout << "PASS " << count << " whole original squad/global membership and reference-clear cases"
            << (source_owned ? " through shared source-backed actor bank APIs" : " in owned banks") << '\n';
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n';return 1; }
