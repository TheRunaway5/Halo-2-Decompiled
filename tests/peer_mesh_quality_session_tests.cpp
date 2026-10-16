#include "halo2/online_match_session.hpp"
#include "halo2/frontend.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void wait(F ready,const char *stage){const auto until=std::chrono::steady_clock::now()+30s;
    while(!ready()){check(std::chrono::steady_clock::now()<until,stage);std::this_thread::sleep_for(5ms);}}
}
int main(int argc,char **argv)try{
    check(argc==3,"Supply ORIGIN PRIVATE_DESCRIPTOR_FILE");const std::filesystem::path path=argv[2];
    Json d;{std::ifstream in(path);in>>d;}
    std::vector<OnlineAdmission> roster;
    for(const auto &row:d.at("players")){OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");
        p.peer=row.at("peer");p.guest=row.at("guest");const std::string id=row.at("identity");check(id.size()==16,"Invalid private room identity");
        for(unsigned n=0;n<8;++n)p.identity[n]=std::byte(std::stoul(id.substr(n*2,2),nullptr,16));roster.push_back(p);}
    OnlineMatchTransportOptions transport;transport.direct_udp=true;
    auto session=std::make_shared<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),d.at("slot"),roster,true,false,false,transport);
    const auto healthy=[&]{if(session->phase()==OnlineMatchSession::Phase::Failed)throw std::runtime_error(session->error());};
    wait([&]{healthy();return session->phase()==OnlineMatchSession::Phase::Loading;},"Actual measured session loading timeout");
    check(session->roster().size()==8&&session->local_slots().size()==4,"Physical/couch fixture was not admitted");
    check(session->host()==0||session->host()==4,"A guest seat was selected as physical host");
    wait([&]{healthy();return session->host_candidates().size()==2;},"Fresh actual running-generation mesh matrix timeout");
    check(!session->source_players()&&session->validated_authority().empty(),"QoS manufactured prepared native world admission");
    session->loaded(0x12345678,UiVariant{"Slayer",0,{}});
    wait([&]{healthy();return session->phase()==OnlineMatchSession::Phase::Running;},"Actual measured session running timeout");
    const auto self=session->self(),other=self==0?4u:0u;const bool hosting=self==session->host();
    std::vector<std::byte> sent{std::byte{'H'},std::byte{'2'},std::byte{'N'},std::byte{'2'},
        std::byte(native_multiplayer_protocol),std::byte(native_multiplayer_protocol>>8),std::byte(hosting?3:1),std::byte(self)};
    unsigned received=0;const auto until=std::chrono::steady_clock::now()+2s;
    while(std::chrono::steady_clock::now()<until){healthy();session->send(sent,other);std::vector<std::byte> bytes;unsigned from{};
        while(session->receive(bytes,from)){check(from==other&&bytes.size()==8&&bytes[6]==std::byte(hosting?1:3)&&bytes[7]==std::byte(other),"Direct payload or authenticated peer changed");++received;}
        std::this_thread::sleep_for(10ms);
    }
    check(received>50,"Direct game datagrams did not continue beside telemetry");
    Json result{{"self",self},{"host",session->host()},{"received",received},{"protocol",native_multiplayer_protocol},{"candidates",Json::array()}};
    for(const auto &q:session->host_candidates())result["candidates"].push_back({{"peer",q.peer},{"upload",q.minimum_upload_bytes_per_second},{"rtt_ms",q.mean_rtt_ms},{"loss_per_mille",q.worst_loss_per_mille}});
    const auto output=path.parent_path()/("result-"+std::to_string(self)+".json");{std::ofstream out(output);out<<result.dump(2);}
    wait([&]{return std::filesystem::exists(path.parent_path()/("result-"+std::to_string(other)+".json"));},"Other native session did not complete");
    std::cout<<"PASS real admitted eight-seat/two-PC session: measured host "<<session->host()<<", received "<<received<<" direct datagrams, fresh backup candidates, no world certificate\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
