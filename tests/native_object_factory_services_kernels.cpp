// Oracle harness kernels for track "services": the script host's vehicle exit
// evaluator tails (HS names; tools/native_object_factory_cases_services.py).
// They run the services track's script_* evaluators the host registers.
#include "halo2/native_object_factory_services_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
#include <array>
#include <stdexcept>
#include <string>

namespace {
using namespace halo2;
namespace f=native_object_factory_tu15;
using native_object_factory_tests::Register;
using M=SourcePhysicsContactMemory;

std::uint32_t r32(const M& m,std::uint32_t a){std::array<std::byte,4> b{};m.read(a,b);
  return std::uint32_t(b[0])|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
std::uint32_t body(const M& m,std::uint32_t object){return r32(m,r32(m,r32(m,0x4ecbd0u)+0x44u)+(object&0xffffu)*12u+8u);}
bool refusal(const std::string& what){return what.rfind("Native object factory: ",0)==0;}

// A refusal ends the case where the original's run stops at the refused
// callee's entry (the cases' stop entries). 11D990's result there is the
// original's AX at that entry: D0520 receives the rider's seat word (body
// +1FC); a refused table callback is entered through its address.
std::uint32_t vehicle_unload_boundary(const M& m,const std::string& what) {
  const std::string prefix="Native object factory: ";
  const auto owner=what.substr(prefix.size(),what.find(' ',prefix.size())-prefix.size());
  if(owner=="D0520") {
    const auto at=what.find(" of ");
    return r32(m,body(m,std::uint32_t(std::stoul(what.substr(at+4),nullptr,16)))+0x1fcu)&0xffffu;
  }
  if(owner.size()<=6 && owner.find_first_not_of("0123456789ABCDEF")==std::string::npos)
    return std::uint32_t(std::stoul(owner,nullptr,16))&0xffffu;
  throw std::runtime_error(what);
}

const Register vehicle_unload("vehicle_unload",[](auto& world,auto& bank,const auto& a){
  try{return std::uint32_t(f::script_vehicle_unload(bank,a.at(0),a.at(1)));}
  catch(const std::runtime_error& error) {
    if(!refusal(error.what()))throw;
    return vehicle_unload_boundary(world,error.what());
  }});
const Register unit_exit_vehicle("unit_exit_vehicle",[](auto&,auto& bank,const auto& a){
  try{f::script_unit_exit_vehicle(bank,a.at(0),std::uint16_t(a.at(1)));}
  catch(const std::runtime_error& error){if(!refusal(error.what()))throw;}
  return 0u;});
const Register ai_vehicle_exit("ai_vehicle_exit",[](auto&,auto& bank,const auto& a){
  try{f::script_ai_vehicle_exit(bank,a.at(0));}
  catch(const std::runtime_error& error){if(!refusal(error.what()))throw;}
  return 0u;});
}
