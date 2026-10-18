#include "halo2/peer_mesh_quality.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <map>
#include <queue>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
constexpr std::array<unsigned,4> peers{0,4,8,12};
AuthorityTransferContext context(){AuthorityTransferContext c;c.session[0]=std::byte{7};c.epoch=3;c.membership_revision=5;c.host=0;
    for(auto p:peers)c.peers|=1u<<p;
    return c;}
unsigned kind(const std::vector<std::byte> &p){return std::to_integer<unsigned>(p.at(6));}
struct Mesh {
    struct Event {std::uint64_t at,order;unsigned from,to;std::vector<std::byte> bytes;};
    struct Later {bool operator()(const Event &a,const Event &b)const{return std::pair{a.at,a.order}>std::pair{b.at,b.order};}};
    std::array<std::unique_ptr<PeerMeshQuality>,16> nodes;
    std::priority_queue<Event,std::vector<Event>,Later> events;
    std::array<std::uint64_t,16> next{};
    std::array<unsigned,16> rate{};
    std::uint64_t now=1000,order{},packets{},probes{},reports{};
    bool drop{},duplicate{};
    std::uint16_t active=context().peers;
    Mesh(){unsigned n=0;for(auto p:peers){nodes[p]=std::make_unique<PeerMeshQuality>(p,context());rate[p]=1'200'000-n++*250'000;}}
    void step(){
        while(!events.empty()&&events.top().at<=now){auto e=events.top();events.pop();
            check(nodes[e.to]->receive(e.from,e.bytes,now),"QoS packet fell through as gameplay");}
        for(auto from:peers)if(active&(1u<<from)){auto out=nodes[from]->poll(active&~(1u<<from),now,16);
            check(out.size()<=16,"Packet budget exceeded");
            for(auto &p:out){check(p.bytes.size()<=1200&&p.peer!=from,"Datagram bound or physical destination invalid");++packets;
                if(!(active&(1u<<p.peer)))continue;
                if(kind(p.bytes)==3){++probes;if(drop&&from==12&&(std::to_integer<unsigned>(p.bytes[60])%8==0))continue;}
                if(kind(p.bytes)==5)++reports;
                next[from]=std::max(next[from],now)+std::max<std::uint64_t>(1,(p.bytes.size()*1000+rate[from]-1)/rate[from]);
                const auto at=next[from]+5+from;
                if(duplicate&&kind(p.bytes)==4)events.push({at+100,order++,from,p.peer,p.bytes});
                events.push({at,order++,from,p.peer,std::move(p.bytes)});
            }
        }
        now+=2;
    }
    void run(unsigned ms){const auto end=now+ms;while(now<end)step();}
};
void measured_mesh(){
    Mesh mesh;mesh.run(8000);
    check(mesh.probes==4*3*64,"Initial bounded upload test did not cover every directed physical path exactly once");
    check(mesh.reports>24,"Full mesh rows were not broadcast continuously");
    for(auto p:peers){const auto rows=mesh.nodes[p]->matrix(mesh.now);const auto ranked=mesh.nodes[p]->candidates(mesh.now);
        check(ranked.size()==4,"Fresh reciprocal complete mesh did not qualify all physical hosts");
        check(ranked.front().peer==0&&ranked.back().peer==12,"Observed upload delivery did not affect host ordering");
        for(auto other:peers)if(p!=other)check(rows[p]->links[other].measured&&rows[p]->links[other].upload_bytes_per_second&&
            rows[p]->links[other].loss_per_mille==0,"Measured lossless path missing");
        check(mesh.nodes[p]->candidates(mesh.now+5001).empty(),"Stale matrix qualified a host");
    }
    // A disappeared local transport is ineligible even if no packet budget is
    // available and other PCs still have fresh reports about it.
    mesh.nodes[0]->poll(0,mesh.now,0);
    check(mesh.nodes[0]->candidates(mesh.now).empty(),"Disconnected local paths retained host eligibility");
    std::cout<<"PASS four physical peers: reciprocal capacity/latency/loss, all-directed probes, bounded broadcast, stale/disconnected rejection\n";
}
void duplicate_receipts(){
    const auto drive=[](bool duplicate){
        auto c=context();c.peers=(1u<<0)|(1u<<4);PeerMeshQuality a(0,c),b(4,c);
        std::multimap<std::uint64_t,std::vector<std::byte>> deliveries;
        for(std::uint64_t now=1000;now<3500;now+=2){
            while(!deliveries.empty()&&deliveries.begin()->first<=now){a.receive(4,deliveries.begin()->second,now);deliveries.erase(deliveries.begin());}
            for(auto p:a.poll(c.peers&~1u,now,16)){
                b.receive(0,p.bytes,now);
            }
            for(auto reply:b.poll(1u,now,16))if(kind(reply.bytes)==2||kind(reply.bytes)==4){
                deliveries.emplace(now+4,reply.bytes);
                if(duplicate&&kind(reply.bytes)==4)deliveries.emplace(now+200,reply.bytes);
            }
        }
        return a.local(3500)->links[4].upload_bytes_per_second;
    };
    check(drive(false)==drive(true)&&drive(false)>0,"Duplicate receipt changed measured upload");
    std::cout<<"PASS duplicate receipt cannot change upload duration\n";
}
void survivor_measurements(){
    Mesh mesh;mesh.run(8000);mesh.active&=~1u;mesh.run(6000);
    for(auto p:{4u,8u,12u}){
        check(!mesh.nodes[p]->matrix(mesh.now)[0]&&mesh.nodes[p]->candidates(mesh.now).empty(),
            "Failed predecessor still supplied a fresh full-membership host row");
        const auto ranked=mesh.nodes[p]->migration_candidates(mesh.now);
        if(ranked.size()!=3||ranked.front().peer!=8||ranked.back().peer!=12)
            for(const auto &q:ranked)std::cerr<<"survivor observer="<<p<<" peer="<<q.peer<<" upload="<<q.minimum_upload_bytes_per_second<<" rtt="<<q.mean_rtt_ms<<" loss="<<q.worst_loss_per_mille<<'\n';
        // With the predecessor path removed, PC8 has the best observed
        // bottleneck under this bounded offered load. PC4's nominal link rate
        // is higher, but its acknowledged inter-PC delivery is lower.
        check(ranked.size()==3&&ranked.front().peer==8&&ranked.back().peer==12,
            "Survivor ordering did not use actual reciprocal upload/latency measurements");
        check(mesh.nodes[p]->context()==context(),"Scoring rewrote the agreed physical membership");
        check(mesh.nodes[p]->migration_candidates(mesh.now+5001).empty(),"Stale survivor measurements qualified a replacement");
    }
    mesh.active&=~(1u<<12);mesh.run(6000);
    for(auto p:{4u,8u})check(mesh.nodes[p]->migration_candidates(mesh.now).empty(),
        "A second local timeout silently shrank the fixed successor membership");
    auto two=context();two.peers=0x11;PeerMeshQuality only(4,two);
    check(only.migration_candidates(10000).empty(),"A sole survivor fabricated inter-PC performance measurements");
    std::cout<<"PASS expired predecessor row excluded from fixed survivor scoring, retained measured order, stale and second-disconnect rejection, sole-survivor unknown metrics\n";
}
void loss_and_context(){
    Mesh mesh;mesh.drop=true;mesh.run(8000);
    const auto row=mesh.nodes[12]->local(mesh.now);check(row&&row->links[0].loss_per_mille==125,"Dropped upload chunks not counted exactly");
    auto old=context(),next=old;++next.epoch;PeerMeshQuality old_peer(0,old),replacement(4,next);
    for(auto packet:old_peer.poll(1u<<4,1000,4))check(replacement.receive(0,packet.bytes,1001),"Rejected context packet not consumed");
    check(replacement.candidates(1001).empty(),"Prior generation created a qualified replacement");
    auto sample=old_peer.poll(1u<<4,1300,4).front().bytes;
    PeerMeshQuality isolated(4,old);
    check(isolated.receive(8,sample,1301),"Non-destination packet not consumed");
    // Peer 8 is admitted but cannot forge peer 0's outstanding ping echo.
    sample[6]=std::byte{2};check(isolated.receive(8,sample,1301),"Forged echo fell through");
    for(std::size_t n=4;n<sample.size();++n)check(isolated.receive(0,std::span(sample).first(n),1400),"Truncated quality packet fell through");
    check(isolated.receive(1,sample,1400),"Nonmember packet fell through");
    check(!isolated.receive(0,std::vector<std::byte>(20),1400),"Unrelated packet consumed");
    check(isolated.candidates(1400).empty(),"Malformed, wrong-generation or unsolicited data qualified host");
    auto pre=old;pre.host=16;PeerMeshQuality before_host(4,pre);check(before_host.context()==pre,"Pregame context changed");
    std::cout<<"PASS exact directional loss, context/physical sender binding, truncation and pregame admission\n";
}
void rejected_metadata(){
    auto c=context();PeerMeshQuality sender(0,c);const auto ping=sender.poll(c.peers&~1u,1000,4).front().bytes;
    for(const auto at:{4u,7u,8u,40u,44u,52u,54u,55u}){
        auto forged=ping;forged[at]^=std::byte{1};PeerMeshQuality receiver(4,c);
        receiver.receive(0,forged,1001);check(receiver.poll(0,1002,4).empty(),"Wrong version/flags/session/generation/membership/host produced a response");
    }
    PeerMeshQuality valid(4,c);valid.receive(0,ping,1001);const auto echo=valid.poll(0,1002,4);
    check(echo.size()==1&&echo[0].peer==0&&kind(echo[0].bytes)==2,"Valid admitted quality ping was rejected");
    Mesh mesh;mesh.run(8000);std::vector<std::byte> report;
    const auto until=mesh.now+2000;
    while(report.empty()){
        check(mesh.now<until,"Quality row broadcast stalled");
        for(auto p:mesh.nodes[0]->poll(c.peers&~1u,mesh.now,16))if(p.peer==4&&kind(p.bytes)==5)report=p.bytes;
        mesh.now+=2;
    }
    PeerMeshQuality receiver(4,c);receiver.receive(0,report,mesh.now);
    check(receiver.matrix(mesh.now)[0].has_value(),"Valid complete row was rejected");
    receiver.receive(0,report,mesh.now+4000);
    check(!receiver.matrix(mesh.now+5001)[0],"Duplicate row refreshed stale telemetry");
    auto corrupt=report;corrupt[59]|=std::byte{0x40};corrupt.push_back(std::byte{0});
    receiver.receive(0,corrupt,mesh.now+5002);
    check(!receiver.matrix(mesh.now+5002)[0],"Trailing bytes were admitted into the matrix");
    corrupt.pop_back();receiver.receive(0,corrupt,mesh.now+5003);
    check(receiver.matrix(mesh.now+5003)[0].has_value(),"Rejected row advanced the report sequence");
    std::cout<<"PASS all context fields, malformed row rejection and duplicate report freshness\n";
}
void concurrent_snapshots(){
    Mesh mesh;mesh.run(8000);std::atomic<bool> stop=false;
    std::jthread reader([&]{while(!stop.load()){for(auto p:peers){mesh.nodes[p]->matrix(9000);mesh.nodes[p]->candidates(9000);mesh.nodes[p]->migration_candidates(9000);}}});
    mesh.run(2000);stop=true;reader.join();
    std::cout<<"PASS concurrent matrix/candidate reads with poll/receive writes\n";
}
}
int main()try{measured_mesh();duplicate_receipts();survivor_measurements();loss_and_context();rejected_metadata();concurrent_snapshots();return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
