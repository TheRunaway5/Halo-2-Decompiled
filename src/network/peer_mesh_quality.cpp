#include "halo2/peer_mesh_quality.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
bridge::MeshQualityContext context(const AuthorityTransferContext &value){
    bridge::MeshQualityContext out;
    std::ranges::transform(value.session,out.session.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;
    // Preserve the entire native unsigned index. Rust distinguishes the
    // pregame sentinel16 from invalid17/256 without a narrowing alias.
    out.host=value.host;return out;
}
AuthorityTransferContext context(const bridge::MeshQualityContext &value){
    AuthorityTransferContext out;
    std::ranges::transform(value.session,out.session.begin(),[](auto b){return std::byte(b);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=value.host;return out;
}
PeerQualityRow row(const bridge::MeshQualityRow &value){
    PeerQualityRow out;out.received_ms=value.received_ms;
    for(unsigned n=0;n<out.links.size();++n){const auto &q=value.links[n];out.links[n]={q.rtt_ms,q.upload_bytes_per_second,q.loss_per_mille,q.measured};}
    return out;
}
auto bytes(std::span<const std::byte> p){return rust::Slice<const std::uint8_t>{reinterpret_cast<const std::uint8_t*>(p.data()),p.size()};}
template<class F>auto checked(F &&f){try{return f();}catch(const rust::Error &e){throw std::invalid_argument(e.what());}}
std::vector<PeerHostQuality> candidates(const rust::Vec<bridge::MeshQualityHost> &values){
    std::vector<PeerHostQuality> out;out.reserve(values.size());
    for(const auto &q:values)out.push_back({q.peer,q.mean_rtt_ms,q.minimum_upload_bytes_per_second,q.worst_loss_per_mille});
    return out;
}
}
// The native owner serializes access. Probe/receipt/codec, clocks, freshness,
// bounded schedules, measurements and reciprocal ranking are one Rust owner.
struct PeerMeshQuality::Impl {
    mutable std::mutex mutex;
    rust::Box<bridge::PeerMeshQuality> mesh;
    Impl(unsigned self,const AuthorityTransferContext &c):mesh(checked([&]{return bridge::new_peer_mesh_quality(self,halo2::context(c));})){}
};
PeerMeshQuality::PeerMeshQuality(unsigned self,AuthorityTransferContext c):impl_(std::make_unique<Impl>(self,c)){}
PeerMeshQuality::~PeerMeshQuality()=default;
AuthorityTransferContext PeerMeshQuality::context()const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);return halo2::context(checked([&]{return bridge::peer_mesh_quality_context(*s.mesh);}));
}
bool PeerMeshQuality::is_packet(std::span<const std::byte> p)noexcept{return bridge::peer_mesh_quality_is_packet(bytes(p));}
std::vector<AuthorityTransferPacket> PeerMeshQuality::poll(std::uint16_t ready,std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::lock_guard lock(s.mutex);const auto values=checked([&]{return bridge::peer_mesh_quality_poll(*s.mesh,ready,now,budget);});
    std::vector<AuthorityTransferPacket> out;out.reserve(values.size());
    for(const auto &p:values){AuthorityTransferPacket packet{p.peer,{}};packet.bytes.reserve(p.bytes.size());for(auto b:p.bytes)packet.bytes.push_back(std::byte(b));out.push_back(std::move(packet));}
    return out;
}
bool PeerMeshQuality::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::lock_guard lock(s.mutex);return checked([&]{return bridge::peer_mesh_quality_receive(*s.mesh,peer,bytes(packet),now);});
}
std::array<std::optional<PeerQualityRow>,16> PeerMeshQuality::matrix(std::uint64_t now)const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);const auto value=checked([&]{return bridge::peer_mesh_quality_matrix(*s.mesh,now);});
    std::array<std::optional<PeerQualityRow>,16> out;
    for(unsigned p=0;p<out.size();++p)if(value.rows[p].present)out[p]=row(value.rows[p]);
    return out;
}
std::optional<PeerQualityRow> PeerMeshQuality::local(std::uint64_t now)const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);const auto value=checked([&]{return bridge::peer_mesh_quality_local(*s.mesh,now);});
    return value.present?std::optional(row(value)):std::nullopt;
}
std::vector<PeerHostQuality> PeerMeshQuality::candidates(std::uint64_t now)const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);return halo2::candidates(checked([&]{return bridge::peer_mesh_quality_candidates(*s.mesh,now,false);}));
}
std::vector<PeerHostQuality> PeerMeshQuality::migration_candidates(std::uint64_t now)const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);return halo2::candidates(checked([&]{return bridge::peer_mesh_quality_candidates(*s.mesh,now,true);}));
}
}
