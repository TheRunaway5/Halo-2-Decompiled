// Public API reader shared by preserved C++ and Rust-backed snapshot owners.
// Actual private native Delivery identities and move-only byte banks are used.
// Foreground calls, endpoint identity, clocks and archive data are fixtures;
// they do not certify a native world, original Live or a real provider.
#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
using Owner=PeerSnapshotCandidate;
std::vector<std::byte> bytes(const std::string &s){
    if(s.size()%2)throw std::invalid_argument("odd hex");std::vector<std::byte> out;
    for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;
}
template<class T>std::string hex(const T &v){const char digits[]="0123456789abcdef";std::string out;for(auto b:v){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
AuthorityTransferContext context(const Json &v){AuthorityTransferContext c;const auto b=bytes(v.at("session"));if(b.size()!=32)throw std::invalid_argument("session");std::copy(b.begin(),b.end(),c.session.begin());c.epoch=v.at("epoch");c.membership_revision=v.at("revision");c.peers=v.at("peers");c.host=v.at("host");return c;}
Json context(const AuthorityTransferContext &c){return {{"session",hex(c.session)},{"epoch",c.epoch},{"revision",c.membership_revision},{"peers",c.peers},{"host",c.host}};}
PeerSnapshotToken token(const Json &v){return {context(v.at("context")),v.at("recipients"),v.at("nonce")};}
Json token(const PeerSnapshotToken &t){return {{"context",context(t.context)},{"recipients",t.recipients},{"nonce",t.nonce}};}
Json delivery(const std::optional<Owner::Delivery> &d){return d?Json{{"token",token(d->token())},{"source",d->source()},{"recipient",d->recipient()},{"bytes",hex(d->bytes())}}:Json(nullptr);}
PeerSnapshotLimits limits(const Json &v){PeerSnapshotLimits l;l.maximum_archive_bytes=v.value("maximum",l.maximum_archive_bytes);l.retry_ms=v.value("retry",l.retry_ms);l.idle_ms=v.value("idle",l.idle_ms);l.window_chunks=v.value("window",l.window_chunks);return l;}
Json state(const Owner &s){const auto v=s.state();return {{"self",s.self()},{"protocol",native_multiplayer_protocol},{"context",context(v.context)},{"recipients",v.recipients},{"pending",v.pending?token(*v.pending):Json(nullptr)},
    {"armed",v.armed},{"delivered",v.delivered},{"captured",v.captured},{"retired",v.retired},{"rejected",v.rejected},{"closing",v.closing?Json(unsigned(*v.closing)):Json(nullptr)},
    {"published",v.published},{"outstanding",v.delivery_outstanding},{"stalled",v.stalled},{"buffered",v.buffered_bytes},{"maximum",v.maximum_buffered_bytes},{"rejected_packets",v.rejected_packets}};}
}
int main(){
    std::map<unsigned,std::unique_ptr<Owner>> owners;
    std::map<unsigned,std::optional<Owner::Delivery>> leases;
    std::string line;
    while(std::getline(std::cin,line)){
        Json out;
        try {const auto v=Json::parse(line);const std::string op=v.at("op");
            if(op=="new"){auto next=std::make_unique<Owner>(v.at("self"),context(v.at("context")),limits(v.value("limits",Json::object())));owners[v.at("id")]=std::move(next);out={{"ok",true}};}
            else if(op=="is_packet")out={{"value",Owner::is_packet(bytes(v.at("bytes")))}};
            else if(op=="lease")out={{"delivery",delivery(leases[v.at("lease")])}};
            else if(op=="move"){const unsigned from=v.at("from"),to=v.at("to");leases[to]=std::move(leases[from]);out={{"from",delivery(leases[from])},{"to",delivery(leases[to])}};}
            else if(op=="drop"){leases[v.at("lease")].reset();out={{"ok",true}};}
            else {auto &s=*owners.at(v.at("id").get<unsigned>());
                if(op=="state")out=state(s);
                else if(op=="begin"){const auto t=s.begin(v.at("now"));out={{"token",t?token(*t):Json(nullptr)}};}
                else if(op=="arm")out={{"value",s.acknowledge_arm(token(v.at("token")))}};
                else if(op=="close")out={{"value",s.close(token(v.at("token")),PeerSnapshotClose(v.at("mode").get<unsigned>()))}};
                else if(op=="reject")out={{"value",s.reject(token(v.at("token")))}};
                else if(op=="publish"){std::vector<PeerSnapshotArchive> batch;for(const auto &a:v.at("archives"))batch.push_back({a.at("peer"),bytes(a.at("bytes"))});out={{"value",s.publish(token(v.at("token")),std::move(batch),v.at("now"))}};}
                else if(op=="poll"){Json packets=Json::array();for(const auto &p:s.poll(v.at("now"),v.at("budget")))packets.push_back({{"peer",p.peer},{"bytes",hex(p.bytes)}});out={{"packets",packets}};}
                else if(op=="receive")out={{"value",s.receive(v.at("peer"),bytes(v.at("bytes")),v.at("now"))}};
                else if(op=="take"){auto d=s.take_delivery();const bool present=bool(d);if(present)leases[v.at("lease")]=std::move(d);out={{"present",present},{"delivery",delivery(leases[v.at("lease")])}};}
                else if(op=="capture"){auto &d=leases[v.at("lease")];out={{"value",d&&s.acknowledge_captured(std::move(*d))},{"delivery",delivery(d)}};}
                else if(op=="can_retire"||op=="retire"){
                    std::optional<Owner::Delivery> empty;auto &d=v.at("lease").is_null()?empty:leases[v.at("lease")];
                    const bool value=op=="can_retire"?s.can_acknowledge_retired(token(v.at("token")),d):s.acknowledge_retired(token(v.at("token")),std::move(d));out={{"value",value},{"delivery",delivery(d)}};
                }else throw std::invalid_argument("op");
            }
        }catch(const std::invalid_argument &){out={{"error",true},{"category","invalid_argument"}};}
        catch(const std::exception &){out={{"error",true},{"category","exception"}};}
        std::cout<<out.dump()<<std::endl;
    }
}
