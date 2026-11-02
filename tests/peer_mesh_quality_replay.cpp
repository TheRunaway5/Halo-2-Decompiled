// Identical public-API replay reader for the preserved C++ and Rust owners.
// This test protocol never participates in production matchmaking.
#include "halo2/peer_mesh_quality.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> bytes(const std::string &text){
  if(text.size()%2)throw std::invalid_argument("odd hex");std::vector<std::byte> out;
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return out;
}
std::string hex(std::span<const std::byte> value){
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto b:value){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;
}
Json row(const std::optional<PeerQualityRow> &value){
  if(!value)return nullptr;Json links=Json::array();for(const auto &q:value->links)links.push_back({q.rtt_ms,q.upload_bytes_per_second,q.loss_per_mille,q.measured});
  return Json{{"received_ms",value->received_ms},{"links",links}};
}
Json hosts(const std::vector<PeerHostQuality> &values){Json out=Json::array();for(const auto &v:values)out.push_back({v.peer,v.mean_rtt_ms,v.minimum_upload_bytes_per_second,v.worst_loss_per_mille});return out;}
}
int main(){
  std::array<std::unique_ptr<PeerMeshQuality>,16> mesh;std::string line;
  while(std::getline(std::cin,line)){
    Json out;
    try{
      const auto command=Json::parse(line);const auto operation=command.at("op").get<std::string>();const unsigned id=command.value("id",0u);
      if(operation=="new"){
        AuthorityTransferContext c;const auto session=bytes(command.at("session"));if(session.size()!=32)throw std::invalid_argument("session extent");
        std::copy(session.begin(),session.end(),c.session.begin());c.epoch=command.at("epoch");c.membership_revision=command.at("revision");c.peers=command.at("peers");c.host=command.at("host");
        if(id>=mesh.size())throw std::invalid_argument("object index");
        mesh[id]=std::make_unique<PeerMeshQuality>(command.at("self"),c);out={{"ok",true}};
      }else if(operation=="is_packet")out={{"recognized",PeerMeshQuality::is_packet(bytes(command.at("bytes")))}};
      else{
        if(id>=mesh.size()||!mesh[id])throw std::invalid_argument("missing object");auto &m=*mesh[id];const auto now=command.at("now").get<std::uint64_t>();
        if(operation=="poll"){
          Json packets=Json::array();for(const auto &p:m.poll(command.at("ready"),now,command.at("budget")))packets.push_back({{"peer",p.peer},{"bytes",hex(p.bytes)}});out={{"packets",packets}};
        }else if(operation=="receive")out={{"recognized",m.receive(command.at("peer"),bytes(command.at("bytes")),now)}};
        else if(operation=="state"){
          const auto c=m.context();Json matrix=Json::array();for(const auto &v:m.matrix(now))matrix.push_back(row(v));
          out={{"context",{{"session",hex(c.session)},{"epoch",c.epoch},{"revision",c.membership_revision},{"peers",c.peers},{"host",c.host}}},
            {"matrix",matrix},{"local",row(m.local(now))},{"candidates",hosts(m.candidates(now))},{"migration",hosts(m.migration_candidates(now))}};
        }else throw std::invalid_argument("unknown operation");
      }
    }catch(const std::exception &e){out={{"error",e.what()}};}
    std::cout<<out.dump()<<std::endl;
  }
}
