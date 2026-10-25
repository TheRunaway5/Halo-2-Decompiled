#include "halo2/snapshot_stream.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <cstring>
namespace halo2 {
namespace {
auto slice(std::span<const std::byte> p){return rust::Slice<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.data()),p.size());}
template<class T> auto words(std::span<const T> p){return rust::Slice<const T>(p.data(),p.size());}
std::vector<std::byte> copy(const rust::Vec<std::uint8_t> &p){
  if(p.empty())return {};
  const auto *first=reinterpret_cast<const std::byte*>(p.data());return {first,first+p.size()};
}
SnapshotStreamDigest digest(const std::array<std::uint8_t,32> &value){
  SnapshotStreamDigest result;std::memcpy(result.data(),value.data(),result.size());return result;
}
}
struct SnapshotStreamSender::Impl {rust::Box<online_protocol_bridge::SnapshotSender> sender;};
SnapshotStreamSender::SnapshotStreamSender(std::size_t chunk,std::size_t overhead)
    :impl_(std::make_unique<Impl>(Impl{online_protocol_bridge::new_snapshot_sender(chunk,overhead)})){}
SnapshotStreamSender::~SnapshotStreamSender()=default;
std::optional<SnapshotStreamSender::Emission> SnapshotStreamSender::encode(const Frame &frame){
  if(frame.anchor_offsets.size()!=frame.anchor_keys.size())return std::nullopt;
  const auto poses=frame.poses.value_or(std::pair<std::size_t,std::size_t>{});
  online_protocol_bridge::SnapshotFrameInfo info{frame.poses.has_value(),poses.first,poses.second,frame.tick,
      frame.sequence,frame.now_ms,{},{},{}};
  for(const auto value:frame.viewers)info.viewers.push_back(value);
  for(const auto value:frame.own)info.own.push_back(value);
  for(const auto value:frame.hot)info.hot.push_back(value);
  try{
    const auto value=online_protocol_bridge::snapshot_sender_encode(*impl_->sender,slice(frame.body),words(frame.floats),
        words(frame.anchor_offsets),words(frame.anchor_keys),info);
    Emission result;result.emitted=value.emitted;result.keyframe=value.keyframe;
    result.digest=digest(value.digest);result.view_bytes=value.view_len;
    result.packets.reserve(value.packets.size());
    for(const auto &packet:value.packets)result.packets.push_back(copy(packet.data));
    return result;
  }catch(const rust::Error &){return std::nullopt;}
}
void SnapshotStreamSender::ack(const SnapshotStreamAck &ack,std::uint64_t now_ms){
  online_protocol_bridge::snapshot_sender_ack(*impl_->sender,ack.latest,ack.mask,ack.flags,now_ms);
}
SnapshotStreamSender::Stats SnapshotStreamSender::stats()const{
  const auto s=online_protocol_bridge::snapshot_sender_stats(*impl_->sender);
  return {s.messages,s.wire_bytes,s.packets,s.keyframes,s.resets,s.over_budget,s.full_refreshes,s.root_updates,
      s.forced_refreshes,s.held_refreshes,s.lost_messages,s.budget_bps,s.rtt_ms,s.loss_percent,s.retired};
}
struct SnapshotStreamReceiver::Impl {rust::Box<online_protocol_bridge::SnapshotReceiver> receiver;};
SnapshotStreamReceiver::SnapshotStreamReceiver(std::size_t chunk)
    :impl_(std::make_unique<Impl>(Impl{online_protocol_bridge::new_snapshot_receiver(chunk)})){}
SnapshotStreamReceiver::~SnapshotStreamReceiver()=default;
std::optional<SnapshotStreamReceiver::Delivery> SnapshotStreamReceiver::push(std::span<const std::byte> packet,
    std::uint64_t now_ms,bool &failed){
  auto value=online_protocol_bridge::snapshot_receiver_push(*impl_->receiver,slice(packet),now_ms);
  failed=value.failed;
  if(!value.complete)return std::nullopt;
  return Delivery{value.sequence,value.tick,value.view_len,digest(value.digest),copy(value.body)};
}
bool SnapshotStreamReceiver::commit(std::uint32_t sequence){
  return online_protocol_bridge::snapshot_receiver_commit(*impl_->receiver,sequence);
}
void SnapshotStreamReceiver::reject(std::uint32_t sequence){
  online_protocol_bridge::snapshot_receiver_reject(*impl_->receiver,sequence);
}
SnapshotStreamAck SnapshotStreamReceiver::ack()const{
  const auto value=online_protocol_bridge::snapshot_receiver_ack(*impl_->receiver);
  return {value.latest,value.mask,value.flags};
}
void SnapshotStreamReceiver::reset(){online_protocol_bridge::snapshot_receiver_reset(*impl_->receiver);}
std::size_t SnapshotStreamReceiver::buffered_bytes()const{return online_protocol_bridge::snapshot_receiver_buffered(*impl_->receiver);}
SnapshotStreamReceiver::Stats SnapshotStreamReceiver::stats()const{
  const auto s=online_protocol_bridge::snapshot_receiver_stats(*impl_->receiver);
  return {s.messages,s.failures,s.held_poses,s.missing_poses};
}
}
