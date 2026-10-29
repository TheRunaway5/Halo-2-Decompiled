#include "halo2/source_shadow_transport.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/online_match_session.hpp"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <zlib.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <mutex>
#include <thread>

namespace halo2 {
namespace {
using Packet=std::vector<std::byte>;
using Digest=AuthorityTransferDigest;
using Clock=std::chrono::steady_clock;
constexpr std::uint64_t body_limit=8ull*1024*1024;
constexpr std::uint64_t frame_limit=authority_network_encoded_bytes+body_limit+16384;
void need(bool valid){if(!valid)throw FormatError("Invalid authenticated native shadow presentation stream");}
std::uint64_t now_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();}
std::uint64_t micros(Clock::time_point start){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();}
Digest digest(std::span<const std::byte> bytes){Digest out{};need(SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char*>(out.data())));return out;}
Digest name_digest(std::string_view name){return digest(std::as_bytes(std::span(name)));}
struct Writer {
    Packet bytes;
    void u(std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)bytes.push_back(std::byte(value>>(8*i)));}
    void raw(std::span<const std::byte> value){need(value.size()<=frame_limit-bytes.size());bytes.insert(bytes.end(),value.begin(),value.end());}
    void blob(std::span<const std::byte> value,std::uint64_t maximum){need(value.size()<=maximum);u(value.size(),4);raw(value);}
};
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    auto raw(std::uint64_t n){need(n<=bytes.size()-at);auto result=bytes.subspan(at,std::size_t(n));at+=std::size_t(n);return result;}
    std::uint64_t u(unsigned n){std::uint64_t value{};auto data=raw(n);for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint64_t>(data[i])<<(8*i);return value;}
    auto blob(std::uint64_t maximum){const auto n=u(4);need(n<=maximum);return raw(n);}
    Digest hash(){Digest out{};auto value=raw(out.size());std::copy(value.begin(),value.end(),out.begin());return out;}
    void end(){need(at==bytes.size());}
};
void context(Writer& w,const AuthorityTransferContext& c){w.raw(c.session);w.u(c.epoch,4);w.u(c.membership_revision,8);w.u(c.peers,2);w.u(c.host,1);}
AuthorityTransferContext context(Reader& r){AuthorityTransferContext c;c.session=r.hash();c.epoch=r.u(4);c.membership_revision=r.u(8);c.peers=r.u(2);c.host=r.u(1);need(c.host==0&&c.peers==3&&c.epoch==0&&c.membership_revision&&std::ranges::any_of(c.session,[](auto b){return b!=std::byte{};}));return c;}
void slots(Writer& w,std::span<const unsigned> values,bool allow_empty=false){need((allow_empty||!values.empty())&&values.size()<=4);w.u(values.size(),1);std::uint16_t seen{};for(auto slot:values){need(slot<16&&!(seen&(1u<<slot)));seen|=std::uint16_t(1u<<slot);w.u(slot,1);}}
std::vector<unsigned> slots(Reader& r,bool allow_empty=false){const auto n=r.u(1);need((allow_empty||n)&&n<=4);std::vector<unsigned> out;std::uint16_t seen{};for(unsigned i=0;i<n;++i){const auto slot=unsigned(r.u(1));need(slot<16&&!(seen&(1u<<slot)));seen|=std::uint16_t(1u<<slot);out.push_back(slot);}return out;}
void stamp(Writer& w,const SourceShadowSnapshotStamp& s){w.u(s.fingerprint,8);w.u(s.nonce,8);w.u(s.cookie,8);w.u(s.sequence,4);w.u(s.tick,4);w.u(s.bytes,4);w.raw(s.digest);}
SourceShadowSnapshotStamp stamp(Reader& r){SourceShadowSnapshotStamp s;s.fingerprint=r.u(8);s.nonce=r.u(8);s.cookie=r.u(8);s.sequence=r.u(4);s.tick=r.u(4);s.bytes=r.u(4);s.digest=r.hash();need(s.nonce&&s.cookie&&s.bytes&&s.bytes<=body_limit);return s;}
AuthorityTransferLimits transfer_limits(){AuthorityTransferLimits l;l.maximum_bytes=frame_limit;l.bytes_per_second=16ull*1024*1024;l.burst_bytes=128*1024;l.retry_ms=25;l.offer_ms=50;l.idle_ms=5000;l.lifetime_ms=30000;l.window_chunks=256;l.features=authority_feature_source_pipeline;return l;}
Packet compressed(std::span<const std::byte> bytes){need(!bytes.empty()&&bytes.size()<=body_limit);Packet out(compressBound(uLong(bytes.size())));uLongf n=out.size();need(compress2(reinterpret_cast<Bytef*>(out.data()),&n,reinterpret_cast<const Bytef*>(bytes.data()),uLong(bytes.size()),Z_BEST_SPEED)==Z_OK);out.resize(n);return out;}
Packet expanded(std::span<const std::byte> bytes,std::uint64_t size){need(size&&size<=body_limit&&!bytes.empty()&&size<=bytes.size()*2048+65536);Packet out(static_cast<std::size_t>(size));uLongf n=size;uLong consumed=bytes.size();need(uncompress2(reinterpret_cast<Bytef*>(out.data()),&n,reinterpret_cast<const Bytef*>(bytes.data()),&consumed)==Z_OK&&n==size&&consumed==bytes.size());return out;}
struct Admission {
    AuthorityTransferContext context;
    Digest digest{};
    std::vector<SourcePlayerAdmission> players;
    std::vector<OnlineAdmission> roster;
    std::vector<Digest> names;
    std::vector<unsigned> producer,local;
};
Packet admission_body(const Admission& a) {
    Writer w;context(w,a.context);slots(w,a.producer,true);slots(w,a.local);need(!a.players.empty()&&a.players.size()<=16&&a.players.size()==a.roster.size());w.u(a.players.size(),1);
    std::uint16_t seen{};
    for(unsigned i=0;i<a.players.size();++i){const auto& p=a.players[i];const auto& row=a.roster[i];need(p.slot<16&&row.slot==p.slot&&p.player!=0xffffffffu&&!(seen&(1u<<p.slot)));seen|=std::uint16_t(1u<<p.slot);
        w.u(p.slot,1);w.u(p.player,4);w.raw(std::as_bytes(std::span(p.identity)));w.u(online_admission_peer(row),1);w.u(row.guest,1);w.u(std::uint8_t(std::int8_t(row.team)),1);w.raw(row.identity);w.raw(a.names.at(i));w.u(row.matchmaking_level.value_or(0),1);}
    return std::move(w.bytes);
}
Admission read_admission(std::span<const std::byte> body) {
    Reader r{body};Admission a;a.context=context(r);a.producer=slots(r,true);a.local=slots(r);const auto count=r.u(1);need(count&&count<=16);std::uint16_t seen{};
    for(unsigned i=0;i<count;++i){SourcePlayerAdmission p;p.slot=r.u(1);p.player=r.u(4);auto identity=r.raw(p.identity.size());std::memcpy(p.identity.data(),identity.data(),identity.size());OnlineAdmission row;row.slot=p.slot;row.peer=r.u(1);const auto guest=r.u(1);need(guest<=1);row.guest=guest!=0;row.team=std::bit_cast<std::int8_t>(std::uint8_t(r.u(1)));auto public_id=r.raw(row.identity.size());std::copy(public_id.begin(),public_id.end(),row.identity.begin());auto name=r.hash();const auto level=r.u(1);need(level<=50);if(level)row.matchmaking_level=level;
        need(p.slot<16&&p.player!=0xffffffffu&&(p.player&65535u)==p.slot&&!(seen&(1u<<p.slot))&&row.peer<16&&row.team>=-1&&row.team<=7);seen|=std::uint16_t(1u<<p.slot);a.players.push_back(p);a.roster.push_back(std::move(row));a.names.push_back(name);}
    r.end();for(auto slot:a.producer)need(seen&(1u<<slot));for(auto slot:a.local)need(seen&(1u<<slot));a.digest=digest(body);return a;
}
Packet control(unsigned type,const Admission& a){Writer w;w.u(0x31504853u,4);w.u(type,1);if(type==0){auto body=admission_body(a);w.blob(body,1200);w.raw(a.digest);}else{context(w,a.context);w.raw(a.digest);}need(w.bytes.size()<=1300);return std::move(w.bytes);}
Packet transfer_packet(std::span<const std::byte> inner){Writer w;w.u(0x31504853u,4);w.u(2,1);w.raw(inner);need(w.bytes.size()<=1300);return std::move(w.bytes);}
bool same_roster(std::span<const OnlineAdmission> a,std::span<const OnlineAdmission> b){if(a.size()!=b.size())return false;for(unsigned i=0;i<a.size();++i)if(a[i].slot!=b[i].slot||a[i].name!=b[i].name||a[i].team!=b[i].team||a[i].identity!=b[i].identity||online_admission_peer(a[i])!=online_admission_peer(b[i])||a[i].guest!=b[i].guest||a[i].matchmaking_level!=b[i].matchmaking_level)return false;return true;}

class HostLane final:public SourceShadowHostLane {
    SourceMatchAuthority& authority_;std::vector<unsigned> producer_;std::vector<OnlineAdmission> accepted_;bool team_mode_{};
    const std::thread::id executor_=std::this_thread::get_id();AuthorityTransferContext context_;
    struct Pending {AuthorityCutSeal seal;AuthorityArchiveImage image;std::uint64_t publication{},sequence{},published_at_ms{};};
    struct Queued {Packet bytes;AuthorityArchiveImage image;SourceShadowTransportStatistics::Cut timing;};
    struct Pipe {
        SourceShadowPeer peer;Admission admission;bool admitted{};std::uint64_t offered_at{};
        std::unique_ptr<PeerAuthorityTransfer> transfer;std::optional<AuthorityArchiveImage> base;
        std::uint64_t base_serial{};std::deque<Pending> pending;std::optional<Queued> queued;
    };
    std::map<std::uint64_t,Pipe> pipes_;
    struct Emission {SourceShadowPeer peer;SourceShadowSnapshotStamp stamp;Packet body;};
    std::map<std::uint64_t,Emission> emitted_;
    struct Target {Emission emission;Admission admission;std::optional<AuthorityArchiveImage> base;std::uint64_t base_serial{};};
    struct Job {std::uint64_t generation;SourceMatchAuthority::Checkpoint checkpoint;std::vector<Target> targets;std::uint64_t capture_us{};};
    struct Encoded {Packet bytes;SourceShadowTransportStatistics::Cut timing;};
    struct Output {std::uint64_t generation{},tick{},encode_us{},full_bytes{},raw_bytes{};std::optional<AuthorityArchiveImage> image;std::map<std::uint64_t,Encoded> frames;std::string error;};
    mutable std::mutex mutex_;std::condition_variable_any cv_;std::optional<Job> job_;std::optional<Output> output_;bool working_{};std::jthread worker_;
    SourceShadowTransportStatistics stats_;
    std::vector<SourcePlayerAdmission> players_;
    std::vector<OnlineAdmission> rows_;
    std::size_t retained_images()const {
        // Peers may acknowledge different generations. Count each shared
        // immutable bank once, including queued and in-flight successors.
        std::set<const std::byte*> images;
        const auto add=[&](const AuthorityArchiveImage& image){images.insert(image.raw().data());};
        for(const auto& [key,p]:pipes_) {
            if(p.base)add(*p.base);
            if(p.queued)add(p.queued->image);
            for(const auto& cut:p.pending)add(cut.image);
        }
        return images.size();
    }
    bool room_for_image() {
        if(retained_images()<source_shadow_archive_host_images)return true;
        // ACKed banks are optional compression dictionaries. Evicting one
        // never acknowledges, cancels or discards a pending native cut; the
        // next capture for that peer simply uses a complete full envelope.
        // This lets a healthy peer progress when other peers retain old cuts.
        for(auto& [key,p]:pipes_)if(p.base) {
            p.base.reset();p.base_serial=0;
            if(retained_images()<source_shadow_archive_host_images)return true;
        }
        return false;
    }
    void next_generation() {
        need(context_.membership_revision!=UINT64_MAX);++context_.membership_revision;
        for(auto& [key,p]:pipes_){p.transfer.reset();p.base.reset();p.pending.clear();p.queued.reset();p.admitted=false;p.base_serial=0;}
    }
    void own()const{need(executor_==std::this_thread::get_id());}
    void encode(std::stop_token stop) {
        while(!stop.stop_requested()){
            std::optional<Job> job;{std::unique_lock lock(mutex_);cv_.wait(lock,stop,[&]{return job_.has_value();});if(stop.stop_requested())return;job=std::move(job_);job_.reset();working_=true;}
            Output out;out.generation=job->generation;out.tick=job->checkpoint.match.simulation.ticks;const auto started=Clock::now();
            try{
                out.image=encode_source_authority_image(job->checkpoint,source_shadow_archive_codec_limits());
                out.raw_bytes=out.image->decoded_size();const auto serialized_us=micros(started);
                std::optional<Packet> full;
                const auto full_archive=[&]() -> const Packet& {
                    if(!full)full=out.image->encode(source_shadow_archive_delta_limits());
                    return *full;
                };
                for(const auto& target:job->targets) {
                    const auto target_started=Clock::now();Packet native;bool delta=bool(target.base);
                    if(delta) {
                        try{native=encode_authority_delta(*target.base,*out.image,source_shadow_archive_delta_limits());}
                        catch(const std::invalid_argument&){delta=false;native=full_archive();}
                    }else native=full_archive();
                    SourceShadowTransportStatistics::Cut timing;
                    timing.generation=job->generation;timing.tick=out.tick;timing.sequence=target.emission.stamp.sequence;
                    timing.base_serial=delta?target.base_serial:0;timing.delta=delta;timing.raw_bytes=out.raw_bytes;
                    timing.native_bytes=native.size();timing.capture_us=job->capture_us;
                    Writer w;w.u(0x31434853u,4);w.u(job->generation,8);w.raw(target.admission.digest);stamp(w,target.emission.stamp);
                    w.u(delta,1);w.u(timing.base_serial,8);w.blob(native,authority_network_encoded_bytes);
                    w.u(target.emission.body.size(),4);auto body=compressed(target.emission.body);w.blob(body,body_limit+65536);
                    w.u(target.admission.roster.size(),1);
                    for(const auto& row:target.admission.roster){need(row.name.size()<=256);w.u(row.slot,1);w.blob(std::as_bytes(std::span(row.name)),256);}
                    need(w.bytes.size()<=frame_limit);timing.encode_us=serialized_us+micros(target_started);
                    out.frames.emplace(target.emission.peer.key,Encoded{std::move(w.bytes),timing});
                }
                out.full_bytes=full?full->size():0;
            }catch(const std::exception& e){out.error=e.what();out.frames.clear();out.image.reset();}
            out.encode_us=micros(started);{std::lock_guard lock(mutex_);working_=false;if(!stop.stop_requested())output_=std::move(out);}
        }
    }
    void update() {
        own();std::optional<Output> output;{std::lock_guard lock(mutex_);if(output_){output=std::move(output_);output_.reset();}}
        if(output) {
            stats_.encode_us+=output->encode_us;
            if(!output->error.empty()){
                if(stats_.error!=output->error&&std::getenv("HALO2_SHADOW_TRANSPORT_TRACE"))
                    std::fprintf(stderr,"[shadow-transport] native encode rejected generation=%llu tick=%llu error=%.512s\n",
                        static_cast<unsigned long long>(output->generation),static_cast<unsigned long long>(output->tick),output->error.c_str());
                stats_.error=output->error;++stats_.rejected;
            }
            else if(output->generation==context_.membership_revision) {
                stats_.error.clear();++stats_.encoded;stats_.full_bytes+=output->full_bytes;stats_.raw_bytes+=output->raw_bytes;
                for(auto& [key,frame]:output->frames)if(auto found=pipes_.find(key);found!=pipes_.end()) {
                    need(!found->second.queued);
                    if(frame.timing.delta){++stats_.delta_cuts;stats_.delta_bytes+=frame.timing.native_bytes;}else ++stats_.full_cuts;
                    found->second.queued.emplace(Queued{std::move(frame.bytes),*output->image,frame.timing});
                }
            }
        }
        for(auto& [key,p]:pipes_)if(p.transfer)for(const auto& receipt:p.transfer->validated_cuts()){
            const auto found=std::find_if(p.pending.begin(),p.pending.end(),[&](const auto& old){return old.seal==receipt.seal&&receipt.context==context_;});
            if(found==p.pending.end())continue;if(receipt.seal.id>p.base_serial){p.base=found->image;p.base_serial=receipt.seal.id;}
            stats_.latest_acknowledged_sequence=found->sequence;
            for(auto& timing:stats_.cuts)if(timing.publication==found->publication)
                timing.native_ready_us=(now_ms()-found->published_at_ms)*1000;
            while(!p.pending.empty()&&p.pending.front().seal.id<=receipt.seal.id)p.pending.pop_front();++stats_.acknowledged;
        }
    }
    std::vector<OnlineAdmission> roster(std::span<const SourcePlayerAdmission> players,const SourceMatchAuthority::Checkpoint& saved) {
        if(!accepted_.empty()){
            std::vector<OnlineAdmission> out;
            for(const auto& player:players){const auto at=std::ranges::find(accepted_,player.slot,&OnlineAdmission::slot);need(at!=accepted_.end());out.push_back(*at);}
            return out;
        }
        std::vector<OnlineAdmission> out;
        for(const auto& player:players){const auto& state=saved.match.players.at(player.slot);OnlineAdmission row;row.slot=player.slot;row.name=state.name;row.team=team_mode_?state.source_membership:-1;row.peer=player.slot;
            for(unsigned n=0;n<row.identity.size();++n)row.identity[n]=std::byte(player.identity[n]);
            if(std::ranges::find(producer_,player.slot)!=producer_.end())row.peer=producer_.front();
            else for(const auto& [key,p]:pipes_)if(std::ranges::find(p.peer.slots,player.slot)!=p.peer.slots.end())row.peer=p.peer.slots.front();
            row.guest=row.peer!=row.slot;out.push_back(std::move(row));}return out;
    }
public:
    HostLane(SourceMatchAuthority& authority,std::span<const unsigned> producer,std::span<const OnlineAdmission> accepted,bool team_mode)
        :authority_(authority),producer_(producer.begin(),producer.end()),accepted_(accepted.begin(),accepted.end()),team_mode_(team_mode) {
        Writer checked;slots(checked,producer_,true);context_.host=0;context_.peers=3;context_.membership_revision=1;
        need(RAND_bytes(reinterpret_cast<unsigned char*>(context_.session.data()),context_.session.size())==1);worker_=std::jthread([this](auto stop){encode(stop);});
    }
    ~HostLane()override{worker_.request_stop();cv_.notify_all();if(worker_.joinable())worker_.join();}
    void connection(const SourceShadowPeer& peer)override {own();need(peer.key&&peer.cookie&&peer.nonce&&!peer.slots.empty());need(pipes_.contains(peer.key)||pipes_.size()<source_shadow_archive_host_peers);auto [at,created]=pipes_.try_emplace(peer.key);if(created)at->second.peer=peer;else need(at->second.peer==peer);}
    void retire(std::uint64_t key)override{own();pipes_.erase(key);emitted_.erase(key);}
    void update_roster(std::span<const OnlineAdmission> roster)override {
        own();std::uint16_t seen{};
        for(const auto& row:roster){need(row.slot<16&&!(seen&(1u<<row.slot))&&online_admission_peer(row)<16);seen|=std::uint16_t(1u<<row.slot);
            const auto at=std::ranges::find(accepted_,row.slot,&OnlineAdmission::slot);
            if(at==accepted_.end())accepted_.push_back(row);else *at=row;}
    }
    bool ready(std::uint64_t key)override {
        update();const auto found=pipes_.find(key);need(found!=pipes_.end());auto& p=found->second;
        {std::lock_guard lock(mutex_);if(working_||job_||output_){++stats_.backpressure;return false;}}
        // Leave one whole-image slot for the next worker result. Slow peers
        // cannot turn independent exact-base retention into an unbounded bank.
        if(!room_for_image()){++stats_.backpressure;return false;}
        if(emitted_.contains(key)||p.queued){++stats_.backpressure;return false;}
        if(p.pending.empty())return true;
        // Bootstrap needs its first genuine native ACK before another capture:
        // a byte-delivered full archive does not yet supply a validated base.
        const bool ahead=p.base&&p.pending.size()==1&&p.transfer&&p.transfer->feature_supported(1,authority_feature_source_pipeline)&&p.transfer->delivered_to(context_,p.pending.front().seal,1);
        if(!ahead)++stats_.backpressure;return ahead;
    }
    void emitted(const SourceShadowPeer& peer,const SourceShadowSnapshotStamp& saved,std::span<const std::byte> body)override {
        own();need(saved.fingerprint==peer.fingerprint&&saved.nonce==peer.nonce&&saved.cookie==peer.cookie&&saved.bytes==body.size()&&digest(body)==saved.digest);connection(peer);
        if(emitted_.contains(peer.key))++stats_.coalesced;emitted_[peer.key]={peer,saved,Packet(body.begin(),body.end())};
    }
    bool pending()const noexcept override{return !emitted_.empty();}
    bool capture()override {
        update();if(emitted_.empty())return false;const auto started=Clock::now();
        try{
            {std::lock_guard lock(mutex_);need(!working_&&!job_&&!output_);}
            need(room_for_image());
            const auto tick=authority_.execution_tick();for(const auto& [key,e]:emitted_)need(e.stamp.tick==tick);
            // Capture the actual full allocated roster independently of the
            // incoming/native archive. No cut supplies these identities.
            const auto boundary=authority_.execution_boundary();auto players=authority_.allocated_player_admissions();
            const bool allocation_changed=!players_.empty()&&players_!=players;
            if(allocation_changed)next_generation();
            players_=players;stats_.generation=context_.membership_revision;
            Job job{context_.membership_revision,authority_.checkpoint(source_authority_binding(context_)),{}};
            auto rows=roster(players,job.checkpoint);
            if(!allocation_changed&&!rows_.empty()&&!same_roster(rows_,rows)) {
                next_generation();job.generation=context_.membership_revision;
                job.checkpoint=authority_.checkpoint(source_authority_binding(context_));
            }
            rows_=rows;stats_.generation=context_.membership_revision;need(authority_.execution_boundary()==boundary);
            need(job.checkpoint.match.simulation.ticks==tick);
            for(auto& [key,e]:emitted_)if(auto found=pipes_.find(key);found!=pipes_.end()){
                auto& p=found->second;
                if(!p.transfer){p.admission={context_,{},players,rows,{},producer_,p.peer.slots};for(const auto& row:rows)p.admission.names.push_back(name_digest(row.name));p.admission.digest=digest(admission_body(p.admission));p.transfer=std::make_unique<PeerAuthorityTransfer>(0,context_,transfer_limits());p.offered_at=0;}
                job.targets.push_back({std::move(e),p.admission,p.base,p.base_serial});
            }
            emitted_.clear();need(!job.targets.empty());const auto now=now_ms();if(stats_.latest_capture_at_ms)stats_.largest_capture_gap_ms=std::max(stats_.largest_capture_gap_ms,now-stats_.latest_capture_at_ms);stats_.latest_capture_at_ms=now;stats_.latest_tick=tick;stats_.capture_us+=micros(started);++stats_.captures;job.capture_us=micros(started);
            {std::lock_guard lock(mutex_);job_=std::move(job);}cv_.notify_one();return true;
        }catch(const std::exception& error){stats_.error=error.what();stats_.coalesced+=emitted_.size();emitted_.clear();++stats_.rejected;return false;}
    }
    void receive(std::uint64_t key,std::span<const std::byte> bytes,std::uint64_t now)override {
        own();auto found=pipes_.find(key);if(found==pipes_.end())return;auto& p=found->second;
        try{Reader r{bytes};need(r.u(4)==0x31504853u);const auto type=r.u(1);if(type==1){need(p.transfer&&context(r)==context_&&r.hash()==p.admission.digest);r.end();p.admitted=true;}else if(type==2){need(p.admitted&&p.transfer);p.transfer->receive(1,r.raw(bytes.size()-r.at),now);}else need(false);}catch(const std::exception&){++stats_.rejected;}
    }
    std::vector<Packet> poll(std::uint64_t key,std::uint64_t now)override {
        update();std::vector<Packet> out;auto found=pipes_.find(key);if(found==pipes_.end())return out;auto& p=found->second;if(!p.transfer)return out;
        if(!p.admitted){if(!p.offered_at||now-p.offered_at>=50){out.push_back(control(0,p.admission));p.offered_at=now;}return out;}
        if(p.queued&&p.transfer->supported(1)){
            AuthorityCutSeal seal;
            if(p.pending.empty())seal=p.transfer->publish(context_,p.queued->timing.tick,std::move(p.queued->bytes),std::array<unsigned,1>{1},now);
            else if(p.pending.size()==1&&p.transfer->delivered_to(context_,p.pending.front().seal,1))seal=p.transfer->publish_ahead(context_,p.pending.front().seal,p.queued->timing.tick,std::move(p.queued->bytes),1,now);
            if(seal.id){
                auto timing=p.queued->timing;timing.serial=seal.id;timing.publication=++stats_.published;
                p.pending.push_back({seal,p.queued->image,timing.publication,timing.sequence,now});
                stats_.cuts.push_back(timing);if(stats_.cuts.size()>16)stats_.cuts.erase(stats_.cuts.begin());p.queued.reset();
            }
        }
        for(auto& packet:p.transfer->poll(now,32)){
            stats_.wire_bytes+=packet.bytes.size();
            if(!p.pending.empty())for(auto& timing:stats_.cuts)if(timing.publication==p.pending.back().publication)
                timing.wire_bytes+=packet.bytes.size();
            out.push_back(transfer_packet(packet.bytes));
        }return out;
    }
    SourceShadowTransportStatistics statistics()const override {own();auto out=stats_;{std::lock_guard lock(mutex_);out.encoding=working_||job_.has_value();}for(const auto& [key,p]:pipes_)out.awaiting_native|=!p.pending.empty();return out;}
};

class ClientLane final:public SourceShadowClientLane {
    std::shared_ptr<OnlineMatchSession> online_;std::optional<SourceShadowPeer> peer_;std::optional<Admission> admission_;std::optional<AuthorityTransferContext> online_context_;
    std::unique_ptr<PeerAuthorityTransfer> transfer_;
    // Two cuts may be in flight while native validation advances. A cut names
    // its exact acknowledged base; it never silently uses the newest image.
    std::map<std::uint64_t,AuthorityArchiveImage> bases_;
    struct Delivered {AuthorityCutSeal seal;AuthorityArchiveImage image;SourceShadowPayload payload;std::uint64_t decoded_at_ms{};};
    std::map<std::uint64_t,Delivered> delivered_;std::deque<SourceShadowPayload> completed_;std::optional<SourceShadowPayload> acknowledged_;
    bool admission_ack_{};Digest binding_{};std::optional<Packet> pending_offer_;SourceShadowTransportStatistics stats_;
    void offer(std::span<const std::byte> body,const Digest& expected) {
        need(peer_&&digest(body)==expected);auto next=read_admission(body);
        if(!peer_->slots.empty())need(next.local==peer_->slots);
        if(online_){const auto context=online_->authority_context();const auto native=online_->source_players();if(!context||!native)return;
            need(native->context==*context);
            // Recovery retains its active-roster contract. This independent
            // presentation admission also names native allocations retained
            // after departure; every currently accepted ONLINE player still
            // has to match its independently delivered full source identity.
            for(const auto& player:native->players)need(std::ranges::find(next.players,player)!=next.players.end());
            const auto roster=online_->roster();for(const auto& row:roster){const auto at=std::ranges::find(next.roster,row.slot,&OnlineAdmission::slot);need(at!=next.roster.end());const auto i=std::size_t(at-next.roster.begin());need(online_admission_peer(row)==online_admission_peer(*at)&&row.guest==at->guest&&row.team==at->team&&row.identity==at->identity&&row.matchmaking_level==at->matchmaking_level&&name_digest(row.name)==next.names[i]);}online_context_=context;
        }
        if(admission_){need(next.context.session==admission_->context.session&&next.context.membership_revision>=admission_->context.membership_revision);
            if(next.context==admission_->context){need(next.digest==admission_->digest);admission_ack_=true;return;}}
        admission_=std::move(next);binding_=source_authority_binding(admission_->context);transfer_=std::make_unique<PeerAuthorityTransfer>(1,admission_->context,transfer_limits());bases_.clear();delivered_.clear();completed_.clear();acknowledged_.reset();admission_ack_=true;stats_.generation=admission_->context.membership_revision;
    }
    void decode(ReceivedAuthorityCut value) {
        const auto started=Clock::now();
        need(admission_&&value.context==admission_->context&&value.peer==0&&value.bytes&&value.bytes->size()<=frame_limit);
        Reader r{*value.bytes};need(r.u(4)==0x31434853u&&r.u(8)==admission_->context.membership_revision&&r.hash()==admission_->digest);
        SourceShadowPayload payload;payload.snapshot=stamp(r);const auto delta=r.u(1);const auto base_serial=r.u(8);need(delta<=1&&bool(delta)==bool(base_serial)&&base_serial<value.seal.id&&payload.snapshot.tick==value.seal.tick);
        const auto encoded=r.blob(authority_network_encoded_bytes);const auto base=bases_.find(base_serial);
        AuthorityArchiveImage image=delta?(need(base!=bases_.end()),apply_authority_delta(base->second,encoded,source_shadow_archive_delta_limits())):AuthorityArchiveImage::read(encoded,source_shadow_archive_delta_limits());
        // This immutable complete named-field image already passed envelope
        // or exact-base delta checks. Native preparation still decodes/checks
        // its schema and digest; it need not recompress and expand it again.
        payload.archive=std::make_shared<const AuthorityArchiveImage>(image);
        const auto body_size=r.u(4);payload.snapshot_body=std::make_shared<const Packet>(expanded(r.blob(body_limit+65536),body_size));need(body_size==payload.snapshot.bytes&&digest(*payload.snapshot_body)==payload.snapshot.digest);
        auto roster=admission_->roster;need(r.u(1)==roster.size());for(unsigned i=0;i<roster.size();++i){need(r.u(1)==roster[i].slot);const auto name=r.blob(256);roster[i].name.assign(reinterpret_cast<const char*>(name.data()),name.size());need(name_digest(roster[i].name)==admission_->names[i]);}r.end();
        payload.serial=value.seal.id;payload.generation=admission_->context.membership_revision;payload.binding=source_authority_binding(admission_->context);payload.admission_digest=admission_->digest;payload.world_identity=admission_->context.session;payload.admissions=admission_->players;payload.roster=std::move(roster);payload.producer_views=admission_->producer;payload.local_views=admission_->local;
        SourceShadowTransportStatistics::Cut timing;
        timing.generation=payload.generation;timing.serial=payload.serial;timing.tick=payload.snapshot.tick;
        timing.sequence=payload.snapshot.sequence;timing.base_serial=base_serial;timing.delta=delta;
        timing.native_bytes=encoded.size();timing.raw_bytes=image.decoded_size();timing.archive_decode_us=micros(started);
        stats_.cuts.push_back(timing);if(stats_.cuts.size()>16)stats_.cuts.erase(stats_.cuts.begin());
        need(delivered_.size()<source_shadow_archive_inflight);delivered_.emplace(payload.serial,Delivered{value.seal,std::move(image),payload,now_ms()});completed_.push_back(payload);++stats_.received;stats_.latest_tick=value.seal.tick;
    }
public:
    explicit ClientLane(std::shared_ptr<OnlineMatchSession> online):online_(std::move(online)){}
    bool requires_continuity()const noexcept override{return bool(online_);}
    void connection(const SourceShadowPeer& next)override {
        need(next.key&&next.cookie&&next.nonce);
        if(peer_&&(peer_->key!=next.key||peer_->fingerprint!=next.fingerprint||peer_->nonce!=next.nonce||peer_->cookie!=next.cookie||peer_->revision!=next.revision)){
            admission_.reset();transfer_.reset();bases_.clear();delivered_.clear();completed_.clear();acknowledged_.reset();pending_offer_.reset();online_context_.reset();
        }
        if(admission_&&!next.slots.empty())need(next.slots==admission_->local);peer_=next;
    }
    void receive(std::span<const std::byte> bytes,std::uint64_t now)override {
        try{Reader r{bytes};need(r.u(4)==0x31504853u);const auto type=r.u(1);if(type==0){const auto body=r.blob(1200);const auto hash=r.hash();r.end();offer(body,hash);if(!admission_)pending_offer_=Packet(bytes.begin(),bytes.end());}
            else if(type==2){need(transfer_&&admission_);transfer_->receive(0,r.raw(bytes.size()-r.at),now);}else need(false);
            if(transfer_)while(auto value=transfer_->take_completed()){const auto received_context=value->context;const auto seal=value->seal;try{decode(std::move(*value));stats_.error.clear();}catch(const std::exception& e){transfer_->reject_delivered(received_context,seal);stats_.error=e.what();++stats_.rejected;}}
        }catch(const std::exception& e){stats_.error=e.what();++stats_.rejected;}
    }
    std::vector<Packet> poll(std::uint64_t now)override {
        if(pending_offer_&&!admission_){auto packet=std::move(*pending_offer_);pending_offer_.reset();receive(packet,now);}
        std::vector<Packet> out;if(!admission_||!transfer_)return out;
        if(admission_ack_){out.push_back(control(1,*admission_));admission_ack_=false;}
        for(auto& packet:transfer_->poll(now,32)){stats_.wire_bytes+=packet.bytes.size();out.push_back(transfer_packet(packet.bytes));}return out;
    }
    std::optional<SourceShadowPayload> completed()override {if(completed_.empty())return {};auto value=std::move(completed_.front());completed_.pop_front();return value;}
    bool acknowledge(const SourceShadowPayload& payload)override {
        if(!current(payload))return false;if(acknowledged_&&acknowledged_->serial==payload.serial&&acknowledged_->archive==payload.archive)return true;
        const auto found=delivered_.find(payload.serial);if(found==delivered_.end()||found->second.payload.archive!=payload.archive||found->second.payload.snapshot!=payload.snapshot)return false;
        if(!transfer_->acknowledge_validated(admission_->context,found->second.seal))return false;
        for(auto& timing:stats_.cuts)if(timing.generation==payload.generation&&timing.serial==payload.serial)
            timing.native_ready_us=(now_ms()-found->second.decoded_at_ms)*1000;
        stats_.latest_acknowledged_sequence=payload.snapshot.sequence;
        bases_.insert_or_assign(payload.serial,found->second.image);while(bases_.size()>source_shadow_archive_acknowledged_bases)bases_.erase(bases_.begin());
        acknowledged_=payload;for(auto at=delivered_.begin();at!=delivered_.end()&&at->first<=payload.serial;)at=delivered_.erase(at);++stats_.acknowledged;return true;
    }
    bool current(const SourceShadowPayload& payload)const noexcept override {
        try{return peer_&&admission_&&payload.world_identity==admission_->context.session&&payload.generation==admission_->context.membership_revision&&payload.admission_digest==admission_->digest&&payload.binding==binding_&&payload.snapshot.fingerprint==peer_->fingerprint&&payload.snapshot.nonce==peer_->nonce&&payload.snapshot.cookie==peer_->cookie&&(!online_||online_->authority_context()==online_context_);}catch(...){return false;}
    }
    SourceShadowTransportStatistics statistics()const override{auto out=stats_;out.awaiting_native=!delivered_.empty();return out;}
};
}
void LanHost::enable_shadow_lane(SourceMatchAuthority& authority,std::span<const unsigned> producer,std::span<const OnlineAdmission> roster) {
    need(authority.for_match(continuation_match()));
    bind_shadow_lane(std::make_unique<HostLane>(authority,producer,roster,continuation_match().team_mode()));
}
void LanClient::enable_shadow_lane(std::shared_ptr<OnlineMatchSession> online) {
    if(shadow_lane_enabled())return;
    bind_shadow_lane(std::make_unique<ClientLane>(std::move(online)));
}
}
