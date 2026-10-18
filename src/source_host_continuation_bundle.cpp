#include "halo2/source_host_continuation_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include "continuation_wire.hpp"
#include <limits>

namespace halo2 {
struct SourceHostContinuationAccess {
  static MultiplayerMatch &match(LanHost &host){return host.continuation_match();}
};
namespace {
using namespace continuation_wire;
constexpr std::string_view bundle_magic="H2HSC001";
constexpr std::string_view replay_magic="H2HRP001";
bool starts(std::span<const std::byte> bytes,std::string_view magic)noexcept{
  return bytes.size()>=magic.size()&&std::ranges::equal(bytes.first(magic.size()),std::as_bytes(std::span(magic)));
}
void context(Writer &w,const AuthorityTransferContext &c){
  w.octets(c.session);w.u(c.epoch,4);w.u(c.membership_revision,8);w.u(c.peers,2);w.u(c.host,1);
}
auto context(Reader &r){AuthorityTransferContext c;r.octets(c.session);c.epoch=r.u(4);c.membership_revision=r.u(8);
  c.peers=r.u(2);c.host=r.u(1);return c;}
std::size_t maximum(SourceAuthorityCodecLimits limits){
  need(limits.encoded_bytes>=44&&limits.encoded_bytes<=UINT32_MAX&&limits.encoded_bytes<=std::numeric_limits<std::size_t>::max());
  return std::size_t(limits.encoded_bytes);
}
}
bool source_host_continuation_owner(LanHost &host,const SourceMatchAuthority &authority){
  return authority.for_match(SourceHostContinuationAccess::match(host));
}
bool source_host_continuation_payload(std::span<const std::byte> bytes)noexcept{return starts(bytes,bundle_magic);}
bool source_host_replay_payload(std::span<const std::byte> bytes)noexcept{return starts(bytes,replay_magic);}
SourceHostContinuation capture_source_host_continuation(LanHost &host,const AuthorityTransferContext &context,std::uint64_t serial){
  using namespace continuation_wire;need(serial!=0);auto &match=SourceHostContinuationAccess::match(host);
  auto *world=match.source_object_runtime();need(world&&world->owns_physics());SourceMatchAuthority authority(match,*world);
  const auto before=authority.execution_boundary();const auto binding=source_authority_binding(context);
  auto peers=host.checkpoint(binding);auto native=authority.checkpoint(binding);
  need(authority.execution_boundary()==before);
  return {context,serial,std::move(native),std::move(peers)};
}
std::vector<std::byte> encode_source_host_continuation(const SourceHostContinuation &cut,SourceAuthorityCodecLimits limits){
  using namespace continuation_wire;const auto max=maximum(limits);need(cut.serial&&cut.world.match.source_backed&&
      cut.world.match.admitted_binding==source_authority_binding(cut.context));
  const auto native=encode_source_authority(cut.world,limits);
  const LanHost::ContinuationBinding bound{cut.context,{cut.serial,cut.world.match.simulation.ticks,native.size(),digest(native)}};
  const auto host=LanHost::encode_continuation(cut.host,bound);
  Writer w{max,{}};context(w,cut.context);w.u(bound.world.id,8);w.u(bound.world.tick,8);w.u(bound.world.bytes,8);w.octets(bound.world.digest);
  w.blob(native,max);w.blob(host,512*1024);return frame(bundle_magic,w.bytes,max);
}
SourceHostContinuation decode_source_host_continuation(std::span<const std::byte> bytes,const AuthorityTransferContext &expected,
    std::uint64_t fingerprint,std::span<const OnlineAdmission> admissions,std::span<const SourcePlayerAdmission> roster,SourceAuthorityCodecLimits limits){
  using namespace continuation_wire;Reader r{unframe(bytes,bundle_magic,maximum(limits))};need(context(r)==expected);
  LanHost::ContinuationBinding bound;bound.context=expected;bound.world.id=r.u(8);bound.world.tick=r.u(8);
  bound.world.bytes=r.u(8);r.octets(bound.world.digest);const auto native=r.blob(maximum(limits));const auto host=r.blob(512*1024);r.end();
  need(bound.world.id&&native.size()==bound.world.bytes&&digest(native)==bound.world.digest);
  // Validate cheap bounded host metadata/independent identities before the
  // much larger world decompression and native-state allocations.
  auto peers=LanHost::decode_continuation(host,bound,fingerprint,admissions,roster);
  auto world=decode_source_authority(native,limits);
  need(world.match.source_backed&&world.match.admitted_binding==source_authority_binding(expected)&&world.match.simulation.ticks==bound.world.tick);
  return {expected,bound.world.id,std::move(world),std::move(peers)};
}
std::vector<std::byte> encode_source_host_replay(const SourceHostReplay &cut,SourceReplayLimits limits){
  using namespace continuation_wire;need(!cut.replay.steps.empty()&&limits.encoded_bytes<=UINT32_MAX&&
      cut.replay.steps.size()<=UINT64_MAX-cut.replay.binding.first_sequence);
  const auto bytes=encode_source_replay_batch(cut.replay,limits);
  const auto serial=cut.replay.binding.first_sequence+cut.replay.steps.size()-1;
  const LanHost::ContinuationBinding proof{cut.replay.binding.context,{serial,cut.replay.steps.back().after_tick,bytes.size(),digest(bytes)}};
  const auto host=LanHost::encode_continuation(cut.host,proof);Writer w{std::size_t(limits.encoded_bytes),{}};
  w.blob(bytes,std::size_t(limits.encoded_bytes));w.blob(host,512*1024);return frame(replay_magic,w.bytes,std::size_t(limits.encoded_bytes));
}
SourceHostReplay decode_source_host_replay(std::span<const std::byte> bytes,const SourceReplayBinding &expected,
    std::uint64_t fingerprint,std::span<const OnlineAdmission> admissions,std::span<const SourcePlayerAdmission> roster,SourceReplayLimits limits){
  using namespace continuation_wire;need(limits.encoded_bytes<=UINT32_MAX);
  Reader r{unframe(bytes,replay_magic,std::size_t(limits.encoded_bytes))};const auto replay=r.blob(std::size_t(limits.encoded_bytes));
  const auto host=r.blob(512*1024);r.end();auto batch=decode_source_replay_batch(replay,expected,limits);
  const auto serial=batch.binding.first_sequence+batch.steps.size()-1;
  const LanHost::ContinuationBinding proof{expected.context,{serial,batch.steps.back().after_tick,replay.size(),digest(replay)}};
  auto state=LanHost::decode_continuation(host,proof,fingerprint,admissions,roster);
  return {std::move(batch),std::move(state)};
}
}
