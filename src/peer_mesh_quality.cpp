#include "halo2/peer_mesh_quality.hpp"
#include <algorithm>
#include <bit>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <tuple>

namespace halo2 {
namespace {
constexpr std::uint32_t magic=0x4f513248u; // H2QO, separate from game/voice.
constexpr unsigned header_bytes=62,probe_bytes=1200,probe_packets=64;
enum Kind:unsigned {Ping=1,Echo,Upload,Receipt,Report};
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid direct mesh quality packet or context");}
void put(std::vector<std::byte> &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
struct Reader {std::span<const std::byte> bytes;std::size_t at{};
    std::uint64_t get(unsigned n){need(n<=bytes.size()-at);std::uint64_t value{};for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<unsigned>(bytes[at++]))<<(8*i);return value;}
    void end(){need(at==bytes.size());}
};
bool member(const AuthorityTransferContext &c,unsigned p){return p<16&&(c.peers&(1u<<p));}
std::uint64_t age(std::uint64_t now,std::uint64_t before){return now>=before?now-before:UINT64_MAX;}
std::vector<std::byte> header(const AuthorityTransferContext &c,unsigned kind,std::uint32_t round,unsigned index){
    std::vector<std::byte> p;put(p,magic,4);put(p,1,2);put(p,kind,1);put(p,0,1);p.insert(p.end(),c.session.begin(),c.session.end());
    put(p,c.epoch,4);put(p,c.membership_revision,8);put(p,c.peers,2);put(p,c.host,1);put(p,0,1);put(p,round,4);put(p,index,2);return p;
}
}
struct PeerMeshQuality::Impl {
    const unsigned self;const AuthorityTransferContext context;mutable std::mutex mutex;
    struct Link {
        std::uint32_t ping_sequence{},round{},report_sequence{};std::array<std::pair<std::uint32_t,std::uint64_t>,8> pings{};
        unsigned ping_samples{},next_packet{};std::uint64_t ping_at{},echo_at{},probe_at{},probe_started{},acked{},received_span{};
        std::uint64_t incoming_round{};std::uint64_t incoming_packets{},incoming_started{};
        PeerLinkQuality quality;bool probing{};
    };
    std::array<Link,16> links;std::array<std::optional<PeerQualityRow>,16> rows;
    std::deque<AuthorityTransferPacket> replies;std::array<std::uint64_t,16> report_at{};unsigned cursor{},report_cursor{};std::uint32_t report_sequence{};
    explicit Impl(unsigned s,AuthorityTransferContext c):self(s),context(c){need(member(c,s)&&(c.host==16||member(c,c.host))&&std::popcount(c.peers)>=2);}
    void finish(Link &link,std::uint64_t now){
        if(!link.probing||link.next_packet!=probe_packets||age(now,link.probe_started)<1000)return;
        const auto count=std::popcount(link.acked);const auto elapsed=std::max<std::uint64_t>(1,link.received_span);
        link.quality.upload_bytes_per_second=count?std::uint32_t(std::min<std::uint64_t>(64ull*1024*1024,
            std::uint64_t(count-1)*probe_bytes*1000/elapsed)):0;
        link.quality.loss_per_mille=std::uint16_t((probe_packets-count)*1000/probe_packets);
        link.quality.measured=link.ping_samples>=3&&count>1&&link.quality.upload_bytes_per_second;link.probing=false;link.probe_at=now;
    }
};
PeerMeshQuality::PeerMeshQuality(unsigned self,AuthorityTransferContext c):impl_(std::make_unique<Impl>(self,c)){}
PeerMeshQuality::~PeerMeshQuality()=default;
AuthorityTransferContext PeerMeshQuality::context()const{return impl_->context;}
bool PeerMeshQuality::is_packet(std::span<const std::byte> p)noexcept{
    return p.size()>=4&&p[0]==std::byte{'H'}&&p[1]==std::byte{'2'}&&p[2]==std::byte{'Q'}&&p[3]==std::byte{'O'};
}
std::vector<AuthorityTransferPacket> PeerMeshQuality::poll(std::uint16_t ready,std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::lock_guard lock(s.mutex);budget=std::min(budget,64u);std::vector<AuthorityTransferPacket> out;
    const auto reply_budget=budget>1?budget/2:budget;
    while(!s.replies.empty()&&out.size()<reply_budget){out.push_back(std::move(s.replies.front()));s.replies.pop_front();}
    for(unsigned attempt=0;attempt<16*budget&&out.size()<budget;++attempt){const auto p=s.cursor++%16;
        if(p==s.self||!member(s.context,p))continue;
        auto &link=s.links[p];s.finish(link,now);
        if(!(ready&(1u<<p))){link.quality.measured=false;continue;}
        link.quality.measured=link.ping_samples>=3&&link.quality.upload_bytes_per_second&&
            age(now,link.echo_at)<=2000&&age(now,link.probe_at)<=90'000;
        if(!link.ping_sequence||age(now,link.ping_at)>=250){
            if(link.ping_sequence==UINT32_MAX)continue;
            const auto sequence=++link.ping_sequence;link.ping_at=now;
            link.pings[sequence%link.pings.size()]={sequence,now};out.push_back({p,header(s.context,Ping,sequence,0)});continue;
        }
        if(!link.probing&&link.ping_samples>=3&&(!link.round||age(now,link.probe_at)>=60'000)){
            if(link.round==UINT32_MAX)continue;
            ++link.round;link.probing=true;link.next_packet=0;link.acked=0;link.probe_started=now;link.received_span=0;}
        if(link.probing&&link.next_packet<probe_packets){auto data=header(s.context,Upload,link.round,link.next_packet++);
            data.resize(probe_bytes);out.push_back({p,std::move(data)});}
    }
    PeerQualityRow local;local.received_ms=now;for(unsigned p=0;p<16;++p){
        const auto &link=s.links[p];local.links[p]=link.quality;
        local.links[p].measured=link.quality.measured&&(ready&(1u<<p))&&
            age(now,link.echo_at)<=2000&&age(now,link.probe_at)<=90'000;
    }s.rows[s.self]=local;
    // Broadcast one complete row to one endpoint per call, sharing the bounded
    // packet budget with probes. Native gameplay/media are drained first.
    if(out.size()<budget){
        for(unsigned attempt=0;attempt<16;++attempt){const auto p=s.report_cursor++%16;
            if(p==s.self||!member(s.context,p)||!(ready&(1u<<p))||age(now,s.report_at[p])<500)continue;
            if(s.report_sequence==UINT32_MAX)break;
            auto data=header(s.context,Report,++s.report_sequence,0);for(const auto &q:local.links){put(data,q.rtt_ms,4);put(data,q.upload_bytes_per_second,4);put(data,q.loss_per_mille,2);put(data,q.measured,1);}
            out.push_back({p,std::move(data)});s.report_at[p]=now;break;
        }
    }
    return out;
}
bool PeerMeshQuality::receive(unsigned peer,std::span<const std::byte> p,std::uint64_t now){
    if(!is_packet(p))return false;
    auto &s=*impl_;std::lock_guard lock(s.mutex);
    try{
        need(peer!=s.self&&member(s.context,peer)&&p.size()>=header_bytes&&p.size()<=probe_bytes);Reader in{p};
        need(in.get(4)==magic&&in.get(2)==1);const auto kind=in.get(1);need(in.get(1)==0);
        for(auto b:s.context.session)need(in.get(1)==std::to_integer<unsigned>(b));
        need(in.get(4)==s.context.epoch&&in.get(8)==s.context.membership_revision&&in.get(2)==s.context.peers&&
            in.get(1)==s.context.host&&in.get(1)==0);const auto round=std::uint32_t(in.get(4));const auto index=unsigned(in.get(2));auto &link=s.links[peer];
        const auto reply=[&](unsigned type){if(s.replies.size()<256)s.replies.push_back({peer,header(s.context,type,round,index)});};
        if(kind==Ping){need(round&&index==0);in.end();reply(Echo);}
        else if(kind==Echo){need(round&&index==0);in.end();auto &sent=link.pings[round%link.pings.size()];need(sent.first==round&&age(now,sent.second)<=60'000);
            const auto rtt=std::uint32_t(age(now,sent.second));link.quality.rtt_ms=link.ping_samples?(link.quality.rtt_ms*3u+rtt)/4u:rtt;
            link.ping_samples=std::min(link.ping_samples+1,1000u);link.echo_at=now;sent.first=0;}
        else if(kind==Upload){need(round&&index<probe_packets&&p.size()==probe_bytes);
            if(round>link.incoming_round){link.incoming_round=round;link.incoming_packets=0;link.incoming_started=now;}
            need(round==link.incoming_round&&age(now,link.incoming_started)<=10'000);
            const auto bit=std::uint64_t(1)<<index;
            if(!(link.incoming_packets&bit)&&s.replies.size()<256){
                auto receipt=header(s.context,Receipt,round,index);put(receipt,age(now,link.incoming_started),4);
                s.replies.push_back({peer,std::move(receipt)});link.incoming_packets|=bit;
            }}
        else if(kind==Receipt){need(round==link.round&&link.probing&&index<link.next_packet);
            const auto elapsed=in.get(4);need(elapsed<=10'000);in.end();
            const auto bit=std::uint64_t(1)<<index;
            if(!(link.acked&bit)){link.acked|=bit;link.received_span=std::max(link.received_span,elapsed);}s.finish(link,now);}
        else if(kind==Report){need(round>link.report_sequence&&!index);PeerQualityRow row;row.received_ms=now;
            for(unsigned n=0;n<16;++n){auto &q=row.links[n];q.rtt_ms=std::uint32_t(in.get(4));q.upload_bytes_per_second=std::uint32_t(in.get(4));q.loss_per_mille=std::uint16_t(in.get(2));const auto measured=in.get(1);
                need(measured<=1&&q.loss_per_mille<=1000&&q.upload_bytes_per_second<=64u*1024*1024&&
                    (q.rtt_ms==UINT32_MAX||q.rtt_ms<=60'000));q.measured=measured;
                if(!member(s.context,n)||n==peer)need(!q.measured&&!q.upload_bytes_per_second&&q.rtt_ms==UINT32_MAX&&q.loss_per_mille==1000);}
            in.end();s.rows[peer]=row;link.report_sequence=round;}
        else need(false);
    }catch(const std::exception &){} // Reject the measurement, never evict a peer.
    return true;
}
std::array<std::optional<PeerQualityRow>,16> PeerMeshQuality::matrix(std::uint64_t now)const{
    auto &s=*impl_;std::lock_guard lock(s.mutex);auto out=s.rows;
    for(auto &row:out)if(row&&age(now,row->received_ms)>5000)row.reset();
    return out;
}
std::optional<PeerQualityRow> PeerMeshQuality::local(std::uint64_t now)const{return matrix(now)[impl_->self];}
std::vector<PeerHostQuality> PeerMeshQuality::candidates(std::uint64_t now)const{
    auto &s=*impl_;const auto rows=matrix(now);std::vector<PeerHostQuality> result;
    for(unsigned p=0;p<16;++p)if(member(s.context,p)&&rows[p]){
        PeerHostQuality quality;quality.peer=p;quality.minimum_upload_bytes_per_second=UINT32_MAX;std::uint64_t total{};unsigned count{};bool eligible=true;
        for(unsigned other=0;other<16;++other)if(other!=p&&member(s.context,other)){
            if(!rows[other]){eligible=false;break;}const auto &a=rows[p]->links[other],&b=rows[other]->links[p];
            if(!a.measured||!b.measured||!a.upload_bytes_per_second||!b.upload_bytes_per_second||a.rtt_ms==UINT32_MAX||b.rtt_ms==UINT32_MAX){eligible=false;break;}
            total+=std::max(a.rtt_ms,b.rtt_ms);++count;quality.minimum_upload_bytes_per_second=std::min(quality.minimum_upload_bytes_per_second,a.upload_bytes_per_second);
            quality.worst_loss_per_mille=std::max({quality.worst_loss_per_mille,a.loss_per_mille,b.loss_per_mille});
        }
        if(eligible&&count){quality.mean_rtt_ms=std::uint32_t(total/count);result.push_back(quality);}
    }
    std::sort(result.begin(),result.end(),[](const auto &a,const auto &b){return std::tuple{a.worst_loss_per_mille,UINT32_MAX-a.minimum_upload_bytes_per_second,a.mean_rtt_ms,a.peer}<
        std::tuple{b.worst_loss_per_mille,UINT32_MAX-b.minimum_upload_bytes_per_second,b.mean_rtt_ms,b.peer};});return result;
}
}
