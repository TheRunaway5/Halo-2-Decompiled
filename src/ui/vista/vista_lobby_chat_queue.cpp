#include "halo2/vista_lobby_chat_queue.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || b.size()-at<4)throw FormatError("Vista lobby chat source field is truncated");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(i*8);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(i*8));
}
void source(std::span<const std::byte> b) {
  if(b.size()<vista_lobby_chat_queue_source_size)
    throw FormatError("Vista lobby chat requires its complete retained queue owner");
}
struct Ring {std::uint32_t write,read,count;};
Ring ring(std::span<const std::byte> b) {
  source(b);Ring r{get(b,0x1084),get(b,0x1088),get(b,0x108c)};
  if(r.write>=16 || r.read>=16 || r.count>16)
    throw FormatError("Vista lobby chat reached an unowned source ring index/count");
  return r;
}
bool muted(std::span<const std::byte> bank,std::uint32_t player,
    const VistaLobbyChatQueueServices& services) {
  if(get(bank,0x1c)!=2)return false;
  const auto at=0x125cu+player*0x128u;
  const auto xuid=std::uint64_t(get(bank,at))|(std::uint64_t(get(bank,at+4))<<32);
  if(!services.mute_list_query)
    throw FormatError("Vista lobby chat requires its consumed XUser mute-list service");
  std::uint32_t ignored{};const auto hr=services.mute_list_query(0,xuid,ignored);
  return hr!=0 || ignored!=0;
}
}
void vista_lobby_chat_queue_enable(std::span<std::byte> b) {
  source(b);b[0]=std::byte{1};
}
void vista_lobby_chat_queue_enqueue(std::span<std::byte> b,const VistaLobbyChatRecord& message) {
  auto r=ring(b);
  if(r.count==16){r.read=(r.read+1)%16;--r.count;put(b,0x1088,r.read);put(b,0x108c,r.count);}
  std::copy(message.begin(),message.end(),b.begin()+4+r.write*vista_lobby_chat_record_size);
  put(b,0x1084,(r.write+1)%16);put(b,0x108c,r.count+1);
}
bool vista_lobby_chat_queue_pop(std::span<std::byte> b,VistaLobbyChatRecord& output) {
  const auto r=ring(b);if(r.count==0)return false;
  std::copy_n(b.begin()+4+r.read*vista_lobby_chat_record_size,output.size(),output.begin());
  put(b,0x1088,(r.read+1)%16);put(b,0x108c,r.count-1);return true;
}
bool vista_lobby_chat_receive(VistaLobbyChatRecord& output,const VistaLobbyChatQueueServices& services) {
  if(!services.current_session)
    throw FormatError("Vista lobby chat requires its consumed current-session service");
  const auto first=services.current_session();if(!first)return false;
  const auto state=get(first->bank,0x73a0);
  // Original jump table5B1908 admits only3..8, with no output clear otherwise.
  if(state<3 || state>8)return false;
  auto queue=first->queue;
  while(ring(queue).count!=0) {
    const auto current=services.current_session();
    if(!current)throw FormatError("Vista lobby chat lost the required session while its queue is nonempty");
    if(!vista_lobby_chat_queue_pop(queue,output))
      throw FormatError("Vista lobby chat ring changed during its synchronous source dequeue");
    const auto bank=current->bank;
    if(get(bank,0x73a0)==0 || bank.size()<=0x48 || bank[0x48]==std::byte{})
      throw FormatError("Vista lobby chat reached the original null session identity boundary");
    //5C2054 supplies the actual session key ID at20h. The original compares
    // all8bytes before considering player IDs or platform mute state.
    if(!std::equal(output.begin(),output.begin()+8,bank.begin()+0x20))continue;
    const auto player=get(output,8);
    if(player==0xffffffffu)return true;
    if(std::bit_cast<std::int32_t>(player)<0 || player>=16)continue;
    if(!(get(bank,0x1258)&(1u<<player)) || get(bank,0x1264+player*0x128)==0xffffffffu)continue;
    if(muted(bank,player,services))continue;
    return true;
  }
  output.fill(std::byte{});return false;
}
} //namespace halo2
