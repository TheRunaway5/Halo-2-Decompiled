#include "halo2/vista_local_user.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
template<class Byte> void require_bank(std::span<Byte> bank,std::size_t size) {
  if(bank.size()<size)throw FormatError("Vista local-user owner bank is truncated");
}
void require_slot(std::uint32_t slot) {
  if(slot>=vista_local_user_count)
    throw FormatError("Vista local-user slot is outside the bounded source bank");
}
template<class Byte> auto record(std::span<Byte> bank,std::uint32_t slot) {
  require_bank(bank,vista_local_user_bank_size);require_slot(slot);
  return bank.subspan(slot*vista_local_user_stride,vista_local_user_stride);
}
std::uint32_t get(std::span<const std::byte> bank,std::size_t offset) noexcept {
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)
    value|=std::uint32_t(std::to_integer<unsigned char>(bank[offset+i]))<<(i*8);
  return value;
}
void put(std::span<std::byte> bank,std::size_t offset,std::uint32_t value) noexcept {
  for(unsigned i=0;i<4;++i)bank[offset+i]=std::byte(value>>(i*8));
}
struct OutputRange {const void* address;std::size_t size;};
bool overlaps(OutputRange left,OutputRange right) noexcept {
  if(!left.address || !right.address)return false;
  const auto a=reinterpret_cast<std::uintptr_t>(left.address);
  const auto b=reinterpret_cast<std::uintptr_t>(right.address);
  return a<=b ? b-a<left.size : a-b<right.size;
}
template<std::size_t N> void require_detached(std::span<const std::byte> bank,
    const std::array<OutputRange,N>& outputs) {
  const OutputRange source{bank.data(),bank.size()};
  for(std::size_t i=0;i<N;++i) {
    if(overlaps(source,outputs[i]))
      throw FormatError("Vista local-user output aliases the source owner bank");
    for(std::size_t j=0;j<i;++j)if(overlaps(outputs[i],outputs[j]))
      throw FormatError("Vista local-user outputs overlap");
  }
}
}

std::uint32_t vista_local_user_allocate(std::span<std::byte> bank,
    VistaLocalUserIdentity identity) {
  require_bank(bank,vista_local_user_bank_size);
  std::uint32_t slot{};
  while(slot<vista_local_user_count &&
      bank[slot*vista_local_user_stride]!=std::byte{})++slot;
  if(slot==vista_local_user_count)
    throw FormatError("Vista full local-user allocation would underflow the original bank");
  auto user=record(bank,slot);
  std::fill(user.begin(),user.end(),std::byte{});
  user[0]=std::byte{1};std::copy(identity.begin(),identity.end(),user.begin()+1);
  return slot;
}

void vista_local_user_remove(std::span<std::byte> bank,std::uint32_t slot) {
  auto user=record(bank,slot);std::fill(user.begin(),user.end(),std::byte{});
}

bool vista_local_user_read_identity(std::span<const std::byte> bank,
    std::uint32_t slot,VistaLocalUserIdentity& output) {
  const auto user=record(bank,slot);
  if(user[0]==std::byte{})return false;
  require_detached(bank,std::array{OutputRange{output.data(),output.size()}});
  std::copy_n(user.begin()+1,output.size(),output.begin());return true;
}

bool vista_local_user_read_payload(std::span<const std::byte> bank,
    std::uint32_t slot,VistaLocalUserPayloadOutputs outputs) {
  const auto user=record(bank,slot);
  if(user[0]==std::byte{})return false;
  require_detached(bank,std::array{
      OutputRange{outputs.controller_word,sizeof(std::uint32_t)},
      OutputRange{outputs.payload,sizeof(VistaLocalUserPayload)},
      OutputRange{outputs.first_trailer_word,sizeof(std::uint32_t)},
      OutputRange{outputs.second_trailer_word,sizeof(std::uint32_t)}});
  if(outputs.controller_word)*outputs.controller_word=get(user,0xc);
  if(outputs.payload)std::copy_n(user.begin()+0x10,outputs.payload->size(),outputs.payload->begin());
  if(outputs.first_trailer_word)*outputs.first_trailer_word=get(user,0x94);
  if(outputs.second_trailer_word)*outputs.second_trailer_word=get(user,0x98);
  return true;
}

void vista_local_user_write_identity(std::span<std::byte> bank,
    std::uint32_t slot,VistaLocalUserIdentity identity) {
  auto user=record(bank,slot);std::copy(identity.begin(),identity.end(),user.begin()+1);
}

void vista_local_user_write_payload(std::span<std::byte> bank,
    std::uint32_t slot,std::uint32_t controller,VistaLocalUserPayload payload,
    std::uint32_t first,std::uint32_t second) {
  auto user=record(bank,slot);put(user,0xc,controller);
  std::copy(payload.begin(),payload.end(),user.begin()+0x10);
  put(user,0x94,first);put(user,0x98,second);
}

std::uint32_t vista_controller_local_user(std::span<const std::byte> bank,
    std::uint32_t slot) {
  require_bank(bank,vista_controller_user_bank_size);require_slot(slot);
  return get(bank,slot*vista_controller_user_stride+4);
}
} // namespace halo2
