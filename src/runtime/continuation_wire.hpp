#pragma once
#include "halo2/cache.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <span>
#include <string_view>

namespace halo2::continuation_wire {
inline void need(bool yes){if(!yes)throw FormatError("Invalid native continuation envelope or value");}
inline auto digest(std::span<const std::byte> bytes){
  std::array<std::byte,32> result{};
  need(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),
      reinterpret_cast<unsigned char *>(result.data()))!=nullptr);return result;
}
struct Writer {
  std::size_t limit;std::vector<std::byte> bytes;
  void raw(std::span<const std::byte> value){need(bytes.size()<=limit&&value.size()<=limit-bytes.size());bytes.insert(bytes.end(),value.begin(),value.end());}
  void u(std::uint64_t value,unsigned width){need(width<=8);std::array<std::byte,8> b{};
    for(unsigned i=0;i<width;++i)b[i]=std::byte(value>>(8*i));raw(std::span(b).first(width));}
  void boolean(bool value){u(value,1);}
  void i32(std::int32_t value){u(std::bit_cast<std::uint32_t>(value),4);}
  void real(float value){need(std::isfinite(value));u(std::bit_cast<std::uint32_t>(value),4);}
  template<class T,std::size_t N>void octets(const std::array<T,N> &value){static_assert(sizeof(T)==1);raw(std::as_bytes(std::span(value)));}
  void blob(std::span<const std::byte> value,std::size_t bound){need(value.size()<=bound);u(value.size(),4);raw(value);}
  void string(std::string_view value){blob(std::as_bytes(std::span(value)),1024);}
};
struct Reader {
  std::span<const std::byte> bytes;std::size_t at{};
  auto raw(std::size_t count){need(at<=bytes.size()&&count<=bytes.size()-at);auto result=bytes.subspan(at,count);at+=count;return result;}
  std::uint64_t u(unsigned width){need(width<=8);const auto b=raw(width);std::uint64_t result{};
    for(unsigned i=0;i<width;++i)result|=std::uint64_t(std::to_integer<unsigned>(b[i]))<<(i*8);return result;}
  bool boolean(){const auto value=u(1);need(value<=1);return bool(value);}
  std::int32_t i32(){return std::bit_cast<std::int32_t>(std::uint32_t(u(4)));}
  float real(){const auto value=std::bit_cast<float>(std::uint32_t(u(4)));need(std::isfinite(value));return value;}
  template<class T,std::size_t N>void octets(std::array<T,N> &value){static_assert(sizeof(T)==1);
    const auto b=raw(N);std::copy(b.begin(),b.end(),std::as_writable_bytes(std::span(value)).begin());}
  auto blob(std::size_t bound){const auto count=u(4);need(count<=bound);return raw(std::size_t(count));}
  std::string string(){const auto b=blob(1024);return {reinterpret_cast<const char *>(b.data()),b.size()};}
  void end(){need(at==bytes.size());}
};
inline std::vector<std::byte> frame(std::string_view magic,std::span<const std::byte> body,std::size_t limit){
  need(magic.size()==8&&body.size()<=UINT32_MAX);Writer w{limit,{}};
  w.raw(std::as_bytes(std::span(magic)));w.u(body.size(),4);w.octets(digest(body));w.raw(body);return std::move(w.bytes);
}
inline auto unframe(std::span<const std::byte> bytes,std::string_view magic,std::size_t limit){
  need(magic.size()==8&&bytes.size()>=44&&bytes.size()<=limit);Reader r{bytes};
  need(std::ranges::equal(r.raw(8),std::as_bytes(std::span(magic))));const auto count=r.u(4);
  std::array<std::byte,32> hash{};r.octets(hash);need(count==bytes.size()-44);
  const auto body=r.raw(std::size_t(count));need(hash==digest(body));r.end();return body;
}
}
