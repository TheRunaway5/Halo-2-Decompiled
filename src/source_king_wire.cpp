#include "halo2/source_king_wire.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace halo2 {
SourceEngineBitstream::SourceEngineBitstream(SourceEngineBitstreamState& value,std::span<std::uint8_t> storage)
    :state(value),backing(storage) {
  // A live original stream has nonnegative sizes/cursors, backed rounded
  // dwords, and a guard byte readable at exactly capacity. Missing backing is
  // an unavailable input contract, not a substituted original read result.
  if(state.capacity_bytes<0 || state.capacity_bytes>std::numeric_limits<std::int32_t>::max()/8 ||
      state.position<0 || state.position>std::numeric_limits<std::int32_t>::max()-256 ||
      backing.size()<(std::size_t(state.capacity_bytes)+3u)/4u*4u+4u)
    throw std::runtime_error("Original engine bitstream requires nonnegative bounds and explicit word padding");
}
bool SourceEngineBitstream::overflowed()const noexcept {
  return state.position>state.capacity_bytes*8 || (state.error_and_padding&255u)!=0;
}
namespace {
std::uint32_t word(const SourceEngineBitstream& stream,unsigned at) {
  if(at>stream.backing.size() || stream.backing.size()-at<4)
    throw std::runtime_error("Original engine bitstream word is unavailable");
  std::uint32_t value=0;
  for(unsigned i=0;i<4;++i)value|=std::uint32_t(stream.backing[at+i])<<(i*8);
  return value;
}
void word(SourceEngineBitstream& stream,unsigned at,std::uint32_t value) {
  if(at>stream.backing.size() || stream.backing.size()-at<4)
    throw std::runtime_error("Original engine bitstream word is unavailable");
  for(unsigned i=0;i<4;++i)stream.backing[at+i]=std::uint8_t(value>>(i*8));
}
std::uint32_t mask(unsigned bits){return bits==32 ? 0xffffffffu : (1u<<bits)-1u;}
void write(SourceEngineBitstream& stream,std::uint32_t value,int width) {
  const auto position=stream.state.position;
  const auto available=std::min(width,stream.state.capacity_bytes*8-position);
  if(available>0) {
    const auto at=unsigned(position/32)*4u,shift=unsigned(position)&31u;
    value&=mask(unsigned(available));
    word(stream,at,word(stream,at)|(value<<shift));
    // Original195720 assigns the whole second word, rather than ORing it.
    if(position/32!=(position+available-1)/32)word(stream,at+4,value>>(32u-shift));
  }
  stream.state.position+=width;
}
std::uint32_t read(SourceEngineBitstream& stream,int width) {
  const auto position=stream.state.position;
  const auto available=std::min(width,stream.state.capacity_bytes*8-position);
  std::uint32_t value=0;
  if(available>0) {
    const auto at=unsigned(position/32)*4u,shift=unsigned(position)&31u;
    value=word(stream,at)>>shift;
    if(position/32!=(position+available-1)/32)value|=word(stream,at+4)<<(32u-shift);
    value&=mask(unsigned(available));
  }
  stream.state.position+=width;return value;
}
bool read_bit(SourceEngineBitstream& stream) {
  const auto position=stream.state.position;bool value=false;
  if(position<=stream.state.capacity_bytes*8)
    value=(stream.backing[unsigned(position)/8u]&(1u<<(unsigned(position)&7u)))!=0;
  ++stream.state.position;return value;
}
void write_bit(SourceEngineBitstream& stream,bool value) {
  const auto position=stream.state.position;
  if(stream.state.capacity_bytes*8-position>=1 && value)
    stream.backing[unsigned(position)/8u]|=std::uint8_t(1u<<(unsigned(position)&7u));
  ++stream.state.position;
}
void discard(SourceEngineBitstream& stream) {
  auto& state=stream.state;
  if(state.checkpoint_count<1 || state.checkpoint_count>4)
    throw std::runtime_error("Original engine stream checkpoint is unavailable");
  const auto checkpoint=state.checkpoints[unsigned(--state.checkpoint_count)];
  if(state.mode==1 && checkpoint<state.position) {
    state.discarded_bits=state.position-checkpoint;++state.discard_count;
    const auto at=checkpoint/8;
    if(at<state.capacity_bytes)stream.backing[unsigned(at)]&=std::uint8_t((1u<<(unsigned(checkpoint)&7u))-1u);
    // Original rollback clears the entire remaining declared span, including
    // preexisting future bits, while leaving dword padding untouched.
    for(auto i=at+1;i<state.capacity_bytes;++i)stream.backing[unsigned(i)]=0;
  }
  state.position=checkpoint;
}
struct FlagsWriter {
  SourceEngineBitstream& stream;
  std::uint32_t requested{},written{};
  std::int32_t reserve{};
  bool space{};
  unsigned index{};
  FlagsWriter(SourceEngineBitstream& value,std::uint32_t flags,int count,int reserved)
      :stream(value),requested(flags),reserve(reserved+count),
       space(value.state.capacity_bytes*8-value.state.position>=reserve) {}
  bool begin(unsigned bit) {
    index=bit;auto& state=stream.state;
    if(state.checkpoint_count<0 || state.checkpoint_count>=4)
      throw std::runtime_error("Original engine stream checkpoint capacity unavailable");
    state.checkpoints[unsigned(state.checkpoint_count++)]=state.position;
    if(!(requested&(1u<<bit)))return false;
    write_bit(stream,true);written|=1u<<bit;return true;
  }
  void end() {
    const auto flag=1u<<index;
    if(written&flag) {
      if(stream.state.capacity_bytes*8-stream.state.position<reserve) {
        written&=~flag;discard(stream);
      }else --stream.state.checkpoint_count;
    }else --stream.state.checkpoint_count;
    if(!(written&flag))++stream.state.position;
    --reserve;
  }
};
std::int32_t signed_word(std::uint16_t value){return std::bit_cast<std::int16_t>(value);}
void reserve_supported(std::int32_t value) {
  if(value<0 || value>std::numeric_limits<std::int32_t>::max()-7)
    throw std::runtime_error("Original engine wire reservation is unavailable");
}
}
bool source_engine_wire_write(SourceEngineBitstream& stream,std::uint32_t requested,
    std::uint32_t& written,const SourceEngineReplicationBank& bank,std::int32_t reserved) {
  reserve_supported(reserved);FlagsWriter fields(stream,requested,5,reserved);
  if(!fields.space)return false;
  if(fields.begin(0)) {
    constexpr int widths[]={8,9,8,8,8};
    for(unsigned i=0;i<5;++i)write(stream,bank.words[i],widths[i]);
    for(unsigned i=0;i<9;++i)if(bank.words[1]&(1u<<i))write(stream,std::uint32_t(signed_word(bank.words[i+5])),4);
  }
  fields.end();
  if(fields.begin(1))write(stream,bank.phase,2);
  fields.end();
  if(fields.begin(2))write_bit(stream,bank.game_over!=0);
  fields.end();
  if(fields.begin(3))write(stream,std::uint32_t(signed_word(bank.round)),5);
  fields.end();
  if(fields.begin(4))write(stream,std::uint32_t(signed_word(bank.clock_seconds)+1),16);
  fields.end();written|=fields.written;return true;
}
bool source_engine_wire_read(SourceEngineBitstream& stream,SourceEngineReplicationBank& bank,std::uint32_t& flags) {
  bool valid=true;std::uint32_t changed=0;
  if(read_bit(stream)) {
    constexpr int widths[]={8,9,8,8,8};
    for(unsigned i=0;i<5;++i)bank.words[i]=std::uint16_t(read(stream,widths[i]));
    valid=!(bank.words[2]&~bank.words[0]) && !(bank.words[4]&~bank.words[2]) && !(bank.words[3]&~bank.words[4]);
    for(unsigned i=0;i<9 && valid;++i) {
      if(bank.words[1]&(1u<<i)) {bank.words[i+5]=std::uint16_t(read(stream,4));valid=bank.words[i+5]<8;}
      else bank.words[i+5]=0xffff;
    }
    changed=1;
  }
  if(read_bit(stream)){bank.phase=std::uint8_t(read(stream,2));changed|=2;}
  if(read_bit(stream)){bank.game_over=std::uint8_t(read_bit(stream));changed|=4;}
  if(read_bit(stream)){bank.round=std::uint16_t(read(stream,5));changed|=8;}
  if(read_bit(stream)){bank.clock_seconds=std::uint16_t(read(stream,16)-1u);changed|=0x10;}
  flags|=changed;return valid;
}
bool source_king_wire_write(SourceEngineBitstream& stream,std::uint32_t requested,
    std::uint32_t& written,const SourceKingReplicationBank& bank,std::int32_t reserved) {
  reserve_supported(reserved);
  if(!source_engine_wire_write(stream,requested&0x1f,written,bank.shared,reserved+2))return false;
  FlagsWriter fields(stream,requested&0x60,2,reserved);
  if(!fields.space)return false;
  if(fields.begin(5))write(stream,std::uint32_t(std::int32_t(bank.hill)+1),4);
  fields.end();
  if(fields.begin(6))write(stream,bank.occupants,16);
  fields.end();written|=fields.written;return true;
}
bool source_king_wire_read(SourceEngineBitstream& stream,SourceKingReplicationBank& bank,std::uint32_t& flags) {
  std::uint32_t changed=0;const bool valid=source_engine_wire_read(stream,bank.shared,changed);
  if(read_bit(stream)){bank.hill=std::bit_cast<std::int16_t>(std::uint16_t(read(stream,4)-1u));changed|=0x20;}
  if(read_bit(stream)){bank.occupants=std::uint16_t(read(stream,16));changed|=0x40;}
  flags=changed;return valid && changed!=0;
}
std::int32_t source_engine_wire_minimum_bits(std::uint32_t requested) noexcept {
  auto result=std::numeric_limits<std::int32_t>::max();
  constexpr int widths[]={74,7,6,10,21};
  for(unsigned i=0;i<5;++i)if(requested&(1u<<i))result=std::min(result,widths[i]);
  return result==std::numeric_limits<std::int32_t>::max() ? 0 : result;
}
std::int32_t source_king_wire_minimum_bits(std::uint32_t requested) noexcept {
  auto result=source_engine_wire_minimum_bits(requested)+2;
  if(requested&0x20)result=std::min(result,11);
  if(requested&0x40)result=std::min(result,23);
  return result;
}
} // namespace halo2
