#include "halo2/vista_session_local_peer.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t o) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(8*i);return v;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
void validate(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)throw FormatError("Vista local peer requires the complete source session owner");
}
std::int32_t signed_word(std::uint32_t v) {return std::bit_cast<std::int32_t>(v);}
std::int32_t trim_count(unsigned count,std::uint32_t scale) {
  const float product=float(count)*std::bit_cast<float>(scale);
  const auto bits=std::bit_cast<std::uint32_t>(product);
  const auto exponent=(bits>>23)&255;
  const auto magnitude=((bits&0x7fffffu)|0x800000u)>>((150u-exponent)&31u);
  if(exponent<127)return 0;
  return signed_word((bits>>31)?0u-magnitude:magnitude);
}
void string(std::span<std::byte> out,std::size_t offset,unsigned capacity,std::u16string_view text) {
  const auto end=text.find(u'\0');const auto count=std::min<std::size_t>(capacity-1,end==text.npos?text.size():end);
  for(unsigned i=0;i<count;++i)put(out,offset+i*2,text[i],2);
}
}
std::array<std::uint32_t,4> vista_session_peer_connectivity(
    std::span<const std::byte> b,const VistaSessionConnectivityValues& actual) {
  validate(b);std::array<std::uint32_t,4> result{};
  const auto count=signed_word(get(b,0x84));const auto local=get(b,0x71c0);
  if(count<=0)return result;
  if(count>17)throw FormatError("Vista connectivity membership exceeds its retained association bank");
  std::array<std::int32_t,17> samples{};unsigned sample_count{};
  for(unsigned i=0;i<unsigned(count);++i) {
    if(i!=local) {
      const auto association=0x71c4+i*0x1c;
      if(b[association+1]==std::byte{})continue;
      const auto channel=get(b,association+4);if(channel==0xffffffffu)continue;
      const auto offset=std::uint64_t(channel)*0x740+0x80;
      if(offset+0x84>actual.channel_owner.size())throw FormatError("Vista connectivity channel is not owned by the supplied transport bank");
      if(get(actual.channel_owner,offset)!=7)continue;
      if((std::to_integer<unsigned>(actual.channel_owner[offset+8])&0x10)!=0)
        samples[sample_count++]=std::clamp(signed_word(get(actual.channel_owner,offset+0x80)),50,2000);
    }
    result[0]|=1u<<i;
  }
  if(sample_count) {
    if(!actual.statistic_scale_bits)throw FormatError("Vista connectivity requires its actual statistic scale owner value");
    const auto values=std::span(samples).first(sample_count);
    result[1]=std::uint32_t(*std::min_element(values.begin(),values.end()));
    result[3]=std::uint32_t(*std::max_element(values.begin(),values.end()));
    const auto trim=trim_count(sample_count,*actual.statistic_scale_bits);
    auto selected=values;
    if(trim>0) {
      if(std::uint32_t(trim)*2>=sample_count)throw FormatError("Vista connectivity statistic trim leaves no owned samples");
      std::sort(values.begin(),values.end());selected=values.subspan(trim,sample_count-2*trim);
    }
    std::int32_t sum{};for(const auto v:selected)sum+=v;
    result[2]=std::uint32_t(sum/std::int32_t(selected.size()));
  }
  return result;
}
namespace {
VistaSessionLocalPeerResult publish_local_peer(
    std::span<std::byte> source,std::uint32_t& last,std::int32_t threshold,
    const VistaSessionLocalPeerValues& actual,const VistaSessionLocalPeerClocks* clocks,
    VistaSessionLocalPeerServices* services) {
  validate(source);auto b=source.first(vista_session_source_size);VistaSessionLocalPeerResult result;
  if(last) {
    if(clocks && !clocks->elapsed)throw FormatError("Vista local peer throttle requires its actual elapsed-clock sample");
    result.elapsed_clock_consumed=true;
    const auto elapsed=clocks?*clocks->elapsed:services->network_clock();
    if(signed_word(elapsed-last)<threshold)return result;
  }
  const auto local=get(b,0x71c0);
  if(local>=17)throw FormatError("Vista local peer record is outside its retained membership bank");
  const auto connectivity=vista_session_peer_connectivity(b,actual.connectivity);
  result.built=true;string(result.payload,0,16,actual.name);string(result.payload,0x20,32,actual.party);
  put(result.payload,0x60,actual.map_status);put(result.payload,0x64,actual.map_progress);
  if(actual.optional_metadata_byte) {
    put(result.payload,0x68,actual.metadata68);put(result.payload,0x70,actual.metadata70);
  }
  put(result.payload,0x6c,actual.bandwidth);
  for(unsigned i=0;i<4;++i)put(result.payload,0x74+i*4,connectivity[i]);
  const auto record=0x88+local*0x10c;
  if(b[record+0x24]!=std::byte{} && std::equal(result.payload.begin(),result.payload.end(),b.begin()+record+0x28))return result;
  result.publication_attempted=true;const auto state=get(b,0x73a0);
  if(state<3 || state>8)return result;
  if(clocks && !clocks->publication)throw FormatError("Vista local peer accepted publication requires its actual network clock sample");
  if(state>=5) {
    b[record+0x24]=std::byte{1};std::copy(result.payload.begin(),result.payload.end(),b.begin()+record+0x28);
    put(b,0x70,get(b,0x70)+1);put(b,0x7930,get(b,0x7930)+1);
  } else {
    const auto remote=get(b,0x64);
    if(remote>=17)throw FormatError("Vista remote peer publication has no retained target association");
    const auto association=0x71c4+remote*0x1c;
    if(b[association+1]!=std::byte{}) {
      VistaSessionLocalPeerMessage message{};message.transport_owner=get(b,8);
      message.session_slot=get(b,0x14);message.channel=get(b,association+4);
      std::copy_n(b.begin()+0x20,8,message.payload.begin());
      std::copy(result.payload.begin(),result.payload.end(),message.payload.begin()+8);
      result.message=message;
    }
  }
  if(services && result.message)services->send_peer_message(*result.message);
  last=clocks?*clocks->publication:services->network_clock();
  result.accepted=true;result.publication_clock_consumed=true;return result;
}
} // namespace
VistaSessionLocalPeerResult vista_session_publish_local_peer(
    std::span<std::byte> source,std::uint32_t& last,std::int32_t threshold,
    const VistaSessionLocalPeerValues& actual,const VistaSessionLocalPeerClocks& clocks) {
  return publish_local_peer(source,last,threshold,actual,&clocks,nullptr);
}
VistaSessionLocalPeerResult vista_session_publish_local_peer(
    std::span<std::byte> source,std::uint32_t& last,std::int32_t threshold,
    const VistaSessionLocalPeerValues& actual,VistaSessionLocalPeerServices& services) {
  return publish_local_peer(source,last,threshold,actual,nullptr,&services);
}
} // namespace halo2
