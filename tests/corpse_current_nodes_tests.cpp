#include "halo2/object_current_nodes.hpp"
#include "halo2/source_runtime_memory.hpp"
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
  check(s.size() % 2 == 0, "Original byte extent");
  std::vector<std::byte> v;
  for (unsigned i = 0; i < s.size(); i += 2)
    v.push_back(std::byte(word(s.substr(i, 2))));
  return v;
}
using Memory = halo2::SourceRuntimeMemory;
using Store = halo2::SourcePhysicsContactRecordStore;
struct Plan {
  std::uint32_t entry, esp, result;
  std::array<std::uint32_t, 6> regs;
  std::array<std::uint32_t, 16> args;
  std::vector<Store> effects;
};
struct Case {
  std::string name;
  std::uint32_t datum, esp, result;
  halo2::SourcePhysicsFactoryFloatingPoint fp;
  Memory memory;
  std::map<std::uint32_t, std::vector<std::byte>> after;
  std::vector<Store> stores;
  std::vector<Plan> plans;
  std::vector<std::array<std::uint32_t, 2>> reciprocal;
  std::vector<std::array<std::uint32_t,3>> estimates;
  std::vector<std::array<std::uint32_t,4>> resources;
};
} // namespace
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply original corpse current-node fixture");
    if (!std::filesystem::exists(argv[1]))
      return 77;
    std::ifstream input(argv[1]);
    std::vector<Case> cases;
    bool native_matrices = false, native_animation=false;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      std::string k, t;
      in >> k;
      auto n = [&] {
        in >> t;
        return word(t);
      };
      if (k == "N") {
        native_matrices = n() != 0;
        if(in>>t)native_animation=word(t)!=0;
        continue;
      }
      if (k == "C") {
        Case c{};
        in >> c.name;
        c.datum = n();
        c.esp = n();
        c.fp.x87_control_word = n();
        c.fp.mxcsr = n();
        c.result = n();
        cases.push_back(std::move(c));
        continue;
      }
      check(!cases.empty(), "Original phase case");
      auto &c = cases.back();
      if (k == "M" || k == "A") {
        const auto p = n();
        in >> t;
        const auto raw = bytes(t);
        if (k == "A")
          c.after.emplace(p, raw);
        else
          c.memory.mount(p, raw, Memory::Access::ReadWrite,
                         "original caller backing");
      } else if (k == "W" || k == "L") {
        Store s{};
        s.instruction = n();
        s.address = n();
        in >> t;
        s.before = bytes(t);
        in >> t;
        s.written = bytes(t);
        if (k == "W")
          c.stores.push_back(s);
        else {
          check(!c.plans.empty(), "Original lower owner");
          c.plans.back().effects.push_back(s);
        }
      } else if (k == "E") {
        in>>t;const auto kind=t=="R"?0u:1u;const auto x=n(),y=n();c.estimates.push_back({kind,x,y});
      } else if (k == "D") {
        std::array<std::uint32_t,4> q;for(auto &v:q)v=n();c.resources.push_back(q);
      } else if (k == "R") {
        c.reciprocal.push_back({n(), n()});
      } else if (k == "P") {
        Plan p{};
        p.entry = n();
        p.esp = n();
        p.result = n();
        for (auto &v : p.regs)
          v = n();
        for (auto &v : p.args)
          v = n();
        c.plans.push_back(p);
      } else
        check(false, "Original phase fixture record");
    }
    check(cases.size() == 13, "Complete natural current-node capture");
    std::size_t stores = 0, calls = 0, total_bytes = 0;
    for (auto &c : cases) {
      std::size_t wi = 0, pi = 0;
      auto observe = [&](const Store &s) {
        if (s.address < 0x2000000 || s.address >= 0x2200000) {
          check(wi<c.stores.size(),c.name+" extra retained store");
          const auto &wanted=c.stores[wi++];
          if(s!=wanted) {
            std::ostringstream out;out<<c.name<<" retained store "<<wi<<" actual "<<std::hex<<s.instruction<<':'<<s.address<<" expected "<<wanted.instruction<<':'<<wanted.address;
            throw std::runtime_error(out.str());
          }
        }
      };
      halo2::SourceObjectCurrentNodeServices services;
      std::size_t ri = 0, ei=0, di=0;
      if(native_animation) {
        services.animation.emplace();
        services.animation->estimate=[&](auto kind,std::uint32_t input) {
          check(ei<c.estimates.size(),c.name+" extra sampler CPU estimate");const auto &q=c.estimates[ei++];
          check(q[0]==(kind==halo2::SourceUnitAnimationEstimate::Reciprocal?0u:1u)&&q[1]==input,c.name+" original sampler CPU input");return q[2];
        };
        services.animation->resource_delivery=[&](auto &m,const auto &call,auto &abi,const auto &) {
          check(di<c.resources.size(),c.name+" extra resource byte delivery");const auto &q=c.resources[di++];
          std::array<std::byte,4>b;m.read(abi.gp[4]+4,b);std::uint32_t name=0;
          for(unsigned i=0;i<4;++i)name|=std::uint32_t(b[i])<<(8*i);
          check(call.target==0x124f30&&abi.gp[4]==q[0]&&abi.gp[0]==q[1]&&name==q[2],c.name+" actual cache delivery ABI");
          abi.gp[0]=q[3];abi.gp[4]+=8;
        };
        services.animation->required_owner=[&](auto &,const auto &call,auto &,const auto &) {
          throw std::runtime_error(c.name+" unbound sampler owner "+std::to_string(call.target));
        };
      }
      if (native_matrices)
        services.reciprocal = [&](std::uint32_t input) {
          check(ri < c.reciprocal.size(),
                c.name + " extra original RCPSS boundary");
          const auto pair = c.reciprocal[ri++];
          check(input == pair[0], c.name + " original RCPSS input");
          return pair[1];
        };
      services.required_owner =
          [&](auto &memory, const halo2::SourceObjectNodeOwnerCall &request,
              const auto &observer) {
            const auto &call = request.abi;
            check(pi < c.plans.size(), c.name + " extra lower call");
            const auto &p = c.plans[pi++];
            if (call.entry != p.entry || call.entry_esp != p.esp) {
              std::ostringstream out;
              out << c.name << " call " << pi << " actual " << std::hex
                  << call.entry << ':' << call.entry_esp << " expected "
                  << p.entry << ':' << p.esp;
              throw std::runtime_error(out.str());
            }
            if (request.bl)
              check(*request.bl == (p.regs[5] & 255u),
                    c.name + " original BL input");
            if (request.dl)
              check(*request.dl == (p.regs[2] & 255u),
                    c.name + " original DL input");
            const std::array actual{call.eax, call.ecx, call.edx,
                                    call.esi, call.edi, call.ebx};
            for (unsigned i = 0; i < actual.size(); ++i)
              if (actual[i])
                check(*actual[i] == p.regs[i],
                      c.name + " defined input register " + std::to_string(i));
            for (unsigned i = 0; i < call.arguments.size(); ++i)
              check(call.arguments[i] == p.args.at(i),
                    c.name + " original call argument");
            for (const auto &s : p.effects) {
              std::vector<std::byte> before(s.before.size());
              memory.read(s.address, before);
              if (before != s.before) {
                std::ostringstream out;
                out << c.name << " lower " << std::hex << p.entry << " before "
                    << s.instruction << ':' << s.address;
                throw std::runtime_error(out.str());
              }
              memory.write(s.address, s.written);
              if (observer)
                observer(s);
            }
            return p.result;
          };
      halo2::source_update_object_current_nodes_1_1(c.memory, c.datum, c.esp,
                                                    c.fp, services, observe);
      check(wi==c.stores.size(),c.name+" complete retained store sequence");
      check(ei==c.estimates.size()&&di==c.resources.size(),c.name+" complete native pose boundaries");
      check(pi == c.plans.size(), c.name + " complete owner sequence");
      check(ri == c.reciprocal.size(),
            c.name + " complete original RCPSS sequence");
      for (const auto &[p, e] : c.after) {
        std::vector<std::byte> actual(e.size());
        c.memory.read(p, actual);
        check(actual == e, c.name + " retained final bytes");
        total_bytes += e.size();
      }
      stores += wi;
      calls += pi;
    }
    std::cout << cases.size() << " original current-node phases, " << calls
              << " lower owner boundaries, " << stores << " stores and "
              << total_bytes << " retained bytes match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
