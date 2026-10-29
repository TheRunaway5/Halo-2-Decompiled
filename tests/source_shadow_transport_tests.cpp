#include "source_match_rebind_fixture.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_shadow_transport.hpp"
#include "halo2/source_shadow_presentation_owner.hpp"
#include "halo2/source_native_render_timeline.hpp"
#include "halo2/multiplayer_world.hpp"
#include <chrono>
#include <fstream>
#include <future>
#include <deque>
#include <mutex>
#include <thread>
#include <nlohmann/json.hpp>

namespace {
using Wall=std::chrono::steady_clock;
using namespace std::chrono_literals;
std::uint64_t us(Wall::time_point start){return std::chrono::duration_cast<std::chrono::microseconds>(Wall::now()-start).count();}
thread_local const char* phase="test entry";
constexpr std::uint64_t fingerprint=0x534841444f575431ull;
struct ProducerResult {
    SourceShadowTransportStatistics transport;
    std::uint64_t receive_us{},broadcast_us{},step_us{},receive_calls{},broadcast_calls{},largest_service_gap_us{},ticks{};
};
class Producer {
    std::promise<std::string> address_;
    std::mutex mutex_;std::exception_ptr error_;ProducerResult result_;
public:
    std::atomic<std::uint64_t> tick{},acknowledged_sequence{};
    std::string address;
private:
    // Stop/join before every field captured by this executor, including on
    // constructor readiness timeout where the class destructor does not run.
    std::jthread worker_;
public:
    Producer(const char* xbe,const char* map,bool dedicated,bool production=false) {
        auto ready=address_.get_future();
        worker_=std::jthread([this,xbe=std::string(xbe),map=std::string(map),dedicated,production](std::stop_token stop) {
            bool announced{};
            try {
                const std::array<unsigned,1> local{0};
                const auto views=dedicated?std::span<const unsigned>{}:std::span<const unsigned>(local);
                phase="construct actual native producer";
                std::unique_ptr<Owner> legacy;std::unique_ptr<MultiplayerPreparedWorld> prepared;
                std::unique_ptr<SourceMatchAuthority> prepared_authority;
                if(production) {
                    MultiplayerOptions options;options.map=map;options.gameplay_xbe=xbe;options.dedicated=dedicated;
                    options.local_players=dedicated?0:1;options.variant={"Slayer",0,{{0x59,5}}};
                    prepared=std::make_unique<MultiplayerPreparedWorld>(options);
                    // The ordinary runtime completes its authored local-view
                    // definition bank even for a true zero-user dedicated
                    // host. Exercise that real empty-view call, without a
                    // synthetic player/input user or substitute view0.
                    prepared->bind_source_local_views(views);
                    prepared_authority=std::make_unique<SourceMatchAuthority>(prepared->match,*prepared->source_objects);
                }else legacy=std::make_unique<Owner>(xbe.c_str(),map.c_str(),views,1000,std::span<const OnlineAdmission>{},
                    std::function<std::uint32_t()>{},std::span<const unsigned>{},dedicated);
                auto& match=production?prepared->match:*legacy->match;
                auto& authority=production?*prepared_authority:*legacy->authority;
                phase="construct actual LanHost";LanHost host(match,0,fingerprint);
                phase="enable zero/local native lane";host.enable_shadow_lane(authority,views);
                address_.set_value(host.join_address());announced=true;
                std::array<Controls,16> controls{};ProducerResult observed;
                auto next=Wall::now(),last_service=next;
                while(!stop.stop_requested()) {
                    const auto started=Wall::now();observed.largest_service_gap_us=std::max(observed.largest_service_gap_us,us(last_service));last_service=started;
                    phase="authenticated native host receive";host.receive(controls);observed.receive_us+=us(started);++observed.receive_calls;
                    const auto step=Wall::now();
                    for(unsigned slot=0;slot<16;++slot)if(match.player(slot).connected&&
                        match.player(slot).source_current_object!=null_datum)controls[slot].yaw=match.unit(slot).yaw;
                    // A real zero-local dedicated host has no active match
                    // before its first authenticated native participant.
                    phase="native producer simulation tick";if(!dedicated||host.peers()) {
                        if(production)match.step(controls);else legacy->step(controls);
                    }
                    observed.step_us+=us(step);tick=match.simulation().ticks();
                    const auto broadcast=Wall::now();phase="ordinary native host broadcast";host.broadcast();observed.broadcast_us+=us(broadcast);++observed.broadcast_calls;
                    phase="completed native host capture";if(host.shadow_publication_pending())host.capture_shadow_state();
                    const auto stats=host.shadow_transport_statistics();acknowledged_sequence=stats.latest_acknowledged_sequence;
                    next+=33333us;std::this_thread::sleep_until(next);
                    if(next<Wall::now()-100ms)next=Wall::now();
                }
                observed.transport=host.shadow_transport_statistics();observed.ticks=match.simulation().ticks();
                std::lock_guard lock(mutex_);result_=std::move(observed);
            }catch(...) {
                auto error=std::current_exception();
                try{std::rethrow_exception(error);}catch(const std::exception& e){std::cerr<<"Native producer failure phase="<<phase<<" error="<<e.what()<<std::endl;}
                {std::lock_guard lock(mutex_);error_=error;}
                if(!announced)address_.set_exception(error);
            }
        });
        check(ready.wait_for(45s)==std::future_status::ready,"Actual native rawUDP producer construction timed out");address=ready.get();
    }
    ~Producer(){worker_.request_stop();if(worker_.joinable())worker_.join();}
    void check_error(){std::lock_guard lock(mutex_);if(error_)std::rethrow_exception(error_);}
    ProducerResult finish(){worker_.request_stop();worker_.join();check_error();return result_;}
};
nlohmann::json production_timeline(const char* xbe,const char* map,bool dedicated) {
    // Its producer and receiver both use the canonical production constructor.
    // The low-level ten-cut oracle below deliberately owns a different native
    // service/configuration set; no compatibility guard is waived to reuse it.
    Producer host(xbe,map,dedicated,true);
    const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
    std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
    const auto profile=native_selected_player_configuration("Timeline client",appearance,-1);
    MultiplayerOptions options;options.map=map;options.gameplay_xbe=xbe;options.join=host.address;
    options.local_players=1;options.selected_profiles={profile};options.variant={"Slayer",0,{{0x59,5}}};
    MultiplayerPreparedWorld resident(options);SourceShadowPresentationOwner presentation(options,resident);
    SourceNativeRenderTimeline timeline;std::optional<SourceNativeRenderFrameContext> retained;
    LanClient client(host.address,fingerprint,1,{},std::span(&profile,1));client.enable_shadow_lane();
    std::array<Controls,1> input{};const auto started=Wall::now(),until=started+90s;
    bool pre_ACK_rejected{},sparse_pair{},identity_rejected{},retained_body{};
    unsigned frames{};std::uint32_t last_sequence{};nlohmann::json rows=nlohmann::json::array();
    while(frames<3) {
        check(Wall::now()<until,"Production native owner/timeline pairing timed out");host.check_error();
        client.send(input);client.receive();check(client.error().empty(),client.error().c_str());
        for(const auto& cut:client.received_shadow_cuts()) {
            const auto origin=cut.origin();const auto adopt=Wall::now();
            phase="production foreground presentation adoption";
            check(presentation.adopt(client,cut,origin),"Production presentation owner rejected its genuine production cut");
            const auto adopt_us=us(adopt);
            check(presentation.world(client,origin)&&presentation.match(client,origin),
                "Actual production adoption did not expose its exact foreground native world");
            bool rejected{};
            try{timeline.capture(client,presentation,cut,double(us(started))/1000000.0);}
            catch(const FormatError& e){rejected=std::string_view(e.what()).find("adopted and acknowledged")!=std::string_view::npos;}
            check(rejected,"Native timeline accepted a production world before its actual native ACK");pre_ACK_rejected=true;
            check(client.acknowledge_shadow_cut(cut),"Production native adoption did not issue actual ACK");
            const auto seconds=double(us(started))/1000000.0;
            timeline.capture(client,presentation,cut,seconds);
            const auto frame=timeline.frame(client,presentation,client.state_origin(),client.state(),seconds+.001);
            check(frame&&frame->origin().same(origin)&&frame->world()==presentation.world(client,origin)&&
                frame->match().simulation().ticks()==*origin.tick()&& &frame->current()==&origin.accepted_state(),
                "Production timeline mixed its exact historical body with another native world or latest gameplay");
            timeline.capture(client,presentation,cut,seconds+.001);
            const auto duplicate=timeline.frame(client,presentation,client.state_origin(),client.state(),seconds+.001);
            check(duplicate&&duplicate->blend()==frame->blend(),"Duplicate native capture restarted its interpolation clock");
            if(retained) {
                const auto old_tick=retained->current().tick;
                check(retained->origin().accepted_state().tick==old_tick&&old_tick<frame->current().tick,
                    "A successor native owner replaced its retained immutable historical body");retained_body=true;
                if(frame->previous().tick==old_tick&&frame->current().tick-old_tick>1)sparse_pair=true;
            }
            auto changed=client.state();changed.players[cut.local_views().front()].source_unit^=0x10000u;
            check(!timeline.frame(client,presentation,client.state_origin(),changed,seconds+.001),
                "Native timeline ignored changed local player/unit identity in current gameplay");identity_rejected=true;
            // Complete the synchronous draw before adopting its successor.
            // Only immutable body/interpolation values are inspected later;
            // native world/Match pointers are never used after their borrow.
            retained=frame;last_sequence=*origin.sequence();++frames;
            rows.push_back({{"tick",*origin.tick()},{"sequence",last_sequence},{"adopt_us",adopt_us},
                {"previous_tick",frame->previous().tick},{"serial",frame->serial()},{"generation",frame->generation()}});
            if(frames==3)break;
        }
        std::this_thread::sleep_for(1ms);
    }
    while(host.acknowledged_sequence<last_sequence) {
        check(Wall::now()<until,"Production native owner/timeline final ACK did not reach host");host.check_error();
        client.send(input);client.receive();std::this_thread::sleep_for(1ms);
    }
    const auto observed=host.finish();
    check(pre_ACK_rejected&&sparse_pair&&identity_rejected&&retained_body,
        "Production timeline omitted a real adopted/ACKed sparse pair, retained body or local-identity control");
    return {{"scope","Canonical MultiplayerPreparedWorld producer and independent resident/presentation owner, actual rawUDP native cuts and local ACK; no GPU proof"},
        {"frames",frames},{"pre_ACK_capture_rejected",pre_ACK_rejected},{"sparse_actual_native_pair",sparse_pair},
        {"local_identity_mismatch_rejected",identity_rejected},{"immutable_body_retained_through_successor",retained_body},
        {"native_host_ACKs",observed.transport.acknowledged},{"cuts",rows}};
}
}

int main(int argc,char** argv)try {
    if(argc<3 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    check(argc==4||(argc==5&&std::string_view(argv[4])=="dedicated"),"Supply native reference, map, report and optional dedicated producer");
    check(std::getenv("HALO2_TEST_SCRATCH"),"Native shadow transport must run in owned test scratch");
    const bool dedicated=argc==5;const std::array<unsigned,1> local{0};
    const auto producer_views=dedicated?std::span<const unsigned>{}:std::span<const unsigned>(local);
    Producer host(argv[1],argv[2],dedicated);
    phase="construct recipient prototypes";Owner recipient(argv[1],argv[2],local,900000);
    phase="construct producer-view workspace";Owner workspace(argv[1],argv[2],producer_views,500000);
    const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
    std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
    auto profile=native_selected_player_configuration("Shadow client",appearance,-1);
    LanClient client(host.address,fingerprint,1,{},std::span(&profile,1));client.enable_shadow_lane();
    LanClient foreign(host.address,fingerprint,1,{},std::span(&profile,1));
    std::array<Controls,1> input{};
    const auto wait_without_receive=[&](Wall::duration duration) {
        // Keep real accepted gameplay input fresh. Presentation Ready is
        // deliberately not a gameplay liveness signal; this control isolates
        // ACK flushing from the independent five-second held-input timeout.
        const auto until=Wall::now()+duration;
        do{client.send(input);std::this_thread::sleep_for(33ms);}while(Wall::now()<until);
    };
    constexpr unsigned count=10;std::uint64_t accepted{},first_sequence{},last_sequence{};
    std::optional<AcceptedShadowCut> previous;nlohmann::json cuts=nlohmann::json::array();
    std::deque<AcceptedShadowCut> pending;
    std::optional<std::uint32_t> held_at_tick;
    Wall::time_point held_at{};std::uint64_t ordinary_messages{},hold_messages{};
    std::uint64_t ordinary_at_hold{};double ordinary_hz{};
    bool first_hold_complete{},historical_behind_latest{},ack_flush_checked{};
    const auto started=Wall::now(),until=started+120s;auto diagnostic_at=started+5s;
    std::uint64_t client_receive_us{},client_send_us{},receive_calls{},send_calls{},previous_receive_us{},previous_send_us{};
    while(accepted<count) {
        check(Wall::now()<until,"Dense authenticated rawUDP native shadow delivery timed out");host.check_error();
        auto service=Wall::now();phase="actual client input send";client.send(input);client_send_us+=us(service);++send_calls;
        service=Wall::now();phase="actual client paired receive";client.receive();client_receive_us+=us(service);++receive_calls;
        ordinary_messages+=client.received_origins().size();
        for(const auto& cut:client.received_shadow_cuts()) {
            check(client.owns_shadow_cut(cut,cut.origin())&&!foreign.owns_shadow_cut(cut,cut.origin()),
                "Historical native cut escaped its actual admitted client");
            if(client.state_origin().sequence()&&std::int32_t(*client.state_origin().sequence()-*cut.origin().sequence())>0) {
                historical_behind_latest=true;
                check(client.state().tick>=*cut.origin().tick(),"Historical native cut rolled gameplay clock back");
                if(!client.received_origins().empty())check(client.state_origin().same(client.received_origins().back()),
                    "Historical native cut replaced the latest ordinary gameplay receipt");
            }
            pending.push_back(cut);
            if(!held_at_tick){held_at_tick=client.state().tick;held_at=Wall::now();ordinary_at_hold=ordinary_messages;}
        }
        if(!first_hold_complete&&held_at_tick&&client.state().tick-*held_at_tick>=30) {
            hold_messages=ordinary_messages-ordinary_at_hold;
            ordinary_hz=double(hold_messages)*1000000.0/double(us(held_at));
            check(hold_messages>=25&&ordinary_hz>=25.0,
                "Ordinary host snapshots stopped their actual30Hz cadence behind an unACKed native cut");
            check(client.shadow_transport_statistics().acknowledged==0,
                "Delayed native cadence control acknowledged before real import");
            first_hold_complete=true;
        }
        if(Wall::now()>=diagnostic_at) {
            const auto stats=client.shadow_transport_statistics();
            std::cout<<"Native shadow stream progress: received="<<stats.received<<" accepted="<<accepted
                <<" acknowledged_sequence="<<host.acknowledged_sequence<<" producer_tick="<<host.tick
                <<" client_rejected="<<stats.rejected<<" error="<<stats.error<<" connection_error="<<client.error()<<std::endl;
            diagnostic_at=Wall::now()+5s;
        }
        while(first_hold_complete&&!pending.empty()) {
            const auto held=pending.front();pending.pop_front();
            const auto origin=held.origin();const auto cut=client.shadow_cut(origin);
            check(cut.has_value(),"Historical native event lost its exact owning cut");
            check(client.owns_shadow_cut(*cut,origin)&&!foreign.owns_shadow_cut(*cut,origin),"Native cut escaped its actual admitted client");
            check(origin.tick()&&origin.sequence()&&origin.accepted_state().tick==*origin.tick(),"RawLAN cut did not own its actual decoded body");
            check(!origin.context(),"RawLAN presentation invented ONLINE or migration authority");
            bool kind_rejected{};
            try{client.prove_snapshot(LanHost::Continuation{},ValidatedAuthorityCut{},origin);}
            catch(const FormatError& e){kind_rejected=std::string_view(e.what()).find("Historical native or repaired gameplay receipt")!=std::string_view::npos;}
            check(kind_rejected,"Historical native receipt escaped its candidate/recovery fence");
            check(!client.acknowledged_shadow_cut(*cut)&&!client.owns_native_render_pair(*cut,client.state_origin()),
                "Native render pair claimed an ACK before actual preparation");
            check(cut->producer_views().empty()==dedicated,"Actual dedicated native producer invented a local view");
            check(std::ranges::equal(cut->producer_views(),producer_views),"Native cut changed the actual producer views");
            if(dedicated)check(cut->admissions().size()==1&&cut->local_views().size()==1&&cut->local_views()[0]==0,
                "Dedicated native admission assumed a synthetic host seat instead of the actual first remote slot0");
            phase="strict complete native archive decode";const auto decode=Wall::now();
            const auto saved=decode_source_authority_image(cut->archive_image(),source_shadow_archive_codec_limits());
            const auto decode_us=us(decode);
            check(saved.match.simulation.ticks==*origin.tick()&&saved.match.admitted_binding==cut->binding(),"Native cut differs from its exact accepted snapshot");
            std::vector<SourcePlayerAdmission> local_admissions;
            for(auto slot:cut->local_views()) {
                const auto at=std::ranges::find(cut->admissions(),slot,&SourcePlayerAdmission::slot);
                check(at!=cut->admissions().end(),"Raw recipient local view has no independent full native admission");local_admissions.push_back(*at);
                if(!recipient.match->player(slot).connected) {
                    const auto row=std::ranges::find(cut->roster(),slot,&OnlineAdmission::slot);check(row!=cut->roster().end(),"Raw recipient prototype omitted actual accepted seat");
                    check(recipient.match->join(row->name,std::int8_t(row->team),native_selected_player_configuration(row->name,appearance,std::int8_t(row->team)),slot)==slot,
                        "Actual recipient prototype could not select the admitted slot");
                }
            }
            phase="actual recipient prototype view bind";recipient.local_views(cut->local_views());
            phase="complete native producer-to-recipient preparation";const auto prepare=Wall::now();
            auto prepared=recipient.authority->prepare_with_local_views(saved,cut->binding(),local_admissions,*workspace.authority,cut->admissions(),cut->producer_views());
            const auto prepare_us=us(prepare);const auto commit=Wall::now();
            phase="complete native recipient commit";check(recipient.authority->commit(std::move(prepared)),"Actual recipient native preparation/commit failed");
            const auto commit_us=us(commit);local_views(recipient,cut->local_views());
            check(recipient.authority->execution_tick()==*origin.tick(),"Native recipient imported a different frame");
            if(previous) {
                check(previous->origin().tick()&&*origin.tick()>*previous->origin().tick(),
                    "Ten-cut cadence control accepted repeated or older native simulation ticks");
                check(previous->origin().sequence()&&*origin.sequence()>*previous->origin().sequence(),
                    "Ten-cut cadence control accepted repeated or older native body sequences");
                check(previous->same_world(*cut)&&previous->same_generation(*cut),"Unchanged rawLAN native admission lost its generation");
                check(!client.owns_shadow_cut(*cut,previous->origin()),"Matching peer/tick metadata forged another body receipt");
            }
            if(!accepted)wait_without_receive(2s); // Actual import complete; controlled cold-owner gap before ACK.
            check(client.acknowledge_shadow_cut(*cut),"Natively imported exact cut did not acknowledge");
            check(client.acknowledge_shadow_cut(*cut),"Identical native acknowledgement was not idempotent");
            check(client.acknowledged_shadow_cut(*cut)&&client.owns_native_render_pair(*cut,client.state_origin()),
                "Actual native ACK did not admit its exact historical/current connection pair");
            if(!accepted) {
                // The cold caller does not pump receive. Ready must already
                // have left through its genuine authenticated ACK owner.
                wait_without_receive(4s);
                check(host.acknowledged_sequence>=*origin.sequence(),
                    "Actual native ACK waited for next client receive and expired behind a cold frame");
                ack_flush_checked=true;
            }
            if(!accepted)first_sequence=*origin.sequence();last_sequence=*origin.sequence();previous=*cut;++accepted;
            cuts.push_back({{"tick",*origin.tick()},{"sequence",*origin.sequence()},{"producer_tick_at_native_ready",host.tick.load()},
                {"named_field_decode_us",decode_us},{"native_prepare_us",prepare_us},{"native_commit_us",commit_us},
                {"client_receive_service_us",client_receive_us-previous_receive_us},{"client_send_service_us",client_send_us-previous_send_us}});
            previous_receive_us=client_receive_us;previous_send_us=client_send_us;
            if(accepted==count)break;
        }
        std::this_thread::sleep_for(1ms);
    }
    const auto ack_until=Wall::now()+15s;
    while(host.acknowledged_sequence<last_sequence) {
        check(Wall::now()<ack_until,"Exact last native shadow acknowledgement did not reach producer");host.check_error();
        client.send(input);client.receive();std::this_thread::sleep_for(1ms);
    }
    const auto observed=host.finish();const auto& hs=observed.transport;const auto cs=client.shadow_transport_statistics();
    check(first_hold_complete&&historical_behind_latest&&ack_flush_checked,
        "Raw native transport omitted dense ordinary delivery, older historical receipt or immediate authenticated ACK controls");
    check(first_sequence<last_sequence&&hs.captures>=count&&hs.encoded>=count&&cs.received>=count&&hs.delta_cuts>0,
        "Native presentation did not progress actual snapshots with validated exact-base reuse");
    check(hs.full_cuts==1&&hs.delta_cuts>=count-1,
        "Unchanged native admission sent a redundant full archive before genuine native-base acknowledgement");
    check(observed.ticks>=cuts.back()["tick"].get<std::uint64_t>()&&observed.ticks>cuts.front()["tick"].get<std::uint64_t>(),
        "Continuously advancing native producer did not reach the genuinely imported tick frontier");
    for(auto& row:cuts)for(const auto& source:hs.cuts)if(source.sequence==row["sequence"].get<std::uint64_t>()) {
        row["producer"]={{"delta",source.delta},{"base_serial",source.base_serial},{"serial",source.serial},
            {"raw_bytes",source.raw_bytes},{"native_payload_bytes",source.native_bytes},{"transfer_service_bytes",source.wire_bytes},
            {"capture_us",source.capture_us},{"encode_us",source.encode_us},{"native_ready_us",source.native_ready_us}};
    }
    for(auto& row:cuts)for(const auto& received:cs.cuts)if(received.sequence==row["sequence"].get<std::uint64_t>())
        row["archive_receive_decode_us"]=received.archive_decode_us;
    const auto timeline_report=production_timeline(argv[1],argv[2],dedicated);
    nlohmann::json report{{"passed",true},{"scope","Actual authenticated rawUDP with independently advancing native producer, exact gameplay/native pairs, complete named-field schema decode, actual recipient view rebind/import before ACK; no GPU/ONLINE acceptance claim"},
        {"dedicated_zero_local_producer",dedicated},{"accepted_cuts",accepted},{"producer_ticks",observed.ticks},{"elapsed_us",us(started)},
        {"captures",hs.captures},{"encoded",hs.encoded},{"acknowledged",hs.acknowledged},{"latest_native_ack_sequence",hs.latest_acknowledged_sequence},
        {"full_bytes",hs.full_bytes},{"delta_bytes",hs.delta_bytes},{"raw_bytes",hs.raw_bytes},{"full_cuts",hs.full_cuts},{"delta_cuts",hs.delta_cuts},
        {"application_transfer_bytes",hs.wire_bytes},{"capture_us",hs.capture_us},{"encode_us",hs.encode_us},{"largest_capture_gap_ms",hs.largest_capture_gap_ms},
        {"producer_receive_us",observed.receive_us},{"producer_broadcast_us",observed.broadcast_us},{"producer_step_us",observed.step_us},
        {"producer_receive_calls",observed.receive_calls},{"producer_broadcast_calls",observed.broadcast_calls},{"largest_producer_service_gap_us",observed.largest_service_gap_us},
        {"client_receive_service_us",client_receive_us},{"client_send_service_us",client_send_us},{"client_receive_calls",receive_calls},{"client_send_calls",send_calls},
        {"ordinary_messages",ordinary_messages},{"ordinary_messages_during_unACKed_cut",hold_messages},
        {"ordinary_cadence_hz_during_unACKed_cut",ordinary_hz},{"historical_cut_behind_latest_gameplay",historical_behind_latest},
        {"native_ack_flushed_without_next_receive",ack_flush_checked},{"controlled_pre_ACK_gap_ms",2000},{"controlled_post_ACK_no_receive_ms",4000},
        {"delay_control_keeps_actual_gameplay_input_fresh",true},
        {"production_native_timeline",timeline_report},
        {"backpressure",hs.backpressure},{"coalesced",hs.coalesced},{"client_rejections",cs.rejected},{"cuts",cuts},
        {"wire_scope","Per-cut service bytes include retransmission/control while that serial is active; outer rawUDP authentication overhead is excluded"},
        {"retained_large_artifacts",nlohmann::json::array()}};
    std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';check(bool(output),"Could not retain bounded native transport report");
    std::cout<<"PASS authenticated ordinary rawUDP native stream: "<<accepted<<" actual four-stage imports/ACKs; dedicated="<<dedicated
        <<" advancing ticks="<<observed.ticks<<" full="<<hs.full_cuts<<" delta="<<hs.delta_cuts<<" largest capture gap="<<hs.largest_capture_gap_ms<<"ms\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"Native transport failure phase="<<phase<<" error="<<e.what()<<'\n';return 1;}
