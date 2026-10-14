#include "halo2/death_camera_lifecycle.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool b, const std::string &s) {
  if (!b)
    throw std::runtime_error(s);
}
std::uint32_t word(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  std::vector<std::byte> out;
  check(s.size() % 2 == 0, "Original byte extent");
  for (std::size_t i = 0; i < s.size(); i += 2)
    out.push_back(std::byte(word(s.substr(i, 2))));
  return out;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::byte> data;
  void read(std::uint32_t p, std::span<std::byte> out) const override {
    for (std::size_t i = 0; i < out.size(); ++i) {
      auto q = data.find(p + i);
      if (q == data.end()) {
        std::ostringstream s;
        s << "Required original lifecycle backing " << std::hex << p + i;
        throw std::runtime_error(s.str());
      }
      out[i] = q->second;
    }
  }
  void write(std::uint32_t p, std::span<const std::byte> in) override {
    for (std::size_t i = 0; i < in.size(); ++i)
      data.at(p + i) = in[i];
  }
};
struct Mutation {
  unsigned phase;
  std::uint32_t address;
  std::vector<std::byte> bytes;
};
struct Case {
  std::string name;
  std::uint32_t entry, view, camera, unit, force, reset, mode, transition;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  std::uint32_t query_mode, query_result, phases;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<halo2::SourcePhysicsContactRecordStore> writes;
  std::vector<Mutation> mutations;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original camera lifecycle fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream in(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(in, line);) {
      std::istringstream row(line);
      std::string kind, token;
      row >> kind;
      auto n = [&] {
        row >> token;
        return word(token);
      };
      if (kind == "C") {
        Case c{};
        row >> c.name;
        c.entry = n();
        c.view = n();
        c.camera = n();
        c.unit = n();
        c.force = n();
        c.reset = n();
        c.mode = n();
        c.transition = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.query_mode = n();
        c.query_result = n();
        c.phases = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original camera lifecycle case");
      auto &c = cases.back();
      if (kind == "M" || kind == "A") {
        const auto p = n();
        row >> token;
        const auto raw = bytes(token);
        if (kind == "M") {
          for (std::size_t i = 0; i < raw.size(); ++i)
            check(c.memory.data.emplace(p + i, raw[i]).second,
                  "Unique original lifecycle input byte");
        } else
          c.after.emplace(p, raw);
      } else if (kind == "W") {
        halo2::SourcePhysicsContactRecordStore s{};
        s.instruction = n();
        s.address = n();
        row >> token;
        s.before = bytes(token);
        row >> token;
        s.written = bytes(token);
        c.writes.push_back(s);
      } else if (kind == "P") {
        Mutation m{};
        m.phase = n();
        m.address = n();
        row >> token;
        m.bytes = bytes(token);
        c.mutations.push_back(m);
      } else
        check(false, "Original camera fixture record");
    }
    check(cases.size() == 97 || cases.size() == 125, "All original camera lifecycle cases");
    std::size_t stores = 0, state_bytes = 0, phases = 0;
    for (auto &c : cases) {
      std::size_t wi = 0;
      auto observe = [&](const halo2::SourcePhysicsContactRecordStore &s) {
        check(wi < c.writes.size(), c.name + " extra retained camera store");
        const auto &expected = c.writes[wi++];
        if (s.instruction != expected.instruction ||
            s.address != expected.address || s.before != expected.before ||
            s.written != expected.written) {
          std::ostringstream message;
          message << c.name << " store " << wi << " actual " << std::hex
                  << s.instruction << ':' << s.address << " expected "
                  << expected.instruction << ':' << expected.address;
          throw std::runtime_error(message.str());
        }
      };
      const auto view = std::bit_cast<std::int32_t>(c.view);
      for (unsigned phase = 0; phase < c.phases; ++phase) {
        for (const auto &m : c.mutations)
          if (m.phase == phase)
            c.memory.write(m.address, m.bytes);
        switch (c.entry) {
        case 0x156f60: {
          const auto mode =
              halo2::source_unit_camera_mode_1_1(c.memory, c.unit);
          check(mode.mode == c.query_mode &&
                    mode.seat_camera == (c.query_result == 1),
                c.name + " original seat-camera query");
          break;
        }
        case 0x157070:
          halo2::source_restore_unit_camera_1_1(c.memory, view, c.force != 0,
                                                observe);
          break;
        case 0x157520:
          halo2::source_set_camera_mode_1_1(c.memory, view, c.mode,
                                            c.transition != 0, observe);
          break;
        case 0x1573b0:
          halo2::source_select_death_camera_1_1(c.memory, view, c.reset != 0,
                                                c.fp, observe);
          break;
        case 0x23fb00:
          halo2::source_construct_death_camera_1_1(c.memory, c.camera, view, -1,
                                                   c.fp, observe);
          break;
        case 0x23fea0:
          halo2::source_replace_death_camera_target_1_1(c.memory, c.camera,
                                                        observe);
          break;
        case 0x23fca0:
          check(halo2::source_death_camera_team_restriction_1_1(c.memory,0x10000u)==
                    (c.query_result!=0),c.name+" original campaign team restriction");
          break;
        case 0x23fe30:
          check(halo2::source_death_camera_fallback_1_1(c.memory)==c.query_result,
                c.name+" original campaign object fallback");
          break;
        default:
          check(false, "Original camera entry");
        }
      }
      check(wi == c.writes.size(),
            c.name + " complete original camera store sequence");
      for (const auto &[p, expected] : c.after) {
        std::vector<std::byte> actual(expected.size());
        c.memory.read(p, actual);
        check(actual == expected, c.name + " retained camera state");
        state_bytes += actual.size();
      }
      stores += wi;
      phases += c.phases;
    }
    std::cout << "Original camera lifecycle: " << cases.size() << " cases, "
              << phases << " calls, " << stores << " stores, " << state_bytes
              << " bytes; no supplied effects\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
