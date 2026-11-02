// Same public-API reader compiled against the preserved C++ and Rust owners.
// NDJSON is test-only and carries synthetic private identities, never live ones.
#include "halo2/peer_source_admissions.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> bytes(const std::string &s){
    if(s.size()%2)throw std::invalid_argument("odd hex");std::vector<std::byte> out;
    for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;
}
template<class T>std::string hex(const T &value){
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto b:value){unsigned n;if constexpr(std::is_same_v<decltype(b),std::byte>)n=std::to_integer<unsigned>(b);else n=b;out+=digits[n>>4];out+=digits[n&15];}return out;
}
template<class T>void copy_hex(const Json &j,T &out){const auto v=bytes(j.get<std::string>());if(v.size()!=out.size())throw std::invalid_argument("extent");
    for(std::size_t i=0;i<v.size();++i){if constexpr(std::is_same_v<typename T::value_type,std::byte>)out[i]=v[i];else out[i]=std::to_integer<std::uint8_t>(v[i]);}}
AuthorityTransferContext context(const Json &v){AuthorityTransferContext out;copy_hex(v.at("session"),out.session);
    out.epoch=v.at("epoch");out.membership_revision=v.at("revision");out.peers=v.at("peers");out.host=v.at("host");return out;}
Json context(const AuthorityTransferContext &v){return Json{{"session",hex(v.session)},{"epoch",v.epoch},{"revision",v.membership_revision},{"peers",v.peers},{"host",v.host}};}
std::vector<OnlineAdmission> roster(const Json &v){std::vector<OnlineAdmission> out;
    for(const auto &r:v){OnlineAdmission p;p.slot=r.at(0);p.peer=r.at(1);p.guest=r.at(2);copy_hex(r.at(3),p.identity);out.push_back(std::move(p));}return out;}
std::vector<SourcePlayerAdmission> players(const Json &v){std::vector<SourcePlayerAdmission> out;
    for(const auto &r:v){SourcePlayerAdmission p;p.slot=r.at(0);p.player=r.at(1);copy_hex(r.at(2),p.identity);out.push_back(p);}return out;}
Json players(const std::vector<SourcePlayerAdmission> &v){Json out=Json::array();for(const auto &p:v)out.push_back({p.slot,p.player,hex(p.identity)});return out;}
AcceptedSourcePlayers accepted(const Json &v){AcceptedSourcePlayers out;out.context=context(v.at("context"));copy_hex(v.at("digest"),out.digest);out.players=players(v.at("players"));return out;}
Json accepted(const std::optional<AcceptedSourcePlayers> &v){if(!v)return nullptr;return Json{{"context",context(v->context)},{"digest",hex(v->digest)},{"players",players(v->players)}};}
}
int main(){
    std::array<std::unique_ptr<PeerSourceAdmissions>,16> exchanges;std::string line;
    while(std::getline(std::cin,line)){
        Json out;
        try{
            const auto v=Json::parse(line);const auto op=v.at("op").get<std::string>();const unsigned id=v.value("id",0u);
            if(op=="is_packet")out={{"recognized",PeerSourceAdmissions::is_packet(bytes(v.at("bytes")))}};
            else{
                if(id>=exchanges.size())throw std::invalid_argument("object index");
                if(op=="new"){exchanges[id]=std::make_unique<PeerSourceAdmissions>(v.at("self"),context(v.at("context")),roster(v.at("roster")));out={{"ok",true}};}
                else if(op=="resume"){exchanges[id]=PeerSourceAdmissions::resume_after_host_loss(v.at("self"),context(v.at("context")),roster(v.at("roster")),accepted(v.at("previous")));out={{"ok",true}};}
                else{
                    if(!exchanges[id])throw std::invalid_argument("missing object");auto &s=*exchanges[id];
                    if(op=="publish")out={{"published",s.publish(context(v.at("context")),players(v.at("players")))}};
                    else if(op=="replace")out={{"replaced",s.replace_context(context(v.at("expected")),context(v.at("context")))}};
                    else if(op=="receive")out={{"recognized",s.receive(v.at("peer"),bytes(v.at("bytes")))}};
                    else if(op=="poll"){Json packets=Json::array();for(const auto &p:s.poll(v.at("now"),v.at("budget")))packets.push_back({{"peer",p.peer},{"bytes",hex(p.bytes)}});out={{"packets",packets}};}
                    else if(op=="state"){Json confirmed=Json::array();for(unsigned peer=0;peer<17;++peer)confirmed.push_back(s.confirmed(peer));confirmed.push_back(s.confirmed(256));confirmed.push_back(s.confirmed(~0u));
                        out={{"context",context(s.context())},{"accepted",accepted(s.accepted())},{"confirmed",confirmed},{"protocol",native_multiplayer_protocol}};}
                    else throw std::invalid_argument("operation");
                }
            }
        }catch(const std::exception &){out={{"error",true}};}
        std::cout<<out.dump()<<std::endl;
    }
}
