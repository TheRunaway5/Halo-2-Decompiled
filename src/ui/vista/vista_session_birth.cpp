#include "halo2/vista_session_birth.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <initializer_list>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> bank,std::size_t offset) noexcept {
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(bank[offset+i]))<<(i*8);
  return value;
}
void put(std::span<std::byte> bank,std::size_t offset,std::uint32_t value,unsigned size=4) noexcept {
  for(unsigned i=0;i<size;++i)bank[offset+i]=std::byte(value>>(i*8));
}
void clear(std::span<std::byte> bank,std::size_t offset,std::size_t size) noexcept {
  std::fill_n(bank.begin()+offset,size,std::byte{});
}
template<std::size_t N> void copy(std::span<std::byte> bank,std::size_t offset,
    const std::array<std::byte,N>& source) noexcept {
  std::copy(source.begin(),source.end(),bank.begin()+offset);
}
void call(VistaSessionBirthResult& result,VistaSessionBirthService service,
    std::initializer_list<std::uint32_t> arguments={},std::uint32_t owner=0) {
  auto& out=result.calls[result.call_count++];out.service=service;out.owner=owner;
  out.argument_count=unsigned(arguments.size());
  std::copy(arguments.begin(),arguments.end(),out.arguments.begin());
}
}

VistaSessionBirthResult vista_session_local_birth(std::span<std::byte> source,
    const VistaSessionBirthQueries& query) {
  if(source.size()<vista_session_source_size)
    throw FormatError("Vista local session owner bank is truncated");
  VistaSessionBirthResult result;
  if(get(source,0x73a0)!=0)return result; // Actual 5AD7D0 guard.
  if(source[0x48]!=std::byte{} || get(source,0x5c)!=0)
    throw FormatError("Vista local birth requires the proved primary quiescent transport");
  if(query.local_machine_available && query.network_created && query.session_key_available && !query.values)
    throw FormatError("Vista local birth consumed source service values are missing");
  auto bank=source.first(vista_session_source_size);
  // All missing/unsupported facts are rejected before native mutation. The
  // ordinary original failures below are supported and retain this write.
  clear(bank,0x28,0x20);
  call(result,VistaSessionBirthService::local_machine,{1,0});
  if(!query.local_machine_available)return result;
  call(result,VistaSessionBirthService::network_create,{get(bank,0x5c),1,0});
  if(!query.network_created)return result;
  call(result,VistaSessionBirthService::network_key,{get(bank,0x5c),1,1});
  if(!query.session_key_available)return result;
  const auto& values=*query.values;
  put(bank,0x48,1,1);copy(bank,0x20,values.session_key_id);copy(bank,0x49,values.session_key);put(bank,0x60,1);
  call(result,VistaSessionBirthService::network_register,{get(bank,0x14),get(bank,0x5c),1},get(bank,8));
  call(result,VistaSessionBirthService::network_qos_listen);

  put(bank,0x1c,1);clear(bank,0x4c60,0x12b0);
  put(bank,0x4c68,1);
  call(result,VistaSessionBirthService::network_clock);put(bank,0x4c6c,values.network_clocks[0]);
  call(result,VistaSessionBirthService::language);put(bank,0x4c70,values.language);
  put(bank,0x4c7c,0x10);put(bank,0x4c80,0x10);put(bank,0x4cb8,0xffffffff);
  put(bank,0x5100,1,1);put(bank,0x4dd8,0xffffffff);
  call(result,VistaSessionBirthService::random_bytes,{8});copy(bank,0x4dc8,values.random);
  put(bank,0x4dd0,0xdeadbeef);put(bank,0x4dd4,1);put(bank,0x5eb8,0xffff,2);
  put(bank,0x4ca4,0xffffffff);put(bank,0x5f08,0xffffffff);put(bank,0x5f0c,0xffffffff);

  clear(bank,0x70,0x2470);put(bank,0x6c,0,1);put(bank,0x64,0);put(bank,0x71c0,0);
  // Actual first 5C35C9 insertion: count increments from the cleared bank.
  put(bank,0x84,1);copy(bank,0x88,values.local_machine);
  for(unsigned offset=0x184;offset<0x194;offset+=4)put(bank,offset,0xffffffff);
  // Actual 5C34B3(index0, byte0, channelFFFFFFFF) association.
  put(bank,0x71c4,1);put(bank,0x71c8,0xffffffff);
  for(unsigned offset=0x71cc;offset<0x71dc;offset+=4)put(bank,offset,0xffffffff);
  call(result,VistaSessionBirthService::network_clock);put(bank,0x71dc,values.network_clocks[1]);
  put(bank,0x7930,get(bank,0x7930)+1);

  clear(bank,0x24e0,0x2470);put(bank,0x24e0,0xffffffff);
  clear(bank,0x5f10,0x12b0);put(bank,0x5f10,0xffffffff);
  put(bank,0x4950,0);clear(bank,0x4a18,0xc8);put(bank,0x4a18,0xffffffff);
  put(bank,0x7934,0);put(bank,0x4ae0,0);clear(bank,0x4b80,0xa0);put(bank,0x4b80,0xffffffff);put(bank,0x7938,0);
  call(result,VistaSessionBirthService::network_clock);
  put(bank,0x7b7c,1,1);put(bank,0x7b80,0u-values.network_clocks[2]);
  call(result,VistaSessionBirthService::process_clock);put(bank,0x7b84,values.process_clock);

  // 5C79E1 enters from state0: its complete 5C4E7F(1) reset is included.
  clear(bank,0x7978,0x200);put(bank,0x796c,0,1);put(bank,0x68,0xffffffff);
  put(bank,0x71c6,0,1);put(bank,0x71c7,0,1);put(bank,0x4c60,1);
  clear(bank,0x73a4,0x58c);put(bank,0x73a0,5);
  result.accepted=true;
  call(result,VistaSessionBirthService::publish_peer_payload);
  return result;
}

void vista_session_lifecycle_initialize(std::span<std::byte> bank,
    const std::array<std::uint32_t,5>& owners) {
  if(bank.size()<0x44)throw FormatError("Vista lifecycle owner bank is truncated");
  clear(bank,4,0x1c);
  put(bank,0x2c,owners[0]);put(bank,0x30,owners[1]);put(bank,0x20,owners[2]);
  put(bank,0x24,owners[3]);put(bank,0x28,owners[4]);put(bank,0,0);
  put(bank,0x35,0,1);put(bank,0x34,0,1);put(bank,0x36,0,1);
}
} // namespace halo2
