// Oracle harness kernels for track "unitactions" (see the registry header).
#include "halo2/native_object_factory_unitactions_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
// The 1EFA70 history native (native_campaign_unit_notify_action_tu15) is a
// source of the harness target (cmake/NativeCampaignScriptBuiltinsTu15.cmake).
#include <array>
#include <stdexcept>
#include <string>

namespace {
using namespace halo2;
namespace ua=native_object_factory_tu15::unit_actions;
using native_object_factory_tests::Register;
using M=SourcePhysicsContactMemory;

ua::Fp fp(const NativeCampaignScriptBank& bank) {
  return {bank.floating_point().x87_control_word,0,bank.floating_point().mxcsr};
}
std::uint32_t r32(const M& m,std::uint32_t a){std::array<std::byte,4> b{};m.read(a,b);
  return std::uint32_t(b[0])|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
std::uint8_t r8(const M& m,std::uint32_t a){std::array<std::byte,1> b{};m.read(a,b);return std::uint8_t(b[0]);}
void w32(M& m,std::uint32_t a,std::uint32_t v){const std::array b{std::byte(v),std::byte(v>>8),std::byte(v>>16),std::byte(v>>24)};m.write(a,b);}
void w16(M& m,std::uint32_t a,std::uint16_t v){const std::array b{std::byte(v),std::byte(v>>8)};m.write(a,b);}
std::uint32_t body(const M& m,std::uint32_t object){return r32(m,r32(m,r32(m,0x4ecbd0u)+0x44u)+(object&0xffffu)*12u+8u);}
std::uint32_t slot_weapon(const M& m,std::uint32_t unit,std::uint32_t slot_byte) {
  const auto b=body(m,unit);const auto slot=std::int8_t(r8(m,b+slot_byte));
  return slot==-1?0xffffffffu:r32(m,b+0x218u+std::uint32_t(std::int32_t(slot))*4u);
}
ua::Request request_at(const M& m,std::uint32_t at) {
  ua::Request r;std::array<std::byte,0x20> b{};m.read(at,b);
  for(unsigned n=0;n<0x20;++n)r.bytes[n]=std::uint8_t(b[n]);
  return r;
}
// A refusal ends the case where the original's run stops at the refused
// callee's entry (STOPS in the cases); the kernel reports the original's EAX
// there: a refusal naming "(original AL xx)" carries it, 106670 the slot
// weapon, D0520 the rider's seat word, a refused callback its address.
// `unit`/`request` are the kernel's.
std::uint32_t boundary(const std::string& what,const std::string& owner,const M& m,std::uint32_t unit,
    const ua::Request& request);
// The oracle compares the result register under the kernel's kind (AL for
// boolean kernels, AX for short ones): `mask`.
template<class F>
std::uint32_t at_boundary(F&& run,const M& m,std::uint32_t unit,const ua::Request& request,std::uint32_t mask=0xffu) {
  try{return run();}
  catch(const std::runtime_error& error) {
    const std::string what=error.what(),prefix="Native object factory: ";
    if(what.rfind(prefix,0)!=0)throw;
    const std::string owner=what.substr(prefix.size(),what.find(' ',prefix.size())-prefix.size());
    native_object_factory_tests::stopped_at=std::uint32_t(std::stoul(owner,nullptr,16));
    return boundary(what,owner,m,unit,request)&mask;
  }
}
std::uint32_t boundary(const std::string& what,const std::string& owner,const M& m,std::uint32_t unit,
    const ua::Request&) {
  const auto al=what.find("(original AL ");
  if(al!=std::string::npos)return std::uint32_t(std::stoul(what.substr(al+13),nullptr,16));
  if(owner=="106670") {
    const auto primary=slot_weapon(m,unit,0x212u);
    return primary!=0xffffffffu?primary:slot_weapon(m,unit,0x213u);
  }
  if(owner=="D0520") {
    const auto at=what.find(" of ");
    const auto rider=std::uint32_t(std::stoul(what.substr(at+4),nullptr,16));
    return r32(m,body(m,rider)+0x1fcu)&0xffffu;
  }
  if(owner.size()<=6 && owner.find_first_not_of("0123456789ABCDEF")==std::string::npos)
    return std::uint32_t(std::stoul(owner,nullptr,16));
  throw std::runtime_error(what);
}
// Void kernels: any refusal ends the case (the original stops at its entry).
template<class F>
std::uint32_t stopped(F&& run) {
  try{run();}
  catch(const std::runtime_error& error) {
    const std::string what=error.what(),prefix="Native object factory: ";
    if(what.rfind(prefix,0)!=0)throw;
    native_object_factory_tests::stopped_at=std::uint32_t(std::stoul(what.substr(prefix.size()),nullptr,16));
  }
  return 0;
}

const Register request("unitactions_request",[](auto& world,auto& bank,const auto& a){
  const auto r=request_at(world,a.at(1));
  return at_boundary([&]{return std::uint32_t(ua::request(world,a.at(0),r,fp(bank)));},world,a.at(0),r);});
const Register request_any("unitactions_request_any",[](auto& world,auto& bank,const auto& a){
  const auto r=request_at(world,a.at(1));
  return stopped([&]{ua::request(world,a.at(0),r,fp(bank));});});
const Register request_type("unitactions_request_type",[](auto& world,auto& bank,const auto& a){
  const auto r=ua::Request::of(a.at(0));
  return at_boundary([&]{return std::uint32_t(ua::request_type(world,a.at(1),a.at(0),fp(bank)));},world,a.at(1),r);});
const Register update("unitactions_update",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{return std::uint32_t(ua::update(world,a.at(0),fp(bank)));},world,a.at(0),ua::Request{});});
const Register stop("unitactions_stop",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::stop(world,a.at(0),a.at(1),fp(bank));});});
const Register interrupt("unitactions_interrupt",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::interrupt(world,a.at(0),a.at(1),fp(bank));});});
const Register active("unitactions_active",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::active(world,a.at(0),a.at(1)));});
const Register weapon_ready("unitactions_weapon_ready",[](auto& world,auto& bank,const auto& a){
  const auto r=request_at(world,a.at(1));
  return at_boundary([&]{return std::uint32_t(ua::weapon_ready_start(world,a.at(0),r,fp(bank)));},world,a.at(0),r);});
const Register ready_interrupt("unitactions_ready_interrupt",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::weapon_ready_interrupt(world,a.at(0),a.at(1),fp(bank));});});
const Register put_away("unitactions_put_away",[](auto& world,auto& bank,const auto& a){
  const auto r=request_at(world,a.at(1));
  return at_boundary([&]{return std::uint32_t(ua::weapon_put_away_start(world,a.at(0),r,fp(bank)));},world,a.at(0),r);});
const Register channels_clear("unitactions_channels_clear",[](auto& world,auto&,const auto& a){
  ua::weapon_channels_clear(world,a.at(0));return 0u;});
const Register seat_actions_reset("unitactions_seat_actions_reset",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::seat_weapon_actions_reset(world,a.at(0),fp(bank));});});
const Register trigger_update("unitactions_trigger_update",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::weapon_trigger_update(world,a.at(0),a.at(1)));});
const Register trigger_ready("unitactions_trigger_ready",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::weapon_trigger_ready(world,a.at(0),a.at(1)));});
const Register trigger_interrupt("unitactions_trigger_interrupt",[](auto& world,auto&,const auto& a){
  return stopped([&]{ua::weapon_trigger_interrupt(world,a.at(0),a.at(1));});});
const Register target_clear("unitactions_target_clear",[](auto& world,auto&,const auto& a){
  ua::action_target_clear(world,a.at(0));return 0u;});
const Register vehicle_exit("unitactions_vehicle_exit",[](auto& world,auto& bank,const auto& a){
  const auto r=request_at(world,a.at(1));
  return at_boundary([&]{return std::uint32_t(ua::vehicle_exit_start(world,a.at(0),r,fp(bank)));},world,a.at(0),r);});
const Register seat_release("unitactions_seat_release",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{return std::uint32_t(ua::seat_release(world,a.at(0),fp(bank)));},world,a.at(0),ua::Request{});});
const Register seat_drivers("unitactions_seat_drivers",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::seat_drivers_update(world,a.at(0),fp(bank));});});
const Register occupant("unitactions_seat_occupant",[](auto& world,auto&,const auto& a){
  return ua::seat_occupant(world,a.at(0),std::uint16_t(a.at(1)));});
const Register admits("unitactions_mapping_admits",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::seat_mapping_admits(world,a.at(0),a.at(1),a.at(2)));});
// CAF20 stores each entry (+4 seat word, +0 object, +8 seat row) and then
// increments the count word; production keeps the entries natively.
const Register object_seats("unitactions_object_seats",[](auto& world,auto&,const auto& a){
  const std::uint32_t out=a.at(1),count=a.at(2);
  const auto first=std::int16_t(r32(world,count)&0xffffu);
  std::vector<ua::Seat> seats(std::size_t(first),ua::Seat{});
  ua::object_seats(world,a.at(0),seats,std::int16_t(a.at(3)&0xffffu));
  for(std::size_t n=std::size_t(first);n<seats.size();++n) {
    const std::uint32_t entry=out+std::uint32_t(n)*12u;
    w16(world,entry+4u,std::uint16_t(seats[n].seat));
    w32(world,entry,seats[n].object);
    w32(world,entry+8u,seats[n].definition);
    w16(world,count,std::uint16_t(n+1u));
  }
  return 0u;});
// 2199C0 copies each kept 12-byte entry to `out` (+0, +4 dword: the seat
// word over CAF20's zeroed stack, +8) and returns the count.
const Register mapped_seats("unitactions_mapped_seats",[](auto& world,auto&,const auto& a){
  const auto kept=ua::mapped_seats(world,a.at(0),a.at(5),a.at(1),a.at(2),std::int32_t(a.at(4)));
  for(std::size_t n=0;n<kept.size();++n) {
    const std::uint32_t entry=a.at(3)+std::uint32_t(n)*12u;
    w32(world,entry,kept[n].object);
    w32(world,entry+4u,std::uint16_t(kept[n].seat));
    w32(world,entry+8u,kept[n].definition);
  }
  return std::uint32_t(kept.size());});
const Register vehicle_unload("unitactions_vehicle_unload",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{return std::uint32_t(ua::vehicle_unload(world,a.at(0),a.at(1),fp(bank)));},world,0,ua::Request{},0xffffu);});
const Register exit_vehicle("unitactions_exit_vehicle",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::unit_exit_vehicle(world,a.at(0),std::uint16_t(a.at(1)),fp(bank));});});
// 27EAB0's walk for squad references (tag 0): the 20CFF0/20D030 member chain
// ([[0x52BE7C]+44] + 0x98 * index, +68 first, actor +20 next) while the AI
// is enabled ([[0x5020A8]+1]); production binds the script host's walk.
const Register ai_vehicle_exit("unitactions_ai_vehicle_exit",[](auto& world,auto& bank,const auto& a){
  const auto enabled=[&]{return r8(world,r32(world,0x5020a8u)+1u)!=0;};
  std::uint32_t next=0xffffffffu;
  if(enabled())next=r32(world,r32(world,r32(world,0x52be7cu)+0x44u)+(a.at(0)&0xffffu)*0x98u+0x68u);
  const auto walk=[&]()->std::uint32_t {
    if(!enabled() || next==0xffffffffu)return 0;
    const auto row=r32(world,r32(world,0x5020c8u)+0x44u)+(next&0xffffu)*0x888u;
    next=r32(world,row+0x20u);
    return row;
  };
  return stopped([&]{ua::ai_vehicle_exit(world,a.at(1),walk,fp(bank));});});
// ---- Weapon readiness owners ----
// 111D10 stores the mode key to [ESI] and the state key to [EDX].
const Register animation_keys("unitactions_animation_keys",[](auto& world,auto&,const auto& a){
  std::uint32_t mode=0,state=0;
  const auto result=ua::unit_animation_keys(world,a.at(0),mode,state);
  w32(world,a.at(1),mode);
  w32(world,a.at(2),state);
  return std::uint32_t(result);});
const Register state_active("unitactions_state_active",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::unit_animation_state_active(world,a.at(0),a.at(1)));});
const Register mode_locked("unitactions_mode_locked",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::unit_animation_mode_locked(world,a.at(0)));});
const Register change_blocked("unitactions_change_blocked",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::weapon_change_blocked(world,a.at(0)));});
const Register animation_available("unitactions_animation_available",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::weapon_animation_available(world,a.at(0),a.at(1),a.at(2),a.at(3)));});
const Register ready_allowed("unitactions_ready_allowed",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::weapon_ready_allowed(world,a.at(0),a.at(2),a.at(1)));});
const Register slot_flagged("unitactions_slot_flagged",[](auto& world,auto&,const auto& a){
  return std::uint32_t(ua::weapon_slot_flagged(world,a.at(0),a.at(1)));});
const Register pickup_interrupt("unitactions_pickup_interrupt",[](auto& world,auto& bank,const auto& a){
  return stopped([&]{ua::weapon_pickup_interrupt(world,a.at(0),fp(bank));});});
const Register weapon_select("unitactions_weapon_select",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{return std::uint32_t(ua::weapon_select(world,a.at(0),std::uint8_t(a.at(1)),std::uint8_t(a.at(2)),
      a.at(3)!=0,fp(bank)));},world,a.at(0),ua::Request{});});
// EAC70's every path ends at a refused callee today; the kernel reports AL there.
const Register ready_animation("unitactions_ready_animation",[](auto& world,auto& bank,const auto& a){
  return at_boundary([&]{ua::weapon_ready_animation(world,a.at(1),a.at(0)!=0,a.at(2),std::uint8_t(a.at(3)),
      std::uint8_t(a.at(4)),fp(bank));return 0u;},world,a.at(1),ua::Request{});});
}
