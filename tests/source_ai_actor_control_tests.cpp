#include "halo2/source_ai_actor_control.hpp"
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
  check(hex.size()%2 == 0,"Incomplete actor control bytes");
  RecordBytes bytes(hex.size()/2);
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return bytes;
}
RecordBytes apply(RecordBytes bytes, const Json &changes) {
  for (const auto &change : changes) {
    const auto offset = change.at(0).get<unsigned>();
    const auto value = decode(change.at(1).get<std::string>());
    check(offset <= bytes.size() && value.size() <= bytes.size()-offset,"Actor control delta exceeds record");
    std::copy(value.begin(),value.end(),bytes.begin()+offset);
  }
  return bytes;
}
void write(std::span<std::uint8_t> bytes, unsigned offset, std::uint32_t value, unsigned width = 4) {
  check(offset+width <= bytes.size(),"Actor control fixture store exceeds backing");
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
        "Supply original actor control fixture and optional source-owned mode");
  const bool source_owned = argc == 3;
  std::ifstream file(argv[1]);std::string line;
  while (std::getline(file,line) && line != "```json") {}
  check(bool(file),"Actor control fixture has no JSON block");
  Json fixture;file >> fixture;
  check(fixture.at("passed").get<bool>(),"Original actor control oracle failed");
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
        check(index < bodies.size() && id == 0xe26f0000u+index,"Foreign actor control object datum");
        return bodies[index];
      },
      [](std::uint32_t) -> std::uint8_t { throw std::runtime_error("Unrequired object-type owner entered"); },
      [](std::uint32_t) -> const NativeUnitInteractionDefinition & { throw std::runtime_error("Unrequired unit tag decoder entered"); },
      [](std::uint32_t) { throw std::runtime_error("Unrequired item-effect owner entered"); },
      [](std::uint32_t) { throw std::runtime_error("Unrequired driver owner entered"); }});
    SourceAiActorControlServices services{objects,*pools.at("perceptions"),*pools.at("clumps"),globals,
      [&] { return std::bit_cast<std::int32_t>(row.at("tick").get<std::uint32_t>()); }};
    const auto id = row.at("actor").get<std::uint32_t>();
    const auto compare_state = [&](const Json &state) {
      for (const auto &[name,pool] : pools)
        compare(*pool,state.at(name),name == "actors" ? baseline : RecordBytes(pool->stride(),0xa5),name);
      check(globals == decode(state.at("globals").get<std::string>()),"Actor control AI globals differ");
      for (unsigned i = 0; i < bodies.size(); ++i)
        check(bodies[i] == apply(RecordBytes(bodies[i].size(),0xa5),state.at("objects").at(i)),"Actor control object bytes differ");
      if (bank)
        check(std::equal(bank->record(id).begin(),bank->record(id).end(),actors.lookup(id).begin()),
              "Actor bank view diverged from the shared source pool");
    };
    try {
      check((bank ? bank->can_activate(id,services) : SourceAiActorControl::can_activate(actors,id,services)) ==
            row.at("eligible").get<bool>(),"Original eligibility differs");
      compare_state(row.at("before"));
      check((bank ? bank->set_active(id,row.at("requested").get<bool>(),services) :
             SourceAiActorControl::set_active(actors,id,row.at("requested").get<bool>(),services)) ==
            row.at("activated").get<bool>(),"Original activation result differs");
      compare_state(row.at("after_activation"));
      if (bank) bank->set_team(id,row.at("team").get<std::int16_t>(),services);
      else SourceAiActorControl::set_team(actors,id,row.at("team").get<std::int16_t>(),services);
      compare_state(row.at("after_team"));
    } catch (const std::exception &error) {
      throw std::runtime_error("Actor control case="+std::to_string(count)+": "+error.what());
    }
    ++count;
  }
  std::cout << "PASS " << count << " whole original actor eligibility/activation/team cases"
            << (source_owned ? " through shared source-backed actor bank APIs" : " in owned banks") << '\n';
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n';return 1; }
