#include "halo2/physics_impulse_journal.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
constexpr int exponent_floor=-24,exponent_ceiling=24;
bool newer_serial(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a-b)>0;}
bool finite(Vec3 v){return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);}
bool same_bits(Vec3 a,Vec3 b) {
  return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
      std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
      std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
}
bool published(Vec3 v){return finite(v) && same_bits(physics_impulse_quantize(v),v);}
// Clamped before rounding: a saturated component stays inside long's range.
long mantissa(float value,int exponent){return std::lround(std::clamp(std::ldexp(value,-exponent),-1024.f,1024.f));}
}
bool same_physics_impulse(const PhysicsImpulseEvent &a,const PhysicsImpulseEvent &b) {
  return a.serial==b.serial && a.tick==b.tick && a.object==b.object &&
      same_bits(a.linear_velocity,b.linear_velocity) && same_bits(a.angular_velocity,b.angular_velocity);
}
std::array<std::uint8_t,4> physics_impulse_vector_bits(Vec3 v) {
  if(!finite(v))throw FormatError("Non-finite physics impulse value");
  const float largest=std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)});
  int exponent=exponent_floor;
  if(largest>0) {
    int k{};std::frexp(largest,&k);        // largest = m * 2^k, m in [0.5, 1)
    exponent=std::clamp(k-7,exponent_floor,exponent_ceiling);
    // Rounding the largest component can still reach 128.
    while(exponent<exponent_ceiling && std::abs(mantissa(largest,exponent))>127)++exponent;
  }
  std::array<std::uint8_t,4> bits{std::uint8_t(std::int8_t(exponent))};
  const float components[3]{v.x,v.y,v.z};
  for(unsigned i=0;i<3;++i)
    bits[i+1]=std::uint8_t(std::int8_t(std::clamp<long>(mantissa(components[i],exponent),-127,127)));
  return bits;
}
Vec3 physics_impulse_vector_value(const std::array<std::uint8_t,4> &bits) {
  const int exponent=std::int8_t(bits[0]);
  if(exponent<exponent_floor || exponent>exponent_ceiling)throw FormatError("Physics impulse exponent outside its range");
  float components[3]{};
  for(unsigned i=0;i<3;++i) {
    const int value=std::int8_t(bits[i+1]);
    if(value==-128)throw FormatError("Physics impulse mantissa outside its range");
    components[i]=std::ldexp(float(value),exponent);
  }
  return {components[0],components[1],components[2]};
}
Vec3 physics_impulse_quantize(Vec3 v){return physics_impulse_vector_value(physics_impulse_vector_bits(v));}
void validate_physics_impulse_journal(std::span<const PhysicsImpulseEvent> rows,std::uint32_t tick) {
  if(rows.size()>physics_impulse_journal_capacity)throw FormatError("Physics impulse journal exceeds retention");
  for(std::size_t i=0;i<rows.size();++i) {
    const auto &e=rows[i];
    if(!e.serial || newer_serial(e.tick,tick) || tick-e.tick>physics_impulse_retention_ticks ||
        e.object==0xffffffffu || !(e.object>>16) || (e.object&0xffffu)>=2048u ||
        (i && (e.serial!=rows[i-1].serial+1 || newer_serial(rows[i-1].tick,e.tick))) ||
        !published(e.linear_velocity) || !published(e.angular_velocity))
      throw FormatError("Invalid physics impulse journal event");
    for(std::size_t j=0;j<i;++j)if(rows[j].object==e.object && rows[j].tick==e.tick)
      throw FormatError("Physics impulse journal repeats an object within one tick");
  }
}
void retain_physics_impulses(std::vector<PhysicsImpulseEvent> &journal,std::uint32_t &serial,
    std::span<const PhysicsImpulseEvent> fresh,std::uint32_t tick) {
  for(auto event:fresh) {
    if(!++serial)serial=1;
    // A wrapped serial restarts the contiguous journal.
    if(!journal.empty() && journal.back().serial+1!=serial)journal.clear();
    event.serial=serial;event.tick=tick;
    event.linear_velocity=physics_impulse_quantize(event.linear_velocity);
    event.angular_velocity=physics_impulse_quantize(event.angular_velocity);
    journal.push_back(event);
  }
  std::erase_if(journal,[&](const auto &e){return newer_serial(e.tick,tick) || tick-e.tick>physics_impulse_retention_ticks;});
  if(journal.size()>physics_impulse_journal_capacity)
    journal.erase(journal.begin(),journal.end()-std::ptrdiff_t(physics_impulse_journal_capacity));
}
}
