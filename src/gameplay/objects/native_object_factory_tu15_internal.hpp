#pragma once
// Shared world-bank access for the native object factory owners. Every track
// source (native_object_factory_*_tu15.cpp) uses these helpers so loads and
// stores keep the original widths; stores go through memory.write() in the
// order the caller issues them (production passes a journaled world memory).
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>

namespace halo2::native_object_factory_tu15::detail {
using M=SourcePhysicsContactMemory;
inline constexpr std::uint32_t none=0xffffffffu;
inline constexpr std::uint32_t object_pool=0x4ecbd0u;     // object data array
inline constexpr std::uint32_t object_memory=0x4eabbcu;   // object memory pool
inline constexpr std::uint32_t tag_instances=0x4f0414u;   // 16-byte tag rows, data at +8
inline constexpr std::uint32_t one=0x3f800000u;           // [0x46A0D0] = 1.0f

inline std::uint32_t r32(const M& m,std::uint32_t a){std::array<std::byte,4> b{};m.read(a,b);
  return std::uint32_t(b[0])|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
inline std::uint16_t r16(const M& m,std::uint32_t a){std::array<std::byte,2> b{};m.read(a,b);
  return std::uint16_t(std::uint32_t(b[0])|std::uint32_t(b[1])<<8);}
inline std::uint8_t r8(const M& m,std::uint32_t a){std::array<std::byte,1> b{};m.read(a,b);return std::uint8_t(b[0]);}
inline void w32(M& m,std::uint32_t a,std::uint32_t v){const std::array b{std::byte(v),std::byte(v>>8),std::byte(v>>16),std::byte(v>>24)};m.write(a,b);}
inline void w16(M& m,std::uint32_t a,std::uint16_t v){const std::array b{std::byte(v),std::byte(v>>8)};m.write(a,b);}
inline void w8(M& m,std::uint32_t a,std::uint8_t v){const std::array b{std::byte(v)};m.write(a,b);}
inline std::int32_t s32(std::uint32_t v){return std::int32_t(v);}
inline std::uint32_t sx16(std::uint16_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t sx8(std::uint8_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline float f32(std::uint32_t bits){return std::bit_cast<float>(bits);}
inline std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
// Object row (12 bytes) and body; like the original, no salt check.
inline std::uint32_t object_row(const M& m,std::uint32_t object){return r32(m,r32(m,object_pool)+0x44u)+(object&0xffffu)*12u;}
inline std::uint32_t object_body(const M& m,std::uint32_t object){return r32(m,object_row(m,object)+8u);}
inline std::uint32_t tag_data(const M& m,std::uint32_t tag){return r32(m,(tag&0xffffu)*16u+r32(m,tag_instances)+8u);}
// REP STOSD then REP STOSB over `bytes` from `at`.
inline void zero_fill(M& m,std::uint32_t at,std::uint32_t bytes) {
  for(std::uint32_t n=0;n<bytes>>2;++n)w32(m,at+n*4u,0);
  for(std::uint32_t n=0;n<(bytes&3u);++n)w8(m,at+(bytes&~3u)+n,0);
}
// A path the native factory has not ported. It is a hard error, never a
// fall back into original code.
[[noreturn]] inline void refuse(const char* owner,const std::string& why) {
  throw std::runtime_error(std::string("Native object factory: ")+owner+" "+why+" is not native yet");
}
}
