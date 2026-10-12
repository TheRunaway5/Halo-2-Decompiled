#include "halo2/biped_body_placement.hpp"
#include "halo2/biped_ragdoll_admission.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool b, const std::string &message) {
  if (!b)
    throw std::runtime_error(message);
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
      const auto q = data.find(p + i);
      if (q == data.end()) {
        std::ostringstream s;
        s << "Required original contact backing " << std::hex << p + i;
        throw std::runtime_error(s.str());
      }
      out[i] = q->second;
    }
  }
  void write(std::uint32_t p, std::span<const std::byte> in) override {
    for (std::size_t i = 0; i < in.size(); ++i)
      data.at(p + i) = in[i];
  }
  std::uint32_t get(std::uint32_t p) const {
    std::array<std::byte, 4> b{};
    read(p, b);
    std::uint32_t v{};
    for (unsigned i = 0; i < 4; ++i)
      v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
    return v;
  }
};
struct Delta {
  std::uint32_t component, slot, vector, esp;
  std::array<std::uint32_t, 3> words;
};
struct Case {
  std::string name;
  std::uint32_t datum, node, frame;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  bool applied;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<halo2::SourcePhysicsContactRecordStore> writes;
  std::vector<Delta> deltas;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original contact impulse fixture");
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
        c.datum = n();
        c.node = n();
        c.frame = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.applied = n() != 0;
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original contact case");
      auto &c = cases.back();
      if (kind == "M" || kind == "A") {
        const auto p = n();
        row >> token;
        const auto raw = bytes(token);
        if (kind == "M") {
          for (std::size_t i = 0; i < raw.size(); ++i)
            check(c.memory.data.emplace(p + i, raw[i]).second,
                  "Unique original input byte");
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
      } else if (kind == "D") {
        Delta d{};
        d.component = n();
        d.slot = n();
        d.vector = n();
        d.esp = n();
        for (auto &v : d.words)
          v = n();
        c.deltas.push_back(d);
      } else
        check(false, "Original contact fixture record");
    }
    check(cases.size() == 55, "All original contact cases");
    std::size_t total_writes = 0, total_bytes = 0, total_deltas = 0;
    for (auto &c : cases) {
      std::size_t wi = 0, di = 0;
      auto observe = [&](const halo2::SourcePhysicsContactRecordStore &s) {
        if (s.address >= 0x21f0000 && s.address < 0x2200000)
          return;
        check(wi < c.writes.size(), c.name + " extra retained store");
        const auto &expected = c.writes[wi++];
        if (s.instruction != expected.instruction ||
            s.address != expected.address || s.before != expected.before ||
            s.written != expected.written) {
          std::ostringstream message;
          message << c.name << " retained store " << wi << " at " << std::hex
                  << s.instruction << ':' << s.address << " expected "
                  << expected.instruction << ':' << expected.address;
          throw std::runtime_error(message.str());
        }
      };
      const auto applied = halo2::source_biped_contact_velocity_1_1(
          c.memory, c.datum, std::bit_cast<std::int32_t>(c.node), c.frame, c.fp,
          [&](std::uint32_t component, std::int32_t slot,
              std::uint32_t vector) {
            check(di < c.deltas.size(),
                  c.name + " extra body velocity request");
            const auto &d = c.deltas[di++];
            check(component == d.component && std::uint32_t(slot) == d.slot &&
                      vector == d.vector && d.esp == c.frame - 4,
                  c.name + " original velocity owner identity");
            for (unsigned i = 0; i < 3; ++i) {
              if (c.memory.get(vector + 4 * i) != d.words[i]) {
                std::ostringstream s;
                s << c.name << " slot " << slot << " velocity word " << i
                  << " actual " << std::hex << c.memory.get(vector + 4 * i)
                  << " expected " << d.words[i];
                throw std::runtime_error(s.str());
              }
            }
            halo2::source_add_biped_body_velocity_1_1(
                c.memory, component, std::uint32_t(slot), vector, d.esp, c.fp,
                {}, observe);
          },
          observe);
      check(applied == c.applied,
            c.name + " original distribution/fallback branch");
      check(wi == c.writes.size() && di == c.deltas.size(),
            c.name + " complete original store and request sequence");
      for (const auto &[p, expected] : c.after) {
        std::vector<std::byte> actual(expected.size());
        c.memory.read(p, actual);
        check(actual == expected, c.name + " retained state");
        total_bytes += actual.size();
      }
      total_writes += wi;
      total_deltas += di;
    }
    std::cout << "Original contact distribution: " << cases.size() << " cases, "
              << total_writes << " retained stores, " << total_bytes
              << " bytes, " << total_deltas
              << " native velocity requests; no supplied effects\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
