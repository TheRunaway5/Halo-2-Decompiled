#pragma once
// Oracle-harness registry for native object factory track kernels. Each track
// adds tests/native_object_factory_<track>_kernels.cpp with static
// registrations; the harness consults this map before its built-in kernels.
// A kernel receives the raw world memory (stores are recorded in order), the
// script bank over it, and the case's argument words; it returns the result
// register the oracle compares (0 for void kernels).
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace halo2::native_object_factory_tests {
using Kernel=std::function<std::uint32_t(SourcePhysicsContactMemory& world,NativeCampaignScriptBank& bank,
    const std::vector<std::uint32_t>& args)>;
inline std::map<std::string,Kernel>& kernels(){static std::map<std::string,Kernel> table;return table;}
// Optional test-only observation of a named unported callee. A normal return
// reaches the physical harness's STOP address. It is reset for every request.
inline thread_local std::uint32_t stopped_at=0x3000000u;
struct Register {
  Register(const char* name,Kernel kernel){kernels().emplace(name,std::move(kernel));}
};
}
