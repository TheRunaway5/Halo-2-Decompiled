#include "halo2/physics_body_factory.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v, const std::string &s) {
  if (!v)
    throw std::runtime_error("Original body factory differs: " + s);
}
std::uint32_t number(const std::string &s) {
  return std::stoul(s, nullptr, 16);
}
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size() % 2 == 0, "literal source byte extent");
  std::vector<std::byte> r;
  r.reserve(s.size() / 2);
  for (std::size_t n = 0; n < s.size(); n += 2)
    r.push_back(std::byte(number(s.substr(n, 2))));
  return r;
}
struct Memory : halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t, std::vector<std::byte>> buffers;
  auto at(std::uint32_t a, std::size_t n) const {
    auto p = buffers.upper_bound(a);
    if (p == buffers.begin()) {
      std::ostringstream message;
      message << "Required real physics backing unavailable at " << std::hex
              << a << " size " << n;
      throw std::runtime_error(message.str());
    }
    --p;
    if (std::uint64_t(a - p->first) + n > p->second.size()) {
      std::ostringstream message;
      message << "Required real physics backing unavailable at " << std::hex
              << a << " size " << n;
      throw std::runtime_error(message.str());
    }
    return p;
  }
  void read(std::uint32_t a, std::span<std::byte> out) const override {
    auto p = at(a, out.size());
    std::copy_n(p->second.begin() + (a - p->first), out.size(), out.begin());
  }
  void write(std::uint32_t a, std::span<const std::byte> in) override {
    auto p = at(a, in.size());
    std::copy(in.begin(), in.end(),
              buffers.at(p->first).begin() + (a - p->first));
  }
};
struct Case {
  std::string name;
  std::uint32_t entry, eax, ecx, edi, result, mxcsr;
  std::uint16_t cw;
  halo2::SourcePhysicsBodyFactoryScratch scratch;
  std::array<std::uint32_t, 16> args;
  Memory before;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<halo2::SourcePhysicsContactRecordStore> writes;
  std::vector<std::array<std::uint32_t, 2>> estimates;
};
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(
          "Supply original independently measured factory fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::vector<std::string> v;
      for (std::string x; in >> x;)
        v.push_back(x);
      if (v.empty())
        continue;
      if (v[0] == "C") {
        check(v.size() == 29, "complete actual constructor ABI");
        Case c{};
        c.name = v[1];
        c.entry = number(v[2]);
        c.eax = number(v[3]);
        c.ecx = number(v[4]);
        c.edi = number(v[5]);
        c.result = number(v[6]);
        c.cw = std::uint16_t(number(v[7]));
        c.mxcsr = number(v[8]);
        c.scratch = {number(v[9]), number(v[10]), number(v[11]), number(v[12])};
        for (unsigned n = 0; n < 16; ++n)
          c.args[n] = number(v[n + 13]);
        cases.push_back(std::move(c));
      } else {
        check(!cases.empty(), "actual owning call");
        auto &c = cases.back();
        if (v[0] == "M" || v[0] == "A") {
          check(v.size() == 3, "complete retained source buffer");
          auto &bank = v[0] == "M" ? c.before.buffers : c.after;
          check(bank.emplace(number(v[1]), bytes(v[2])).second,
                "unique source memory span");
        } else if (v[0] == "W") {
          check(v.size() == 5, "owning source store");
          c.writes.push_back(
              {number(v[1]), number(v[2]), bytes(v[3]), bytes(v[4])});
        } else if (v[0] == "Q") {
          check(v.size() == 3, "actual RSQRTSS operand and result");
          c.estimates.push_back({number(v[1]), number(v[2])});
        } else
          throw std::runtime_error("Unknown original factory fixture record");
      }
    }
    std::size_t stores = 0, totalbytes = 0, estimate_count = 0;
    for (const auto &c : cases) {
      auto m = c.before;
      std::vector<halo2::SourcePhysicsContactRecordStore> writes;
      const auto observe = [&](const auto &s) { writes.push_back(s); };
      const halo2::SourcePhysicsFactoryFloatingPoint fp{c.cw, c.mxcsr};
      std::size_t estimated = 0;
      // Fixture-only mathematical instruction boundary. This compares the owner
      // around original emulator RSQRTSS; it is never a live estimate provider,
      // shape/body success service or captured address allocator.
      const auto estimate = [&](std::uint32_t x) {
        check(estimated < c.estimates.size(),
              c.name + " actual estimate request count");
        const auto q = c.estimates[estimated++];
        check(x == q[0], c.name + " original RSQRTSS operand");
        return q[1];
      };
      switch (c.entry) {
      case 0x1ee380:
        halo2::source_initialize_physics_convex_model_1_1(m, c.eax, observe);
        break;
      case 0x1daac0: {
        std::array<std::uint32_t, 14> a;
        std::copy_n(c.args.begin(), 14, a.begin());
        check(halo2::source_create_physics_rigid_body_1_1(
                  m, c.eax, a, c.scratch, fp, estimate, observe) == c.result,
              c.name + " physical body return");
        break;
      }
      case 0x318100:
        check(halo2::source_construct_physics_rigid_body_1_1(
                  m, c.ecx, c.args[0], c.scratch, fp, observe) == c.result,
              c.name + " rigid body return");
        break;
      case 0x317ea0:
        check(halo2::source_allocate_physics_motion_1_1(
                  m, {c.edi, c.ecx, c.args[2], c.args[3], c.args[1], c.args[0]},
                  fp, observe) == c.result,
              c.name + " source motion allocation identity");
        break;
      case 0x320cc0:
      case 0x321320:
        check(halo2::source_construct_physics_motion_1_1(
                  m, c.ecx, c.args[0], c.args[1], c.entry == 0x320cc0 ? 7 : 4,
                  fp, observe) == c.result,
              c.name + " actual motion return");
        break;
      case 0x378ab0:
        halo2::source_physics_quaternion_matrix_1_1(m, c.ecx, c.args[0], fp,
                                                    observe);
        break;
      case 0x2e0860:
        halo2::source_physics_matrix_quaternion_1_1(m, c.ecx, c.args[0], fp,
                                                    observe);
        break;
      case 0x1dc750:
        halo2::source_physics_normalize_quaternion_1_1(m, c.ecx, estimate, fp,
                                                       observe);
        break;
      case 0x303e30:
        halo2::source_physics_convex_bounds_1_1(m, c.ecx, c.args[0], c.args[1],
                                                c.args[2], fp, observe);
        break;
      default:
        throw std::runtime_error("Unmeasured source factory entry");
      }
      check(estimated == c.estimates.size(),
            c.name + " full original math boundary consumption");
      if (writes != c.writes) {
        std::size_t n = 0;
        while (n < writes.size() && n < c.writes.size() &&
               writes[n] == c.writes[n])
          ++n;
        std::ostringstream msg;
        msg << c.name << " store " << n << "/" << writes.size() << " expected "
            << c.writes.size();
        if (n < writes.size())
          msg << " actual " << std::hex << writes[n].instruction << " at "
              << writes[n].address;
        if (n < c.writes.size())
          msg << " original " << std::hex << c.writes[n].instruction << " at "
              << c.writes[n].address;
        throw std::runtime_error(msg.str());
      }
      check(m.buffers == c.after,
            c.name + " full source buffers including untouched state");
      for (const auto &[a, b] : m.buffers) {
        (void)a;
        totalbytes += b.size();
      }
      stores += writes.size();
      estimate_count += estimated;
    }
    // Independently compose all measured normalization/root cases with a real
    // host instruction provider. No captured estimate or constructor result is
    // returned. These actual final buffers match, despite different RSQRTSS
    // intermediate estimates; this is bounded host evidence, not Xbox parity.
    std::size_t hardware_cases = 0;
    for (const auto &c : cases) {
      if (c.estimates.empty())
        continue;
      auto m = c.before;
      std::vector<halo2::SourcePhysicsContactRecordStore> observed;
      const auto observer = [&](const auto &s) { observed.push_back(s); };
      const halo2::SourcePhysicsFactoryFloatingPoint fp{c.cw, c.mxcsr};
      if (c.entry == 0x1daac0) {
        std::array<std::uint32_t, 14> a;
        std::copy_n(c.args.begin(), 14, a.begin());
        check(halo2::source_create_physics_rigid_body_1_1(
                  m, c.eax, a, c.scratch, fp,
                  halo2::source_host_x86_rsqrtss_estimate,
                  observer) == c.result,
              c.name + " real host factory identity");
      } else {
        check(c.entry == 0x1dc750, "actual source normalization owning call");
        halo2::source_physics_normalize_quaternion_1_1(
            m, c.ecx, halo2::source_host_x86_rsqrtss_estimate, fp, observer);
      }
      check(observed == c.writes && m.buffers == c.after,
            c.name + " real host instruction final full buffers/stores");
      ++hardware_cases;
    }
    check(hardware_cases == 4, "actual host estimate composition coverage");
    check(cases.size() == 16, "measured original constructor roster");
    Memory absent;
    bool failed = false;
    try {
      halo2::source_construct_physics_motion_1_1(absent, 1, 2, 3, 5,
                                                 {0x27f, 0x1f80});
    } catch (const std::runtime_error &) {
      failed = true;
    }
    check(failed && absent.buffers.empty(),
          "unmeasured motion cannot manufacture output");
    const auto &normal =
        *std::find_if(cases.begin(), cases.end(),
                      [](const auto &c) { return c.entry == 0x1dc750; });
    auto m = normal.before;
    std::vector<halo2::SourcePhysicsContactRecordStore> writes;
    failed = false;
    try {
      halo2::source_physics_normalize_quaternion_1_1(
          m, normal.ecx, {}, {normal.cw, normal.mxcsr},
          [&](const auto &s) { writes.push_back(s); });
    } catch (const std::runtime_error &) {
      failed = true;
    }
    check(failed && writes.empty() && m.buffers == normal.before.buffers,
          "missing RSQRTSS cannot choose a successful estimate");
    std::cout << cases.size() << " original crate factory calls matched "
              << totalbytes << " retained bytes, " << stores
              << " ordered stores and " << estimate_count
              << " explicit measured RSQRTSS boundaries and " << hardware_cases
              << " actual host instruction compositions\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
