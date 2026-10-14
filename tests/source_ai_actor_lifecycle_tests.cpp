#include "halo2/source_ai_actor_lifecycle.hpp"
#include "halo2/source_unit_control.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <algorithm>
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
  check(hex.size()%2 == 0,"Incomplete lifecycle fixture bytes");
  RecordBytes bytes(hex.size()/2);
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return bytes;
}
RecordBytes apply(RecordBytes bytes, const Json &changes) {
  for (const auto &change : changes) {
    const auto offset = change.at(0).get<unsigned>();
    const auto value = decode(change.at(1).get<std::string>());
    check(offset <= bytes.size() && value.size() <= bytes.size()-offset,
          "Lifecycle delta exceeds actual record");
    std::copy(value.begin(),value.end(),bytes.begin()+offset);
  }
  return bytes;
}
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset,
                   unsigned width = 4) {
  check(offset+width <= bytes.size(),"Lifecycle test read exceeds body");
  std::uint32_t value = 0;
  for (unsigned i = 0; i < width; ++i) value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned width = 4) {
  check(offset+width <= bytes.size(),"Lifecycle test write exceeds body");
  for (unsigned i = 0; i < width; ++i) bytes[offset+i] = std::uint8_t(value >> (8*i));
}
void compare(const DatumPool &pool, const Json &state, const RecordBytes &baseline,
             const std::string &name) {
  check(pool.cursor() == state.at("cursor") && pool.high_water() == state.at("high_water") &&
        pool.count() == state.at("count") && pool.next_salt() == state.at("next_salt"),
        name+" allocation metadata differs from original");
  const auto bits = state.at("bitmap").get<std::vector<std::uint32_t>>();
  check(std::equal(bits.begin(),bits.end(),pool.bitmap().begin()),name+" occupancy differs");
  for (unsigned i = 0; i < state.at("records").size(); ++i) {
    const auto expected = apply(baseline,state.at("records").at(i));
    const auto actual = pool.elements().subspan(i*pool.stride(),pool.stride());
    if (!std::equal(expected.begin(),expected.end(),actual.begin())) {
      const auto at = std::mismatch(expected.begin(),expected.end(),actual.begin()).first-expected.begin();
      throw std::runtime_error(name+" record="+std::to_string(i)+" byte="+std::to_string(at)+
                               " differs from original");
    }
  }
}
}
int main(int argc, char **argv) try {
  check(argc == 2 || (argc == 3 && std::string(argv[2]) == "source-owned"),
        "Supply the original lifecycle fixture and optional source-owned mode");
  const bool source_owned = argc == 3;
  std::ifstream file(argv[1]);
  std::string line;
  while (std::getline(file,line) && line != "```json") {}
  check(bool(file),"Lifecycle fixture has no JSON block");
  Json fixture; file >> fixture;
  check(fixture.at("passed").get<bool>(),"Original lifecycle oracle failed");
  const auto &slots = source_retail_ai_actor_slot_callbacks();
  for (unsigned i = 0; i < slots.size(); ++i)
    check(Json::array({slots[i].handler,slots[i].stop,slots[i].release,slots[i].forget,
                      slots[i].notify_location}) ==
          fixture.at("slot_callbacks").at(i),"Original slot-handler table differs");
  unsigned count = 0;
  for (const auto &row : fixture.at("cases")) {
    SourceRuntimeMemory source_memory;
    std::map<std::string,std::unique_ptr<DatumPool>> pools;
    const auto baseline = decode(row.at("actor_baseline_hex").get<std::string>());
    for (const auto &[name,spec] : fixture.at("pools").items()) {
      const auto capacity = spec.at("capacity").get<unsigned>();
      const auto stride = spec.at("stride").get<unsigned>();
      std::unique_ptr<DatumPool> pool;
      const auto &before = row.at("before").at(name);
      const auto records = spec.at("records").get<unsigned>();
      if (source_owned) {
        // Import declared original bank state once into its real memory owner.
        // The DatumPool is a view: no live copied record/bitmap authority.
        const auto address = 0x100000u+unsigned(pools.size())*0x100000u;
        RecordBytes header(0x4c), storage(capacity*stride), bitmap((capacity+31)/32*4);
        const auto label = spec.at("name").get<std::string>();
        std::copy(label.begin(),label.end(),header.begin());
        write(header,0x20,capacity); write(header,0x24,stride); write(header,0x29,1,1);
        write(header,0x34,before.at("cursor")); write(header,0x38,before.at("high_water"));
        write(header,0x3c,before.at("count")); write(header,0x40,before.at("next_salt"),2);
        write(header,0x44,address+0x1000); write(header,0x48,address+0xff000);
        for (unsigned i = 0; i < bitmap.size()/4; ++i)
          write(bitmap,i*4,before.at("bitmap").at(i));
        for (unsigned i = 0; i < records; ++i) {
          const auto bytes = apply(name == "actors" ? baseline : RecordBytes(stride,0xa5),
                                   before.at("records").at(i));
          std::copy(bytes.begin(),bytes.end(),storage.begin()+i*stride);
        }
        const auto mount = [&](std::uint32_t at, const RecordBytes &bytes, const char *part) {
          source_memory.mount(at,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,
                              name+part);
        };
        mount(address,header," header"); mount(address+0x1000,storage," records");
        mount(address+0xff000,bitmap," bitmap");
        pool = std::make_unique<DatumPool>(source_memory,address);
      } else {
        pool = std::make_unique<DatumPool>(spec.at("name").get<std::string>(),capacity,stride,0);
        std::vector<std::uint32_t> ids;
        for (unsigned i = 0; i < records; ++i) ids.push_back(pool->allocate());
        for (unsigned i = 0; i < records; ++i)
          if (!(before.at("bitmap").at(i/32).get<std::uint32_t>() & (1u << (i%32))))
            pool->release_unchecked(ids[i]);
      }
      for (unsigned i = 0; i < records; ++i) {
        const auto bytes = apply(name == "actors" ? baseline : RecordBytes(pool->stride(),0xa5),
                                 before.at("records").at(i));
        std::copy(bytes.begin(),bytes.end(),pool->element_storage().begin()+i*pool->stride());
      }
      compare(*pool,before,name == "actors" ? baseline : RecordBytes(pool->stride(),0xa5),name);
      pools.emplace(name,std::move(pool));
    }
    auto &actors = *pools.at("actors");
    std::unique_ptr<SourceAiActorBank> bank;
    if (source_owned) {
      bank = std::make_unique<SourceAiActorBank>(source_memory,actors.address());
      check(bank->pool().source_memory() == &source_memory,"Lifecycle actor bank copied its actual owner");
    }
    const auto id = row.at("actor").get<std::uint32_t>();
    std::vector<RecordBytes> bodies;
    for (unsigned i = 0; i < fixture.at("body_sizes").size(); ++i)
      bodies.push_back(apply(RecordBytes(fixture.at("body_sizes").at(i).get<unsigned>(),0xa5),
                             row.at("before").at("objects").at(i)));
    auto globals = decode(row.at("before").at("globals").get<std::string>());
    auto statistics = decode(row.at("before").at("statistics").get<std::string>());
    Json events = Json::array();
    NativeUnitInteractionDefinition unit_definition;
    SourceUnitControl units({
      [&](std::uint32_t object) -> std::span<std::uint8_t> {
        const auto index = object&0xffffu;
        check(index < bodies.size() && object == 0xe26f0000u+index,"Foreign lifecycle object datum");
        return bodies[index];
      },
      [&](std::uint32_t object) { return std::uint8_t(read(bodies[object&0xffffu],0xaa,1)); },
      [&](std::uint32_t definition) -> const NativeUnitInteractionDefinition & {
        check(definition == 0xe1000002u,"Foreign lifecycle unit definition"); return unit_definition;
      },
      [](std::uint32_t) { throw std::runtime_error("Unprovided live item-effect owner reached"); },
      [&](std::uint32_t unit) {
        check(!globals[1] || read(bodies[unit&0xffffu],0x10) == DatumPool::none,
              "Driver callback needs its active AI owner");
        events.push_back({{"kind","driver"},{"unit",unit},{"actor_unit",read(actors.lookup(id),0x18)},
          {"live_count",actors.count()},{"flag3_live",read(statistics,6,2)},
          {"active_count",read(globals,0x36a,2)}});
        // Complete original1BBDF0 returned through its observed early-out.
      }});
    SourceAiActorLifecycleServices services{units,*pools.at("squads"),*pools.at("joints"),
      *pools.at("props"),*pools.at("references"),*pools.at("tracking"),
      *pools.at("clumps"),*pools.at("locations"),*pools.at("perceptions"),
      *pools.at("commands"),*pools.at("joint_commands"),*pools.at("threads"),globals,statistics,
      slots,source_retail_ai_actor_constructor_constants().empty_reference,
      [&] { return row.at("tick").get<std::int32_t>(); },
      [&](std::uint32_t definition) -> std::optional<SourceAiActorDefinition> {
        check(definition == 0xe1000000u,"Foreign lifecycle character");
        return SourceAiActorDefinition{definition,row.at("flags").get<std::uint32_t>(),1};
      }};
    try {
      if (bank) bank->release_actor(id,row.at("keep").get<bool>(),services);
      else SourceAiActorLifecycle::release(actors,id,row.at("keep").get<bool>(),services);
      for (const auto &[name,pool] : pools)
        compare(*pool,row.at("after").at(name),
                name == "actors" ? baseline : RecordBytes(pool->stride(),0xa5),name);
      check(globals == decode(row.at("after").at("globals").get<std::string>()),"AI globals differ");
      check(statistics == decode(row.at("after").at("statistics").get<std::string>()),"AI counters differ");
      check(events == row.at("events"),"Lifecycle driver dispatch order differs");
      for (unsigned i = 0; i < bodies.size(); ++i)
        check(bodies[i] == apply(RecordBytes(bodies[i].size(),0xa5),row.at("after").at("objects").at(i)),
              "Lifecycle unit/item bytes differ");
    } catch (const std::exception &error) {
      throw std::runtime_error("Lifecycle case="+std::to_string(count)+": "+error.what());
    }
    ++count;
  }
  std::cout << "PASS " << count << " whole original lifecycle cases"
            << (source_owned ? " in actual source-backed bank views" : " in owned banks")
            << ": all bank records, allocation metadata, "
               "units, globals, counters and callback order\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n'; return 1;
}
